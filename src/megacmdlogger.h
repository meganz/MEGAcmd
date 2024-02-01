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

#define OUTSTREAM Instance<ThreadLookupTable>::Get().getCurrentOutStream()

namespace megacmd {
class LoggedStream
{
public:
    LoggedStream(){out = NULL;}
    LoggedStream(OUTSTREAMTYPE *_out):out(_out){
    }
    virtual ~LoggedStream() = default;

    virtual bool isClientConnected(){return true;}

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
  virtual const LoggedStream& operator<<(std::wstring v) const override { return *this; }
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
            mTheStream = ::mega::make_unique<LoggedStreamNull>();
        }
        assert(mTheStream);
        return *mTheStream.get();
    }
};


class LoggedStreamOutStream : public LoggedStream
{
public:
  LoggedStreamOutStream(OUTSTREAMTYPE *_out):out(_out){}

  virtual bool isClientConnected() override {return true;}

  virtual const LoggedStream& operator<<(const char& v) const override {*out << v;return *this;}
  virtual const LoggedStream& operator<<(const char* v) const override {*out << v;return *this;}
#ifdef _WIN32
  virtual const LoggedStream& operator<<(std::wstring v) const override {*out << v;return *this;}
#endif
  virtual const LoggedStream& operator<<(std::string v) const override {*out << v;return *this;}
  virtual const LoggedStream& operator<<(int v) const override {*out << v;return *this;}
  virtual const LoggedStream& operator<<(unsigned int v) const override {*out << v;return *this;}
  virtual const LoggedStream& operator<<(long unsigned int v) const override {*out << v;return *this;}
  virtual const LoggedStream& operator<<(long long int v) const override {*out << v;return *this;}
  virtual const LoggedStream& operator<<(std::ios_base v) const override {*out << &v;return *this;}
  virtual const LoggedStream& operator<<(std::ios_base *v) const override {*out << v;return *this;}

  virtual LoggedStream const& operator<<(OUTSTREAMTYPE& (*F)(OUTSTREAMTYPE&)) const override { if (out) F(*out); return *this; }

  virtual ~LoggedStreamOutStream() = default;
protected:
  OUTSTREAMTYPE * out;
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

  LoggedStreamPartialOutputs(ComunicationsManager *_cm, CmdPetition *_inf):cm(_cm),inf(_inf){}
  virtual bool isClientConnected(){return inf && !inf->clientDisconnected;}

  virtual const LoggedStream& operator<<(const char& v) const override {OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this;}
  virtual const LoggedStream& operator<<(const char* v) const override {OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this;}
#ifdef _WIN32
  virtual const LoggedStream& operator<<(std::wstring v) const override {cm->sendPartialOutput(inf, &v); return *this;}
  virtual const LoggedStream& operator<<(std::string v) const override {cm->sendPartialOutput(inf, (char *)v.data(), v.size()); return *this;}
#else
  virtual const LoggedStream& operator<<(std::string v) const override {cm->sendPartialOutput(inf, &v); return *this;}
#endif
  virtual const LoggedStream& operator<<(int v) const override {OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this;}
  virtual const LoggedStream& operator<<(unsigned int v) const override {OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this;}
  virtual const LoggedStream& operator<<(long unsigned int v) const override {OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this;}
  virtual const LoggedStream& operator<<(long long int v) const override {OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this;}
  virtual const LoggedStream& operator<<(std::ios_base v) const override {*out << &v;return *this;}
  virtual const LoggedStream& operator<<(std::ios_base *v) const override {OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this;}

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

class ThreadLookupTable final
{
    struct ThreadData
    {
        LoggedStream *outStream;
        int logLevel;
        int outCode;
        CmdPetition *cmdPetition;
        bool isCmdShell;

        ThreadData();
    };

    mutable std::mutex mMapMutex;
    std::map<uint64_t, ThreadData> mThreadMap;

    ThreadData getThreadData(uint64_t id) const;
    ThreadData getCurrentThreadData() const;
    bool threadDataExists(uint64_t id) const;

public:
    LoggedStream &getCurrentOutStream() const;
    int getCurrentLogLevel() const;
    int getCurrentOutCode() const;
    CmdPetition *getCurrentCmdPetition() const;
    bool isCurrentCmdShell() const;

    void setCurrentOutStream(LoggedStream &outStream);
    void setCurrentLogLevel(int logLevel);
    void setCurrentOutCode(int outCode);
    void setCurrentCmdPetition(CmdPetition *cmdPetition);
    void setCurrentIsCmdShell(bool isCmdShell);

    bool isCurrentThreadInteractive() const;
    const char* getModeCommandPrefix() const;
};

inline int getCurrentThreadLogLevel()             { return Instance<ThreadLookupTable>::Get().getCurrentLogLevel(); }
inline int getCurrentThreadOutCode()              { return Instance<ThreadLookupTable>::Get().getCurrentOutCode(); }
inline CmdPetition *getCurrentThreadCmdPetition() { return Instance<ThreadLookupTable>::Get().getCurrentCmdPetition(); }
inline bool isCurrentThreadCmdShell()             { return Instance<ThreadLookupTable>::Get().isCurrentCmdShell(); }

inline void setCurrentThreadOutStream(LoggedStream &outStream)    { Instance<ThreadLookupTable>::Get().setCurrentOutStream(outStream); }
inline void setCurrentThreadOutCode(int outCode)                  { Instance<ThreadLookupTable>::Get().setCurrentOutCode(outCode); }
inline void setCurrentThreadLogLevel(int logLevel)                { Instance<ThreadLookupTable>::Get().setCurrentLogLevel(logLevel); }
inline void setCurrentThreadCmdPetition(CmdPetition *cmdPetition) { Instance<ThreadLookupTable>::Get().setCurrentCmdPetition(cmdPetition); }
inline void setCurrentThreadIsCmdShell(bool isCmdShell)           { Instance<ThreadLookupTable>::Get().setCurrentIsCmdShell(isCmdShell); }

inline bool isCurrentThreadInteractive()         { return Instance<ThreadLookupTable>::Get().isCurrentThreadInteractive(); }
inline const char *getCommandPrefixBasedOnMode() { return Instance<ThreadLookupTable>::Get().getModeCommandPrefix(); }

class MegaCmdLogger : public mega::MegaLogger
{
    int mSdkLoggerLevel;
    int mCmdLoggerLevel;
public:
    MegaCmdLogger() :
        mSdkLoggerLevel(mega::MegaApi::LOG_LEVEL_ERROR),
        mCmdLoggerLevel(mega::MegaApi::LOG_LEVEL_ERROR)
    {
    }

    virtual ~MegaCmdLogger() = default;

    virtual void log(const char *time, int loglevel, const char *source, const char *message) override = 0;

    void setSdkLoggerLevel(int sdkLoggerLevel) { mSdkLoggerLevel = sdkLoggerLevel; }
    void setCmdLoggerLevel(int cmdLoggerLevel) { mCmdLoggerLevel = cmdLoggerLevel; }

    int getSdkLoggerLevel() const { return mSdkLoggerLevel; }
    int getCmdLoggerLevel() const { return mCmdLoggerLevel; }

    virtual int getMaxLogLevel() const { return std::max(mSdkLoggerLevel, mCmdLoggerLevel); }
};

class MegaCmdSimpleLogger final : public MegaCmdLogger
{
    LoggedStream &mLoggedStream;
public:
    MegaCmdSimpleLogger();

    void log(const char *time, int loglevel, const char *source, const char *message) override;

    int getMaxLogLevel() const override;
};

}//end namespace
#endif // MEGACMDLOGGER_H
