/**
 * @file src/megacmdlogger.cpp
 * @brief MEGAcmd: Controls message logging
 *
 * (c) 2013 by Mega Limited, Auckland, New Zealand
 *
 * This file is part of the MEGAcmd.
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

#include "megacmdlogger.h"
#include "megacmdcommonutils.h"
#include "megacmd_src_file_list.h"

#include <map>
#include <filesystem>

#include <sys/types.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#ifndef _O_U16TEXT
#define _O_U16TEXT 0x00020000
#endif
#ifndef _O_U8TEXT
#define _O_U8TEXT 0x00040000
#endif
#endif

namespace fs = std::filesystem;
using namespace mega;

namespace megacmd {

namespace {
    thread_local ThreadData threadData;
    thread_local bool isThreadDataSet = false;
}

ThreadData &getCurrentThreadData()
{
    return threadData;
}

const char* getCommandPrefixBasedOnMode()
{
    if (isCurrentThreadInteractive())
    {
        return "";
    }
    return "mega-";
}

bool isCurrentThreadInteractive()
{
    return isCurrentThreadCmdShell() || !isThreadDataSet;
}

void setCurrentThreadOutStream(LoggedStream &outStream)
{
    isThreadDataSet = true;
    getCurrentThreadData().mOutStream = &outStream;
}

void setCurrentThreadOutCode(int outCode)
{
    isThreadDataSet = true;
    getCurrentThreadData().mOutCode = outCode;
}

void setCurrentThreadLogLevel(int logLevel)
{
    isThreadDataSet = true;
    getCurrentThreadData().mLogLevel = logLevel;
}

void setCurrentThreadCmdPetition(CmdPetition *cmdPetition)
{
    isThreadDataSet = true;
    getCurrentThreadData().mCmdPetition = cmdPetition;
}

void setCurrentThreadIsCmdShell(bool isCmdShell)
{
    isThreadDataSet = true;
    getCurrentThreadData().mIsCmdShell = isCmdShell;
}

MegaCmdLogger::MegaCmdLogger() :
    mSdkLoggerLevel(mega::MegaApi::LOG_LEVEL_ERROR),
    mCmdLoggerLevel(mega::MegaApi::LOG_LEVEL_ERROR),
    mFlushOnLevel(mega::MegaApi::LOG_LEVEL_WARNING)
{
}

OUTSTRING MegaCmdLogger::getDefaultFilePath()
{
    auto dirs = PlatformDirectories::getPlatformSpecificDirectories();
    assert(!dirs->configDirPath().empty());

    auto logFilePath = fs::path(dirs->configDirPath()) / "megacmdserver.log";
    return logFilePath.native();
}

bool MegaCmdLogger::isMegaCmdSource(const std::string &source)
{
    static const std::set<std::string_view> megaCmdSourceFiles = MEGACMD_SRC_FILE_LIST;

    // Remove the line number (since source has the format "filename.cpp:1234")
    std::string_view filename(source);
    filename = filename.substr(0, filename.find(':'));

    return megaCmdSourceFiles.find(filename) != megaCmdSourceFiles.end();
}

void MegaCmdLogger::formatLogToStream(LoggedStream &stream, const char *time, int logLevel, const char *source, const char *message)
{
    stream << "[";
    if (!isMegaCmdSource(source))
    {
        stream << "SDK:";
    }
    stream << SimpleLogger::toStr(LogLevel(logLevel)) << ": " << time << "] " << message << '\n';

    if (logLevel <= mFlushOnLevel)
    {
        stream.flush();
    }
}

bool MegaCmdLogger::shouldIgnoreMessage(int logLevel, const char *source, const char *message) const
{
    UNUSED(logLevel);

    if (!isMegaCmdSource(source))
    {
        const int sdkLoggerLevel = getSdkLoggerLevel();
        if (sdkLoggerLevel <= MegaApi::LOG_LEVEL_DEBUG && !strcmp(message, "Request (RETRY_PENDING_CONNECTIONS) starting"))
        {
            return true;
        }
        if (sdkLoggerLevel <= MegaApi::LOG_LEVEL_DEBUG && !strcmp(message, "Request (RETRY_PENDING_CONNECTIONS) finished"))
        {
            return true;
        }
    }
    return false;
}

bool MegaCmdSimpleLogger::shouldLogToStream(int logLevel, const char* source) const
{
    if (isMegaCmdSource(source))
    {
        return logLevel <= getCmdLoggerLevel();
    }
    return logLevel <= getSdkLoggerLevel();
}

bool MegaCmdSimpleLogger::shouldLogToClient(int logLevel, const char* source) const
{
    // If comming from this logger current thread
    if (&OUTSTREAM == &mLoggedStream)
    {
        return false;
    }

    const int defaultLogLevel = isMegaCmdSource(source) ? getCmdLoggerLevel() : getSdkLoggerLevel();

    int currentThreadLogLevel = getCurrentThreadLogLevel();
    if (currentThreadLogLevel < 0) // this thread has no log level assigned
    {
        currentThreadLogLevel = defaultLogLevel;
    }

    return logLevel <= currentThreadLogLevel;
}

MegaCmdSimpleLogger::MegaCmdSimpleLogger(bool logToOutStream, int sdkLoggerLevel, int cmdLoggerLevel) :
    MegaCmdLogger(),
    mLoggedStream(Instance<DefaultLoggedStream>::Get().getLoggedStream()),
    mOutStream(&COUT),
    mLogToOutStream(logToOutStream)
{
    setSdkLoggerLevel(sdkLoggerLevel);
    setCmdLoggerLevel(cmdLoggerLevel);
}

int MegaCmdSimpleLogger::getMaxLogLevel() const
{
    return std::max(getCurrentThreadLogLevel(), MegaCmdLogger::getMaxLogLevel());
}

void MegaCmdSimpleLogger::log(const char *time, int logLevel, const char *source, const char *message)
{
    if (shouldIgnoreMessage(logLevel, source, message))
    {
        return;
    }

    if (shouldLogToStream(logLevel, source))
    {
        formatLogToStream(mLoggedStream, time, logLevel, source, message);

        if (mLogToOutStream)
        {
#ifdef _WIN32
            std::lock_guard<std::mutex> g(mSetmodeMtx);
            int oldmode = _setmode(_fileno(stdout), _O_U8TEXT);
#endif
            formatLogToStream(mOutStream, time, logLevel, source, message);

#ifdef _WIN32
            assert(oldmode != -1);
            _setmode(_fileno(stdout), oldmode);
#endif
        }
    }

    if (shouldLogToClient(logLevel, source))
    {
        assert(isMegaCmdSource(source)); // if this happens in the sdk thread, this shall be false
        formatLogToStream(OUTSTREAM, time, logLevel, source, message);
    }
}

LoggedStreamDefaultFile::LoggedStreamDefaultFile() :
    LoggedStreamOutStream(nullptr),
    mFstream(MegaCmdLogger::getDefaultFilePath())
{
    out = &mFstream;
    if (!mFstream.is_open())
    {
        CERR << "Cannot open default log file " << MegaCmdLogger::getDefaultFilePath() << std::endl;
    }
}

}//end namespace
