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

#include <memory>
#ifndef _WIN32
#include <pwd.h>
#include <unistd.h>
#endif
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

#ifdef _WIN32
namespace mega{

// Note: these was in megacmd namespace, but after one overload of operator<< was created within mega,
// ADL did not seem to be able to find it when solving template resolution in loggin.h (namespace mega),
// even for code coming from megacmd namespace.
// placing this operator in mega namespace eases ADL lookup and allows compilation
std::ostringstream & operator<< ( std::ostringstream & ostr, std::wstring const &str); //override for the log, otherwise SimpleLog won't compile.
}
#endif

namespace megacmd {

/* platform dependent */
#ifdef _WIN32

#define OUTSTREAMTYPE std::wostream
#define OUTSTRINGSTREAM std::wostringstream
#define OUTSTRING std::wstring
#define COUT std::wcout
#define CERR std::wcerr

//override << operators for wostream for string and const char *
std::wostream & operator<< ( std::wostream & ostr, std::string const & str );
std::wostream & operator<< ( std::wostream & ostr, const char * str );

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

#ifndef _WIN32
#define ARRAYSIZE(a) (sizeof((a)) / sizeof(*(a)))
#endif

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

static std::vector<std::string> loginInValidCommands { "log", "debug", "speedlimit", "help", "logout", "version", "quit",
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
                             "thumbnail", "preview", "find", "completion", "clear", "https"
#ifdef HAVE_DOWNLOADS_COMMAND
                                                   , "downloads"
#endif
                             , "transfers", "exclude", "exit", "errorcode", "graphics",
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

bool isPublicLink(const std::string &link);

bool isEncryptedLink(std::string link);

std::string getPublicLinkHandle(const std::string &link);
std::string getPublicLinkObjectId(const std::string &link);

bool hasWildCards(std::string &what);

std::string removeTrailingSeparators(std::string &path);


/* Strings related */

std::vector<std::string> split(const std::string& input, const std::string& pattern);

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

template <typename WHERE>
void printCenteredContentsT(WHERE &&o, const std::string &msj, unsigned int width, bool encapsulated)
{
    OUTSTRINGSTREAM os;
    printCenteredContents(os, msj, width, encapsulated);
    o << os.str();
}

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
    ColumnDisplayer(std::map<std::string, int> *clflags, std::map<std::string, std::string> *cloptions);

    void printHeaders(OUTSTREAMTYPE &os);
    void print(OUTSTREAMTYPE &os, bool printHeader = true);


    void addHeader(const std::string &name, bool fixed = true, int minWidth = 0);
    void addValue(const std::string &name, const std::string & value, bool replace = false);
    void endregistry();

    void setPrefix(const std::string &prefix);

private:
    std::map<std::string, int> *mClflags;
    std::map<std::string, std::string> *mCloptions;

    std::map<std::string, Field> fields;
    std::vector<std::string> mFieldnames;
    std::vector<std::map<std::string, std::string>> values;
    std::vector<int> lengths;

    std::map<std::string, std::string> currentRegistry;
    int currentlength = 0;

    int mUnfixedColsMinSize = 0;

    std::string mPrefix;

    void print(OUTSTREAMTYPE &os, int fullWidth, bool printHeader=true, bool onlyHeaders = false);


};

/**
 * @name PlatformDirectories
 * @brief PlatformDirectories provides methods for accessing directories for storing user data for
 * MegaCMD.
 * To preserve backwards compatibility with existing setups, all implementations of
 * PlatformDirectories should return the legacy config directory (~/.megaCmd on UNIX), if it exists.
 */
class PlatformDirectories
{
public:
    static std::unique_ptr<PlatformDirectories> getPlatformSpecificDirectories();
    /**
     * @brief RuntimeDirPath returns the base path for storing non-essential runtime files.
     *
     * Meant for sockets, named pipes, file locks, etc.
     */
    virtual std::string runtimeDirPath() = 0;
    /**
     * @brief CacheDirPath returns the base path for storing non-essential data files.
     *
     * Meant for cached data which can be safely deleted.
     */
    virtual std::string cacheDirPath() = 0;
    /**
     * @brief ConfigDirPath returns the base path for storing configuration files.
     *
     * Solely for user-editable configuration files.
     */
    virtual std::string configDirPath() = 0;
    /**
     * @brief DataDirPath returns the base path for storing data files.
     *
     * For user data files that should not be deleted (session credentials, SDK workding directory,
     * etc).
     */
    virtual std::string dataDirPath()
    {
        return configDirPath();
    }
    /**
     * @brief StateDirPath returns the base path for storing state files. Specifically, data that
     * can persist between restarts, but not significant enough for DataDirPath().
     *
     * Meant for recent command history, logs, crash dumps, etc.
     */
    virtual std::string stateDirPath()
    {
        return runtimeDirPath();
    }
};

#ifdef _WIN32
class WindowsDirectories : public PlatformDirectories
{
    std::string runtimeDirPath() override
    {
        return configDirPath();
    }
    std::string cacheDirPath() override
    {
        return configDirPath();
    }
    std::string configDirPath() override;
};
#else // !defined(_WIN32)
class PosixDirectories : public PlatformDirectories
{
public:
    std::string homeDirPath();
    std::string runtimeDirPath() override;
    std::string cacheDirPath() override
    {
        return PosixDirectories::configDirPath();
    };
    std::string configDirPath() override;
    std::string dataDirPath() override
    {
        return PosixDirectories::configDirPath();
    }
    std::string stateDirPath() override
    {
        return PosixDirectories::runtimeDirPath();
    }
    bool legacyConfigDirExists();
};
#ifdef __APPLE__
class MacOSDirectories : public PosixDirectories
{
    std::string cacheDirPath() override;
    std::string configDirPath() override;
    std::string dataDirPath() override;
};
#else // !defined(__APPLE__)
class XDGDirectories : public PosixDirectories
{
    std::string runtimeDirPath() override;
    std::string cacheDirPath() override;
    std::string configDirPath() override;
    std::string dataDirPath() override;
    std::string stateDirPath() override;
};
#endif // defined(__APPLE__)
std::string getOrCreateSocketPath(bool ensure);
#endif // _WIN32
}//end namespace
#endif // MEGACMDCOMMONUTILS_H
