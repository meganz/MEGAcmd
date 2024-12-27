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

#include "megacmd_rotating_logger.h"

#include <cassert>
#include <unordered_set>
#include <optional>
#include <zlib.h>

namespace {
class ReopenScope final
{
    OUTFSTREAMTYPE& mOutFile;
    const std::string mOutFilePath;
public:
    ReopenScope(OUTFSTREAMTYPE& outFile, const std::string& outFilePath) :
        mOutFile(outFile),
        mOutFilePath(outFilePath)
    {
        mOutFile.close();
    }

    ~ReopenScope()
    {
        mOutFile.open(mOutFilePath, std::ofstream::out | std::ofstream::app);
    }
};

std::string outstringToString(const OUTSTRING& outstring)
{
#ifdef _WIN32
    std::string str;
    megacmd::localwtostring(&outstring, &str);
    return str;
#else
    return outstring;
#endif
}
}

namespace megacmd {

MessageBuffer::MemoryBlock::MemoryBlock(size_t capacity) :
    mBuffer(new char[capacity]()),
    mSize(0),
    mCapacity(capacity),
    mMemoryAllocationFailed(mBuffer == nullptr)
{
    assert(mCapacity > 0);
}

bool MessageBuffer::MemoryBlock::canAppendData(size_t dataSize) const
{
    // The last byte of buffer is reserved for '\0'
    return !mMemoryAllocationFailed && mSize + dataSize < mCapacity;
}

bool MessageBuffer::MemoryBlock::isNearCapacity() const
{
    return mSize + mCapacity/8 > mCapacity;
}

bool MessageBuffer::MemoryBlock::memoryAllocationFailed() const
{
    return mMemoryAllocationFailed;
}

const char* MessageBuffer::MemoryBlock::getBuffer() const
{
    return mBuffer.get();
}

void MessageBuffer::MemoryBlock::markMemoryAllocationFailed()
{
    mMemoryAllocationFailed = true;
}

void MessageBuffer::MemoryBlock::appendData(const char* data, size_t size)
{
    assert(canAppendData(size));
    assert(data && size > 0);

    std::memcpy(mBuffer.get() + mSize, data, size);
    mSize += size;

    // Append null after the last character so the buffer
    // is properly treated as a C-string
    mBuffer[mSize] = '\0';
}

MessageBuffer::MessageBuffer(size_t defaultBlockCapacity, size_t failSafeSize) :
    mDefaultBlockCapacity(defaultBlockCapacity),
    mFailSafeSize(failSafeSize),
    mInitialMemoryGap(false)
{
    assert(mDefaultBlockCapacity > 0);
    assert(mFailSafeSize > 0);
    assert(mFailSafeSize > mDefaultBlockCapacity);
}

void MessageBuffer::append(const char* data, size_t size)
{
    std::lock_guard lock(mListMtx);

    auto* lastBlock = mList.empty() ? nullptr : &mList.back();
    if (!lastBlock || !lastBlock->canAppendData(size))
    {
        try
        {
            mList.emplace_back(mDefaultBlockCapacity);
        }
        catch (const std::bad_alloc&)
        {
            if (lastBlock)
            {
                // If there's a bad_alloc exception when adding to a vector, it remains unchanged
                // So we note that the existing last block has a gap in memory
                lastBlock->markMemoryAllocationFailed();
            }
            else
            {
               // If there's not even a last block, then there is a memory gap at the start
                mInitialMemoryGap = true;
            }
            return;
        }

        lastBlock = &mList.back();
    }

    if (lastBlock->canAppendData(size))
    {
        lastBlock->appendData(data, size);
    }
}

MessageBuffer::MemoryBlockList MessageBuffer::popMemoryBlockList(bool& initialMemoryGap)
{
    std::lock_guard lock(mListMtx);
    initialMemoryGap = mInitialMemoryGap;
    mInitialMemoryGap = false;
    return std::move(mList);
}

bool MessageBuffer::isEmpty() const
{
    std::lock_guard lock(mListMtx);
    return mList.empty();
}

bool MessageBuffer::isNearLastBlockCapacity() const
{
    std::lock_guard lock(mListMtx);
    return !mList.empty() && mList.back().isNearCapacity();
}

bool MessageBuffer::reachedFailSafeSize() const
{
    std::lock_guard lock(mListMtx);
    return mList.size() > mFailSafeSize / mDefaultBlockCapacity;
}

class BaseEngine
{
protected:
    std::stringstream mErrorStream;

public:
    std::string popErrors();
};

class RotationEngine : public BaseEngine
{
protected:
    template<typename F>
    void walkRotatedFiles(const fs::path& dir, const fs::path& baseFilename, F&& walker);

public:
    virtual ~RotationEngine() = default;

