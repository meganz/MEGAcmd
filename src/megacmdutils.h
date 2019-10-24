/**
 * @file src/megacmdutils.h
 * @brief MEGAcmd: Auxiliary methods
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

#ifndef MEGACMDUTILS_H
#define MEGACMDUTILS_H

#include "megacmdcommonutils.h"
#include "megacmd.h"

#include <string>

namespace megacmd {
using ::mega::m_time_t;

/* mega::MegaNode info extracting*/
void getNumFolderFiles(mega::MegaNode *, mega::MegaApi *, long long *nfiles, long long *nfolders);

std::string getUserInSharedNode(mega::MegaNode *n, mega::MegaApi *api);


/* code translation*/
const char* getAccessLevelStr(int level);

const char* getSyncPathStateStr(int state);

const char* getSyncStateStr(int state);

std::string visibilityToString(int visibility);

const char * getMCMDErrorString(int errorCode);

const char * getErrorCodeStr(mega::MegaError *e);

const char * getLogLevelStr(int loglevel);

int getLogLevelNum(const char* level);

const char * getShareLevelStr(int sharelevel);

int getShareLevelNum(const char* level);

const char * getTransferStateStr(int transferState);

std::string backupSatetStr(int backupstate);

const char * getProxyTypeStr(int proxyType);

/* Files and folders */

int getLinkType(std::string link);

/* Time related */
const char *fillStructWithSYYmdHMS(std::string &stime, struct tm &dt);

enum MegaCmdStrfTimeFormatId
{
    MCMDTIME_RFC2822,
    MCMDTIME_ISO6081,
    MCMDTIME_ISO6081WITHTIME,
    MCMDTIME_SHORT,
    MCMDTIME_SHORTWITHUTCDEVIATION,

    MCMDTIME_TOTAL
};

const char *getTimeFormatFromSTR(std::string formatName);
const char *getFormatStrFromId(int strftimeformatid);
const char *getTimeFormatNameFromId(int strftimeformatid);
std::string secondsToText(m_time_t seconds, bool humanreadable = true);

std::string getReadableTime(const m_time_t rawtime, const char* strftimeformat);
std::string getReadableTime(const m_time_t rawtime, int strftimeformatid = MCMDTIME_RFC2822);
std::string getReadableShortTime(const m_time_t rawtime, bool showUTCDeviation = false);

std::string getReadablePeriod(const m_time_t rawtime);

m_time_t getTimeStampAfter(m_time_t initial, std::string timestring);

m_time_t getTimeStampAfter(std::string timestring);

m_time_t getTimeStampBefore(m_time_t initial, std::string timestring);

m_time_t getTimeStampBefore(std::string timestring);

bool getMinAndMaxTime(std::string timestring, m_time_t *minTime, m_time_t *maxTime);

bool getMinAndMaxTime(m_time_t initial, std::string m_timestring, m_time_t *minTime, m_time_t *maxTime);


/* Strings related */
bool isRegExp(std::string what);

std::string unquote(std::string what);

bool megacmdWildcardMatch(const char *pszString, const char *pszMatch);

bool patternMatches(const char *what, const char *pattern, bool usepcre);

bool nodeNameIsVersion(std::string &nodeName);


/* Flags and Options */
bool setOptionsAndFlags(std::map<std::string, std::string> *opts, std::map<std::string, int> *flags, std::vector<std::string> *ws,
                        std::set<std::string> vvalidOptions, bool global = false);

bool getMinAndMaxSize(std::string sizestring, int64_t *minSize, int64_t *maxSize);

/* Others */
std::string readablePermissions(int permvalue);
int permissionsFromReadable(std::string permissions);

}//end namespace
#endif // MEGACMDUTILS_H
