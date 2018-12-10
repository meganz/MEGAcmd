/**
 * @file src/megacmdcommonutils.h
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

#ifndef MEGACMDCOMMONUTILS_H
#define MEGACMDCOMMONUTILS_H

#include <string>
#include <vector>
#include <iomanip>
#include <map>
#include <set>
#include <stdint.h>
#include <sstream>
#include <iostream>
#include <iomanip>

using std::setw;
using std::left;

/* platform dependent */
#ifdef _WIN32

#define OUTSTREAMTYPE std::wostream
#define OUTSTRINGSTREAM std::wostringstream
#define OUTSTRING std::wstring
#define COUT std::wcout
#define CERR std::wcerr

std::wostream & operator<< ( std::wostream & ostr, std::string const & str );
std::wostream & operator<< ( std::wostream & ostr, const char * str );
std::ostringstream & operator<< ( std::ostringstream & ostr, std::wstring const &str);

void stringtolocalw(const char* path, std::wstring* local);
void localwtostring(const std::wstring* wide, std::string *multibyte);
void utf16ToUtf8(const wchar_t* utf16data, int utf16size, std::string* utf8string);

#else
#define OUTSTREAMTYPE std::ostream
#define OUTSTRINGSTREAM std::ostringstream
#define OUTSTRING std::string
#define COUT std::cout
#define CERR std::cerr

#endif

#define OUTSTREAM COUT


// Our own version of time_t which we can be sure is 64 bit.
// Utils.h has functions m_time() and so on corresponding to time() which help us to use this type and avoid arithmetic overflow when working with time_t on systems where it's 32-bit
typedef int64_t m_time_t;

/* Files and folders */

//tests if a path is writable  //TODO: move to fsAccess
bool canWrite(std::string path);

bool isPublicLink(std::string link);

bool isEncryptedLink(std::string link);

bool hasWildCards(std::string &what);


/* Strings related */

long long charstoll(const char *instr);

// trim from start
std::string &ltrim(std::string &s, const char &c);

// trim at the end
std::string &rtrim(std::string &s, const char &c);

std::vector<std::string> getlistOfWords(char *ptr, bool escapeBackSlashInCompletion = false, bool ignoreTrailingSpaces = false);

bool stringcontained(const char * s, std::vector<std::string> list);

char * dupstr(char* s);

bool replace(std::string& str, const std::string& from, const std::string& to);

void replaceAll(std::string& str, const std::string& from, const std::string& to);

int toInteger(std::string what, int failValue = -1);

std::string joinStrings(const std::vector<std::string>& vec, const char* delim = " ", bool quoted=true);

std::string getFixLengthString(const std::string origin, unsigned int size, const char delimm=' ', bool alignedright = false);

std::string getRightAlignedString(const std::string origin, unsigned int minsize);

template<typename T>
OUTSTRING getLeftAlignedStr(T what, int n)
{
    OUTSTRINGSTREAM os;
    os << setw(n) << left << what;
    return os.str();
}


template<typename T>
OUTSTRING getRightAlignedStr(T what, int n)
{
    OUTSTRINGSTREAM os;
    os << setw(n) << what;
    return os.str();
}

void printCenteredLine(std::string msj, unsigned int width, bool encapsulated = true);
void printCenteredContents(std::string msj, unsigned int width, bool encapsulated = true);
void printCenteredContentsCerr(std::string msj, unsigned int width, bool encapsulated = true);
void printCenteredLine(OUTSTREAMTYPE &os, std::string msj, unsigned int width, bool encapsulated = true);
void printCenteredContents(OUTSTREAMTYPE &os, std::string msj, unsigned int width, bool encapsulated = true);

void printPercentageLineCerr(const char *title, long long completed, long long total, float percentDowloaded, bool cleanLineAfter = true);

/* Flags and Options */
int getFlag(std::map<std::string, int> *flags, const char * optname);

std::string getOption(std::map<std::string, std::string> *cloptions, const char * optname, std::string defaultValue = "");

int getintOption(std::map<std::string, std::string> *cloptions, const char * optname, int defaultValue = 0);

void discardOptionsAndFlags(std::vector<std::string> *ws);

//This has been defined in megacmdutils.h because it depends on logging. If ever require offer a version without it here
//bool setOptionsAndFlags(map<string, string> *opts, map<string, int> *flags, vector<string> *ws, set<string> vvalidOptions, bool global)


/* Others */
std::string sizeToText(long long totalSize, bool equalizeUnitsLength = true, bool humanreadable = true);
std::string sizeProgressToText(long long partialSize, long long totalSize, bool equalizeUnitsLength = true, bool humanreadable = true);

int64_t textToSize(const char *text);

std::string percentageToText(float percentage);

unsigned int getNumberOfCols(unsigned int defaultwidth = 90);

void sleepSeconds(int seconds);
void sleepMilliSeconds(long microseconds);

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

#endif // MEGACMDCOMMONUTILS_H
