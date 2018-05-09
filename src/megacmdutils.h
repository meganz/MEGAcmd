/**
 * @file src/megacmdutils.h
 * @brief MEGAcmd: Auxiliary methods
 *
 * (c) 2013-2016 by Mega Limited, Auckland, New Zealand
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

#include "megacmd.h"

/* mega::MegaNode info extracting*/
void getNumFolderFiles(mega::MegaNode *, mega::MegaApi *, long long *nfiles, long long *nfolders);

std::string getUserInSharedNode(mega::MegaNode *n, mega::MegaApi *api);


/* code translation*/
const char* getAttrStr(int attr);

int getAttrNum(const char* attr);

const char* getAccessLevelStr(int level);

const char* getSyncPathStateStr(int state);

const char* getSyncStateStr(int state);

std::string visibilityToString(int visibility);

const char * getErrorCodeStr(mega::MegaError *e);

const char * getLogLevelStr(int loglevel);

int getLogLevelNum(const char* level);

const char * getShareLevelStr(int sharelevel);

int getShareLevelNum(const char* level);

const char * getTransferStateStr(int transferState);

std::string backupSatetStr(int backupstate);



/* Files and folders */

bool canWrite(std::string path);

int getLinkType(std::string link);

bool isPublicLink(std::string link);

bool hasWildCards(std::string &what);


/* Time related */
const char *fillStructWithSYYmdHMS(std::string &stime, struct tm &dt);

std::string getReadableTime(const ::mega::m_time_t rawtime);
std::string getReadableShortTime(const ::mega::m_time_t rawtime, bool showUTCDeviation = false);

std::string getReadablePeriod(const ::mega::m_time_t rawtime);

::mega::m_time_t getTimeStampAfter(::mega::m_time_t initial, std::string timestring);

::mega::m_time_t getTimeStampAfter(std::string timestring);

time_t getTimeStampBefore(time_t initial, std::string timestring);

time_t getTimeStampBefore(std::string timestring);

bool getMinAndMaxTime(std::string timestring, time_t *minTime, time_t *maxTime);

bool getMinAndMaxTime(time_t initial, std::string timestring, time_t *minTime, time_t *maxTime);


/* Strings related */

// trim from start
std::string &ltrim(std::string &s, const char &c);

// trim at the end
std::string &rtrim(std::string &s, const char &c);

std::vector<std::string> getlistOfWords(char *ptr, bool ignoreTrailingSpaces = false);

bool stringcontained(const char * s, std::vector<std::string> list);

char * dupstr(char* s);

bool replace(std::string& str, const std::string& from, const std::string& to);

void replaceAll(std::string& str, const std::string& from, const std::string& to);

bool isRegExp(std::string what);

std::string unquote(std::string what);

bool patternMatches(const char *what, const char *pattern, bool usepcre);

int toInteger(std::string what, int failValue = -1);

std::string joinStrings(const std::vector<std::string>& vec, const char* delim = " ", bool quoted=true);

std::string getFixLengthString(const std::string origin, unsigned int size, const char delimm=' ', bool alignedright = false);

std::string getRightAlignedString(const std::string origin, unsigned int minsize);

bool nodeNameIsVersion(std::string &nodeName);

/* Flags and Options */
int getFlag(std::map<std::string, int> *flags, const char * optname);

std::string getOption(std::map<std::string, std::string> *cloptions, const char * optname, std::string defaultValue = "");

int getintOption(std::map<std::string, std::string> *cloptions, const char * optname, int defaultValue = 0);

bool setOptionsAndFlags(std::map<std::string, std::string> *opts, std::map<std::string, int> *flags, std::vector<std::string> *ws,
                        std::set<std::string> vvalidOptions, bool global = false);

bool getMinAndMaxSize(std::string sizestring, int64_t *minSize, int64_t *maxSize);

/* Others */
std::string sizeToText(long long totalSize, bool equalizeUnitsLength = true, bool humanreadable = true);
std::string sizeProgressToText(long long partialSize, long long totalSize, bool equalizeUnitsLength = true, bool humanreadable = true);

int64_t textToSize(const char *text);

std::string secondsToText(::mega::m_time_t seconds, bool humanreadable = true);

std::string percentageToText(float percentage);

std::string readablePermissions(int permvalue);
int permissionsFromReadable(std::string permissions);

unsigned int getNumberOfCols(unsigned int defaultwidth = 90);

void sleepSeconds(int seconds);
void sleepMicroSeconds(long microseconds);

bool isValidEmail(std::string email);

/* Properties */
std::string &ltrimProperty(std::string &s, const char &c);
std::string &rtrimProperty(std::string &s, const char &c);
std::string &trimProperty(std::string &what);
std::string getPropertyFromFile(const char *configFile, const char *propertyName);
template <typename T>
T getValueFromFile(const char *configFile, const char *propertyName, T defaultValue)
{
    std::string propValue = getPropertyFromFile(configFile, propertyName);
    if (!propValue.size()) return defaultValue;

    T i;
    std::istringstream is(propValue);
    is >> i;
    return i;

}

#endif // MEGACMDUTILS_H
