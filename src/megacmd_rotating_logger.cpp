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

bool FileRotatingLoggedStream::shouldForceRenew() const
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

void FileRotatingLoggedStream::flushToFile()
{
    {
        std::lock_guard<std::mutex> lock(mWriteMutex);
        mFlush = false;
    }
    mOutputFile.flush();
    mNextFlushTime = std::chrono::steady_clock::now() + mFlushPeriod;
}

void FileRotatingLoggedStream::writeToFileLoop()
{
    while (!shouldExit() || !mMessageBuffer.isEmpty())
    {
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
            auto memoryBlockList = mMessageBuffer.popMemoryBlockList();
            for (const auto& memoryBlock : memoryBlockList)
            {
                if (memoryBlock.isOutOfMemory())
                {
                    mOutputFile << "<log gap - out of logging memory at this point>\n";
                    continue;
                }

                mOutputFile << memoryBlock.getBuffer();
            }
        }

        if (shouldFlush())
        {
            flushToFile();
        }
    }
}

FileRotatingLoggedStream::FileRotatingLoggedStream(const std::string &outputFilePath) :
    mMessageBuffer(2048),
    mOutputFile(outputFilePath, std::ofstream::out | std::ofstream::app),
    mForceRenew(false), // maybe this should be true by default?
    mExit(false),
    mFlush(false),
    mFlushPeriod(std::chrono::seconds(10)),
    mNextFlushTime(std::chrono::steady_clock::now() + mFlushPeriod),
    mWriteThread([this] () { writeToFileLoop(); })
{
    mOutputFile << "----------------------------- program start -----------------------------\n";
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
