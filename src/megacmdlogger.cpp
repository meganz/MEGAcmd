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

#include <map>

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

using namespace mega;

namespace megacmd {

ThreadLookupTable::ThreadData::ThreadData() :
    outStream(&Instance<DefaultLoggedStream>::Get().getLoggedStream()),
    logLevel(-1),
    outCode(0),
    cmdPetition(nullptr),
    isCmdShell(false)
{
}

ThreadLookupTable::ThreadData ThreadLookupTable::getThreadData(uint64_t id) const
{
    std::lock_guard<std::mutex> lock(mMapMutex);
    auto it = mThreadMap.find(id);
    if (it == mThreadMap.end())
    {
        return {};
    }
    return it->second;
}

ThreadLookupTable::ThreadData ThreadLookupTable::getCurrentThreadData() const
{
    return getThreadData(MegaThread::currentThreadId());
}

bool ThreadLookupTable::threadDataExists(uint64_t id) const
{
    std::lock_guard<std::mutex> lock(mMapMutex);
    return mThreadMap.find(id) != mThreadMap.end();
}

LoggedStream& ThreadLookupTable::getCurrentOutStream() const
{
    return *getCurrentThreadData().outStream;
}

int ThreadLookupTable::getCurrentLogLevel() const
{
    return getCurrentThreadData().logLevel;
}

int ThreadLookupTable::getCurrentOutCode() const
{
    return getCurrentThreadData().outCode;
}

CmdPetition* ThreadLookupTable::getCurrentCmdPetition() const
{
    return getCurrentThreadData().cmdPetition;
}

bool ThreadLookupTable::isCurrentCmdShell() const
{
    return getCurrentThreadData().isCmdShell;
}

void ThreadLookupTable::setCurrentOutStream(LoggedStream &outStream)
{
    std::lock_guard<std::mutex> lock(mMapMutex);
    mThreadMap[MegaThread::currentThreadId()].outStream = &outStream;
}

void ThreadLookupTable::setCurrentLogLevel(int logLevel)
{
    std::lock_guard<std::mutex> lock(mMapMutex);
    mThreadMap[MegaThread::currentThreadId()].logLevel = logLevel;
}

void ThreadLookupTable::setCurrentOutCode(int outCode)
{
    std::lock_guard<std::mutex> lock(mMapMutex);
    mThreadMap[MegaThread::currentThreadId()].outCode = outCode;
}

void ThreadLookupTable::setCurrentCmdPetition(CmdPetition *cmdPetition)
{
    std::lock_guard<std::mutex> lock(mMapMutex);
    mThreadMap[MegaThread::currentThreadId()].cmdPetition = cmdPetition;
}

void ThreadLookupTable::setCurrentIsCmdShell(bool isCmdShell)
{
    std::lock_guard<std::mutex> lock(mMapMutex);
    mThreadMap[MegaThread::currentThreadId()].isCmdShell = isCmdShell;
}

bool ThreadLookupTable::isCurrentThreadInteractive() const
{
    if (isCurrentCmdShell())
    {
        return true;
    }
    return !threadDataExists(MegaThread::currentThreadId());
}

const char* ThreadLookupTable::getModeCommandPrefix() const
{
    if (isCurrentThreadInteractive())
    {
        return "";
    }
    return "mega-";
}

MegaCmdLogger::MegaCmdLogger() :
    mSdkLoggerLevel(mega::MegaApi::LOG_LEVEL_ERROR),
    mCmdLoggerLevel(mega::MegaApi::LOG_LEVEL_ERROR)
{
}

bool MegaCmdLogger::isMegaCmdSource(const std::string &source)
{
    //TODO: this seem to be broken. source does not have the entire path but just leaf names
    return source.find("src/megacmd") != string::npos ||
           source.find("src\\megacmd") != string::npos ||
           source.find("listeners.cpp") != string::npos ||
           source.find("configurationmanager.cpp") != string::npos ||
           source.find("comunicationsmanager") != string::npos ||
           source.find("megacmd") != string::npos;
}

void MegaCmdLogger::formatLogToStream(LoggedStream &stream, const char *time, int logLevel, const char *source, const char *message)
{
    stream << "[";
    if (!isMegaCmdSource(source))
    {
        stream << "API:";
    }
    stream << SimpleLogger::toStr(LogLevel(logLevel)) << ": " << time << "] " << message << endl;
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

MegaCmdSimpleLogger::MegaCmdSimpleLogger() :
    MegaCmdLogger(),
    mLoggedStream(Instance<DefaultLoggedStream>::Get().getLoggedStream())
{
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
#ifdef _WIN32
        std::lock_guard<std::mutex> g(mOutputMutex);
        int oldmode = _setmode(_fileno(stdout), _O_U8TEXT);
#endif
        formatLogToStream(mLoggedStream, time, logLevel, source, message);

#ifdef _WIN32
        _setmode(_fileno(stdout), oldmode);
#endif
    }

    if (shouldLogToClient(logLevel, source))
    {
        assert(isMegaCmdSource(source)); // if this happens in the sdk thread, this shall be false
        formatLogToStream(OUTSTREAM, time, logLevel, source, message);
    }
}

OUTFSTREAMTYPE streamForDefaultFile()
{
    //TODO: get this one from new dirs folders utilities (pending CMD-307) and refactor the .log retrieval

    OUTSTRING path;
#ifdef _WIN32

    TCHAR szPath[MAX_PATH];
     if (!SUCCEEDED(GetModuleFileName(NULL, szPath , MAX_PATH)))
     {
         LOG_fatal << "Couldnt get EXECUTABLE folder";
     }
     else
     {
         if (SUCCEEDED(PathRemoveFileSpec(szPath)))
         {
             if (PathAppend(szPath,TEXT(".megaCmd")))
             {
                 if (PathAppend(szPath,TEXT("megacmdserver.log")))
                 {
                     path = szPath;
                 }
             }
         }
     }
#else
    const char* home = getenv("HOME");
    if (home)
    {
        path.append(home);
        path.append("/.megaCmd/");
        path.append("/megacmdserver.log");
    }
#endif

    return OUTFSTREAMTYPE(path);
}

LoggedStreamDefaultFile::LoggedStreamDefaultFile() :
    mFstream(streamForDefaultFile()),
    LoggedStreamOutStream (&mFstream)
{
}

}//end namespace
