/**
 * @file src/megacmdcommonutils.h
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
#include <mutex>


using std::setw;
using std::left;

namespace megacmd {

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
std::string getutf8fromUtf16(const wchar_t *ws);
void utf16ToUtf8(const wchar_t* utf16data, int utf16size, std::string* utf8string);

#else
#define OUTSTREAMTYPE std::ostream
#define OUTSTRINGSTREAM std::ostringstream
#define OUTSTRING std::string
#define COUT std::cout
#define CERR std::cerr

#endif

#define OUTSTREAM COUT


/* commands */
static std::vector<std::string> validGlobalParameters {"v", "help"};
static std::vector<std::string> localremotefolderpatterncommands {"sync"};
static std::vector<std::string> remotepatterncommands {"export", "attr"};
static std::vector<std::string> remotefolderspatterncommands {"cd", "share"};

static std::vector<std::string> multipleremotepatterncommands {"ls", "tree", "mkdir", "rm", "du", "find", "mv", "deleteversions", "cat", "mediainfo"
#ifdef HAVE_LIBUV
                                           , "webdav", "ftp"
#endif
                                          };

static std::vector<std::string> remoteremotepatterncommands {"cp"};

static std::vector<std::string> remotelocalpatterncommands {"get", "thumbnail", "preview"};

static std::vector<std::string> localfolderpatterncommands {"lcd"};

static std::vector<std::string> emailpatterncommands {"invite", "signup", "ipc", "users"};

static std::vector<std::string> loginInValidCommands { "log", "debug", "speedlimit","help", "logout", "version", "quit",
                            "clear", "https", "exit", "errorcode", "proxy"
#if defined(_WIN32) && defined(NO_READLINE)
                             , "autocomplete", "codepage"
#elif defined(_WIN32)
                             , "unicode"
#endif
#if defined(_WIN32) || defined(__APPLE__)
                             , "update"
#endif
                           };

static std::vector<std::string> allValidCommands { "login", "signup", "confirm", "session", "mount", "ls", "cd", "log", "debug", "pwd", "lcd", "lpwd", "import", "masterkey",
                             "put", "get", "attr", "userattr", "mkdir", "rm", "du", "mv", "cp", "sync", "export", "share", "invite", "ipc", "df",
                             "showpcr", "users", "speedlimit", "killsession", "whoami", "help", "passwd", "reload", "logout", "version", "quit",
                             "thumbnail", "preview", "find", "completion", "clear", "https", "transfers", "exclude", "exit", "errorcode", "graphics",
                             "cancel", "confirmcancel", "cat", "tree", "psa", "proxy"
                             , "mediainfo"
#ifdef HAVE_LIBUV
                             , "webdav", "ftp"
#endif
#ifdef ENABLE_BACKUPS
                             , "backup"
#endif
                             , "deleteversions"
#if defined(_WIN32) && defined(NO_READLINE)
                             , "autocomplete", "codepage"
#elif defined(_WIN32)
                             , "unicode"
#else
                             , "permissions"
#endif
#if defined(_WIN32) || defined(__APPLE__)
                             , "update"
#endif
                           };


static const int RESUME_SESSION_TIMEOUT = 10;

/* Files and folders */

//tests if a path is writable  //TODO: move to fsAccess
bool canWrite(std::string path);

bool isPublicLink(std::string link);

bool isEncryptedLink(std::string link);

std::string getPublicLinkHandle(const std::string &link);

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

std::string getFixLengthString(const std::string &origin, unsigned int size, const char delimm=' ', bool alignedright = false);

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

#ifdef __linux__
std::string getCurrentExecPath();
#endif

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

class Field
{
public:
    Field();
    Field(std::string name, bool fixed = false, int fixedWidth = 0);

public:
    std::string name;
    int fixedWidth;
    bool fixedSize;
    int dispWidth = 0;
    int maxValueLength = 0;

    void updateMaxValue(int newcandidate);
};

class ColumnDisplayer
{
public:
    ColumnDisplayer(int unfixedColsMinSize = 0);

    void print(OUTSTREAMTYPE &os, int fullWidth, bool printHeader=true);
    void addHeader(const std::string &name, bool fixed = true, int minWidth = 0);
    void addValue(const std::string &name, const std::string & value, bool replace = false);
    void endregistry();

private:
    std::map<std::string, Field> fields;
    std::vector<std::string> fieldnames;
    std::vector<std::map<std::string, std::string>> values;
    std::vector<int> lengths;

    std::map<std::string, std::string> currentRegistry;
    int currentlength = 0;

    int mUnfixedColsMinSize = 0;

};

}//end namespace
#endif // MEGACMDCOMMONUTILS_H
