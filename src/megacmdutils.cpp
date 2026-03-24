/**
 * @file src/megacmdutils.cpp
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

#include "megacmdutils.h"
#include "mega/types.h"

#ifdef USE_PCRE
#include <pcrecpp.h>
#elif __cplusplus >= 201103L && !defined(__MINGW32__)
#include <regex>
#endif

#ifdef _WIN32
#else
#include <sys/ioctl.h> // console size
#endif

#include <iomanip>
#include <fstream>
#include <time.h>

using namespace mega;

namespace megacmd {
string getUserInSharedNode(MegaNode *n, MegaApi *api)
{
    MegaShareList * msl = api->getInSharesList();
    for (int i = 0; i < msl->size(); i++)
    {
        MegaShare *share = msl->get(i);

        if (share->getNodeHandle() == n->getHandle())
        {
            string suser = share->getUser();
            delete ( msl );
            return suser;
        }
    }

    delete ( msl );
    return "";
}

const char* getAccessLevelStr(int level)
{
    switch (level)
    {
        case MegaShare::ACCESS_UNKNOWN:
            return "unknown access";

            break;

        case MegaShare::ACCESS_READ:
            return "read access";

            break;

        case MegaShare::ACCESS_READWRITE:
            return "read/write access";

            break;

        case MegaShare::ACCESS_FULL:
            return "full access";

            break;

        case MegaShare::ACCESS_OWNER:
            return "owner access";

            break;
    }
    return "undefined";
}

const char* getSyncPathStateStr(int state)
{
    static std::vector<const char*> svalue {
#define SOME_GENERATOR_MACRO(_, ShortName, __) ShortName,
      GENERATE_FROM_SYNC_PATH_STATE(SOME_GENERATOR_MACRO)
#undef SOME_GENERATOR_MACRO
    };
    if (state < static_cast<int>(svalue.size()))
    {
        return svalue[state];
    }
    return "undefined";
}

const char * syncRunStateStr(unsigned e)
{
    static std::vector<const char*> svalue {
#define SOME_GENERATOR_MACRO(_, ShortName, __) ShortName,
      GENERATE_FROM_SYNC_RUN_STATE(SOME_GENERATOR_MACRO)
#undef SOME_GENERATOR_MACRO
    };
    if (e < svalue.size())
    {
        return svalue[e];
    }
    return "Unknown";
}


string visibilityToString(int visibility)
{
    if (visibility == MegaUser::VISIBILITY_VISIBLE)
    {
        return "Visible";
    }
    if (visibility == MegaUser::VISIBILITY_HIDDEN)
    {
        return "Hidden";
    }
    if (visibility == MegaUser::VISIBILITY_UNKNOWN)
    {
        return "Unknown visibility";
    }
    if (visibility == MegaUser::VISIBILITY_INACTIVE)
    {
        return "Inactive";
    }
    if (visibility == MegaUser::VISIBILITY_BLOCKED)
    {
        return "Blocked";
    }
    return "Undefined visibility";
}

const char * getMCMDErrorString(int errorCode)
{
    switch(errorCode)
    {
    case MCMD_OK:
        return "Everything OK";
    case MCMD_EARGS:
        return "Wrong arguments";
    case MCMD_INVALIDEMAIL:
        return "Invalid email";
    case MCMD_NOTFOUND:
        return "Resource not found";
    case MCMD_INVALIDSTATE:
        return "Invalid state";
    case MCMD_INVALIDTYPE:
        return "Invalid type";
    case MCMD_NOTPERMITTED:
        return "Operation not allowed";
    case MCMD_NOTLOGGEDIN:
        return "Needs logging in";
    case MCMD_NOFETCH:
        return "Nodes not fetched";
    case MCMD_EUNEXPECTED:
        return "Unexpected failure";
    case MCMD_REQCONFIRM:
        return "Confirmation required";
    default:
        return "UNKNOWN";
    }
}

const char * getErrorCodeStr(MegaError *e)
{
    if (e)
    {
        return MegaError::getErrorString(e->getErrorCode());
    }
    return "NullError";
}

const char * getMountResultStr(int mountResult)
{
#ifdef WIN32
    if (mountResult == mega::MegaMount::BACKEND_UNAVAILABLE)
    {
        return "WinFSP is not installed. It needs to be installed for the operation to succeed. You can get it "
               "from https://github.com/winfsp/winfsp/releases/download/v2.1/winfsp-2.1.25156.msi";
    }
#endif

    return MegaMount::getResultDescription(mountResult);
}

const char * getLogLevelStr(int loglevel)
{
    switch (loglevel)
    {
        case MegaApi::LOG_LEVEL_FATAL:
            return "FATAL";

            break;

        case MegaApi::LOG_LEVEL_ERROR:
            return "ERROR";

            break;

        case MegaApi::LOG_LEVEL_WARNING:
            return "WARNING";

            break;

        case MegaApi::LOG_LEVEL_INFO:
            return "INFO";

            break;

        case MegaApi::LOG_LEVEL_DEBUG:
            return "DEBUG";

            break;

        case MegaApi::LOG_LEVEL_MAX:
            return "VERBOSE";

            break;

        default:
            return "UNKNOWN";

            break;
    }
}

std::optional<int> getLogLevelNum(const char* level)
{
    if (!strcmp(level, "FATAL") || !strcmp(level, "fatal"))
    {
        return MegaApi:: LOG_LEVEL_FATAL;
    }
    if (!strcmp(level, "ERROR") || !strcmp(level, "error"))
    {
        return MegaApi:: LOG_LEVEL_ERROR;
    }
    if (!strcmp(level, "WARNING") || !strcmp(level, "warning"))
    {
        return MegaApi:: LOG_LEVEL_WARNING;
    }
    if (!strcmp(level, "INFO") || !strcmp(level, "info"))
    {
        return MegaApi:: LOG_LEVEL_INFO;
    }
    if (!strcmp(level, "DEBUG") || !strcmp(level, "debug"))
    {
        return MegaApi:: LOG_LEVEL_DEBUG;
    }
    if (!strcmp(level, "VERBOSE") || !strcmp(level, "verbose"))
    {
        return MegaApi:: LOG_LEVEL_MAX;
    }

    int i = -1;

    std::istringstream is(level);
    if (!(is >> i))
    {
        return {};
    }

    return std::clamp(i, (int) MegaApi::LOG_LEVEL_FATAL, (int) MegaApi::LOG_LEVEL_MAX);
}


const char * getShareLevelStr(int sharelevel)
{
    switch (sharelevel)
    {
        case MegaShare::ACCESS_UNKNOWN:
            return "UNKNOWN";

            break;

        case MegaShare::ACCESS_READ:
            return "READ";

            break;

        case MegaShare::ACCESS_READWRITE:
            return "READWRITE";

            break;

        case MegaShare::ACCESS_FULL:
            return "FULL";

            break;

        case MegaShare::ACCESS_OWNER:
            return "OWNER";

            break;

        default:
            return "UNEXPECTED";

            break;
    }
}

int getShareLevelNum(const char* level)
{
    if (!strcmp(level, "UNKNOWN"))
    {
        return MegaShare::ACCESS_UNKNOWN;
    }
    if (!strcmp(level, "READ"))
    {
        return MegaShare::ACCESS_READ;
    }
    if (!strcmp(level, "READWRITE"))
    {
        return MegaShare::ACCESS_READWRITE;
    }
    if (!strcmp(level, "FULL"))
    {
        return MegaShare::ACCESS_FULL;
    }
    if (!strcmp(level, "OWNER"))
    {
        return MegaShare::ACCESS_OWNER;
    }
    if (!strcmp(level, "UNEXPECTED"))
    {
        return -9;
    }
    return atoi(level);
}

const char * getTransferStateStr(int transferState)
{
    switch (transferState)
    {
    case MegaTransfer::STATE_QUEUED:
        return "QUEUED";
        break;
    case MegaTransfer::STATE_ACTIVE:
        return "ACTIVE";
        break;
    case MegaTransfer::STATE_PAUSED:
        return "PAUSED";
        break;
    case MegaTransfer::STATE_RETRYING:
        return "RETRYING";
        break;
    case MegaTransfer::STATE_COMPLETING:
        return "COMPLETING";
        break;
    case MegaTransfer::STATE_COMPLETED:
        return "COMPLETED";
        break;
    case MegaTransfer::STATE_CANCELLED:
        return "CANCELLED";
        break;
    case MegaTransfer::STATE_FAILED:
        return "FAILED";
        break;
    default:
        return "";
        break;
    }

}

string backupSatetStr(int backupstate)
{
    if (backupstate == MegaScheduledCopy::SCHEDULED_COPY_FAILED)
    {
        return "FAILED";
    }
    if (backupstate == MegaScheduledCopy::SCHEDULED_COPY_CANCELED)
    {
        return "CANCELED";
    }
    if (backupstate == MegaScheduledCopy::SCHEDULED_COPY_INITIALSCAN)
    {
        return "INITIALSCAN";
    }
    if (backupstate == MegaScheduledCopy::SCHEDULED_COPY_ACTIVE)
    {
        return "ACTIVE";
    }
    if (backupstate == MegaScheduledCopy::SCHEDULED_COPY_ONGOING)
    {
        return "ONGOING";
    }
    if (backupstate == MegaScheduledCopy::SCHEDULED_COPY_SKIPPING)
    {
        return "SKIPPING";
    }
    if (backupstate == MegaScheduledCopy::SCHEDULED_COPY_REMOVING_EXCEEDING)
    {
        return "EXCEEDREMOVAL";
    }

    return "UNDEFINED";
}

const char * getProxyTypeStr(int proxyType)
{
    switch (proxyType)
    {
    case MegaProxy::PROXY_AUTO:
        return "PROXY_AUTO";
        break;
    case MegaProxy::PROXY_NONE:
        return "PROXY_NONE";
        break;
    case MegaProxy::PROXY_CUSTOM:
        return "PROXY_CUSTOM";
        break;
    default:
        return "INVALID";
        break;
    }

}

int getLinkType(string link)
{
    if (link.find("/folder/") != string::npos)
    {
        return MegaNode::TYPE_FOLDER;
    }

    if (link.find("/file/") != string::npos)
    {
        return MegaNode::TYPE_FILE;
    }

    size_t posHash = link.find_first_of("#");
    if (( posHash == string::npos ) || !( posHash + 1 < link.length()))
    {
        return MegaNode::TYPE_UNKNOWN;
    }
    if (link.at(posHash + 1) == 'F')
    {
        return MegaNode::TYPE_FOLDER;
    }
    return MegaNode::TYPE_FILE;
}


const char *fillStructWithSYYmdHMS(string &stime, struct tm &dt)
{
    memset(&dt, 0, sizeof(struct tm));
#ifdef _WIN32
    if (stime.size() != 14)
    {
        return NULL;
    }
    for(int i=0;i<14;i++)
    {
        if ( (stime.at(i) < '0') || (stime.at(i) > '9') )
        {
            return NULL;
        }
    }

    dt.tm_year = atoi(stime.substr(0,4).c_str()) - 1900;
    dt.tm_mon = atoi(stime.substr(4,2).c_str()) - 1;
    dt.tm_mday = atoi(stime.substr(6,2).c_str());
    dt.tm_hour = atoi(stime.substr(8,2).c_str());
    dt.tm_min = atoi(stime.substr(10,2).c_str());
    dt.tm_sec = atoi(stime.substr(12,2).c_str());
#else
    strptime(stime.c_str(), "%Y%m%d%H%M%S", &dt);
#endif
    dt.tm_isdst = -1; //let mktime interprete if time has Daylight Saving Time flag correction
                        //TODO: would this work cross platformly? At least I believe it'll be consistent with localtime. Otherwise, we'd need to save that
    return stime.c_str();

}

const char *getFormatStrFromId(int strftimeformatid)
{
    switch (strftimeformatid)
    {

    case  MCMDTIME_RFC2822:
        return "%a, %d %b %Y %T %z";
        break;
    case MCMDTIME_ISO6081:
        return "%F";
        break;
    case MCMDTIME_ISO6081WITHTIME:
        return "%FT%T";
        break;
    case MCMDTIME_SHORT:
        return "%d%b%Y %T";
        break;
    case MCMDTIME_SHORTWITHUTCDEVIATION:
        return "%d%b%Y %T %z";
        break;

    default:
        return "%a, %d %b %Y %T %z";
        break;
    }
}

const char *getTimeFormatNameFromId(int strftimeformatid)
{
    switch (strftimeformatid)
    {

    case  MCMDTIME_RFC2822:
        return "RFC2822";
        break;
    case MCMDTIME_ISO6081:
        return "ISO6081";
        break;
    case MCMDTIME_ISO6081WITHTIME:
        return "ISO6081_WITH_TIME";
        break;
    case MCMDTIME_SHORT:
        return "SHORT";
        break;
    case MCMDTIME_SHORTWITHUTCDEVIATION:
        return "SHORT_UTC";
        break;
    default:
        return "INVALID";
        break;
    }
}

string secondsToText(m_time_t seconds, bool humanreadable)
{
    ostringstream os;
    os.precision(2);
    if (humanreadable)
    {
        m_time_t reducedSize = m_time_t( seconds > 3600 * 2 ? seconds / 3600.0 : ( seconds > 60 * 2 ? seconds / 60.0 : seconds) );
        os << std::fixed << reducedSize;
        os << ( seconds > 3600 * 2 ? " hours" : ( seconds > 60 * 2 ? " minutes" : " seconds" ));
    }
    else
    {
        os << seconds;
    }

    return os.str();
}

const char *getTimeFormatFromSTR(string formatName)
{
    string lformatName = formatName;
    transform(lformatName.begin(), lformatName.end(), lformatName.begin(), [](char c) { return (char)::tolower(c); });

    if (lformatName == "rfc2822")
    {
        return getFormatStrFromId(MCMDTIME_RFC2822);
    }
    else if (lformatName == "iso6081")
    {
        return getFormatStrFromId(MCMDTIME_ISO6081);
    }
    else if (lformatName == "iso6081_with_time")
    {
        return getFormatStrFromId(MCMDTIME_ISO6081WITHTIME);
    }
    else if (lformatName == "short")
    {
        return getFormatStrFromId(MCMDTIME_SHORT);
    }
    else if (lformatName == "short_utc")
    {
        return getFormatStrFromId(MCMDTIME_SHORTWITHUTCDEVIATION);
    }
    else
    {
        return formatName.c_str();
    }
}

std::string getReadableTime(const m_time_t rawtime, const char* strftimeformat)
{
    struct tm dt;
    char buffer [40];
    m_localtime(rawtime, &dt);
    strftime(buffer, sizeof( buffer ), strftimeformat, &dt); // Following RFC 2822 (as in date -R)
    return std::string(buffer);
}

std::string getReadableTime(const m_time_t rawtime, int strftimeformatid)
{
    return getReadableTime(rawtime, getFormatStrFromId(strftimeformatid));
}

std::string getReadableShortTime(const m_time_t rawtime, bool showUTCDeviation)
{
    if (rawtime != -1)
    {
        return getReadableTime(rawtime, showUTCDeviation?MCMDTIME_SHORTWITHUTCDEVIATION:MCMDTIME_SHORT);
    }
    else
    {
        char buffer [40];
        sprintf(buffer,"INVALID_TIME %lld", (long long)rawtime);
        return std::string(buffer);
    }
}

std::string getReadablePeriod(const m_time_t rawtime)
{

    long long rest = rawtime;
    long long years = rest/31557600; //365.25 days
    rest = rest%31557600;
    long long months = rest/2629800; // average month 365.25/12 days
    rest = rest%2629800;
    long long days = rest/86400;
    rest = rest%86400;
    long long hours = rest/3600;
    rest = rest%3600;
    long long minutes = rest/60;
    long long seconds = rest%60;

    ostringstream ostoret;
    if (years) ostoret << years << "y";
    if (months) ostoret << months << "m";
    if (days) ostoret << days << "d";
    if (hours) ostoret << hours << "h";
    if (minutes) ostoret << minutes << "M";
    if (seconds) ostoret << seconds << "s";

    string toret = ostoret.str();
    return toret.size()?toret:"0s";
}

m_time_t getTimeStampAfter(m_time_t initial, string timestring)
{
    char *buffer = new char[timestring.size() + 1];
    strcpy(buffer, timestring.c_str());

    int days = 0, hours = 0, minutes = 0, seconds = 0, months = 0, years = 0;

    char * ptr = buffer;
    char * last = buffer;
    while (*ptr != '\0')
    {
        if (( *ptr < '0' ) || ( *ptr > '9' ))
        {
            switch (*ptr)
            {
                case 'd':
                    *ptr = '\0';
                    days = atoi(last);
                    break;

                case 'h':
                    *ptr = '\0';
                    hours = atoi(last);
                    break;

                case 'M':
                    *ptr = '\0';
                    minutes = atoi(last);
                    break;

                case 's':
                    *ptr = '\0';
                    seconds = atoi(last);
                    break;

                case 'm':
                    *ptr = '\0';
                    months = atoi(last);
                    break;

                case 'y':
                    *ptr = '\0';
                    years = atoi(last);
                    break;

                default:
                {
                    delete[] buffer;
                    return -1;
                }
            }
            last = ptr + 1;
        }
        char *prev = ptr;
        ptr++;
        if (*ptr == '\0' && ( *prev >= '0' ) && ( *prev <= '9' )) //reach the end with a number
        {
            delete[] buffer;
            return -1;
        }
    }

    struct tm dt;
    m_localtime(initial, &dt);

    dt.tm_mday += days;
    dt.tm_hour += hours;
    dt.tm_min += minutes;
    dt.tm_sec += seconds;
    dt.tm_mon += months;
    dt.tm_year += years;

    delete [] buffer;
    return m_mktime(&dt);
}

m_time_t getTimeStampAfter(string timestring)
{
    m_time_t initial = m_time();
    return getTimeStampAfter(initial, timestring);
}

m_time_t getTimeStampBefore(m_time_t initial, string timestring)
{
    char *buffer = new char[timestring.size() + 1];
    strcpy(buffer, timestring.c_str());

    int days = 0, hours = 0, minutes = 0, seconds = 0, months = 0, years = 0;

    char * ptr = buffer;
    char * last = buffer;
    while (*ptr != '\0')
    {
        if (( *ptr < '0' ) || ( *ptr > '9' ))
        {
            switch (*ptr)
            {
                case 'd':
                    *ptr = '\0';
                    days = atoi(last);
                    break;

                case 'h':
                    *ptr = '\0';
                    hours = atoi(last);
                    break;

                case 'M':
                    *ptr = '\0';
                    minutes = atoi(last);
                    break;

                case 's':
                    *ptr = '\0';
                    seconds = atoi(last);
                    break;

                case 'm':
                    *ptr = '\0';
                    months = atoi(last);
                    break;

                case 'y':
                    *ptr = '\0';
                    years = atoi(last);
                    break;

                default:
                {
                    delete[] buffer;
                    return -1;
                }
            }
            last = ptr + 1;
        }
        char *prev = ptr;
        ptr++;
        if (*ptr == '\0' && ( *prev >= '0' ) && ( *prev <= '9' )) //reach the end with a number
        {
            delete[] buffer;
            return -1;
        }
    }

    struct tm dt;
    m_localtime(initial, &dt);

    dt.tm_mday -= days;
    dt.tm_hour -= hours;
    dt.tm_min -= minutes;
    dt.tm_sec -= seconds;
    dt.tm_mon -= months;
    dt.tm_year -= years;

    delete [] buffer;
    return m_mktime(&dt);
}

m_time_t getTimeStampBefore(string timestring)
{
    m_time_t initial = m_time();
    return getTimeStampBefore(initial, timestring);
}

bool getMinAndMaxTime(string timestring, m_time_t *minTime, m_time_t *maxTime)
{
    m_time_t initial = m_time();
    return getMinAndMaxTime(initial, timestring, minTime, maxTime);
}

bool getMinAndMaxTime(m_time_t initial, string timestring, m_time_t *minTime, m_time_t *maxTime)
{

    *minTime = -1;
    *maxTime = -1;
    if (!timestring.size())
    {
        return false;
    }

    if (timestring.at(0) == '+')
    {
        size_t posmin = timestring.find("-");
        string maxTimestring = timestring.substr(1,posmin-1);
        *maxTime = getTimeStampBefore(initial,maxTimestring);
        if (*maxTime == -1)
        {
            return false;
        }
        if (posmin == string::npos)
        {
            *minTime = 0;
            return true;
        }

        string minTimestring = timestring.substr(posmin+1);
        *minTime = getTimeStampBefore(initial,minTimestring);
        if (*minTime == -1)
        {
            return false;
        }
    }
    else if (timestring.at(0) == '-')
    {
        size_t posmax = timestring.find("+");
        string minTimestring = timestring.substr(1,posmax-1);
        *minTime = getTimeStampBefore(initial,minTimestring);
        if (*minTime == -1)
        {
            return false;
        }
        if (posmax == string::npos)
        {
            *maxTime = initial;
            return true;
        }

        string maxTimestring = timestring.substr(posmax+1);
        *maxTime = getTimeStampBefore(initial,maxTimestring);
        if (*maxTime == -1)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    return true;
}

bool getMinAndMaxSize(string sizestring, int64_t *minSize, int64_t *maxSize)
{

    *minSize = -1;
    *maxSize = -1;
    if (!sizestring.size())
    {
        return false;
    }

    if (sizestring.at(0) == '+')
    {
        size_t posmax = sizestring.find("-");
        string minSizestring = sizestring.substr(1,posmax-1);
        *minSize = textToSize(minSizestring.c_str());
        if (*minSize == -1)
        {
            return false;
        }
        if (posmax == string::npos)
        {
            *maxSize = -1;
            return true;
        }

        string maxSizestring = sizestring.substr(posmax+1);
        *maxSize = textToSize(maxSizestring.c_str());
        if (*maxSize == -1)
        {
            return false;
        }
    }
    else if (sizestring.at(0) == '-')
    {
        size_t posmin = sizestring.find("+");
        string maxSizestring = sizestring.substr(1,posmin-1);
        *maxSize = textToSize(maxSizestring.c_str());
        if (*maxSize == -1)
        {
            return false;
        }
        if (posmin == string::npos)
        {
            *minSize = -1;
            return true;
        }

        string minSizestring = sizestring.substr(posmin+1);
        *minSize = textToSize(minSizestring.c_str());
        if (*minSize == -1)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    return true;
}


bool isRegExp(string what)
{
#ifdef USE_PCRE
    if (( what == "." ) || ( what == ".." ) || ( what == "/" ))
    {
        return false;
    }

    while (true){
        if (what.find("./") == 0)
        {
            what=what.substr(2);
        }
        else if(what.find("../") == 0)
        {
            what=what.substr(3);
        }
        else if(what.size()>=3 && (what.find("/..") == what.size()-3))
        {
            what=what.substr(0,what.size()-3);
        }
        else if(what.size()>=2 && (what.find("/.") == what.size()-2))
        {
            what=what.substr(0,what.size()-2);
        }
        else if(what.size()>=2 && (what.find("/.") == what.size()-2))
        {
            what=what.substr(0,what.size()-2);
        }
        else
        {
            break;
        }
    }
    replaceAll(what, "/../", "/");
    replaceAll(what, "/./", "/");
    replaceAll(what, "/", "");

    string s = pcrecpp::RE::QuoteMeta(what);
    string ns = s;
    replaceAll(ns, "\\\\\\", "\\");
    bool isregex = strcmp(what.c_str(), ns.c_str());
    return isregex;

//TODO: #elif __cplusplus >= 201103L && !defined(__MINGW32__)
    //TODO??
#else
    return hasWildCards(what);
#endif
}

string unquote(string what)
{
#ifdef USE_PCRE
    if (( what == "." ) || ( what == ".." )  || ( what == "/" ))
    {
        return what;
    }
    string pref="";
    while (true){
        if (what.find("./") == 0)
        {
            what=what.substr(2);
            pref+="./";
        }
        else if(what.find("../") == 0)
        {
            what=what.substr(3);
            pref+="../";
        }
        else
        {
            break;
        }
    }

    string s = pcrecpp::RE::QuoteMeta(what.c_str());
    string ns = s;
    replaceAll(ns, "\\\\\\", "\\");

    replaceAll(ns, "\\/\\.\\.\\/", "/../");
    replaceAll(ns, "\\/\\.\\/", "/./");

    return pref+ns;

#else
    return what;
#endif
}

bool megacmdWildcardMatch(const char *pszString, const char *pszMatch)
//  cf. http://www.planet-source-code.com/vb/scripts/ShowCode.asp?txtCodeId=1680&lngWId=3
{
    const char *cp = nullptr;
    const char *mp = nullptr;

    while ((*pszString) && (*pszMatch != '*'))
    {
        if ((*pszMatch != *pszString) && (*pszMatch != '?'))
        {
            return false;
        }
        pszMatch++;
        pszString++;
    }

    while (*pszString)
    {
        if (*pszMatch == '*')
        {
            if (!*++pszMatch)
            {
                return true;
            }
            mp = pszMatch;
            cp = pszString + 1;
        }
        else if ((*pszMatch == *pszString) || (*pszMatch == '?'))
        {
            pszMatch++;
            pszString++;
        }
        else
        {
            pszMatch = mp;
            pszString = cp++;
        }
    }
    while (*pszMatch == '*')
    {
        pszMatch++;
    }
    return !*pszMatch;
}

bool patternMatches(const char *what, const char *pattern, bool usepcre)
{
    if (usepcre)
    {
#ifdef USE_PCRE
        pcrecpp::RE re(pattern);
        if (re.error().length())
        {
            //In case the user supplied non-pcre regexp with * or ? in it.
            string newpattern(pattern);
            replaceAll(newpattern,"*",".*");
            replaceAll(newpattern,"?",".");
            re=pcrecpp::RE(newpattern);
        }

        if (!re.error().length())
        {
            bool toret = re.FullMatch(what);

            return toret;
        }
        else
        {
            LOG_warn << "Invalid PCRE regex: " << re.error();
            return false;
        }
#elif __cplusplus >= 201103L && !defined(__MINGW32__)
        try
        {
            return std::regex_match(what, std::regex(pattern));
        }
        catch (std::regex_error e)
        {
            LOG_warn << "Couldn't compile regex: " << pattern;
            return false;
        }
#else
        LOG_warn << " PCRE not supported";
        return false;
#endif
    }

    return megacmdWildcardMatch(what,pattern);
}

bool nodeNameIsVersion(string &nodeName)
{
    bool isversion = false;
    if (nodeName.size() > 12 && nodeName.at(nodeName.size()-11) =='#') //TODO: def version separator elswhere
    {
        for (size_t i = nodeName.size()-10; i < nodeName.size() ; i++)
        {
            if (nodeName.at(i) > '9' || nodeName.at(i) < '0')
                break;
            if (i == (nodeName.size() - 1))
            {
                isversion = true;
            }
        }
    }
    return isversion;
}

bool setOptionsAndFlags(map<string, string> *opts, map<string, int> *flags, vector<string> *ws, set<string> vvalidOptions, bool global)
{
    bool discarded = false;

    for (std::vector<string>::iterator it = ws->begin(); it != ws->end(); )
    {
        string w = ( string ) * it;
        if (w.length() && ( w.at(0) == '-' )) //begins with "-"
        {
            if (( w.length() > 1 ) && ( w.at(1) != '-' ))  //single character flags!
            {
                for (unsigned int i = 1; i < w.length(); i++)
                {
                    string optname = w.substr(i, 1);
                    if (vvalidOptions.find(optname) != vvalidOptions.end())
                    {
                        ( *flags )[optname] = ( flags->count(optname) ? ( *flags )[optname] : 0 ) + 1;
                    }
                    else
                    {
                        LOG_err << "Invalid argument: " << optname;
                        discarded = true;
                    }
                }
            }
            else if (w == "--")
            {
                it = ws->erase(it);
                return discarded; // cease to look for options & leave the rest
            }
            else if (w.find_first_of("=") == std::string::npos) //flag
            {
                string optname = ltrim(w, '-');
                if (vvalidOptions.find(optname) != vvalidOptions.end())
                {
                    ( *flags )[optname] = ( flags->count(optname) ? ( *flags )[optname] : 0 ) + 1;
                }
                else
                {
                    LOG_err << "Invalid argument: " << optname;
                    discarded = true;
                }
            }
            else //option=value
            {
                string cleared = ltrim(w, '-');
                size_t p = cleared.find_first_of("=");
                string optname = cleared.substr(0, p);
                if (vvalidOptions.find(optname) != vvalidOptions.end())
                {
                    string value = cleared.substr(p + 1);

                    value = rtrim(ltrim(value, '"'), '"');
                    ( *opts )[optname] = value;
                }
                else
                {
                    LOG_err << "Invalid argument: " << optname;
                    discarded = true;
                }
            }
            it = ws->erase(it);
        }
        else //not an option/flag
        {
            if (global)
            {
                return discarded; //leave the others
            }
            ++it;
        }
    }

    return discarded;
}


#ifndef _WIN32
string readablePermissions(int permvalue)
{
    stringstream os;
    int owner  = (permvalue >> 6) & 0x07;
    int group  = (permvalue >> 3) & 0x07;
    int others = permvalue & 0x07;

    os << owner << group << others;
    return os.str();
}

int permissionsFromReadable(string permissions)
{
    if (permissions.size()==3)
    {
        int owner = permissions.at(0) - '0';
        int group = permissions.at(1) - '0';
        int others = permissions.at(2) - '0';
        if ( (owner < 0) || (owner > 7) || (group < 0) || (group > 7) || (others < 0) || (others > 7) )
        {
            LOG_err << "Invalid permissions value: " << permissions;
            return -1;
        }

        return  (owner << 6) + ( group << 3) + others;
    }
    return -1;
}
#endif

std::string handleToBase64(const MegaHandle &handle)
{
    char *base64Handle = MegaApi::handleToBase64(handle);
    std::string toret{base64Handle};
    delete [] base64Handle;
    return toret;
}


std::string syncBackupIdToBase64(const MegaHandle &handle)
{
    char *base64Handle = MegaApi::userHandleToBase64(handle);
    std::string toret{base64Handle};
    delete [] base64Handle;
    return toret;
}

mega::MegaHandle base64ToSyncBackupId(const std::string &shandle)
{
    return MegaApi::base64ToUserHandle(shandle.c_str());
}
}//end namespace
