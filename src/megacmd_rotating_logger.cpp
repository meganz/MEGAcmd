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
    mOutOfMemory(mBuffer == nullptr)
{
    assert(mCapacity > 0);
}

bool MessageBuffer::MemoryBlock::canAppendData(size_t dataSize) const
{
    // The last byte of buffer is reserved for '\0'
    return !mOutOfMemory && mSize + dataSize < mCapacity;
}

bool MessageBuffer::MemoryBlock::isNearCapacity() const
{
    return mSize + mCapacity/8 > mCapacity;
}

bool MessageBuffer::MemoryBlock::isOutOfMemory() const
{
    return mOutOfMemory;
}

const char *MessageBuffer::MemoryBlock::getBuffer() const
{
    return mBuffer.get();
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
    mDefaultBlockCapacity(defaultBlockCapacity)
{
}

void MessageBuffer::append(const char *data, size_t size)
{
    std::lock_guard<std::mutex> lock(mListMutex);

    auto* lastBlock = mList.empty() ? nullptr : &mList.back();
    if (!lastBlock || !lastBlock->canAppendData(size))
    {
        mList.emplace_back(mDefaultBlockCapacity);
        lastBlock = &mList.back();
    }

    if (lastBlock->canAppendData(size))
    {
        lastBlock->appendData(data, size);
    }
}

MessageBuffer::MemoryBlockList MessageBuffer::popMemoryBlockList()
{
    std::lock_guard<std::mutex> lock(mListMutex);
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
    OUTSTRINGSTREAM mErrorStream;
    mega::MegaFileSystemAccess mFsAccess;

public:
    std::string popErrors();
};

class RotationEngine : public BaseEngine
{
protected:
    const std::string mCompressionExt;

public:
    RotationEngine(const std::string &compressionExt);
    virtual ~RotationEngine() = default;

    virtual void cleanupFiles(const mega::LocalPath &dir, const mega::LocalPath &baseFilename);
    virtual mega::LocalPath rotateFiles(const mega::LocalPath &dir, const mega::LocalPath &baseFilename) = 0;
};

class NumberedRotationEngine final : public RotationEngine
{
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

public:
    TimestampRotationEngine(const std::string &archiveExt, int maxFilesToKeep, std::chrono::seconds maxFileAge);

    mega::LocalPath rotateFiles(const mega::LocalPath &dir, const mega::LocalPath &baseFilename) override;
};

class CompressionEngine : public BaseEngine
{
public:
    virtual ~CompressionEngine() = default;

    virtual std::string getExtension() const { return ""; }

    virtual void waitForAll() {}
    virtual void cancelAll() {}
    virtual void compressFile(const mega::LocalPath &filePath) {}
};

class GzipCompressionEngine final : public CompressionEngine
{
    mega::MegaFileSystemAccess mFsAccess;

    void gzipFile(const mega::LocalPath &srcFilePath, const mega::LocalPath &destFilePath);

public:
    std::string getExtension() const override;

    void waitForAll() override;
    void cancelAll() override;
    void compressFile(const mega::LocalPath &filePath);
};

RotatingFileManager::Config::Config() :
    maxBaseFileSize(50 * 1024 * 1024), // 50 MB
    rotationType(RotationType::Numbered),
    maxFileAge(30 * 86400), // one month (in C++20 we can use `std::chrono::month(1)`)
    maxFilesToKeep(50),
    compressionType(CompressionType::Gzip)
{
}

RotatingFileManager::RotatingFileManager(const std::string &filePath, const Config &config) :
    mConfig(config),
    mDirectory(mega::LocalPath::fromAbsolutePath(filePath).parentPath()),
    mBaseFilename(mega::LocalPath::fromAbsolutePath(filePath).leafName())
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
    mCompressionEngine->waitForAll();

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
            rotationEngine = new TimestampRotationEngine(compressionExt, mConfig.maxFilesToKeep, mConfig.maxFileAge);
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
    auto memoryBlockList = mMessageBuffer.popMemoryBlockList();
    for (const auto& memoryBlock : memoryBlockList)
    {
        if (!memoryBlock.isOutOfMemory())
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
    {
        std::lock_guard<std::mutex> lock(mWriteMutex);
        mFlush = false;
    }
    mOutputFile.flush();
    mNextFlushTime = std::chrono::steady_clock::now() + mFlushPeriod;
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

FileRotatingLoggedStream::FileRotatingLoggedStream(const std::string &outputFilePath) :
    mMessageBuffer(2048),
    mOutputFilePath(outputFilePath),
    mOutputFile(outputFilePath, std::ofstream::out | std::ofstream::app),
    mFileManager(outputFilePath),
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

RotationEngine::RotationEngine(const std::string &compressionExt) :
    mCompressionExt(compressionExt)
{
}

void RotationEngine::cleanupFiles(const mega::LocalPath &dir, const mega::LocalPath &baseFilename)
{
    const std::string baseFileStr = baseFilename.toName(mFsAccess);

    mega::LocalPath dirPathCopy(dir);
    mega::LocalPath leafPath;
    mega::nodetype_t entryType;

    auto da = mFsAccess.newdiraccess();
    da->dopen(&dirPathCopy, nullptr, false);

    while (da->dnext(dirPathCopy, leafPath, false, &entryType))
    {
        // Ignore non-files
        if (entryType != mega::FILENODE) continue;

        // Ignore files that don't start with the base filename
        const std::string leafName = leafPath.toName(mFsAccess);
        if (leafName.rfind(baseFileStr, 0) != 0) continue;

        mega::LocalPath leafFullPath(dir);
        leafFullPath.appendWithSeparator(leafPath, false);

        if (!mFsAccess.unlinklocal(leafFullPath))
        {
            mErrorStream << "Error removing archive file " << leafName << std::endl;
        }
    }
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
    RotationEngine(compressionExt),
    mMaxFilesToKeep(maxFilesToKeep)
{
    // Numbered archive does not support keeping unlimited files
    assert(mMaxFilesToKeep >= 0);
}

mega::LocalPath NumberedRotationEngine::rotateFiles(const mega::LocalPath &dir, const mega::LocalPath &baseFilePath)
{
    for (int i = mMaxFilesToKeep - 1; i >= 0; --i)
    {
        auto srcFilePath = getSrcFilePath(dir, baseFilePath, i);

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
        auto dstFilePath = getDstFilePath(dir, baseFilePath, i);
        if (!mFsAccess.renamelocal(srcFilePath, dstFilePath, true))
        {
            mErrorStream << "Error renaming file " << srcFilePath.toPath(true) << " to " << dstFilePath.toPath(true) << std::endl;
        }
    }

    // The newly-rotated file is the destination path of the base file
    return getDstFilePath(dir, baseFilePath, 0);
}

TimestampRotationEngine::TimestampRotationEngine(const std::string &archiveExt, int maxFilesToKeep, std::chrono::seconds maxFileAge) :
    RotationEngine(archiveExt),
    mMaxFilesToKeep(maxFilesToKeep),
    mMaxFileAge(maxFileAge)
{
    assert(false); // not yet implemented
}

mega::LocalPath TimestampRotationEngine::rotateFiles(const mega::LocalPath &dir, const mega::LocalPath &baseFilePath)
{
    return mega::LocalPath::fromRelativePath("");
}

void GzipCompressionEngine::gzipFile(const mega::LocalPath &srcFilePath, const mega::LocalPath &destFilePath)
{
    auto srcFilePathStr = srcFilePath.platformEncoded();
    std::ifstream file(srcFilePathStr, std::ofstream::out);
    if (!file)
    {
        mErrorStream << "Failed to open " << srcFilePathStr << " for compression" << std::endl;
        return;
    }

    auto destFilePathStr = destFilePath.platformEncoded();

    auto gzdeleter = [] (gzFile_s* f) { if (f) gzclose(f); };
    std::unique_ptr<gzFile_s, decltype(gzdeleter)> gzFile(gzopen(destFilePathStr.c_str(), "wb"), gzdeleter);
    if (!gzFile)
    {
        mErrorStream << "Failed to open gzfile " << destFilePathStr << " for writing" << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line))
    {
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

std::string GzipCompressionEngine::getExtension() const
{
    return ".gz";
}

void GzipCompressionEngine::waitForAll()
{
}

void GzipCompressionEngine::cancelAll()
{
}

void GzipCompressionEngine::compressFile(const mega::LocalPath &filePath)
{
    mega::LocalPath tmpFilePath(filePath);
    tmpFilePath.append(mega::LocalPath::fromRelativePath(".zipping"));

    // Ensure there is not a clashing .zipping file
    if (!mFsAccess.unlinklocal(tmpFilePath) && errno != ENOENT /* ignore if file doesn't exist */)
    {
        mErrorStream << "Failed to unlink temporary compression file " << tmpFilePath.toPath(true) << std::endl;
        return;
    }

    if (!mFsAccess.renamelocal(filePath, tmpFilePath, true))
    {
        mErrorStream << "Failed to rename file " << filePath.toPath(true) << " to " << tmpFilePath.toPath(true) << std::endl;
        return;
    }

    mega::LocalPath targetFilePath(filePath);
    targetFilePath.append(mega::LocalPath::fromRelativePath(getExtension()));

    gzipFile(tmpFilePath, targetFilePath);
}

}
