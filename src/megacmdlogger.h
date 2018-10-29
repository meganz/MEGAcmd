/**
 * @file src/megacmdlogger.h
 * @brief MEGAcmd: Controls message logging
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

#ifndef MEGACMDLOGGER_H
#define MEGACMDLOGGER_H

#include "megacmd.h"
#include "comunicationsmanager.h"

#define OUTSTREAM getCurrentOut()

class LoggedStream {
public:
  LoggedStream(){out = NULL;}
  LoggedStream(OUTSTREAMTYPE *_out):out(_out){

  }

  virtual bool isClientConnected(){return true;}

  virtual const LoggedStream& operator<<(const char& v) const {*out << v;return *this;}
  virtual const LoggedStream& operator<<(const char* v) const {*out << v;return *this;}
#ifdef _WIN32
  virtual const LoggedStream& operator<<(std::wstring v) const {*out << v;return *this;}
#endif
  virtual const LoggedStream& operator<<(std::string v) const {*out << v;return *this;}
  virtual const LoggedStream& operator<<(int v) const {*out << v;return *this;}
  virtual const LoggedStream& operator<<(long long int v) const {*out << v;return *this;}
  virtual const LoggedStream& operator<<(std::ios_base v) const {*out << &v;return *this;}
  virtual const LoggedStream& operator<<(std::ios_base *v) const {*out << v;return *this;}

  virtual LoggedStream const& operator<<(OUTSTREAMTYPE& (*F)(OUTSTREAMTYPE&)) const { if (out) F(*out); return *this; }
protected:
  OUTSTREAMTYPE * out;
};

class LoggedStreamPartialOutputs : public LoggedStream{
public:

  LoggedStreamPartialOutputs(ComunicationsManager *_cm, CmdPetition *_inf):cm(_cm),inf(_inf){}
  virtual bool isClientConnected(){return inf && !inf->clientDisconnected;}

  virtual const LoggedStream& operator<<(const char& v) const {OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this;}
  virtual const LoggedStream& operator<<(const char* v) const {OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this;}
#ifdef _WIN32
  virtual const LoggedStream& operator<<(std::wstring v) const {cm->sendPartialOutput(inf, &v); return *this;}
  virtual const LoggedStream& operator<<(std::string v) const {cm->sendPartialOutput(inf, (char *)v.data(), v.size()); return *this;}
#else
  virtual const LoggedStream& operator<<(std::string v) const {cm->sendPartialOutput(inf, &v); return *this;}
#endif
  virtual const LoggedStream& operator<<(int v) const {OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this;}
  virtual const LoggedStream& operator<<(long long int v) const {OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this;}
  virtual const LoggedStream& operator<<(std::ios_base v) const {*out << &v;return *this;}
  virtual const LoggedStream& operator<<(std::ios_base *v) const {OUTSTRINGSTREAM os; os << v; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this;}

  LoggedStream const& operator<<(OUTSTREAMTYPE& (*F)(OUTSTREAMTYPE&)) const
  {
      if (F == (OUTSTREAMTYPE& (*)(OUTSTREAMTYPE&) )(std::endl))
      {
          OUTSTRINGSTREAM os; os << "\n"; OUTSTRING s = os.str(); cm->sendPartialOutput(inf, &s); return *this;
      }
      else
      {
          std::cerr << "unable to identify f:" << std::endl;
      }
      return *this;
  }

protected:
  CmdPetition *inf;
  ComunicationsManager *cm;
};

LoggedStream &getCurrentOut();
bool interactiveThread();
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
private:
    int apiLoggerLevel;
    int cmdLoggerLevel;
    LoggedStream * output;
public:
    MegaCMDLogger(LoggedStream * outstr)
    {
        this->output = outstr;
        this->apiLoggerLevel = mega::MegaApi::LOG_LEVEL_ERROR;
    }

    void log(const char *time, int loglevel, const char *source, const char *message);

    void setApiLoggerLevel(int apiLoggerLevel)
    {
        this->apiLoggerLevel = apiLoggerLevel;
    }

    void setCmdLoggerLevel(int cmdLoggerLevel)
    {
        this->cmdLoggerLevel = cmdLoggerLevel;
    }

    int getMaxLogLevel();

    int getApiLoggerLevel()
    {
        return this->apiLoggerLevel;
    }

    int getCmdLoggerLevel()
    {
        return this->cmdLoggerLevel;
    }
};

#endif // MEGACMDLOGGER_H
