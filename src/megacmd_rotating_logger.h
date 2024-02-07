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
    mutable MessageBuffer mMessageBuffer;

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
}
