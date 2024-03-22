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
#include <zlib.h>

#include "mega/filesystem.h"

namespace {
class ReopenScope final
{
    OUTFSTREAMTYPE& mOutFile;
    const std::string mOutFilePath;
public:
    ReopenScope(OUTFSTREAMTYPE& outFile, const std::string &outFilePath) :
        mOutFile(outFile),
        mOutFilePath(outFilePath)
    {
        mOutFile.close();
    }

    ~ReopenScope()
    {
        mOutFile.open(mOutFilePath, std::ofstream::out);
    }
};
}

namespace megacmd {

MessageBuffer::MemoryBlock::MemoryBlock(size_t capacity) :
    mBuffer(new char[capacity]()),
    mSize(0),
    mCapacity(capacity),
    mMemoryGap(mBuffer == nullptr)
{
    assert(mCapacity > 0);
}

bool MessageBuffer::MemoryBlock::canAppendData(size_t dataSize) const
{
    // The last byte of buffer is reserved for '\0'
    return !mMemoryGap && mSize + dataSize < mCapacity;
}

bool MessageBuffer::MemoryBlock::isNearCapacity() const
{
    return mSize + mCapacity/8 > mCapacity;
}

bool MessageBuffer::MemoryBlock::hasMemoryGap() const
{
    return mMemoryGap;
}

const char *MessageBuffer::MemoryBlock::getBuffer() const
{
    return mBuffer.get();
}

void MessageBuffer::MemoryBlock::markMemoryGap()
{
    mMemoryGap = true;
}

void MessageBuffer::MemoryBlock::appendData(const char *data, size_t size)
{
    assert(canAppendData(size));
    assert(data && size > 0);

    std::memcpy(mBuffer.get() + mSize, data, size);
    mSize += size;

    // Append null after the last character so the buffer
    // is properly treated as a C-string
    mBuffer[mSize] = '\0';
}

MessageBuffer::MessageBuffer(size_t defaultBlockCapacity) :
    mDefaultBlockCapacity(defaultBlockCapacity),
    mInitialMemoryGap(false)
{
}

void MessageBuffer::append(const char *data, size_t size)
{
    std::lock_guard<std::mutex> lock(mListMutex);

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
                lastBlock->markMemoryGap();
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

MessageBuffer::MemoryBlockList MessageBuffer::popMemoryBlockList(bool &initialMemoryGap)
{
    std::lock_guard<std::mutex> lock(mListMutex);
    initialMemoryGap = mInitialMemoryGap;
    mInitialMemoryGap = false;
    return std::move(mList);
}

bool MessageBuffer::isEmpty() const
{
    std::lock_guard<std::mutex> lock(mListMutex);
    return mList.empty();
}

bool MessageBuffer::isNearLastBlockCapacity() const
{
    std::lock_guard<std::mutex> lock(mListMutex);
    return !mList.empty() && mList.back().isNearCapacity();
}

class BaseEngine
{
protected:
    std::stringstream mErrorStream;
    mega::MegaFileSystemAccess mFsAccess;

public:
    std::string popErrors();
};

class RotationEngine : public BaseEngine
{
protected:
    template<typename F>
    void walkRotatedFiles(mega::LocalPath dir, const mega::LocalPath &baseFilename, F&& walker);

public:
    virtual ~RotationEngine() = default;

    virtual void cleanupFiles(const mega::LocalPath &dir, const mega::LocalPath &baseFilename);
    virtual mega::LocalPath rotateFiles(const mega::LocalPath &dir, const mega::LocalPath &baseFilename) = 0;
};

class NumberedRotationEngine final : public RotationEngine
{
    const std::string mCompressionExt;
    const int mMaxFilesToKeep;

    mega::LocalPath getSrcFilePath(const mega::LocalPath &directory, const mega::LocalPath &baseFilename, int i) const;
    mega::LocalPath getDstFilePath(const mega::LocalPath &directory, const mega::LocalPath &baseFilename, int i) const;

public:
    NumberedRotationEngine(const std::string &compressionExt, int maxFilesToKeep);

    mega::LocalPath rotateFiles(const mega::LocalPath &dir, const mega::LocalPath &baseFilePath) override;
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
        mega::LocalPath fullPath;
        Timestamp timestamp;

        TimestampFile(const mega::LocalPath &dir, const mega::LocalPath &filename, const Timestamp &_timestamp);
        bool operator>(const TimestampFile &other) const { return timestamp > other.timestamp; }
    };
    using TimestampFileQueue = std::priority_queue<TimestampFile, std::vector<TimestampFile>, std::greater<TimestampFile>>;

private:
    static std::string timestampToString(const Timestamp &timestamp);
    static Timestamp stringToTimestamp(const std::string &timestampStr, bool &success); // in C++17 we can use `std::optional` instead

    mega::LocalPath rotateBaseFile(const mega::LocalPath &directory, const mega::LocalPath &baseFilename);
    TimestampFileQueue getTimestampFileQueue(const mega::LocalPath &dir, const mega::LocalPath &baseFilename);

    void popAndRemoveFile(TimestampFileQueue &fileQueue);

public:
    TimestampRotationEngine(int maxFilesToKeep, std::chrono::seconds maxFileAge);

    mega::LocalPath rotateFiles(const mega::LocalPath &dir, const mega::LocalPath &baseFilename) override;
};

class CompressionEngine : public BaseEngine
{
public:
    virtual ~CompressionEngine() = default;

