/**
 * @file src/megacmdlogger.h
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

#ifndef MEGACMDLOGGER_H
#define MEGACMDLOGGER_H

#include "megacmdcommonutils.h"

#include "megacmd.h"
#include "comunicationsmanager.h"

#define OUTSTREAM getCurrentThreadOutStream()

namespace megacmd {

#ifdef WIN32
inline ::mega::SimpleLogger &operator<<(::mega::SimpleLogger& sl, const fs::path& path)
{
    return sl << megacmd::pathAsUtf8(path);
}

template <typename T>
inline std::enable_if_t<std::is_same_v<std::decay_t<T>, std::wstring>, ::mega::SimpleLogger &>
operator<<(::mega::SimpleLogger& sl, const T& wstr)
{
    return sl << megacmd::utf16ToUtf8(wstr);
}
#endif

// String used to transmit binary data
class BinaryStringView
{
public:
    BinaryStringView(char *buffer, size_t size)
        : mValue(buffer, size)
    {}

    const std::string_view& get() const {
        return mValue;
    }

    std::string_view& get() {
        return mValue;
    }

private:
    std::string_view mValue;
};

class LoggedStream
{
public:
    LoggedStream() { out = nullptr; }
    LoggedStream(OUTSTREAMTYPE *_out) : out(_out) {}
    virtual ~LoggedStream() = default;

    virtual bool isClientConnected() { return true; }

    virtual const LoggedStream& operator<<(const char& v) const = 0;
    virtual const LoggedStream& operator<<(const char* v) const = 0;
#ifdef _WIN32
    virtual const LoggedStream& operator<<(std::wstring v) const = 0;
#endif
    virtual const LoggedStream& operator<<(std::string v) const = 0;
    virtual const LoggedStream& operator<<(BinaryStringView v) const = 0;
    virtual const LoggedStream& operator<<(std::string_view v) const = 0;
    virtual const LoggedStream& operator<<(int v) const = 0;
    virtual const LoggedStream& operator<<(unsigned int v) const = 0;
    virtual const LoggedStream& operator<<(long unsigned int v) const = 0;
    virtual const LoggedStream& operator<<(long long int v) const = 0;
    virtual const LoggedStream& operator<<(long long unsigned int v) const = 0;
    virtual const LoggedStream& operator<<(std::ios_base v) const = 0;
    virtual const LoggedStream& operator<<(std::ios_base *v) const = 0;

    virtual LoggedStream const& operator<<(OUTSTREAMTYPE& (*F)(OUTSTREAMTYPE&)) const = 0;

    virtual void flush() {}
protected:
    OUTSTREAMTYPE * out;
};

class LoggedStreamNull : public LoggedStream
{
public:
    const LoggedStream& operator<<(const char& v) const override { return *this; }
    const LoggedStream& operator<<(const char* v) const override { return *this; }
#ifdef _WIN32
    const LoggedStream& operator<<(std::wstring v) const override { return *this; }
#endif

    const LoggedStream& operator<<(std::string v) const override { return *this; }
    const LoggedStream& operator<<(BinaryStringView v) const override { return *this; }
    const LoggedStream& operator<<(std::string_view v) const override { return *this; }
    const LoggedStream& operator<<(int v) const override { return *this; }
    const LoggedStream& operator<<(unsigned int v) const override { return *this; }
    const LoggedStream& operator<<(long unsigned int v) const override { return *this; }
    const LoggedStream& operator<<(long long int v) const override { return *this; }
    const LoggedStream& operator<<(long long unsigned int v) const override { return *this; }
    const LoggedStream& operator<<(std::ios_base v) const override { return *this; }
    const LoggedStream& operator<<(std::ios_base *v) const override { return *this; }

    LoggedStream const& operator<<(OUTSTREAMTYPE& (*F)(OUTSTREAMTYPE&)) const override { return *this; }

    virtual ~LoggedStreamNull() = default;
};

class DefaultLoggedStream
{
    std::unique_ptr<LoggedStream> mTheStream;
public:
    void setLoggedStream(std::unique_ptr<LoggedStream> &&loggedStream)
    {
        mTheStream = std::move(loggedStream);
    }

    LoggedStream &getLoggedStream()
    {
        if (!mTheStream)
        {
            mTheStream = std::make_unique<LoggedStreamNull>();
        }
        assert(mTheStream);
        return *mTheStream.get();
    }
};


class LoggedStreamOutStream : public LoggedStream
{
public:
    LoggedStreamOutStream(OUTSTREAMTYPE *out) : LoggedStream(out) {}

    virtual ~LoggedStreamOutStream() = default;

    virtual bool isClientConnected() override { return true; }

    virtual const LoggedStream& operator<<(const char& v) const override { *out << v;return *this; }
    virtual const LoggedStream& operator<<(const char* v) const override { *out << v;return *this; }
#ifdef _WIN32
    virtual const LoggedStream& operator<<(std::wstring v) const override { *out << v;return *this; }
    virtual const LoggedStream& operator<<(std::string_view v) const override { *out << std::string(v);return *this; }
#else
    virtual const LoggedStream& operator<<(std::string_view v) const override { *out << v;return *this; }
#endif
    virtual const LoggedStream& operator<<(std::string v) const override { *out << v;return *this; }
    virtual const LoggedStream& operator<<(BinaryStringView v) const override {
#ifdef _WIN32
        assert(false && "wostream cannot take binary data directly");
#else
        *out << v.get();
#endif
    return *this;
    }

    virtual const LoggedStream& operator<<(int v) const override { *out << v;return *this; }
    virtual const LoggedStream& operator<<(unsigned int v) const override { *out << v;return *this; }
    virtual const LoggedStream& operator<<(long unsigned int v) const override { *out << v;return *this; }
    virtual const LoggedStream& operator<<(long long int v) const override { *out << v;return *this; }
    virtual const LoggedStream& operator<<(long long unsigned int v) const override { *out << v;return *this; }
    virtual const LoggedStream& operator<<(std::ios_base v) const override { *out << &v;return *this; }
    virtual const LoggedStream& operator<<(std::ios_base *v) const override { *out << v;return *this; }

    virtual LoggedStream const& operator<<(OUTSTREAMTYPE& (*F)(OUTSTREAMTYPE&)) const override { if (out) F(*out); return *this; }

    virtual void flush() override { out->flush(); }
};

class LoggedStreamDefaultFile : public LoggedStreamOutStream
{
    OUTFSTREAMTYPE mFstream;
public:
    LoggedStreamDefaultFile();
    virtual ~LoggedStreamDefaultFile() = default;
};

class LoggedStreamPartialOutputs : public LoggedStream
{
public:
    LoggedStreamPartialOutputs(ComunicationsManager *_cm, CmdPetition *_inf) : cm(_cm), inf(_inf) {}
    virtual bool isClientConnected() override { return inf && !inf->clientDisconnected; }

    virtual const LoggedStream& operator<<(const char& v) const override { OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this; }
    virtual const LoggedStream& operator<<(const char* v) const override { OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this; }

#ifdef _WIN32
    virtual const LoggedStream& operator<<(std::wstring v) const override { cm->sendPartialOutput(inf, &v); return *this; }
    virtual const LoggedStream& operator<<(std::string v) const override { cm->sendPartialOutput(inf, (char *)v.data(), v.size()); return *this; }
#else
    virtual const LoggedStream& operator<<(std::string v) const override { cm->sendPartialOutput(inf, &v); return *this; }
#endif
    virtual const LoggedStream& operator<<(BinaryStringView v) const override { cm->sendPartialOutput(inf, (char*) v.get().data(), v.get().size(), true); return *this; }
    virtual const LoggedStream& operator<<(std::string_view v) const override { cm->sendPartialOutput(inf, (char*) v.data(), v.size()); return *this; }

    virtual const LoggedStream& operator<<(int v) const override { OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this; }
    virtual const LoggedStream& operator<<(unsigned int v) const override { OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this; }
    virtual const LoggedStream& operator<<(long unsigned int v) const override { OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this; }
    virtual const LoggedStream& operator<<(long long int v) const override { OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this; }
    virtual const LoggedStream& operator<<(long long unsigned int v) const override { OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this; }
    virtual const LoggedStream& operator<<(std::ios_base v) const override { *out << &v;return *this; }
    virtual const LoggedStream& operator<<(std::ios_base *v) const override { OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this; }

    LoggedStream const& operator<<(OUTSTREAMTYPE& (*F)(OUTSTREAMTYPE&)) const
    {
        OUTSTRINGSTREAM os; os << F; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s);
        return *this;
    }

    virtual ~LoggedStreamPartialOutputs() = default;

protected:
    ComunicationsManager *cm;
    CmdPetition *inf;
};

struct ThreadData
{
    LoggedStream *mOutStream = &Instance<DefaultLoggedStream>::Get().getLoggedStream();
    int mLogLevel = -1;
    int mOutCode = 0;
    CmdPetition *mCmdPetition = nullptr;
    bool mIsCmdShell = false;
};

ThreadData &getCurrentThreadData();
const char* getCommandPrefixBasedOnMode();
bool isCurrentThreadInteractive();

inline LoggedStream &getCurrentThreadOutStream()  { return *getCurrentThreadData().mOutStream; }
inline int getCurrentThreadLogLevel()             { return getCurrentThreadData().mLogLevel; }
inline int getCurrentThreadOutCode()              { return getCurrentThreadData().mOutCode; }
inline CmdPetition *getCurrentThreadCmdPetition() { return getCurrentThreadData().mCmdPetition; }
inline bool isCurrentThreadCmdShell()             { return getCurrentThreadData().mIsCmdShell; }

void setCurrentThreadOutStream(LoggedStream &outStream);
void setCurrentThreadOutCode(int outCode);
void setCurrentThreadLogLevel(int logLevel);
void setCurrentThreadCmdPetition(CmdPetition *cmdPetition);
void setCurrentThreadIsCmdShell(bool isCmdShell);

constexpr size_t LogTimestampSize = std::char_traits<char>::length("2024-12-27_16-33-12.654787");
std::optional<std::chrono::time_point<std::chrono::system_clock>> stringToTimestamp(std::string_view str);
std::string timestampToString(std::chrono::time_point<std::chrono::system_clock> timestamp);

class MegaCmdLogger : public mega::MegaLogger
{
    int mSdkLoggerLevel;
    int mCmdLoggerLevel;
    int mFlushOnLevel;

protected:
    static bool isMegaCmdSource(const std::string &source);

    void formatLogToStream(LoggedStream& stream, std::string_view time, int logLevel, const char *source, const char *message, bool surround = false);
    bool shouldIgnoreMessage(int logLevel, const char *source, const char *message) const;

public:
    MegaCmdLogger();
    virtual ~MegaCmdLogger() = default;

    virtual void log(const char *time, int loglevel, const char *source, const char *message) override = 0;

    void setSdkLoggerLevel(int sdkLoggerLevel) { mSdkLoggerLevel = sdkLoggerLevel; }
    void setCmdLoggerLevel(int cmdLoggerLevel) { mCmdLoggerLevel = cmdLoggerLevel; }
    void setFlushOnLevel(int flushOnLevel)     { mFlushOnLevel = flushOnLevel; }

    int getSdkLoggerLevel() const { return mSdkLoggerLevel; }
    int getCmdLoggerLevel() const { return mCmdLoggerLevel; }
    int getFlushOnLevel()   const { return mFlushOnLevel; }

    virtual int getMaxLogLevel() const { return std::max(mSdkLoggerLevel, mCmdLoggerLevel); }

    static fs::path getDefaultFilePath();
};

class MegaCmdSimpleLogger final : public MegaCmdLogger
{
    LoggedStream &mLoggedStream; // to log into files (e.g. FileRotatingLoggedStream)
    LoggedStreamOutStream mOutStream; // to log into stdout
    bool mLogToOutStream;

    bool shouldLogToStream(int logLevel, const char *source) const;
    bool shouldLogToClient(int logLevel, const char *source) const;

public:
    MegaCmdSimpleLogger(bool logToOutStream, int sdkLoggerLevel, int cmdLoggerLevel);

    void log(const char *time, int loglevel, const char *source, const char *message) override;

    int getMaxLogLevel() const override;
};

}//end namespace
#endif // MEGACMDLOGGER_H