    virtual void cleanupFiles(const fs::path& dir, const fs::path& baseFilename);
    virtual fs::path rotateFiles(const fs::path& dir, const fs::path& baseFilename) = 0;
};

class NumberedRotationEngine final : public RotationEngine
{
    const std::string mCompressionExt;
    const int mMaxFilesToKeep;

    fs::path getSrcFilePath(const fs::path& directory, const fs::path& baseFilename, int i) const;
    fs::path getDstFilePath(const fs::path& directory, const fs::path& baseFilename, int i) const;

public:
    NumberedRotationEngine(const std::string &compressionExt, int maxFilesToKeep);

    fs::path rotateFiles(const fs::path& dir, const fs::path& baseFilePath) override;
};

class TimestampRotationEngine final : public RotationEngine
{
    const int mMaxFilesToKeep;
    const std::chrono::seconds mMaxFileAge;

private:
    using Clock = std::chrono::system_clock;
    using Timestamp = std::chrono::time_point<Clock>;

    struct TimestampFile
    {
        fs::path mPath;
        Timestamp mTimestamp;

        TimestampFile(const fs::path& path, const Timestamp &timestamp);
        bool operator>(const TimestampFile& other) const { return mTimestamp > other.mTimestamp; }
    };
    using TimestampFileQueue = std::priority_queue<TimestampFile, std::vector<TimestampFile>, std::greater<TimestampFile>>;

private:
    static std::string timestampToString(const Timestamp& timestamp);
    static std::optional<Timestamp> stringToTimestamp(const std::string& timestampStr);

    fs::path rotateBaseFile(const fs::path& directory, const fs::path& baseFilename);
    TimestampFileQueue getTimestampFileQueue(const fs::path& dir, const fs::path& baseFilename);

    void popAndRemoveFile(TimestampFileQueue& fileQueue);

public:
    TimestampRotationEngine(int maxFilesToKeep, std::chrono::seconds maxFileAge);

    fs::path rotateFiles(const fs::path& dir, const fs::path& baseFilename) override;
};

class CompressionEngine : public BaseEngine
{
public:
    virtual ~CompressionEngine() = default;

    virtual std::string getExtension() const { return ""; }

    virtual void cancelAll() {}
    virtual void compressFile(const fs::path&) {}
};

class GzipCompressionEngine final : public CompressionEngine
{
    struct GzipJobData
    {
        fs::path mSrcFilePath;
        fs::path mDstFilePath;

        GzipJobData(const fs::path& srcFilePath, const fs::path& dstFilePath);
    };
    using GzipJobQueue = std::queue<GzipJobData>;
    GzipJobQueue mQueue;

    mutable std::mutex mQueueMtx;
    std::condition_variable mQueueCV;
    bool mCancelOngoingJob;
    bool mExit;

    std::thread mGzipThread;

private:
    bool shouldCancelOngoingJob() const;

    void pushToQueue(const fs::path& srcFilePath, const fs::path& dstFilePath);
    void gzipFile(const fs::path& srcFilePath, const fs::path& dstFilePath);

    void mainLoop();

public:
    GzipCompressionEngine();
    ~GzipCompressionEngine();

    std::string getExtension() const override;

