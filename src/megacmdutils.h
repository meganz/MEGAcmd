/**
 * @file examples/megacmd/megacmdutils.h
 * @brief MEGAcmd: Auxiliary methods
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

#ifndef MEGACMDUTILS_H
#define MEGACMDUTILS_H

#include "megacmd.h"

/* mega::MegaNode info extracting*/
/**
 * @brief getNumFolderFiles
 *
 * Ownership of returned value belongs to the caller
 * @param n
 * @param api
 * @return
 */
int * getNumFolderFiles(mega::MegaNode *, mega::MegaApi *);

std::string getUserInSharedNode(mega::MegaNode *n, mega::MegaApi *api);


/* code translation*/
const char* getAttrStr(int attr);

int getAttrNum(const char* attr);

const char* getAccessLevelStr(int level);

const char* getSyncStateStr(int state);

std::string visibilityToString(int visibility);

const char * getErrorCodeStr(mega::MegaError *e);

const char * getLogLevelStr(int loglevel);

int getLogLevelNum(const char* level);

const char * getShareLevelStr(int sharelevel);

int getShareLevelNum(const char* level);

const char * getTransferStateStr(int transferState);


/* Files and folders */

bool canWrite(std::string path);

int getLinkType(std::string link);

bool isPublicLink(std::string link);

bool hasWildCards(std::string &what);


/* Time related */

std::string getReadableTime(const time_t rawtime);

time_t getTimeStampAfter(time_t initial, std::string timestring);

time_t getTimeStampAfter(std::string timestring);


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



/* Flags and Options */
int getFlag(std::map<std::string, int> *flags, const char * optname);

std::string getOption(std::map<std::string, std::string> *cloptions, const char * optname, std::string defaultValue = "");

int getintOption(std::map<std::string, std::string> *cloptions, const char * optname, int defaultValue = 0);

bool setOptionsAndFlags(std::map<std::string, std::string> *opts, std::map<std::string, int> *flags, std::vector<std::string> *ws,
                        std::set<std::string> vvalidOptions, bool global = false);

/* Others */
std::string sizeToText(long long totalSize, bool equalizeUnitsLength = true, bool humanreadable = true);

std::string secondsToText(time_t seconds, bool humanreadable = true);

std::string percentageToText(float percentage);

unsigned int getNumberOfCols(unsigned int defaultwidth = 90);

void sleepSeconds(int seconds);
void sleepMicroSeconds(long microseconds);

#endif // MEGACMDUTILS_H
