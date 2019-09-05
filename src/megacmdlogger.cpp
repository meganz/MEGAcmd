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

using namespace std;
using namespace mega;

// different outstreams for every thread. to gather all the output data
std::mutex threadLookups;
map<uint64_t, LoggedStream *> outstreams;
map<uint64_t, int> threadLogLevel;
map<uint64_t, int> threadoutCode;
map<uint64_t, CmdPetition *> threadpetition;
map<uint64_t, bool> threadIsCmdShell;

LoggedStream LCOUT(&COUT);


LoggedStream &getCurrentOut()
{
    std::lock_guard<std::mutex> g(threadLookups);
    uint64_t currentThread = MegaThread::currentThreadId();
    if (outstreams.find(currentThread) == outstreams.end())
    {
        return LCOUT;
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


MegaCMDLogger::MegaCMDLogger()
{
    this->output = &LCOUT;
    this->apiLoggerLevel = MegaApi::LOG_LEVEL_ERROR;
    this->outputmutex = new std::mutex();
}

MegaCMDLogger::~MegaCMDLogger()
{
    delete this->outputmutex;
}

void MegaCMDLogger::log(const char *time, int loglevel, const char *source, const char *message)
{
    if ( (string(source).find("src/megacmd") != string::npos)
         || (string(source).find("src\\megacmd") != string::npos)
         || (string(source).find("listeners.cpp") != string::npos)
         || (string(source).find("configurationmanager.cpp") != string::npos)
         || (string(source).find("comunicationsmanager") != string::npos)
         )
    {
        if (loglevel <= cmdLoggerLevel)
        {
#ifdef _WIN32
            std::lock_guard<std::mutex> g(*outputmutex);
            int oldmode;
            oldmode = _setmode(_fileno(stdout), _O_U8TEXT);
            *output << "[" << SimpleLogger::toStr(LogLevel(loglevel)) << ": " << time << "] " << message << endl;
            _setmode(_fileno(stdout), oldmode);
#else
            *output << "[" << SimpleLogger::toStr(LogLevel(loglevel)) << ": " << time << "] " << message << endl;
#endif

        }

        int currentThreadLogLevel = getCurrentThreadLogLevel();
        if (currentThreadLogLevel < 0)
        {
            currentThreadLogLevel = cmdLoggerLevel;
        }
        if (( loglevel <= currentThreadLogLevel ) && ( &OUTSTREAM != output ))
        {
            OUTSTREAM << "[" << SimpleLogger::toStr(LogLevel(loglevel)) << ": " << time << "] " << message << endl;
        }
    }
    else
    {
        if (loglevel <= apiLoggerLevel)
        {
            if (( apiLoggerLevel <= MegaApi::LOG_LEVEL_DEBUG ) && !strcmp(message, "Request (RETRY_PENDING_CONNECTIONS) starting"))
            {
                return;
            }
            if (( apiLoggerLevel <= MegaApi::LOG_LEVEL_DEBUG ) && !strcmp(message, "Request (RETRY_PENDING_CONNECTIONS) finished"))
            {
                return;
            }
#ifdef _WIN32
            std::lock_guard<std::mutex> g(*outputmutex);
            int oldmode;
            oldmode = _setmode(_fileno(stdout), _O_U8TEXT);
            *output << "[API:" << SimpleLogger::toStr(LogLevel(loglevel)) << ": " << time << "] " << message << endl;
            _setmode(_fileno(stdout), oldmode);
#else
            *output << "[API:" << SimpleLogger::toStr(LogLevel(loglevel)) << ": " << time << "] " << message << endl;
#endif
        }

        int currentThreadLogLevel = getCurrentThreadLogLevel();

        if (currentThreadLogLevel < 0)
        {
            currentThreadLogLevel = apiLoggerLevel;
        }
        if (( loglevel <= currentThreadLogLevel ) && ( &OUTSTREAM != output )) //since it happens in the sdk thread, this shall be false
        {
            OUTSTREAM << "[API:" << SimpleLogger::toStr(LogLevel(loglevel)) << ": " << time << "] " << message << endl;
        }
    }
}

int MegaCMDLogger::getMaxLogLevel()
{
    return max(max(getCurrentThreadLogLevel(), cmdLoggerLevel), apiLoggerLevel);
}