    void cancelAll() override;
    void compressFile(const fs::path& filePath) override;
};

RotatingFileManager::Config::Config() :
    mMaxBaseFileSize(50 * 1024 * 1024), // 50 MB
    mRotationType(RotationType::Timestamp),
    mMaxFileAge(30 * 86400), // one month (in C++20 we can use `std::chrono::month(1)`)
    mMaxFilesToKeep(50),
    mCompressionType(CompressionType::Gzip)
{
}

RotatingFileManager::RotatingFileManager(const fs::path& filePath, const Config &config) :
    mConfig(config),
    mDirectory(filePath.parent_path()),
    mBaseFilename(filePath.filename())
{
    initializeCompressionEngine();
    initializeRotationEngine();
}

bool RotatingFileManager::shouldRotateFiles(size_t fileSize) const
{
    return fileSize > mConfig.mMaxBaseFileSize;
}

void RotatingFileManager::cleanupFiles()
{
    mCompressionEngine->cancelAll();
    mRotationEngine->cleanupFiles(mDirectory, mBaseFilename);
}

void RotatingFileManager::rotateFiles()
{
    auto newlyRotatedFilePath = mRotationEngine->rotateFiles(mDirectory, mBaseFilename);
    mCompressionEngine->compressFile(newlyRotatedFilePath);
}

std::string RotatingFileManager::popErrors()
{
    return mRotationEngine->popErrors() + mCompressionEngine->popErrors();
}

void RotatingFileManager::initializeCompressionEngine()
{
    CompressionEngine* compressionEngine = nullptr;
    switch(mConfig.mCompressionType)
    {
        case CompressionType::None:
        {
            compressionEngine = new CompressionEngine();
            break;
        }
        case CompressionType::Gzip:
        {
            compressionEngine = new GzipCompressionEngine();
            break;
        }
    }
    assert(compressionEngine);
    mCompressionEngine = std::unique_ptr<CompressionEngine>(compressionEngine);
}

void RotatingFileManager::initializeRotationEngine()
{
    assert(mCompressionEngine);
    const std::string compressionExt = mCompressionEngine->getExtension();

    RotationEngine* rotationEngine = nullptr;
    switch (mConfig.mRotationType)
    {
        case RotationType::Numbered:
        {
            rotationEngine = new NumberedRotationEngine(compressionExt, mConfig.mMaxFilesToKeep);
            break;
        }
        case RotationType::Timestamp:
        {
            rotationEngine = new TimestampRotationEngine(mConfig.mMaxFilesToKeep, mConfig.mMaxFileAge);
            break;
        }
    }
    assert(rotationEngine);
    mRotationEngine = std::unique_ptr<RotationEngine>(rotationEngine);
}

bool FileRotatingLoggedStream::shouldRenew() const
{
    std::lock_guard lock(mWriteMtx);
    return mForceRenew;
}

bool FileRotatingLoggedStream::shouldExit() const
{
    std::lock_guard lock(mExitMtx);
    return mExit;
}

bool FileRotatingLoggedStream::shouldFlush() const
{
    std::lock_guard lock(mWriteMtx);
    return mFlush || mNextFlushTime <= std::chrono::steady_clock::now();
}

void FileRotatingLoggedStream::setForceRenew(bool forceRenew)
{
    std::lock_guard lock(mWriteMtx);
    mForceRenew = forceRenew;
}

void FileRotatingLoggedStream::writeToBuffer(const char* msg, size_t size) const
{
    if (shouldExit())
    {
        std::cerr << msg;
        return;
    }

    mMessageBuffer.append(msg, size);
    if (mMessageBuffer.isNearLastBlockCapacity())
    {
        mWriteCV.notify_one();
    }
}

void FileRotatingLoggedStream::writeMessagesToFile()
{
    bool initialMemoryGap = false;
    auto memoryBlockList = mMessageBuffer.popMemoryBlockList(initialMemoryGap);

    if (initialMemoryGap)
    {
        mOutputFile << "<log gap - out of logging memory at this point>\n";
    }

    for (const auto& memoryBlock : memoryBlockList)
    {
        if (!memoryBlock.memoryAllocationFailed())
        {
            mOutputFile << memoryBlock.getBuffer();
        }
        else
        {
            mOutputFile << "<log gap - out of logging memory at this point>\n";
        }
    }
}

void FileRotatingLoggedStream::flushToFile()
{
    mOutputFile.flush();
    mNextFlushTime = std::chrono::steady_clock::now() + mFlushPeriod;
    {
        std::lock_guard lock(mWriteMtx);
        mFlush = false;
    }
}

void FileRotatingLoggedStream::markForExit()
{
    {
        std::scoped_lock lock(mExitMtx, mWriteMtx);
        mExit = true;
    }
    mWriteCV.notify_one();
    mExitCV.notify_one();
}

bool FileRotatingLoggedStream::waitForOutputFile()
{
    thread_local const int waitTimes[] = {0, 1, 5, 20, 60};
    thread_local int i = 0;

    if (mOutputFile)
    {
        i = 0;
        return true;
    }

    {
        std::unique_lock lock(mExitMtx);
        mExitCV.wait_for(lock, std::chrono::seconds(waitTimes[i]), [this]
        {
            return mExit;
        });
    }

    if (i < std::size(waitTimes) - 1) ++i;
    return false;
}

void FileRotatingLoggedStream::mainLoop()
{
    while (!shouldExit() || !mMessageBuffer.isEmpty())
    {
        std::string errorMessages;
        bool reopenFile = false;

        if (!waitForOutputFile())
        {
            errorMessages += "Error writing to log file " + mOutputFilePath + '\n';
            errorMessages += "Re-opening...\n";
            reopenFile = true;
        }

        const size_t outFileSize = mOutputFile ? static_cast<size_t>(mOutputFile.tellp()) : 0;
        if (reopenFile)
        {
            ReopenScope s(mOutputFile, mOutputFilePath);
        }
        else if (shouldRenew())
        {
            ReopenScope s(mOutputFile, mOutputFilePath);
            mFileManager.cleanupFiles();
            setForceRenew(false);
        }
        else if (mFileManager.shouldRotateFiles(outFileSize))
        {
            ReopenScope s(mOutputFile, mOutputFilePath);
            mFileManager.rotateFiles();
        }

        errorMessages += mFileManager.popErrors();
        std::cerr << errorMessages;

        if (!mOutputFile)
        {
            // If we cannot write to file, do not keep trying if we should exit
            if (shouldExit())
            {
                break;
            }

            // If we've reached the fail-safe size, try to write to file anyway
            // This clears the buffer, ensuring it doesn't grow beyond a certain threshold
            if (!mMessageBuffer.reachedFailSafeSize())
            {
                continue;
            }
        }
        mOutputFile << errorMessages;

        bool writeMessages = false;
        {
            std::unique_lock lock(mWriteMtx);
            writeMessages = mWriteCV.wait_for(lock, std::chrono::milliseconds(500), [this]
            {
                return mForceRenew || mExit || mFlush || !mMessageBuffer.isEmpty();
            });
        }

        if (writeMessages)
        {
            writeMessagesToFile();
        }

        if (shouldFlush())
        {
            flushToFile();
        }
    }
}

FileRotatingLoggedStream::FileRotatingLoggedStream(const OUTSTRING& outputFilePath) :
    mMessageBuffer(2048 /* 2KB */, 500 * 1024 * 1024 /* 500MB */),
    mOutputFilePath(outstringToString(outputFilePath)),
    mOutputFile(outputFilePath, std::ofstream::out | std::ofstream::app),
    mFileManager(mOutputFilePath),
    mForceRenew(false),
    mExit(false),
    mFlush(false),
    mFlushPeriod(std::chrono::seconds(10)),
    mNextFlushTime(std::chrono::steady_clock::now() + mFlushPeriod),
    mWriteThread([this] () { mainLoop(); })
{
}

FileRotatingLoggedStream::~FileRotatingLoggedStream()
{
    markForExit();
    mWriteThread.join();
}

const LoggedStream& FileRotatingLoggedStream::operator<<(const char& c) const
{
    writeToBuffer(&c, sizeof(c));
    return *this;
}

const LoggedStream& FileRotatingLoggedStream::operator<<(const char* str) const
{
    writeToBuffer(str, strlen(str));
    return *this;
}

const LoggedStream& FileRotatingLoggedStream::operator<<(std::string str) const
{
    writeToBuffer(str.c_str(), str.size());
    return *this;
}

const LoggedStream& FileRotatingLoggedStream::operator<<(int v) const
{
    return operator<<(std::to_string(v));
}

const LoggedStream& FileRotatingLoggedStream::operator<<(unsigned int v) const
{
    return operator<<(std::to_string(v));
}

const LoggedStream& FileRotatingLoggedStream::operator<<(long long v) const
{
    return operator<<(std::to_string(v));
}

const LoggedStream& FileRotatingLoggedStream::operator<<(unsigned long v) const
{
    return operator<<(std::to_string(v));
}

#ifdef _WIN32
const LoggedStream& FileRotatingLoggedStream::operator<<(std::wstring wstr) const
{
    std::string str;
    localwtostring(&wstr, &str);
    writeToBuffer(str.c_str(), str.size());
    return *this;
}
#endif

void FileRotatingLoggedStream::flush()
{
    {
        std::lock_guard lock(mWriteMtx);
        mFlush = true;
    }
    mWriteCV.notify_one();
}

std::string BaseEngine::popErrors()
{
    std::string errorString = mErrorStream.str();
    mErrorStream.str("");
    return errorString;
}

template<typename F>
void RotationEngine::walkRotatedFiles(const fs::path& dir, const fs::path& baseFilename, F&& walker)
{
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(dir, ec))
    {
        if (ec)
        {
            mErrorStream << "Failed to walk directory " << dir << " (error: " << ec.message() << ")" << std::endl;
            return;
        }

        // Ignore non-files
        if (!entry.is_regular_file(ec) || ec)
        {
            continue;
        }

        const fs::path& filePath = entry.path();
        std::string filename = filePath.filename().string();

        // Ignore files that don't start with the base filename
        if (filename.rfind(baseFilename.string(), 0) != 0)
        {
            continue;
        }

        walker(filePath);
    }
}

