/**
 * @file examples/megacmd/megacmdlogger.h
 * @brief MEGAcmd: Controls message logging
 *
 * (c) 2013-2016 by Mega Limited, Auckland, New Zealand
 *
 * This file is part of the MEGA SDK - Client Access Engine.
 *
 * Applications using the MEGA API must present a valid application key
 * and comply with the the rules set forth in the Terms of Service.
 *
 * The MEGA SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * @copyright Simplified (2-clause) BSD License.
 *
 * You should have received a copy of the license along with this
 * program.
 */

#ifndef MEGACMDLOGGER_H
#define MEGACMDLOGGER_H

#include "megacmd.h"
#include "comunicationsmanager.h"

#define OUTSTREAM getCurrentOut()

OUTSTREAMTYPE &getCurrentOut();
bool interactiveThread();
void setCurrentThreadOutStream(OUTSTREAMTYPE *);
int getCurrentOutCode();
void setCurrentOutCode(int);
int getCurrentThreadLogLevel();
void setCurrentThreadLogLevel(int);

CmdPetition * getCurrentPetition();
void setCurrentPetition(CmdPetition *petition);


void setCurrentThreadIsCmdShell(bool isit);
bool getCurrentThreadIsCmdShell();


class MegaCMDLogger : public mega::MegaLogger
{
private:
    int apiLoggerLevel;
    int cmdLoggerLevel;
    OUTSTREAMTYPE * output;
public:
    MegaCMDLogger(OUTSTREAMTYPE * outstr)
    {
        this->output = outstr;
        this->apiLoggerLevel = mega::MegaApi::LOG_LEVEL_ERROR;
    }

    void log(const char *time, int loglevel, const char *source, const char *message);

    void setApiLoggerLevel(int apiLoggerLevel)
    {
        this->apiLoggerLevel = apiLoggerLevel;
    }

    void setCmdLoggerLevel(int cmdLoggerLevel)
    {
        this->cmdLoggerLevel = cmdLoggerLevel;
    }

    int getMaxLogLevel();

    int getApiLoggerLevel()
    {
        return this->apiLoggerLevel;
    }

    int getCmdLoggerLevel()
    {
        return this->cmdLoggerLevel;
    }
};

#endif // MEGACMDLOGGER_H
