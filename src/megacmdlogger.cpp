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
#include "megacmdutils.h"
#include "megacmd_src_file_list.h"

#include <map>

#include <sys/types.h>

using namespace mega;

namespace megacmd {

namespace {
    constexpr const char* sLogTimestampFormat = "%04d-%02d-%02d_%02d-%02d-%02d.%06d";
    thread_local bool isThreadDataSet = false;

    std::string getNowTimeStr()
    {
        return timestampToString(std::chrono::system_clock::now());
    }
}

ThreadData &getCurrentThreadData()
{
    thread_local ThreadData threadData;
    return threadData;
}

const char* getCommandPrefixBasedOnMode()
{
    if (isCurrentThreadInteractive())
    {
        return "";
    }
    return "mega-";
}

bool isCurrentThreadInteractive()
{
    return isCurrentThreadCmdShell() || !isThreadDataSet;
}

void setCurrentThreadOutStreams(LoggedStream &outStream, LoggedStream &errStream)
{
    isThreadDataSet = true;
    getCurrentThreadData().mOutStream = &outStream;
    getCurrentThreadData().mErrStream = &errStream;
}

void setCurrentThreadOutCode(int outCode)
{
    isThreadDataSet = true;
    getCurrentThreadData().mOutCode = outCode;
}

void setCurrentThreadLogLevel(int logLevel)
{
    isThreadDataSet = true;
    getCurrentThreadData().mLogLevel = logLevel;
}

void setCurrentThreadCmdPetition(CmdPetition *cmdPetition)
{
    isThreadDataSet = true;
    getCurrentThreadData().mCmdPetition = cmdPetition;
}

void setCurrentThreadIsCmdShell(bool isCmdShell)
{
    isThreadDataSet = true;
    getCurrentThreadData().mIsCmdShell = isCmdShell;
}

std::string formatErrorAndMaySetErrorCode(const MegaError &error)
{
    auto code = error.getErrorCode();
    if (code == MegaError::API_OK)
    {
        return std::string();
    }

    setCurrentThreadOutCode(code);
    if (code == MegaError::API_EBLOCKED)
    {
        //auto reason = sandboxCMD->getReasonblocked();
        std::string reason;

        auto reasonStr = std::string("Account blocked.");
        if (!reason.empty())
        {
            reasonStr.append("Reason: ").append(reason);
        }
        return reasonStr;
    }
    else if (code == MegaError::API_EPAYWALL || (code == MegaError::API_EOVERQUOTA /*&& sandboxCMD->storageStatus == MegaApi::STORAGE_STATE_RED*/))
    {
        return "Reached storage quota. You can change your account plan to increase your quota limit. See \"help --upgrade\" for further details";
    }

    return error.getErrorString();
}

bool checkNoErrors(int errorCode, const std::string &message)
{
    MegaErrorPrivate e(errorCode);
    return checkNoErrors(&e, message);
}

bool checkNoErrors(MegaError *error, const std::string &message, SyncError syncError)
{
    if (!error)
    {
        LOG_fatal << "No MegaError at request: " << message;
        assert(false);
        return false;
    }
    if (error->getErrorCode() == MegaError::API_OK)
    {
        if (syncError)
        {
            std::unique_ptr<const char[]> megaSyncErrorCode(MegaSync::getMegaSyncErrorCode(syncError));
            LOG_info << "Able to " << message << ", but received syncError: " << megaSyncErrorCode.get();
        }
        return true;
    }

    auto apiErrorString = formatErrorAndMaySetErrorCode(*error);

    auto logErrMessage = std::string("Failed to ").append(message).append(": ");
    if (auto mountResult = error->getMountResult(); mountResult != MegaMount::SUCCESS) //if there is mount error, prefer this one for log message
    {
        logErrMessage.append(getMountResultStr(mountResult));
    }
    else
    {
        logErrMessage.append(apiErrorString);
    }

    if (syncError)
    {
        std::unique_ptr<const char[]> megaSyncErrorCode(MegaSync::getMegaSyncErrorCode(syncError));
        logErrMessage.append(". ").append(megaSyncErrorCode.get());
    }

    LOG_err << logErrMessage;
    return false;
}

bool checkNoErrors(::mega::SynchronousRequestListener *listener, const std::string &message, ::mega::SyncError syncError)
{
    assert(listener);
    listener->wait();
    assert(listener->getError());
    return checkNoErrors(listener->getError(), message, syncError);
}

std::optional<std::chrono::time_point<std::chrono::system_clock>> stringToTimestamp(std::string_view str)
{
    if (str.size() != LogTimestampSize)
    {
        return std::nullopt;
    }

    int years, months, days, hours, minutes, seconds, microseconds;
    int parsed = std::sscanf(str.data(), sLogTimestampFormat,
                             &years, &months, &days, &hours, &minutes, &seconds, &microseconds);
    if (parsed != 7)
    {
        return std::nullopt;
    }

    struct std::tm gmt;
    memset(&gmt, 0, sizeof(struct std::tm));
    gmt.tm_year = years - 1900;
    gmt.tm_mon = months - 1;
    gmt.tm_mday = days;
    gmt.tm_hour = hours;
    gmt.tm_min = minutes;
    gmt.tm_sec = seconds;

#ifdef _WIN32
    const time_t t = _mkgmtime(&gmt);
#else
    const time_t t = timegm(&gmt);
#endif

    const auto time_point = std::chrono::system_clock::from_time_t(t);
    return time_point + std::chrono::microseconds(microseconds);
}

std::string timestampToString(std::chrono::time_point<std::chrono::system_clock> timestamp)
{
    std::array<char, LogTimestampSize + 1> timebuf;
    const time_t t = std::chrono::system_clock::to_time_t(timestamp);

    struct std::tm gmt;
    memset(&gmt, 0, sizeof(struct std::tm));
    mega::m_gmtime(t, &gmt);

    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(timestamp - std::chrono::system_clock::from_time_t(t));
    std::snprintf(timebuf.data(), timebuf.size(), sLogTimestampFormat,
                  gmt.tm_year + 1900, gmt.tm_mon + 1, gmt.tm_mday,
                  gmt.tm_hour, gmt.tm_min, gmt.tm_sec, static_cast<int>(microseconds.count() % 1000000));

    return std::string(timebuf.data(), LogTimestampSize);
}

MegaCmdLogger::MegaCmdLogger() :
    mSdkLoggerLevel(mega::MegaApi::LOG_LEVEL_ERROR),
    mCmdLoggerLevel(mega::MegaApi::LOG_LEVEL_ERROR),
    mFlushOnLevel(mega::MegaApi::LOG_LEVEL_WARNING)
{
}

fs::path MegaCmdLogger::getDefaultFilePath()
{
    auto dirs = PlatformDirectories::getPlatformSpecificDirectories();

    assert(!dirs->configDirPath().empty());
    return dirs->configDirPath() / "megacmdserver.log";
}

bool MegaCmdLogger::isMegaCmdSource(const std::string &source)
{
    static const std::set<std::string_view> megaCmdSourceFiles = MEGACMD_SRC_FILE_LIST;

    // Remove the line number (since source has the format "filename.cpp:1234")
    std::string_view filename(source);
    filename = filename.substr(0, filename.find(':'));

    return megaCmdSourceFiles.find(filename) != megaCmdSourceFiles.end();
}


const char * loglevelToShortPaddedString(int loglevel)
{
    static constexpr std::array<const char*, 6> logLevels = {
        "CRIT ", // LOG_LEVEL_FATAL
        "ERR  ", // LOG_LEVEL_ERROR
        "WARN ", // LOG_LEVEL_WARNING
        "INFO ", // LOG_LEVEL_INFO
        "DBG  ", // LOG_LEVEL_DEBUG
        "DTL  "  // LOG_LEVEL_MAX
    };

    assert (loglevel >= 0 && loglevel < static_cast<int>(logLevels.size()));
    return logLevels[static_cast<size_t>(loglevel)];
}

void MegaCmdLogger::formatLogToStream(LoggedStream &stream, std::string_view time, int logLevel, const char *source, const char *message, bool surround)
{
    if (surround)
    {
        stream << "[";
    }
    stream << time;
    if (!isMegaCmdSource(source))
    {
        stream << " sdk ";
    }
    else
    {
        stream << " cmd ";
    }
    stream << loglevelToShortPaddedString(logLevel) << message;
    if (surround)
    {
        stream << "]";
    }
    else
    {
        stream << " [" << source << "]";
    }
    stream << '\n';

    if (logLevel <= mFlushOnLevel)
    {
        stream.flush();
    }
}

bool MegaCmdLogger::shouldIgnoreMessage(int logLevel, const char *source, const char *message) const
{
    UNUSED(logLevel);

    if (!isMegaCmdSource(source))
    {
        const int sdkLoggerLevel = getSdkLoggerLevel();
        if (sdkLoggerLevel <= MegaApi::LOG_LEVEL_DEBUG && !strcmp(message, "Request (RETRY_PENDING_CONNECTIONS) starting"))
        {
            return true;
        }
        if (sdkLoggerLevel <= MegaApi::LOG_LEVEL_DEBUG && !strcmp(message, "Request (RETRY_PENDING_CONNECTIONS) finished"))
        {
            return true;
        }
    }
    return false;
}

bool MegaCmdSimpleLogger::shouldLogToStream(int logLevel, const char* source) const
{
    if (isMegaCmdSource(source))
    {
        return logLevel <= getCmdLoggerLevel();
    }
    return logLevel <= getSdkLoggerLevel();
}

bool MegaCmdSimpleLogger::shouldLogToClient(int logLevel, const char* source) const
{
    // If comming from this logger current thread
    if (&OUTSTREAM == &mLoggedStream)
    {
        return false;
    }

    const int defaultLogLevel = isMegaCmdSource(source) ? getCmdLoggerLevel() : getSdkLoggerLevel();

    int currentThreadLogLevel = getCurrentThreadLogLevel();
    if (currentThreadLogLevel < 0) // this thread has no log level assigned
    {
        currentThreadLogLevel = defaultLogLevel;
    }

    return logLevel <= currentThreadLogLevel;
}

MegaCmdSimpleLogger::MegaCmdSimpleLogger(bool logToOutStream, int sdkLoggerLevel, int cmdLoggerLevel) :
    MegaCmdLogger(),
    mLoggedStream(Instance<DefaultLoggedStream>::Get().getLoggedStream()),
    mOutStream(&COUT),
    mLogToOutStream(logToOutStream)
{
    setSdkLoggerLevel(sdkLoggerLevel);
    setCmdLoggerLevel(cmdLoggerLevel);
}

int MegaCmdSimpleLogger::getMaxLogLevel() const
{
    return std::max(getCurrentThreadLogLevel(), MegaCmdLogger::getMaxLogLevel());
}

void MegaCmdSimpleLogger::log(const char * /*time*/, int logLevel, const char *source, const char *message)
{
    thread_local bool isRecursive = false;
    if (isRecursive)
    {
        std::cerr << "ERROR: Attempt to log message recursively from " << source << ": " << message << std::endl;
        assert(false);
        return;
    }
    ScopeGuard g([] { isRecursive = false; });
    isRecursive = true;

    if (!isValidUtf8(message, strlen(message)))
    {
        constexpr const char* invalid = "<invalid utf8>";
        message = invalid;
        ASSERT_UTF8_BREAK("Attempt to log invalid utf8 string");
    }

    if (shouldIgnoreMessage(logLevel, source, message))
    {
        return;
    }

    if (shouldLogToStream(logLevel, source))
    {
        // log to _file_ (e.g: FileRotatingLoggedStream)
        const std::string nowTimeStr = getNowTimeStr();
        formatLogToStream(mLoggedStream, nowTimeStr, logLevel, source, message);

        if (mLogToOutStream) // log to stdout
        {
#ifdef _WIN32
            WindowsUtf8StdoutGuard utf8Guard;
#endif
            formatLogToStream(mOutStream, nowTimeStr, logLevel, source, message);
        }
    }

    if (shouldLogToClient(logLevel, source))
    {
        const std::string nowTimeStr = getNowTimeStr();
        formatLogToStream(getCurrentThreadErrStream(), nowTimeStr, logLevel, source, message, true);
    }
}

LoggedStreamDefaultFile::LoggedStreamDefaultFile() :
    LoggedStreamOutStream(nullptr),
    mFstream(MegaCmdLogger::getDefaultFilePath())
{
    out = &mFstream;
    if (!mFstream.is_open())
    {
        CERR << "Cannot open default log file " << MegaCmdLogger::getDefaultFilePath() << std::endl;
    }
}

}//end namespace