void RotationEngine::cleanupFiles(const fs::path& dir, const fs::path& baseFilename)
{
    walkRotatedFiles(dir, baseFilename, [this] (const fs::path& filePath)
    {
        std::error_code ec;

        fs::remove(filePath, ec);
        if (ec)
        {
            mErrorStream << "Error removing rotated file " << filePath << " (error: " << ec.message() << ")" << std::endl;
        }
    });
}

fs::path NumberedRotationEngine::getSrcFilePath(const fs::path& directory, const fs::path& baseFilename, int i) const
{
    fs::path filePath = directory / baseFilename;

    // The source path for the base file does not have an index or compression extension
    if (i != 0)
    {
        filePath += "." + std::to_string(i) + mCompressionExt;
    }
    return filePath;
}

fs::path NumberedRotationEngine::getDstFilePath(const fs::path& directory, const fs::path& baseFilename, int i) const
{
    fs::path filePath = directory / baseFilename;

    // The destination path for the base file has an index, but doesn't have a compression extension
    // (since the file still needs to be sent to the compression engine)
    filePath += "." + std::to_string(i + 1);
    if (i != 0)
    {
        filePath += mCompressionExt;
    }
    return filePath;
}

NumberedRotationEngine::NumberedRotationEngine(const std::string& compressionExt, int maxFilesToKeep) :
    mCompressionExt(compressionExt),
    mMaxFilesToKeep(maxFilesToKeep)
{
    // Numbered rotation does not support keeping unlimited files
    assert(mMaxFilesToKeep >= 0);
}

