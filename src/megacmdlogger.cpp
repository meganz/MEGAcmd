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
    return (mThreadMap.find(id) != mThreadMap.end());
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

bool isMEGAcmdSource(const char *source)
{
    //TODO: this seem to be broken. source does not have the entire path but just leaf names
    return (string(source).find("src/megacmd") != string::npos)
            || (string(source).find("src\\megacmd") != string::npos)
            || (string(source).find("listeners.cpp") != string::npos)
            || (string(source).find("configurationmanager.cpp") != string::npos)
            || (string(source).find("comunicationsmanager") != string::npos)
            || (string(source).find("megacmd") != string::npos); // added this one
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

void MegaCmdSimpleLogger::log(const char *time, int loglevel, const char *source, const char *message)
{
    // If comming from this logger current thread
    bool outputIsAlreadyOUTSTREAM = &OUTSTREAM == &mLoggedStream;
    auto needsLoggingToClient = [outputIsAlreadyOUTSTREAM, loglevel](int defaultLogLevel)
    {
        if (outputIsAlreadyOUTSTREAM)
        {
            return false;
        }
        int currentThreadLogLevel = getCurrentThreadLogLevel();
        if (currentThreadLogLevel < 0) // this thread has no log level assigned
        {
            currentThreadLogLevel = defaultLogLevel; // use CMD's level
        }

        return loglevel <= currentThreadLogLevel;
    };

    const int sdkLoggerLevel = getSdkLoggerLevel();
    const int cmdLoggerLevel = getCmdLoggerLevel();

    if (isMEGAcmdSource(source))
    {
        if (loglevel <= cmdLoggerLevel)
        {
#ifdef _WIN32
            std::lock_guard<std::mutex> g(*outputmutex);
            int oldmode;
            oldmode = _setmode(_fileno(stdout), _O_U8TEXT);
            mLoggedStream << "[" << SimpleLogger::toStr(LogLevel(loglevel)) << ": " << time << "] " << message << endl;
            _setmode(_fileno(stdout), oldmode);
#else
            mLoggedStream << "[" << SimpleLogger::toStr(LogLevel(loglevel)) << ": " << time << "] " << message << endl;
#endif
        }

        if (needsLoggingToClient(cmdLoggerLevel))
        {
            OUTSTREAM << "[" << SimpleLogger::toStr(LogLevel(loglevel)) << ": " << time << "] " << message << endl;
        }
    }
    else // SDK's
    {
        if (loglevel <= sdkLoggerLevel)
        {
            if (( sdkLoggerLevel <= MegaApi::LOG_LEVEL_DEBUG ) && !strcmp(message, "Request (RETRY_PENDING_CONNECTIONS) starting"))
            {
                return;
            }
            if (( sdkLoggerLevel <= MegaApi::LOG_LEVEL_DEBUG ) && !strcmp(message, "Request (RETRY_PENDING_CONNECTIONS) finished"))
            {
                return;
            }
#ifdef _WIN32
            std::lock_guard<std::mutex> g(*outputmutex);
            int oldmode;
            oldmode = _setmode(_fileno(stdout), _O_U8TEXT);
            mLoggedStream << "[API:" << SimpleLogger::toStr(LogLevel(loglevel)) << ": " << time << "] " << message << endl;
            _setmode(_fileno(stdout), oldmode);
#else
            mLoggedStream << "[API:" << SimpleLogger::toStr(LogLevel(loglevel)) << ": " << time << "] " << message << endl;
#endif
        }

        if (needsLoggingToClient(sdkLoggerLevel))
        {
            assert(false); //since it happens in the sdk thread, this shall be false
            OUTSTREAM << "[API:" << SimpleLogger::toStr(LogLevel(loglevel)) << ": " << time << "] " << message << endl;
        }
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
