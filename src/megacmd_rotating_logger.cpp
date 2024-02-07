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

class ArchiveEngine
{
protected:
    OUTSTRINGSTREAM mErrorStream;

    const std::string mArchiveExt;
    mega::MegaFileSystemAccess mFsAccess;

public:
    std::string popErrors()
    {
        std::string errorString = mErrorStream.str();
        mErrorStream.str("");
        return errorString;
    }

    ArchiveEngine(const std::string &archiveExtension) :
        mArchiveExt(archiveExtension)
    {
    }

    virtual ~ArchiveEngine() = default;

    virtual void cleanupFiles(const mega::LocalPath &dir, const mega::LocalPath &baseFilename)
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

    virtual void rotateFiles(const mega::LocalPath &dir, const mega::LocalPath &baseFilename) = 0;
};

class NumberedArchiveEngine final : public ArchiveEngine
{
    const int mMaxFilesToKeep;

    mega::LocalPath getFilePath(const mega::LocalPath &directory, const mega::LocalPath &baseFilename, int i)
    {
        mega::LocalPath filePath = directory;
        filePath.appendWithSeparator(baseFilename, false);

        // The "zero file" does not have an index or an archive extension
        if (i != 0)
        {
            filePath.append(mega::LocalPath::fromRelativePath("." + std::to_string(i)));

            // The first rotated file (which comes from the zero file), does not have an archive
            // extension, since for that it first needs to be sent over to the compression queue
            if (i != 1)
            {
                filePath.append(mega::LocalPath::fromRelativePath(mArchiveExt));
            }
        }
        return filePath;
    }

public:
    NumberedArchiveEngine(const std::string &archiveExt, int maxFilesToKeep) :
        ArchiveEngine(archiveExt),
        mMaxFilesToKeep(maxFilesToKeep)
    {
        // Numbered archive does not support keeping unlimited files
        assert(mMaxFilesToKeep >= 0);
    }

    void rotateFiles(const mega::LocalPath &dir, const mega::LocalPath &baseFilePath) override
    {
        for (int i = mMaxFilesToKeep - 1; i >= 0; --i)
        {
            auto oldFilePath = getFilePath(dir, baseFilePath, i);

            // Quietly ignore any numbered files that don't exist
            if (!mFsAccess.fileExistsAt(oldFilePath)) continue;

            // Delete the last file, which is the one with number == maxFilesToKeep
            if (i == mMaxFilesToKeep - 1)
            {
                if (!mFsAccess.unlinklocal(oldFilePath))
                {
                    mErrorStream << "Error removing arhive file " << oldFilePath.toPath(true) << std::endl;
                }
                continue;
            }

            // For the rest, rename them to effectively do number++ (e.g., file.3.gz => file.4.gz)
            auto newFilePath = getFilePath(dir, baseFilePath, i + 1);
            if (!mFsAccess.renamelocal(oldFilePath, newFilePath, true))
            {
                mErrorStream << "Error renaming archive file " << oldFilePath.toPath(true) << std::endl;
            }
        }
    }
};

class TimestampArchiveEngine final : public ArchiveEngine
{
    const int mMaxFilesToKeep;
    const std::chrono::seconds mMaxFileAge;
public:
    TimestampArchiveEngine(const std::string &archiveExt, int maxFilesToKeep, std::chrono::seconds maxFileAge) :
        ArchiveEngine(archiveExt),
        mMaxFilesToKeep(maxFilesToKeep),
        mMaxFileAge(maxFileAge)
    {
        assert(false); // not yet implemented
    }

    void rotateFiles(const mega::LocalPath &dir, const mega::LocalPath &baseFilename) override
    {
    }
};

RotatingFileManager::Config::Config() :
    maxBaseFileSize(50 * 1024 * 1024), // 50 MB
    archiveType(ArchiveType::Numbered),
    maxArchiveAge(30 * 86400), // one month (in C++20 we can use `std::chrono::month(1)`)
    maxArchivesToKeep(50)
{
}

RotatingFileManager::RotatingFileManager(const std::string &filePath, const Config &config) :
    mConfig(config),
    mDirectory(mega::LocalPath::fromAbsolutePath(filePath).parentPath()),
    mBaseFilename(mega::LocalPath::fromAbsolutePath(filePath).leafName())
{
    constexpr const char* archiveExt = "";

    ArchiveEngine* archiveEngine = nullptr;
    switch (mConfig.archiveType)
    {
        case ArchiveType::Numbered:
        {
            archiveEngine = new NumberedArchiveEngine(archiveExt, mConfig.maxArchivesToKeep);
            break;
        }
        case ArchiveType::Timestamp:
        {
            archiveEngine = new TimestampArchiveEngine(archiveExt, mConfig.maxArchivesToKeep, mConfig.maxArchiveAge);
            break;
        }
    }
    assert(archiveEngine);
    mArchiveEngine = std::unique_ptr<ArchiveEngine>(archiveEngine);
}

bool RotatingFileManager::shouldRotateFile(size_t fileSize) const
{
    return fileSize > mConfig.maxBaseFileSize;
}

void RotatingFileManager::cleanupFiles()
{
    mArchiveEngine->cleanupFiles(mDirectory, mBaseFilename);
}

void RotatingFileManager::rotateFiles()
{
    mArchiveEngine->rotateFiles(mDirectory, mBaseFilename);
}

std::string RotatingFileManager::popErrors()
{
    return mArchiveEngine->popErrors();
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
        else if (mFileManager.shouldRotateFile(outFileSize))
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
}
