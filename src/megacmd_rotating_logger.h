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

class MessageBus final
{
public:
    using MemoryBuffer = std::vector<char>;

public:
    // These are:
    //      reservedSize: initial size of the front and back buffers
    //      shouldSwapSize: suggested size at which we can attempt to swap the buffers
    //      failSafeSize: fail safe size at which we should flush the message bus
    //                    after flushing, the underlying buffers are shrunk to fit the fail safe size
    MessageBus(size_t reservedSize, size_t shouldSwapSize, size_t failSafeSize);

    void append(const char* data, size_t size);
    std::pair<bool /* memoryError */, const MemoryBuffer&> swapBuffers();

    bool isEmpty() const;
    bool shouldSwapBuffers() const;
    bool reachedFailSafeSize() const;

private:
    void clearFrontBuffer();

    const size_t mShouldSwapSize;
    const size_t mFailSafeSize;

    mutable std::mutex mListMtx;
    MemoryBuffer mFrontBuffer;
    MemoryBuffer mBackBuffer;
    bool mMemoryError;
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

    static float getCompressionRatio(CompressionType compressionType);

    static RotationType getRotationTypeFromStr(std::string_view str);
    static CompressionType getCompressionTypeFromStr(std::string_view str);

    struct Config
    {
        size_t mMaxBaseFileSize;

        RotationType mRotationType;
        std::chrono::seconds mMaxFileAge;
        int mMaxFilesToKeep;

        CompressionType mCompressionType;
    };

public:
    RotatingFileManager(const fs::path& filePath, const Config& config);

    bool shouldRotateFiles(size_t fileSize) const;

    void cleanupFiles();
    void rotateFiles();

    std::string popErrors();

private:
    void initializeCompressionEngine();
    void initializeRotationEngine();

private:
    const Config mConfig;
    const fs::path mDirectory;
    const fs::path mBaseFilename;

    std::unique_ptr<CompressionEngine> mCompressionEngine;
    std::unique_ptr<RotationEngine> mRotationEngine;
};

class FileRotatingLoggedStream final : public LoggedStream
{
    mutable MessageBus mMessageBus;

    fs::path mOutputFilePath;
    std::ofstream mOutputFile;
    RotatingFileManager mFileManager;

    mutable std::mutex mWriteMtx;
    mutable std::condition_variable mWriteCV;
    mutable std::mutex mExitMtx;
    mutable std::condition_variable mExitCV;

    bool mForceRenew;
    bool mExit;
    bool mFlush;
    std::chrono::seconds mFlushPeriod;
    std::chrono::steady_clock::time_point mNextFlushTime;

    std::thread mWriteThread;

private:
    static size_t loadFailSafeSize();
    static RotatingFileManager::Config loadFileConfig();

    bool shouldRenew() const;
    bool shouldExit() const;
    bool shouldFlush() const;
    void setForceRenew(bool forceRenew);

    void writeToBuffer(const char* msg, size_t size) const;

    void writeMessagesToFile();
    void flushToFile();
    void markForExit();
    bool waitForOutputFile();

    void mainLoop();

public:
    FileRotatingLoggedStream(const OUTSTRING& outputFilePath);
    ~FileRotatingLoggedStream();

    const LoggedStream& operator<<(const char& c) const override;
    const LoggedStream& operator<<(const char* str) const override;
    const LoggedStream& operator<<(std::string str) const override;
    const LoggedStream& operator<<(BinaryStringView v) const override;
    const LoggedStream& operator<<(std::string_view str) const override;
    const LoggedStream& operator<<(int v) const override;
    const LoggedStream& operator<<(unsigned int v) const override;
    const LoggedStream& operator<<(long long v) const override;
    const LoggedStream& operator<<(unsigned long v) const override;

    const LoggedStream& operator<<(std::ios_base)                      const override { return *this; }
    const LoggedStream& operator<<(std::ios_base*)                     const override { return *this; }
    const LoggedStream& operator<<(OUTSTREAMTYPE& (*)(OUTSTREAMTYPE&)) const override { return *this; }

#ifdef _WIN32
    const LoggedStream& operator<<(std::wstring v) const override;
#endif

    virtual void flush() override;
};
}
