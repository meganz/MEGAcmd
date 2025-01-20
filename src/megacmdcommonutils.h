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
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <mutex>
#include <condition_variable>
#include <cassert>
#include <optional>
#include <thread>
#include <filesystem>

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

#ifndef UNUSED
    #define UNUSED(x) (void)(x)
#endif

#define MOVE_CAPTURE(x) x{std::move(x)}
#define FWD_CAPTURE(x) x{std::forward<decltype(x)>(x)}
#define CONST_CAPTURE(x) &x = std::as_const(x)

using std::setw;
using std::left;
namespace fs = std::filesystem;

namespace megacmd {
std::string pathAsUtf8(const fs::path& path);
}

namespace megacmd {

/* platform dependent */
#ifdef _WIN32

#define OUTSTREAMTYPE std::wostream
#define OUTFSTREAMTYPE std::wofstream
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

std::wstring nonMaxPathLimitedWstring(const fs::path &localpath);
std::wstring nonMaxPathLimitedPath(const fs::path &localpath);


#else
#define OUTSTREAMTYPE std::ostream
#define OUTFSTREAMTYPE std::ofstream
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

static std::vector<std::string> localfolderpatterncommands {"lcd", "sync-ignore"};

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
                             "put", "get", "attr", "userattr", "mkdir", "rm", "du", "mv", "cp", "sync", "sync-ignore", "export", "share", "invite", "ipc", "df",
                             "showpcr", "users", "speedlimit", "killsession", "whoami", "help", "passwd", "reload", "logout", "version", "quit",
                             "thumbnail", "preview", "find", "completion", "clear", "https", "sync-issues"
#ifdef HAVE_DOWNLOADS_COMMAND
                                                   , "downloads"
#endif
                             , "transfers", "exclude", "exit", "errorcode", "graphics",
                             "cancel", "confirmcancel", "cat", "tree", "psa", "proxy"
                             , "mediainfo"
#ifdef HAVE_LIBUV
                             , "webdav", "ftp"
#endif
                             , "backup"
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

std::string generateRandomAlphaNumericString(size_t len);

std::vector<std::string> split(const std::string& input, const std::string& pattern);

long long charstoll(const char *instr);

// trim from start
std::string &ltrim(std::string &s, const char &c);
std::string_view ltrim(const std::string_view s, const char &c);

// trim at the end
std::string &rtrim(std::string &s, const char &c);

std::vector<std::string> getlistOfWords(const char *ptr, bool escapeBackSlashInCompletion = false, bool ignoreTrailingSpaces = false);

bool stringcontained(const char * s, std::vector<std::string> list);

char * dupstr(char* s);

bool replace(std::string& str, const std::string& from, const std::string& to);

void replaceAll(std::string& str, const std::string& from, const std::string& to);

int toInteger(std::string what, int failValue = -1);

std::string joinStrings(const std::vector<std::string>& vec, const char* delim = " ", bool quoted=true);

std::string getFixLengthString(const std::string &origin, unsigned int size, const char delimm=' ', bool alignedright = false);

std::string getRightAlignedString(const std::string origin, unsigned int minsize);

bool startsWith(const std::string_view str, const std::string_view prefix);

/* Vector related */
template <typename T>
std::vector<T> operator+(const std::vector<T>& a, const std::vector<T>& b)
{
    std::vector<T> result = a;
    result.insert(result.end(), b.begin(), b.end());
    return result;
}

template <typename T>
std::vector<T> operator+(std::vector<T>&& a, std::vector<T>&& b)
{
    a.reserve(a.size() + b.size());
    a.insert(a.end(), std::make_move_iterator(b.begin()), std::make_move_iterator(b.end()));
    return std::move(a);
}

std::string toLower(const std::string& str);

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

template <typename... Bools>
bool onlyZeroOrOneOf(Bools... args)
{
    return (args + ...) <= 1;
}

void printPercentageLineCerr(const char *title, long long completed, long long total, float percentDowloaded, bool cleanLineAfter = true);




/* Flags and Options */
int getFlag(std::map<std::string, int> *flags, const char * optname);

std::string getOption(std::map<std::string, std::string> *cloptions, const char * optname, std::string defaultValue = "");

std::optional<std::string> getOptionAsOptional(const std::map<std::string, std::string>& cloptions, const char * optname);

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
std::string getPropertyFromFile(const fs::path &configFilePath, const char *propertyName);

template <typename T>
T getValueFromFile(const fs::path &configFilePath, const char *propertyName, T defaultValue)
{
    std::string propValue = getPropertyFromFile(configFilePath, propertyName);
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

    OUTSTRING str(bool printHeader = true);

    void printHeaders(OUTSTREAMTYPE &os);
    void print(OUTSTREAMTYPE &os, bool printHeader = true);
    void clear();

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
 * MEGAcmd.
 *
 * Note: returned values are encoded in utf-8.
 *
 * 	+---------------------------------------------------------------------------------------------------------------------+
 * 	| DirPath | Windows              | Linux          | macOS                            | Linux/macOS (no HOME fallback) |
 * 	|---------|----------------------|----------------|----------------------------------|--------------------------------|
 * 	| runtime | $EXECFOLDER/.megaCmd | $HOME/.megaCmd | $HOME/Library/Caches/megacmd.mac | /tmp/megacmd-UID/.megacmd      |
 * 	| config  | $EXECFOLDER/.megaCmd | $HOME/.megaCmd | $HOME/.megaCmd                   | /tmp/megacmd-UID/.megacmd      |
 * 	+---------------------------------------------------------------------------------------------------------------------+
 *
 */
class PlatformDirectories
{
public:
    static std::unique_ptr<PlatformDirectories> getPlatformSpecificDirectories();
    virtual ~PlatformDirectories() = default;

    /**
     * @brief runtimeDirPath returns the base path for storing runtime files:
     *
     * Meant for sockets, named pipes, file locks, command history, etc.
     */
    virtual fs::path runtimeDirPath()
    {
        return configDirPath();
    }
    /**
     * @brief configDirPath returns the base path for storing configuration and data files.
     *
     * Meant for user-editable configuration files, data files that should not be deleted
     * (session credentials, SDK workding directory, logs, etc).
     */
    virtual fs::path configDirPath() = 0;
};

template <typename T> size_t numberOfDigits(T num)
{
    size_t digits = num < 0 ? 1 : 0;
    while (num != 0)
    {
        digits++;
        num /= 10;
    }
    return digits;
}

#ifdef _WIN32
class WindowsDirectories : public PlatformDirectories
{
    fs::path configDirPath() override;
};
std::wstring getNamedPipeName();
#else // !defined(_WIN32)
class PosixDirectories : public PlatformDirectories
{
public:
    std::string homeDirPath();
    virtual fs::path configDirPath() override;

    static std::string noHomeFallbackFolder();

};
#ifdef __APPLE__
class MacOSDirectories : public PosixDirectories
{
    fs::path runtimeDirPath() override;
};
#endif // defined(__APPLE__)
std::string getOrCreateSocketPath(bool createDirectory);
#endif // _WIN32

/**
* @brief Common infrastructure for classes that are designed to have a single instance for
* the whole lifetime of the application. This approach is used instead of singletons,
* in order to take control of the lifetime of the instance, and to avoid the inherent issues
* of static initialization and destruction order.
*/
template <class T>
class BaseInstance
{
protected:
    inline static T* sInstance = nullptr;

    virtual ~BaseInstance()
    {
        // having a base class allows to set sInstance to nullptr
        // _after_ the (derived) Instance's destructor destroys mInstance,
        // thus allowing T's destructor to still access Instance<T>::Get
        sInstance = nullptr;
    }
};

template <class T>
class Instance : protected BaseInstance<T>
{
    T mInstance;

    inline static std::mutex sPendingRogueOnesMutex;
    inline static std::condition_variable sPendingRogueOnesCV;
    inline static unsigned sPendingRogueOnes = 0;
    inline static bool sAssertOnDestruction = false;
public:
    static bool waitForExplicitDependents()
    {
        // wait for rogue ones
        std::unique_lock<std::mutex> lockGuard(sPendingRogueOnesMutex);
        if (!sPendingRogueOnesCV.wait_for(lockGuard, std::chrono::seconds(20), [](){return !sPendingRogueOnes;}))
        {
            std::cerr << "Shutdown gracetime for explicit dependents for Instance<" << typeid(T).name()
                      << "> passed without success. Rogue ones = " << sPendingRogueOnes
                      << ". Will remove the instance anyway." << std::endl;
            sAssertOnDestruction = true;
            return false;
        }
        return true;
    }

    class ExplicitDependent
    {
    public:
        ExplicitDependent()
        {
            std::lock_guard<std::mutex> g(Instance<T>::sPendingRogueOnesMutex);
            Instance<T>::sPendingRogueOnes++;
        }
        virtual ~ExplicitDependent()
        {
            assert(!sAssertOnDestruction);
            {
                std::lock_guard<std::mutex> g(Instance<T>::sPendingRogueOnesMutex);
                Instance<T>::sPendingRogueOnes--;
            }
            Instance<T>::sPendingRogueOnesCV.notify_one();
        }
    };


    template<typename... Args>
    Instance(Args&&... args) : mInstance(std::forward<Args>(args)...)
    {
        assert(!BaseInstance<T>::sInstance &&
               "There must be one and only one instance of this class");
        BaseInstance<T>::sInstance = &mInstance;
    }
    ~Instance()
    {
        bool explicitDependentsFinishedInTime = waitForExplicitDependents();
        UNUSED(explicitDependentsFinishedInTime);
        assert(explicitDependentsFinishedInTime);
    }

    static T& Get()
    {
        assert(BaseInstance<T>::sInstance &&
               "Must create the unique instance before trying to retrieve it");
        return *BaseInstance<T>::sInstance;
    }
#ifdef MEGACMD_TESTING_CODE
    static bool exists()
    {
        return BaseInstance<T>::sInstance;
    }
#endif //MEGACMD_TESTING_CODE
 };

/**
 * @brief general purpose scope guard class
 */
template <typename ExitCallback>
class ScopeGuard
{
    ExitCallback mExitCb;
public:
    ScopeGuard(ExitCallback&& exitCb) : mExitCb{std::move(exitCb)} { }
    ~ScopeGuard() { mExitCb(); }
};

template <typename _Rep, typename _Period, typename _Rep2, typename _Period2, typename Condition, typename What>
void timelyRetry(const std::chrono::duration<_Rep, _Period> &maxTime, const std::chrono::duration<_Rep2, _Period2> &step
                 , Condition&& endCondition, What&& what)
{
    using Step_t = std::chrono::duration<_Rep2, _Period2>;
    for (auto timeLeft = std::chrono::duration_cast<Step_t>(maxTime); !endCondition() && timeLeft > Step_t(0); timeLeft -= step)
    {
        std::this_thread::sleep_for(step);
        what();
    }
}

#ifdef _WIN32
class WindowsUtf8StdoutGuard final
{
    inline static std::mutex sSetmodeMtx;

    int mOldMode;
    std::lock_guard<std::mutex> mGuard;
public:
    WindowsUtf8StdoutGuard()
        : mGuard(sSetmodeMtx)
    {
        fflush(stdout);
        mOldMode = _setmode(_fileno(stdout), _O_U8TEXT);
        assert(mOldMode != -1);
    }

    ~WindowsUtf8StdoutGuard()
    {
        fflush(stdout);
        assert(mOldMode != -1);
        _setmode(_fileno(stdout), mOldMode);
    }
};
#endif

}//end namespace


namespace mega {
// Note: these was in megacmd namespace, but after one overload of operator<< was created within mega,
// ADL did not seem to be able to find it when solving template resolution in loggin.h (namespace mega),
// even for code coming from megacmd namespace.
// placing this operator in mega namespace eases ADL lookup and allows compilation

// Note2: std::is_same_v is required below, instead of directly using the types; otherwise we'll
// get a compilation error on type resolution because paths are implicitly convertible to strings.

#ifdef _WIN32
template <typename T>
inline std::enable_if_t<std::is_same_v<T, std::wstring>, std::ostringstream&>
operator<<(std::ostringstream& oss, const T& wstr)
{
    std::string s;
    megacmd::localwtostring(&wstr, &s);
    oss << s;
    return oss;
}
#endif

template <typename T>
inline std::enable_if_t<std::is_same_v<T, fs::path>, std::ostringstream&>
operator<<(std::ostringstream& oss, const T& path)
{
    oss << megacmd::pathAsUtf8(path);
    return oss;
}
}

#endif // MEGACMDCOMMONUTILS_H
