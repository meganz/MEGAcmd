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
// different outstreams for every thread. to gather all the output data
std::mutex threadLookups;
map<uint64_t, LoggedStream *> outstreams;
map<uint64_t, int> threadLogLevel;
map<uint64_t, int> threadoutCode;
map<uint64_t, CmdPetition *> threadpetition;
map<uint64_t, bool> threadIsCmdShell;

LoggedStream &getCurrentOut()
{
    std::lock_guard<std::mutex> g(threadLookups);
    uint64_t currentThread = MegaThread::currentThreadId();
    if (outstreams.find(currentThread) == outstreams.end())
    {
        return Instance<DefaultLoggedStream>::Get().getLoggedStream();
    }
    else
    {
        return *outstreams[currentThread];
    }
}

bool interactiveThread()
{
    if (getCurrentThreadIsCmdShell())
    {
        return true;
    }

    unsigned long long currentThread = MegaThread::currentThreadId();

    std::lock_guard<std::mutex> g(threadLookups);
    if (outstreams.find(currentThread) == outstreams.end())
    {
        return true;
    }
    else
    {
        return false;
    }
}

const char *commandPrefixBasedOnMode()
{
    if (interactiveThread())
    {
        return "";
    }
    else
    {
        return "mega-";
    }
}

int getCurrentOutCode()
{
    unsigned long long currentThread = MegaThread::currentThreadId();

    std::lock_guard<std::mutex> g(threadLookups);
    if (threadoutCode.find(currentThread) == threadoutCode.end())
    {
        return 0; //default OK
    }
    else
    {
        return threadoutCode[currentThread];
    }
}


CmdPetition * getCurrentPetition()
{
    unsigned long long currentThread = MegaThread::currentThreadId();

    std::lock_guard<std::mutex> g(threadLookups);
    if (threadpetition.find(currentThread) == threadpetition.end())
    {
        return NULL;
    }
    else
    {
        return threadpetition[currentThread];
    }
}

int getCurrentThreadLogLevel()
{
    unsigned long long currentThread = MegaThread::currentThreadId();

    std::lock_guard<std::mutex> g(threadLookups);
    if (threadLogLevel.find(currentThread) == threadLogLevel.end())
    {
        return -1;
    }
    else
    {
        return threadLogLevel[currentThread];
    }
}

bool getCurrentThreadIsCmdShell()
{
    unsigned long long currentThread = MegaThread::currentThreadId();

    std::lock_guard<std::mutex> g(threadLookups);
    if (threadIsCmdShell.find(currentThread) == threadIsCmdShell.end())
    {
        return false; //default not
    }
    else
    {
        return threadIsCmdShell[currentThread];
    }
}

void setCurrentThreadLogLevel(int level)
{
    std::lock_guard<std::mutex> g(threadLookups);
    threadLogLevel[MegaThread::currentThreadId()] = level;
}

void setCurrentThreadOutStream(LoggedStream *s)
{
    std::lock_guard<std::mutex> g(threadLookups);
    outstreams[MegaThread::currentThreadId()] = s;
}

void setCurrentThreadIsCmdShell(bool isit)
{
    std::lock_guard<std::mutex> g(threadLookups);
    threadIsCmdShell[MegaThread::currentThreadId()] = isit;
}

void setCurrentOutCode(int outCode)
{
    std::lock_guard<std::mutex> g(threadLookups);
    threadoutCode[MegaThread::currentThreadId()] = outCode;
}

void setCurrentPetition(CmdPetition *petition)
{
    std::lock_guard<std::mutex> g(threadLookups);
    threadpetition[MegaThread::currentThreadId()] = petition;
}


MegaCMDLogger::MegaCMDLogger(int sdkLoggerLevel, int cmdLoggerLevel) :
    mSdkLoggerLevel(sdkLoggerLevel),
    mCmdLoggerLevel(cmdLoggerLevel),
    mLoggedStream(Instance<DefaultLoggedStream>::Get().getLoggedStream())
{
}

bool isMEGAcmdSource(const char *source)
{
    //TODO: this seem to be broken. source does not have the entire path but just leaf names
    const string source_str(source);
    return (source_str.find("src/megacmd") != string::npos)
            || (source_str.find("src\\megacmd") != string::npos)
            || (source_str.find("listeners.cpp") != string::npos)
            || (source_str.find("configurationmanager.cpp") != string::npos)
            || (source_str.find("comunicationsmanager") != string::npos)
            || (source_str.find("megacmd") != string::npos) // added this one
            || (source_str.find("sync_ignore.cpp") != string::npos);
}

void MegaCMDLogger::log(const char *time, int loglevel, const char *source, const char *message)
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

    if (isMEGAcmdSource(source))
    {
        if (loglevel <= mCmdLoggerLevel)
        {
            performSafeLog(mLoggedStream, [this, time, loglevel, message]
            {
                mLoggedStream << "[" << SimpleLogger::toStr(LogLevel(loglevel)) << ": " << time << "] " << message << endl;
            });
        }

        if (needsLoggingToClient(mCmdLoggerLevel))
        {
            OUTSTREAM << "[" << SimpleLogger::toStr(LogLevel(loglevel)) << ": " << time << "] " << message << endl;
        }
    }
    else // SDK's
    {
        if (loglevel <= mSdkLoggerLevel)
        {
            if (( mSdkLoggerLevel <= MegaApi::LOG_LEVEL_DEBUG ) && !strcmp(message, "Request (RETRY_PENDING_CONNECTIONS) starting"))
            {
                return;
            }
            if (( mSdkLoggerLevel <= MegaApi::LOG_LEVEL_DEBUG ) && !strcmp(message, "Request (RETRY_PENDING_CONNECTIONS) finished"))
            {
                return;
            }

            performSafeLog(mLoggedStream, [this, time, loglevel, message]
            {
                mLoggedStream << "[SDK:" << SimpleLogger::toStr(LogLevel(loglevel)) << ": " << time << "] " << message << endl;
            });
        }

        if (needsLoggingToClient(mSdkLoggerLevel))
        {
            assert(false); //since it happens in the sdk thread, this shall be false
            OUTSTREAM << "[SDK:" << SimpleLogger::toStr(LogLevel(loglevel)) << ": " << time << "] " << message << endl;
        }
    }
}

int MegaCMDLogger::getMaxLogLevel() const
{
    return max(max(getCurrentThreadLogLevel(), mCmdLoggerLevel), mSdkLoggerLevel);
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
    auto dirs = PlatformDirectories::getPlatformSpecificDirectories();
    path = dirs->configDirPath();

    auto fsAccess = std::make_unique<MegaFileSystemAccess>();
    fsAccess->setdefaultfolderpermissions(0700);
    LocalPath localconfigDir = LocalPath::fromAbsolutePath(path);
    if (!fsAccess->mkdirlocal(localconfigDir, false, false))
    {
        LOG_err << "Data directory not created";
    }
    path.append("/megacmdserver.log");
#endif

    return OUTFSTREAMTYPE(path);
}

LoggedStreamDefaultFile::LoggedStreamDefaultFile() :
    LoggedStreamOutStream(nullptr),
    mFstream(streamForDefaultFile())
{
    out = &mFstream;
}

}//end namespace
