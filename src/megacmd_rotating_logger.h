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

class MessageBuffer final
{
public:
    class MemoryBlock final
    {
        std::unique_ptr<char[]> mBuffer;
        size_t mSize;
        const size_t mCapacity;
        const bool mOutOfMemory;
    public:
        MemoryBlock(size_t capacity);

        bool canAppendData(size_t dataSize) const;
        bool isNearCapacity() const;
        bool isOutOfMemory() const;
        const char *getBuffer() const;

        void appendData(const char *data, size_t size);

    };
    using MemoryBlockList = std::vector<MemoryBlock>;

public:
    MessageBuffer(size_t defaultBlockCapacity);

    void append(const char *data, size_t size);
    MemoryBlockList popMemoryBlockList();

    bool isEmpty() const;
    bool isNearLastBlockCapacity() const;

private:
    const size_t mDefaultBlockCapacity;
    mutable std::mutex mListMutex;
    MemoryBlockList mList;
};

class FileRotatingLoggedStream final : public LoggedStream
{
    mutable MessageBuffer mMessageBuffer;
    mutable OUTFSTREAMTYPE mOutputFile;

    mutable std::mutex mWriteMutex;
    mutable std::condition_variable mWriteCV;
    bool mForceRenew;
    bool mExit;
    mutable bool mFlush;
    std::chrono::seconds mFlushPeriod;
    std::chrono::steady_clock::time_point mNextFlushTime;

    std::thread mWriteThread;

private:
    bool shouldForceRenew() const;
    bool shouldExit() const;
    bool shouldFlush() const;

    void writeToBuffer(const char* msg, size_t size) const;
    void flushToFile();

    void writeToFileLoop();

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
}
