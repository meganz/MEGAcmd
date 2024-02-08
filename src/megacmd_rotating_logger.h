/**
 * (c) 2013 by Mega Limited, Auckland, New Zealand
 *
 * This file is part of MEGAcmd.
 *
 * MEGAcmd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * @copyright Simplified (2-clause) BSD License.
 *
 * You should have received a copy of the license along with this
 * program.
 */

#pragma once

#include <memory>
#include <vector>
#include <fstream>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <thread>

#include "megacmdlogger.h"

namespace megacmd {

template<size_t BlockSize>
class MessageBuffer final
{
public:
    class MemoryBlock final
    {
        size_t mSize;
        bool mOutOfMemory;
        char mBuffer[BlockSize];
    public:
        MemoryBlock();

        bool canAppendData(size_t dataSize) const;
        bool isNearCapacity() const;
        bool isOutOfMemory() const;
        const char *getBuffer() const;

        void markAsOutOfMemory();
        void appendData(const char *data, size_t size);

    };
    using MemoryBlockList = std::vector<MemoryBlock>;

public:
    MessageBuffer();

    void append(const char *data, size_t size);
    MemoryBlockList popMemoryBlockList(bool &outOfMemory); // when we have C++17 we can use optional for this

    bool isEmpty() const;
    bool isNearLastBlockCapacity() const;

private:
    mutable std::mutex mListMutex;
    MemoryBlockList mList;
    bool mOutOfMemory;
};

class RotationEngine;
class CompressionEngine;

class RotatingFileManager final
{
public:
    enum class RotationType
    {
        Numbered,
        Timestamp
    };

    enum class CompressionType
    {
        None,
        Gzip
    };

    struct Config
    {
        size_t maxBaseFileSize;

        RotationType rotationType;
        std::chrono::seconds maxFileAge;
        int maxFilesToKeep;

        CompressionType compressionType;

        Config();
    };

public:
    RotatingFileManager(const std::string &filePath, const Config &config = {});

    bool shouldRotateFiles(size_t fileSize) const;

    void cleanupFiles();
    void rotateFiles();

    std::string popErrors();

private:
    void initializeCompressionEngine();
    void initializeRotationEngine();

private:
    const Config mConfig;
    const mega::LocalPath mDirectory;
    const mega::LocalPath mBaseFilename;

    std::unique_ptr<CompressionEngine> mCompressionEngine;
    std::unique_ptr<RotationEngine> mRotationEngine;
};

class FileRotatingLoggedStream final : public LoggedStream
{
    mutable MessageBuffer<1024> mMessageBuffer;

    const std::string mOutputFilePath;
    mutable OUTFSTREAMTYPE mOutputFile;
    RotatingFileManager mFileManager;

    mutable std::mutex mWriteMutex;
    mutable std::condition_variable mWriteCV;
    bool mForceRenew;
    bool mExit;
    mutable bool mFlush;
    std::chrono::seconds mFlushPeriod;
    std::chrono::steady_clock::time_point mNextFlushTime;

    std::thread mWriteThread;

private:
    bool shouldRenew() const;
    bool shouldExit() const;
    bool shouldFlush() const;
    void setForceRenew(bool forceRenew);

    void writeToBuffer(const char* msg, size_t size) const;

    void writeMessagesToFile();
    void flushToFile();

    void mainLoop();

public:
    FileRotatingLoggedStream(const std::string &outputFilePath);
    ~FileRotatingLoggedStream();

    const LoggedStream &operator<<(const char &c) const override;
    const LoggedStream &operator<<(const char *str) const override;
    const LoggedStream &operator<<(std::string str) const override;
    const LoggedStream &operator<<(int v) const override;
    const LoggedStream &operator<<(unsigned int v) const override;
    const LoggedStream &operator<<(long long v) const override;
    const LoggedStream &operator<<(unsigned long v) const override;

    const LoggedStream& operator<<(std::ios_base v)                     const override { return *this; }
    const LoggedStream& operator<<(std::ios_base *v)                    const override { return *this; }
    const LoggedStream& operator<<(OUTSTREAMTYPE& (*F)(OUTSTREAMTYPE&)) const override { return *this; }

#ifdef _WIN32
    virtual const LoggedStream& operator<<(std::wstring v) const = 0;
#endif

    virtual void flush() override;
};

template<size_t BlockSize>
MessageBuffer<BlockSize>::MemoryBlock::MemoryBlock() :
    mSize(0),
    mOutOfMemory(false)
{
}

template<size_t BlockSize>
bool MessageBuffer<BlockSize>::MemoryBlock::canAppendData(size_t dataSize) const
{
    // The last byte of buffer is reserved for '\0'
    return !mOutOfMemory && mSize + dataSize < BlockSize;
}

template<size_t BlockSize>
bool MessageBuffer<BlockSize>::MemoryBlock::isNearCapacity() const
{
    return mSize + BlockSize/8 > BlockSize;
}

template<size_t BlockSize>
bool MessageBuffer<BlockSize>::MemoryBlock::isOutOfMemory() const
{
    return mOutOfMemory;
}

template<size_t BlockSize>
const char *MessageBuffer<BlockSize>::MemoryBlock::getBuffer() const
{
    return mBuffer;
}

template<size_t BlockSize>
void MessageBuffer<BlockSize>::MemoryBlock::markAsOutOfMemory()
{
    mOutOfMemory = true;
}

template<size_t BlockSize>
void MessageBuffer<BlockSize>::MemoryBlock::appendData(const char *data, size_t size)
{
    assert(canAppendData(size));
    assert(data && size > 0);

    std::memcpy(&mBuffer[mSize], data, size);
    mSize += size;

    // Append null after the last character so the buffer
    // is properly treated as a C-string
    mBuffer[mSize] = '\0';
}

template<size_t BlockSize>
MessageBuffer<BlockSize>::MessageBuffer() :
    mOutOfMemory(false)
{
}

template<size_t BlockSize>
void MessageBuffer<BlockSize>::append(const char *data, size_t size)
{
    std::lock_guard<std::mutex> lock(mListMutex);

    auto* lastBlock = mList.empty() ? nullptr : &mList.back();
    if (!lastBlock || !lastBlock->canAppendData(size))
    {
        try
        {
            mList.emplace_back();
        }
        catch (const std::bad_alloc&)
        {
            if (lastBlock)
            {
                // If there's a bad_alloc exception when adding to a vector, it remains unchanged
                // So we mark the existing last block as O.O.M.
                lastBlock->markAsOutOfMemory();
            }
            else
            {
                // If there's not even a last block, we declare the whole message buffer as O.O.M.
                mOutOfMemory = true;
            }
            return;
        }

        mOutOfMemory = false;
        lastBlock = &mList.back();
    }

    if (lastBlock->canAppendData(size))
    {
        lastBlock->appendData(data, size);
    }
}

template<size_t BlockSize>
typename MessageBuffer<BlockSize>::MemoryBlockList MessageBuffer<BlockSize>::popMemoryBlockList(bool &outOfMemory)
{
    std::lock_guard<std::mutex> lock(mListMutex);
    outOfMemory = mOutOfMemory;
    mOutOfMemory = false;
    return std::move(mList);
}

template<size_t BlockSize>
bool MessageBuffer<BlockSize>::isEmpty() const
{
    std::lock_guard<std::mutex> lock(mListMutex);
    return mList.empty();
}

template<size_t BlockSize>
bool MessageBuffer<BlockSize>::isNearLastBlockCapacity() const
{
    std::lock_guard<std::mutex> lock(mListMutex);
    return !mList.empty() && mList.back().isNearCapacity();
}
}