    virtual std::string getExtension() const { return ""; }

    virtual void cancelAll() {}
    virtual void compressFile(const mega::LocalPath&) {}
};

class GzipCompressionEngine final : public CompressionEngine
{
    struct GzipJobData
    {
        // When we have C++17 we can use an optional for this instead
        bool valid = false;

        mega::LocalPath srcFilePath;
        mega::LocalPath dstFilePath;

        GzipJobData() = default;
        GzipJobData(const mega::LocalPath &_srcFilePath, const mega::LocalPath &_dstFilePath);
    };
    std::queue<GzipJobData> mQueue;

    mutable std::mutex mQueueMutex;
    std::condition_variable mQueueCV;
    bool mCancelOngoingJob;
    bool mExit;

    std::thread mGzipThread;

private:
    bool shouldCancelOngoingJob() const;

    void pushToQueue(const mega::LocalPath &srcFilePath, const mega::LocalPath &dstFilePath);
    void gzipFile(const mega::LocalPath &srcFilePath, const mega::LocalPath &dstFilePath);

    void mainLoop();

public:
    GzipCompressionEngine();
    ~GzipCompressionEngine();

    std::string getExtension() const override;

    void cancelAll() override;
    void compressFile(const mega::LocalPath &filePath) override;
};

RotatingFileManager::Config::Config() :
    maxBaseFileSize(50 * 1024 * 1024), // 50 MB
    rotationType(RotationType::Timestamp),
    maxFileAge(30 * 86400), // one month (in C++20 we can use `std::chrono::month(1)`)
    maxFilesToKeep(50),
    compressionType(CompressionType::Gzip)
{
}

RotatingFileManager::RotatingFileManager(const mega::LocalPath &filePath, const Config &config) :
    mConfig(config),
    mDirectory(filePath.parentPath()),
    mBaseFilename(filePath.leafName())
{
    initializeCompressionEngine();
    initializeRotationEngine();
}

bool RotatingFileManager::shouldRotateFiles(size_t fileSize) const
{
    return fileSize > mConfig.maxBaseFileSize;
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
    switch(mConfig.compressionType)
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
    switch (mConfig.rotationType)
    {
        case RotationType::Numbered:
        {
            rotationEngine = new NumberedRotationEngine(compressionExt, mConfig.maxFilesToKeep);
            break;
        }
        case RotationType::Timestamp:
        {
            rotationEngine = new TimestampRotationEngine(mConfig.maxFilesToKeep, mConfig.maxFileAge);
            break;
        }
    }
    assert(rotationEngine);
    mRotationEngine = std::unique_ptr<RotationEngine>(rotationEngine);
}

bool FileRotatingLoggedStream::shouldRenew() const
{
    std::lock_guard<std::mutex> lock(mWriteMutex);
    return mForceRenew;
}

bool FileRotatingLoggedStream::shouldExit() const
{
    std::lock_guard<std::mutex> lock(mWriteMutex);
    return mExit;
}

bool FileRotatingLoggedStream::shouldFlush() const
{
    std::lock_guard<std::mutex> lock(mWriteMutex);
    return mFlush || mNextFlushTime <= std::chrono::steady_clock::now();
}

void FileRotatingLoggedStream::setForceRenew(bool forceRenew)
{
    std::lock_guard<std::mutex> lock(mWriteMutex);
    mForceRenew = forceRenew;
}

void FileRotatingLoggedStream::writeToBuffer(const char *msg, size_t size) const
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
        if (!memoryBlock.hasMemoryGap())
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
        std::lock_guard<std::mutex> lock(mWriteMutex);
        mFlush = false;
    }
}