fs::path NumberedRotationEngine::rotateFiles(const fs::path& dir, const fs::path& baseFilename)
{
    std::error_code ec;
    for (int i = mMaxFilesToKeep - 1; i >= 0; --i)
    {
        fs::path srcFilePath = getSrcFilePath(dir, baseFilename, i);

        // Quietly ignore any numbered files that don't exist
        if (!fs::exists(srcFilePath) || ec)
        {
            continue;
        }

        // Delete the last file, which is the one with number == maxFilesToKeep
        if (i == mMaxFilesToKeep - 1)
        {
            fs::remove(srcFilePath, ec);
            if (ec)
            {
                mErrorStream << "Error removing file " << srcFilePath << " (error: " << ec.message() << ")" << std::endl;
            }
            continue;
        }


        // For the rest, rename them to effectively do number++ (e.g., file.3.gz => file.4.gz)
        fs::path dstFilePath = getDstFilePath(dir, baseFilename, i);

        fs::rename(srcFilePath, dstFilePath, ec);
        if (ec)
        {
            mErrorStream << "Error renaming file " << srcFilePath << " to " << dstFilePath << " (error: " << ec.message() << ")" << std::endl;
        }
    }

    // The newly-rotated file is the destination path of the base file
    return getDstFilePath(dir, baseFilename, 0);
}

TimestampRotationEngine::TimestampFile::TimestampFile(const fs::path& path, const Timestamp &timestamp) :
    mPath(path),
    mTimestamp(timestamp)
{
}

std::string TimestampRotationEngine::timestampToString(const Timestamp& timestamp)
{
    std::ostringstream oss;
    std::time_t time = Clock::to_time_t(timestamp);
    oss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S_");

    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()) % 1000;
    oss << std::setfill('0') << std::setw(3) << milliseconds.count();

    return oss.str();
}

