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

#include "megacmd.h"
#include "comunicationsmanager.h"

#define OUTSTREAM getCurrentOut()

namespace megacmd {
class LoggedStream
{
public:
    LoggedStream() { out = nullptr; }
    LoggedStream(OUTSTREAMTYPE *_out) : out(_out) {}
    virtual ~LoggedStream() = default;

    virtual bool isClientConnected() { return true; }

    virtual bool isStdOut() const { return out == &COUT; }

    virtual const LoggedStream& operator<<(const char& v) const = 0;
    virtual const LoggedStream& operator<<(const char* v) const = 0;
#ifdef _WIN32
    virtual const LoggedStream& operator<<(std::wstring v) const = 0;
#endif
    virtual const LoggedStream& operator<<(std::string v) const = 0;
    virtual const LoggedStream& operator<<(int v) const = 0;
    virtual const LoggedStream& operator<<(unsigned int v) const = 0;
    virtual const LoggedStream& operator<<(long unsigned int v) const = 0;
    virtual const LoggedStream& operator<<(long long int v) const = 0;
    virtual const LoggedStream& operator<<(std::ios_base v) const = 0;
    virtual const LoggedStream& operator<<(std::ios_base *v) const = 0;

    virtual LoggedStream const& operator<<(OUTSTREAMTYPE& (*F)(OUTSTREAMTYPE&)) const = 0;
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
    const LoggedStream& operator<<(int v) const override { return *this; }
    const LoggedStream& operator<<(unsigned int v) const override { return *this; }
    const LoggedStream& operator<<(long unsigned int v) const override { return *this; }
    const LoggedStream& operator<<(long long int v) const override { return *this; }
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
#endif
    virtual const LoggedStream& operator<<(std::string v) const override { *out << v;return *this; }
    virtual const LoggedStream& operator<<(int v) const override { *out << v;return *this; }
    virtual const LoggedStream& operator<<(unsigned int v) const override { *out << v;return *this; }
    virtual const LoggedStream& operator<<(long unsigned int v) const override { *out << v;return *this; }
    virtual const LoggedStream& operator<<(long long int v) const override { *out << v;return *this; }
    virtual const LoggedStream& operator<<(std::ios_base v) const override { *out << &v;return *this; }
    virtual const LoggedStream& operator<<(std::ios_base *v) const override { *out << v;return *this; }

    virtual LoggedStream const& operator<<(OUTSTREAMTYPE& (*F)(OUTSTREAMTYPE&)) const override { if (out) F(*out); return *this; }
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
    virtual bool isClientConnected() { return inf && !inf->clientDisconnected; }

    virtual const LoggedStream& operator<<(const char& v) const override { OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this; }
    virtual const LoggedStream& operator<<(const char* v) const override { OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this; }

#ifdef _WIN32
    virtual const LoggedStream& operator<<(std::wstring v) const override { cm->sendPartialOutput(inf, &v); return *this; }
    virtual const LoggedStream& operator<<(std::string v) const override { cm->sendPartialOutput(inf, (char *)v.data(), v.size()); return *this; }
#else
    virtual const LoggedStream& operator<<(std::string v) const override { cm->sendPartialOutput(inf, &v); return *this; }
#endif

    virtual const LoggedStream& operator<<(int v) const override { OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this; }
    virtual const LoggedStream& operator<<(unsigned int v) const override { OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this; }
    virtual const LoggedStream& operator<<(long unsigned int v) const override { OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this; }
    virtual const LoggedStream& operator<<(long long int v) const override { OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this; }
    virtual const LoggedStream& operator<<(std::ios_base v) const override { *out << &v;return *this; }
    virtual const LoggedStream& operator<<(std::ios_base *v) const override { OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this; }

    LoggedStream const& operator<<(OUTSTREAMTYPE& (*F)(OUTSTREAMTYPE&)) const
    {
        OUTSTRINGSTREAM os; os << F; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s);
        return *this;
    }

    virtual ~LoggedStreamPartialOutputs() = default;

protected:
    CmdPetition *inf;
    ComunicationsManager *cm;
};

LoggedStream &getCurrentOut();
bool interactiveThread();
const char *commandPrefixBasedOnMode();
void setCurrentThreadOutStream(LoggedStream *);
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
    int sdkLoggerLevel;
    int cmdLoggerLevel;
    LoggedStream &mLoggedStream;
    std::mutex mSetmodeMtx;

    template <typename Cb>
    void performSafeLog(const LoggedStream &stream, Cb &&doLogCb)
    {
#if defined(_WIN32) && !defined(MEGACMD_TESTING_CODE)
        int oldmode = -1;
        std::unique_lock<std::mutex> setmodeLck(mSetmodeMtx, std::defer_lock);
        if (stream.isStdOut())
        {
            setmodeLck.lock();
            oldmode = _setmode(_fileno(stdout), _O_U8TEXT);
        }
#endif

        doLogCb();

#if defined(_WIN32) && !defined(MEGACMD_TESTING_CODE)
        if (stream.isStdOut())
        {
            assert(oldmode != -1);
            _setmode(_fileno(stdout), oldmode);
        }
#endif
    }

public:
    MegaCMDLogger();

    void log(const char *time, int loglevel, const char *source, const char *message);

    void setSdkLoggerLevel(int sdkLoggerLevel)
    {
        this->sdkLoggerLevel = sdkLoggerLevel;
    }

    void setCmdLoggerLevel(int cmdLoggerLevel)
    {
        this->cmdLoggerLevel = cmdLoggerLevel;
    }

    int getMaxLogLevel();

    int getSdkLoggerLevel()
    {
        return this->sdkLoggerLevel;
    }

    int getCmdLoggerLevel()
    {
        return this->cmdLoggerLevel;
    }
};

}//end namespace
#endif // MEGACMDLOGGER_H