void FileRotatingLoggedStream::mainLoop()
{
    while (!shouldExit() || !mMessageBuffer.isEmpty())
    {
        const size_t outFileSize = mOutputFile ? static_cast<size_t>(mOutputFile.tellp()) : 0;
        std::string errorMessages;

        if (!mOutputFile)
        {
            errorMessages += "Error writing to log file " + mOutputFilePath + '\n';
            errorMessages += "Forcing a renew...\n";
            setForceRenew(true);
        }

        if (shouldRenew())
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
        if (!errorMessages.empty())
        {
            mOutputFile << errorMessages;
            std::cerr << errorMessages;
        }

        bool writeMessages = false;
        {
            std::unique_lock<std::mutex> lock(mWriteMutex);
            writeMessages = mWriteCV.wait_for(lock, std::chrono::milliseconds(500), [this] ()
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

FileRotatingLoggedStream::FileRotatingLoggedStream(const OUTSTRING &outputFilePath) :
    mMessageBuffer(2048),
    mOutputFile(outputFilePath, std::ofstream::out | std::ofstream::app),
    mFileManager((megacmd::localwtostring(&outputFilePath, &mOutputFilePath), mega::LocalPath::fromAbsolutePath(mOutputFilePath))),
    mForceRenew(false), // maybe this should be true by default?
    mExit(false),
    mFlush(false),
    mFlushPeriod(std::chrono::seconds(10)),
    mNextFlushTime(std::chrono::steady_clock::now() + mFlushPeriod),
    mWriteThread([this] () { mainLoop(); })
{
}

FileRotatingLoggedStream::~FileRotatingLoggedStream()
{
    {
        std::lock_guard<std::mutex> lock(mWriteMutex);
        mExit = true;
        mWriteCV.notify_one();
    }
    mWriteThread.join();
}

const LoggedStream &FileRotatingLoggedStream::operator<<(const char &c) const
{
    writeToBuffer(&c, sizeof(c));
    return *this;
}

const LoggedStream &FileRotatingLoggedStream::operator<<(const char *str) const
{
    writeToBuffer(str, strlen(str));
    return *this;
}

const LoggedStream &FileRotatingLoggedStream::operator<<(std::string str) const
{
    writeToBuffer(str.c_str(), str.size());
    return *this;
}

const LoggedStream &FileRotatingLoggedStream::operator<<(int v) const
{
    return operator<<(std::to_string(v));
}

const LoggedStream &FileRotatingLoggedStream::operator<<(unsigned int v) const
{
    return operator<<(std::to_string(v));
}

const LoggedStream &FileRotatingLoggedStream::operator<<(long long v) const
{
    return operator<<(std::to_string(v));
}

const LoggedStream &FileRotatingLoggedStream::operator<<(unsigned long v) const
{
    return operator<<(std::to_string(v));
}

#ifdef _WIN32
const LoggedStream &FileRotatingLoggedStream::operator<<(std::wstring wstr) const
{
    std::string str;
    localwtostring(&wstr, &str);
    writeToBuffer(str.c_str(), str.size());
    return *this;
}
#endif

void FileRotatingLoggedStream::flush()
{
    std::lock_guard<std::mutex> lock(mWriteMutex);
    mFlush = true;
    mWriteCV.notify_one();
}

std::string BaseEngine::popErrors()
{
    std::string errorString = mErrorStream.str();
    mErrorStream.str("");
    return errorString;
}

template<typename F>
void RotationEngine::walkRotatedFiles(mega::LocalPath dir, const mega::LocalPath &baseFilename, F&& walker)
{
    const std::string baseFileStr = baseFilename.toName(mFsAccess);

    mega::LocalPath leafPath;
    mega::nodetype_t entryType;

    auto da = mFsAccess.newdiraccess();
    if (!da->dopen(&dir, nullptr, false))
    {
        mErrorStream << "Failed to walk directory " << dir.toName(mFsAccess) << std::endl;
        return;
    }

    while (da->dnext(dir, leafPath, false, &entryType))
    {
        // Ignore non-files
        if (entryType != mega::FILENODE) continue;

        // Ignore files that don't start with the base filename
        const std::string leafName = leafPath.toName(mFsAccess);
        if (leafName.rfind(baseFileStr, 0) != 0) continue;

        walker(dir, leafPath);
    }
}

void RotationEngine::cleanupFiles(const mega::LocalPath &dir, const mega::LocalPath &baseFilename)
{
    walkRotatedFiles(dir, baseFilename, [this] (const mega::LocalPath &dir, const mega::LocalPath &leafPath)
    {
        mega::LocalPath leafFullPath(dir);
        leafFullPath.appendWithSeparator(leafPath, false);

        if (!mFsAccess.unlinklocal(leafFullPath))
        {
            mErrorStream << "Error removing rotated file " << leafPath.toName(mFsAccess) << std::endl;
        }
    });
}

mega::LocalPath NumberedRotationEngine::getSrcFilePath(const mega::LocalPath &directory, const mega::LocalPath &baseFilename, int i) const
{
    mega::LocalPath filePath = directory;
    filePath.appendWithSeparator(baseFilename, false);

    // The source path for the base file does not have an index or compression extension
    if (i != 0)
    {
        filePath.append(mega::LocalPath::fromRelativePath("." + std::to_string(i) + mCompressionExt));
    }
    return filePath;
}

mega::LocalPath NumberedRotationEngine::getDstFilePath(const mega::LocalPath &directory, const mega::LocalPath &baseFilename, int i) const
{
    mega::LocalPath filePath = directory;
    filePath.appendWithSeparator(baseFilename, false);

    // The destination path for the base file has an index, but doesn't have a compression extension
    // (since the file still needs to be sent to the compression engine)
    filePath.append(mega::LocalPath::fromRelativePath("." + std::to_string(i + 1)));
    if (i != 0)
    {
        filePath.append(mega::LocalPath::fromRelativePath(mCompressionExt));
    }
    return filePath;
}

NumberedRotationEngine::NumberedRotationEngine(const std::string &compressionExt, int maxFilesToKeep) :
    mCompressionExt(compressionExt),
    mMaxFilesToKeep(maxFilesToKeep)
{
    // Numbered rotation does not support keeping unlimited files
    assert(mMaxFilesToKeep >= 0);
}

mega::LocalPath NumberedRotationEngine::rotateFiles(const mega::LocalPath &dir, const mega::LocalPath &baseFilename)
{
    for (int i = mMaxFilesToKeep - 1; i >= 0; --i)
    {
        auto srcFilePath = getSrcFilePath(dir, baseFilename, i);

        // Quietly ignore any numbered files that don't exist
        if (!mFsAccess.fileExistsAt(srcFilePath)) continue;

        // Delete the last file, which is the one with number == maxFilesToKeep
        if (i == mMaxFilesToKeep - 1)
        {
            if (!mFsAccess.unlinklocal(srcFilePath))
            {
                mErrorStream << "Error removing file " << srcFilePath.toPath(true) << std::endl;
            }
            continue;
        }

        // For the rest, rename them to effectively do number++ (e.g., file.3.gz => file.4.gz)
        auto dstFilePath = getDstFilePath(dir, baseFilename, i);
        if (!mFsAccess.renamelocal(srcFilePath, dstFilePath, true))
        {
            mErrorStream << "Error renaming file " << srcFilePath.toPath(true) << " to " << dstFilePath.toPath(true) << std::endl;
        }
    }

    // The newly-rotated file is the destination path of the base file
    return getDstFilePath(dir, baseFilename, 0);
}

TimestampRotationEngine::TimestampFile::TimestampFile(const mega::LocalPath &dir, const mega::LocalPath &filename, const Timestamp &_timestamp) :
    fullPath(dir),
    timestamp(_timestamp)
{
    fullPath.appendWithSeparator(filename, false);
}

std::string TimestampRotationEngine::timestampToString(const Timestamp &timestamp)
{
    std::ostringstream oss;
    std::time_t time = Clock::to_time_t(timestamp);
    oss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S_");

    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()) % 1000;
    oss << std::setfill('0') << std::setw(3) << milliseconds.count();

    return oss.str();
}

TimestampRotationEngine::Timestamp TimestampRotationEngine::stringToTimestamp(const std::string &timestampStr, bool &success)
{
    std::tm timeInfo = {};
    int milliseconds = -1;

    std::istringstream iss(timestampStr);
    iss >> std::get_time(&timeInfo, "%Y%m%d_%H%M%S_") >> milliseconds;

    success = milliseconds >= 0 && milliseconds < 1000 && !iss.fail();
    if (!success)
    {
        return {};
    }

    return Clock::from_time_t(std::mktime(&timeInfo)) + std::chrono::milliseconds(milliseconds);
}

mega::LocalPath TimestampRotationEngine::rotateBaseFile(const mega::LocalPath &directory, const mega::LocalPath &baseFilename)
{
    const std::string timestampStr = timestampToString(Clock::now());

    mega::LocalPath srcFilePath = directory;
    srcFilePath.appendWithSeparator(baseFilename, false);

    mega::LocalPath dstFilePath = srcFilePath;
    dstFilePath.append(mega::LocalPath::fromRelativePath("." + timestampStr));

    if (!mFsAccess.renamelocal(srcFilePath, dstFilePath, true))
    {
        mErrorStream << "Error renaming file " << srcFilePath.toPath(true) << " to " << dstFilePath.toPath(true) << std::endl;
    }

    return dstFilePath;
}

TimestampRotationEngine::TimestampFileQueue TimestampRotationEngine::getTimestampFileQueue(const mega::LocalPath &dir, const mega::LocalPath &baseFilename)
{
    TimestampFileQueue fileQueue;
    std::unordered_set<std::string> addedFiles;

    const std::string baseFilenameStr = baseFilename.toName(mFsAccess);

    walkRotatedFiles(dir, baseFilename, [this, &baseFilenameStr, &fileQueue, &addedFiles] (const mega::LocalPath &dir, const mega::LocalPath &leafPath)
    {
        static const std::string exampleTimestampStr = "19970907_193040_000"; // example timestamp to get the string size
        const std::string leafPathStr = leafPath.toName(mFsAccess);

        const std::string timestampStr = leafPathStr.substr(baseFilenameStr.size() + 1, exampleTimestampStr.size());

        bool success = false;
        Timestamp timestamp = stringToTimestamp(timestampStr, success);
        if (!success)
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
        fileQueue.emplace(dir, leafPath, timestamp);
    });

    return fileQueue;
}

void TimestampRotationEngine::popAndRemoveFile(TimestampFileQueue &fileQueue)
{
    const TimestampFile file = fileQueue.top();
    fileQueue.pop();

    if (!mFsAccess.unlinklocal(file.fullPath))
    {
        mErrorStream << "Error removing rotated file " << file.fullPath.toName(mFsAccess) << std::endl;
    }
}

TimestampRotationEngine::TimestampRotationEngine(int maxFilesToKeep, std::chrono::seconds maxFileAge) :
    mMaxFilesToKeep(maxFilesToKeep),
    mMaxFileAge(maxFileAge)
{
}

mega::LocalPath TimestampRotationEngine::rotateFiles(const mega::LocalPath &dir, const mega::LocalPath &baseFilename)
{
    auto newlyRotatedFile = rotateBaseFile(dir, baseFilename);

    if (mMaxFileAge <= std::chrono::seconds(0) && mMaxFilesToKeep < 0)
    {
        return newlyRotatedFile;
    }

    auto fileQueue = getTimestampFileQueue(dir, baseFilename);

    // Rotate by timestamp
    if (mMaxFileAge > std::chrono::seconds(0))
    {
        const Timestamp minFileTimestamp = Clock::now() - mMaxFileAge;
        while (!fileQueue.empty() && fileQueue.top().timestamp < minFileTimestamp)
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

GzipCompressionEngine::GzipJobData::GzipJobData(const mega::LocalPath &_srcFilePath, const mega::LocalPath &_dstFilePath) :
    valid(true),
    srcFilePath(_srcFilePath),
    dstFilePath(_dstFilePath)
{
}

bool GzipCompressionEngine::shouldCancelOngoingJob() const
{
    std::lock_guard<std::mutex> lock(mQueueMutex);
    return mCancelOngoingJob;
}

void GzipCompressionEngine::pushToQueue(const mega::LocalPath &srcFilePath, const mega::LocalPath &dstFilePath)
{
    std::lock_guard<std::mutex> lock(mQueueMutex);

    if (mExit)
    {
        return;
    }

    mQueue.emplace(srcFilePath, dstFilePath);
    mQueueCV.notify_one();
}

void GzipCompressionEngine::gzipFile(const mega::LocalPath &srcFilePath, const mega::LocalPath &dstFilePath)
{
    std::string srcFilePathStr = srcFilePath.toName(mFsAccess);
    std::ifstream file(srcFilePathStr, std::ofstream::out);
    if (!file)
    {
        mErrorStream << "Failed to open " << srcFilePathStr << " for compression" << std::endl;
        return;
    }

    std::string dstFilePathStr = dstFilePath.toName(mFsAccess);

    auto gzdeleter = [] (gzFile_s* f) { if (f) gzclose(f); };
    std::unique_ptr<gzFile_s, decltype(gzdeleter)> gzFile(gzopen(dstFilePathStr.c_str(), "wb"), gzdeleter);
    if (!gzFile)
    {
        mErrorStream << "Failed to open gzfile " << dstFilePathStr << " for writing" << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (shouldCancelOngoingJob())
        {
            return;
        }

        line += '\n';
        if (gzputs(gzFile.get(), line.c_str()) == -1)
        {
            mErrorStream << "Failed to gzip " << srcFilePathStr << std::endl;
            return;
        }
    }

    // Explicitly release the open file handle (otherwise the unlink below will fail)
    file.close();

    if (!mFsAccess.unlinklocal(srcFilePath))
    {
        mErrorStream << "Failed to remove temporary file " << srcFilePathStr << " after compression" << std::endl;
    }
}

void GzipCompressionEngine::mainLoop()
{
    while (true)
    {
        GzipJobData jobData;

        {
            std::unique_lock<std::mutex> lock(mQueueMutex);
            mQueueCV.wait(lock, [this] () { return mExit || !mQueue.empty(); });

            if (mExit && mQueue.empty())
            {
                return;
            }
            assert(!mQueue.empty());

            jobData = mQueue.front();
            mQueue.pop();

            mCancelOngoingJob = false;
        }

        assert(jobData.valid);
        gzipFile(jobData.srcFilePath, jobData.dstFilePath);
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
        std::lock_guard<std::mutex> lock(mQueueMutex);
        mExit = true;
        mQueueCV.notify_one();

        // We want to exit gracefull, so we don't call `cancelAll`
    }
    mGzipThread.join();
}

std::string GzipCompressionEngine::getExtension() const
{
    return ".gz";
}

void GzipCompressionEngine::cancelAll()
{
    std::lock_guard<std::mutex> lock(mQueueMutex);

    // Clear the queue
    mQueue = std::queue<GzipJobData>();

    // This flag will ensure `gzipFile` returns as soon as possible (if it's running)
    // It'll be unset the next time we pop an item from the queue
    mCancelOngoingJob = true;
}

void GzipCompressionEngine::compressFile(const mega::LocalPath &filePath)
{
    mega::LocalPath tmpFilePath(filePath);
    tmpFilePath.append(mega::LocalPath::fromRelativePath(".zipping"));

    // Ensure there is not a clashing .zipping file
    if (mFsAccess.fileExistsAt(tmpFilePath) && !mFsAccess.unlinklocal(tmpFilePath))
    {
        mErrorStream << "Failed to remove temporary compression file " << tmpFilePath.toPath(true) << std::endl;
        return;
    }

    if (!mFsAccess.renamelocal(filePath, tmpFilePath, true))
    {
        mErrorStream << "Failed to rename file " << filePath.toPath(true) << " to " << tmpFilePath.toPath(true) << std::endl;
        return;
    }

    mega::LocalPath targetFilePath(filePath);
    targetFilePath.append(mega::LocalPath::fromRelativePath(getExtension()));

    pushToQueue(tmpFilePath, targetFilePath);
}
}