std::optional<TimestampRotationEngine::Timestamp> TimestampRotationEngine::stringToTimestamp(const std::string& timestampStr)
{
    std::tm timeInfo = {};
    int milliseconds = -1;

    std::istringstream iss(timestampStr);
    iss >> std::get_time(&timeInfo, "%Y%m%d_%H%M%S_") >> milliseconds;

    bool success = milliseconds >= 0 && milliseconds < 1000 && !iss.fail();
    if (!success)
    {
        return {};
    }

    return Clock::from_time_t(std::mktime(&timeInfo)) + std::chrono::milliseconds(milliseconds);
}

fs::path TimestampRotationEngine::rotateBaseFile(const fs::path& directory, const fs::path& baseFilename)
{
    const std::string timestampStr = timestampToString(Clock::now());

    fs::path srcFilePath = directory / baseFilename;
    fs::path dstFilePath = srcFilePath.string() + "." + timestampStr;

    std::error_code ec;

    fs::rename(srcFilePath, dstFilePath, ec);
    if (ec)
    {
        mErrorStream << "Error renaming file " << srcFilePath << " to " << dstFilePath << " (error: " << ec.message() << ")" << std::endl;
    }
    return dstFilePath;
}

TimestampRotationEngine::TimestampFileQueue TimestampRotationEngine::getTimestampFileQueue(const fs::path& dir, const fs::path& baseFilename)
{
    TimestampFileQueue fileQueue;
    std::unordered_set<std::string> addedFiles;

    const std::string baseFilenameStr = baseFilename.string();

    walkRotatedFiles(dir, baseFilename, [this, &baseFilenameStr, &fileQueue, &addedFiles] (const fs::path& filePath)
    {
        constexpr size_t timeStampSize = std::char_traits<char>::length("19970907_193040_000");

        const std::string filenameStr = filePath.filename().string();
        const std::string timestampStr = filenameStr.substr(baseFilenameStr.size() + 1, timeStampSize);

        auto timestampOpt = stringToTimestamp(timestampStr);
        if (!timestampOpt)
        {
            // Ignore if there's not timestamp or it has a different format
            return;
        }

        if (addedFiles.find(timestampStr) != addedFiles.end())
        {
            // Otherwise, in the rare case that a file finishes being zipped while this function is executing,
            // we might add it twice: one with "zipping" prefix and one without. This would lead to an incorrect number
            // of files, which would messs up the max file calculation. Regardless, the zipping file is the newest one so it won't be deleted.
            return;
        }

        addedFiles.emplace(timestampStr);
        fileQueue.emplace(filePath, *timestampOpt);
    });

    return fileQueue;
}

void TimestampRotationEngine::popAndRemoveFile(TimestampFileQueue& fileQueue)
{
    const TimestampFile file = fileQueue.top();
    fileQueue.pop();

    std::error_code ec;

    fs::remove(file.mPath, ec);
    if (ec)
    {
        mErrorStream << "Error removing rotated file " << file.mPath << " (error: " << ec.message() << ")" << std::endl;
    }
}

TimestampRotationEngine::TimestampRotationEngine(int maxFilesToKeep, std::chrono::seconds maxFileAge) :
    mMaxFilesToKeep(maxFilesToKeep),
    mMaxFileAge(maxFileAge)
{
}

fs::path TimestampRotationEngine::rotateFiles(const fs::path& dir, const fs::path& baseFilename)
{
    fs::path newlyRotatedFile = rotateBaseFile(dir, baseFilename);

    if (mMaxFileAge <= std::chrono::seconds(0) && mMaxFilesToKeep < 0)
    {
        return newlyRotatedFile;
    }

    auto fileQueue = getTimestampFileQueue(dir, baseFilename);

    // Rotate by timestamp
    if (mMaxFileAge > std::chrono::seconds(0))
    {
        const Timestamp minFileTimestamp = Clock::now() - mMaxFileAge;
        while (!fileQueue.empty() && fileQueue.top().mTimestamp < minFileTimestamp)
        {
            popAndRemoveFile(fileQueue);
        }
    }

    // Rotate by file count
    if (mMaxFilesToKeep >= 0)
    {
        while (fileQueue.size() > static_cast<size_t>(mMaxFilesToKeep))
        {
            popAndRemoveFile(fileQueue);
        }
    }

    return newlyRotatedFile;
}

GzipCompressionEngine::GzipJobData::GzipJobData(const fs::path& srcFilePath, const fs::path& dstFilePath) :
    mSrcFilePath(srcFilePath),
    mDstFilePath(dstFilePath)
{
}

bool GzipCompressionEngine::shouldCancelOngoingJob() const
{
    std::lock_guard lock(mQueueMtx);
    return mCancelOngoingJob;
}

void GzipCompressionEngine::pushToQueue(const fs::path& srcFilePath, const fs::path& dstFilePath)
{
    std::lock_guard lock(mQueueMtx);

    if (mExit)
    {
        return;
    }

    mQueue.emplace(GzipJobData(srcFilePath, dstFilePath));
    mQueueCV.notify_one();
}

void GzipCompressionEngine::gzipFile(const fs::path& srcFilePath, const fs::path& dstFilePath)
{
    {
        std::ifstream srcFile(srcFilePath);
        if (!srcFile)
        {
            mErrorStream << "Failed to open " << srcFilePath << " for compression" << std::endl;
            return;
        }

        auto gzdeleter = [] (gzFile_s* f) { if (f) gzclose(f); };
        std::unique_ptr<gzFile_s, decltype(gzdeleter)> gzFile(gzopen(dstFilePath.string().c_str(), "wb"), gzdeleter);
        if (!gzFile)
        {
            mErrorStream << "Failed to open gzfile " << dstFilePath << " for writing" << std::endl;
            return;
        }

        std::string line;
        while (std::getline(srcFile, line))
        {
            if (shouldCancelOngoingJob())
            {
                return;
            }

            line += '\n';
            if (gzputs(gzFile.get(), line.c_str()) == -1)
            {
                mErrorStream << "Failed to gzip " << srcFilePath << std::endl;
                return;
            }
        }
    }

    std::error_code ec;

    fs::remove(srcFilePath, ec);
    if (ec)
    {
        mErrorStream << "Failed to remove temporary file " << srcFilePath << " after compression (error: " << ec.message() << ")" << std::endl;
    }
}

void GzipCompressionEngine::mainLoop()
{
    while (true)
    {
        std::optional<GzipJobData> jobDataOpt;

        {
            std::unique_lock lock(mQueueMtx);
            mQueueCV.wait(lock, [this] () { return mExit || !mQueue.empty(); });

            if (mExit && mQueue.empty())
            {
                return;
            }
            assert(!mQueue.empty());

            jobDataOpt = std::move(mQueue.front());
            mQueue.pop();

            mCancelOngoingJob = false;
        }

        assert(jobDataOpt);
        gzipFile(jobDataOpt->mSrcFilePath, jobDataOpt->mDstFilePath);
    }
}

GzipCompressionEngine::GzipCompressionEngine() :
    mCancelOngoingJob(false),
    mExit(false),
    mGzipThread([this] () { mainLoop(); })
{
}

GzipCompressionEngine::~GzipCompressionEngine()
{
    {
        std::lock_guard lock(mQueueMtx);
        mExit = true;
        // We want to exit gracefull, so we don't call `cancelAll`
    }
    mQueueCV.notify_one();

    mGzipThread.join();
}

std::string GzipCompressionEngine::getExtension() const
{
    return ".gz";
}

void GzipCompressionEngine::cancelAll()
{
    std::lock_guard lock(mQueueMtx);

    // Clear the queue
    mQueue = GzipJobQueue();

    // This flag will ensure `gzipFile` returns as soon as possible (if it's running)
    // It'll be unset the next time we pop an item from the queue
    mCancelOngoingJob = true;
}

void GzipCompressionEngine::compressFile(const fs::path& filePath)
{
    std::error_code ec;
    fs::path tmpFilePath = filePath.string() + ".zipping";

    // Ensure there is not a clashing .zipping file
    if (fs::exists(tmpFilePath, ec) && !ec)
    {
        fs::remove(tmpFilePath, ec);
        if (ec)
        {
            mErrorStream << "Failed to remove temporary compression file " << tmpFilePath << " (error: " << ec.message() << ")" << std::endl;
            return;
        }
    }

    fs::rename(filePath, tmpFilePath, ec);
    if (ec)
    {
        mErrorStream << "Failed to rename file " << filePath << " to " << tmpFilePath << " (error: " << ec.message() << ")" << std::endl;
        return;
    }

    fs::path targetFilePath = filePath.string() + getExtension();
    pushToQueue(tmpFilePath, targetFilePath);
}
}
