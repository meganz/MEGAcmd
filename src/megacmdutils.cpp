/**
 * @file src/megacmdutils.cpp
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

#include "megacmdutils.h"

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

using namespace std;
using namespace mega;

void getNumFolderFiles(MegaNode *n, MegaApi *api, long long *nfiles, long long *nfolders)
{
    MegaNodeList *totalnodes = api->getChildren(n);
    for (int i = 0; i < totalnodes->size(); i++)
    {
        if (totalnodes->get(i)->getType() == MegaNode::TYPE_FILE)
        {
            (*nfiles)++;
        }
        else
        {
            (*nfolders)++;
            getNumFolderFiles(totalnodes->get(i), api, nfiles, nfolders);
        }
    }
    delete totalnodes;
}

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


const char* getAttrStr(int attr)
{
    switch (attr)
    {
        case MegaApi::USER_ATTR_AVATAR:
            return "avatar";


        case MegaApi::USER_ATTR_FIRSTNAME:
            return "firstname";


        case MegaApi::USER_ATTR_LASTNAME:
            return "lastname";


        case MegaApi::USER_ATTR_AUTHRING:
            return "authring";


        case MegaApi::USER_ATTR_LAST_INTERACTION:
            return "lastinteraction";


        case MegaApi::USER_ATTR_ED25519_PUBLIC_KEY:
            return "ed25519";


        case MegaApi::USER_ATTR_CU25519_PUBLIC_KEY:
            return "cu25519";


        case MegaApi::USER_ATTR_KEYRING:
            return "keyring";


        case MegaApi::USER_ATTR_SIG_RSA_PUBLIC_KEY:
            return "rsa";


        case MegaApi::USER_ATTR_SIG_CU255_PUBLIC_KEY:
            return "cu255";
    }
    return "undefined";
}

int getAttrNum(const char* attr)
{
    if (!strcmp(attr, "avatar"))
    {
        return MegaApi:: USER_ATTR_AVATAR;
    }
    if (!strcmp(attr, "firstname"))
    {
        return MegaApi:: USER_ATTR_FIRSTNAME;
    }
    if (!strcmp(attr, "lastname"))
    {
        return MegaApi:: USER_ATTR_LASTNAME;
    }
    if (!strcmp(attr, "authring"))
    {
        return MegaApi:: USER_ATTR_AUTHRING;
    }
    if (!strcmp(attr, "lastinteraction"))
    {
        return MegaApi:: USER_ATTR_LAST_INTERACTION;
    }
    if (!strcmp(attr, "ed25519"))
    {
        return MegaApi:: USER_ATTR_ED25519_PUBLIC_KEY;
    }
    if (!strcmp(attr, "cu25519"))
    {
        return MegaApi:: USER_ATTR_CU25519_PUBLIC_KEY;
    }
    if (!strcmp(attr, "keyring"))
    {
        return MegaApi:: USER_ATTR_KEYRING;
    }
    if (!strcmp(attr, "rsa"))
    {
        return MegaApi:: USER_ATTR_SIG_RSA_PUBLIC_KEY;
    }
    if (!strcmp(attr, "cu255"))
    {
        return MegaApi:: USER_ATTR_SIG_CU255_PUBLIC_KEY;
    }
    return atoi(attr);
}

const char* getSyncPathStateStr(int state)
{
    switch (state)
    {
        case MegaApi::STATE_NONE:
            return "NONE";

            break;

        case MegaApi::STATE_SYNCED:
            return "Synced";

            break;

        case MegaApi::STATE_PENDING:
            return "Pending";

            break;

        case MegaApi::STATE_SYNCING:
            return "Syncing";

            break;

        case MegaApi::STATE_IGNORED:
            return "Ignored";

            break;
    }
    return "undefined";
}

const char* getSyncStateStr(int state)
{
    switch (state)
    {
        case MegaSync::SYNC_FAILED:
            return "Failed";

            break;

        case MegaSync::SYNC_CANCELED:
            return "Canceled";

            break;

        case MegaSync::SYNC_INITIALSCAN:
            return "InitScan";

            break;

        case MegaSync::SYNC_ACTIVE:
            return "Active";

            break;
    }
    return "undefined";
}

string visibilityToString(int visibility)
{
    if (visibility == MegaUser::VISIBILITY_VISIBLE)
    {
        return "visible";
    }
    if (visibility == MegaUser::VISIBILITY_HIDDEN)
    {
        return "hidden";
    }
    if (visibility == MegaUser::VISIBILITY_UNKNOWN)
    {
        return "unkown visibility";
    }
    if (visibility == MegaUser::VISIBILITY_INACTIVE)
    {
        return "inactive";
    }
    if (visibility == MegaUser::VISIBILITY_BLOCKED)
    {
        return "blocked";
    }
    return "undefined visibility";
}

const char * getErrorCodeStr(MegaError *e)
{
    if (e)
    {
        return MegaError::getErrorString(e->getErrorCode());
    }
    return "NullError";
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

int getLogLevelNum(const char* level)
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
    return atoi(level);
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
#ifdef ENABLE_BACKUPS

string backupSatetStr(int backupstate)
{
    if (backupstate == MegaBackup::BACKUP_FAILED)
    {
        return "FAILED";
    }
    if (backupstate == MegaBackup::BACKUP_CANCELED)
    {
        return "CANCELED";
    }
    if (backupstate == MegaBackup::BACKUP_INITIALSCAN)
    {
        return "INITIALSCAN";
    }
    if (backupstate == MegaBackup::BACKUP_ACTIVE)
    {
        return "ACTIVE";
    }
    if (backupstate == MegaBackup::BACKUP_ONGOING)
    {
        return "ONGOING";
    }
    if (backupstate == MegaBackup::BACKUP_SKIPPING)
    {
        return "SKIPPING";
    }
    if (backupstate == MegaBackup::BACKUP_REMOVING_EXCEEDING)
    {
        return "EXCEEDREMOVAL";
    }

    return "UNDEFINED";
}
#endif
/**
 * @brief tests if a path is writable
 * @param path
 * @return
 */
bool canWrite(string path) //TODO: move to fsAccess
{
#ifdef _WIN32
    // TODO: Check permissions
    return true;
#else
    if (access(path.c_str(), W_OK) == 0)
    {
        return true;
    }
    return false;
#endif
}

int getLinkType(string link)
{
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

bool isPublicLink(string link)
{
    if (( link.find("http") == 0 ) && ( link.find("#") != string::npos ))
    {
        return true;
    }
    return false;
}

bool hasWildCards(string &what)
{
    return what.find('*') != string::npos || what.find('?') != string::npos;
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
    return stime.c_str();
#else
    return strptime(stime.c_str(), "%Y%m%d%H%M%S", &dt);
#endif
}

std::string getReadableTime(const m_time_t rawtime)
{
    struct tm dt;
    char buffer [40];
    time_t t = time_t(rawtime);
    MegaApiImpl::fillLocalTimeStruct(&t, &dt);
    strftime(buffer, sizeof( buffer ), "%a, %d %b %Y %T %z", &dt); // Following RFC 2822 (as in date -R)
    return std::string(buffer);
}

std::string getReadableShortTime(const m_time_t rawtime, bool showUTCDeviation)
{
    struct tm dt;
    memset(&dt, 0, sizeof(struct tm));
    char buffer [40];
    if (rawtime != -1)
    {
        time_t t = time_t(rawtime);
        MegaApiImpl::fillLocalTimeStruct(&t, &dt);
        if (showUTCDeviation)
        {
            strftime(buffer, sizeof( buffer ), "%d%b%Y %T %z", &dt);
        }
        else
        {
            strftime(buffer, sizeof( buffer ), "%d%b%Y %T", &dt); // Following RFC 2822 (as in date -R)
        }
    }
    else
    {
        sprintf(buffer,"INVALID_TIME %lld", (long long)rawtime);
    }
    return std::string(buffer);
}

std::string getReadablePeriod(const ::mega::m_time_t rawtime)
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

::mega::m_time_t getTimeStampAfter(::mega::m_time_t initial, string timestring)
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
    time_t initialt = time_t(initial);
    MegaApiImpl::fillLocalTimeStruct(&initialt, &dt);

    dt.tm_mday += days;
    dt.tm_hour += hours;
    dt.tm_min += minutes;
    dt.tm_sec += seconds;
    dt.tm_mon += months;
    dt.tm_year += years;

    delete [] buffer;
    return mktime(&dt);
}

::mega::m_time_t getTimeStampAfter(string timestring)
{
    time_t initial = time(NULL);
    return getTimeStampAfter(initial, timestring);
}

time_t getTimeStampBefore(time_t initial, string timestring)
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
    MegaApiImpl::fillLocalTimeStruct(&initial, &dt);

    dt.tm_mday -= days;
    dt.tm_hour -= hours;
    dt.tm_min -= minutes;
    dt.tm_sec -= seconds;
    dt.tm_mon -= months;
    dt.tm_year -= years;

    delete [] buffer;
    return mktime(&dt);
}

time_t getTimeStampBefore(string timestring)
{
    time_t initial = time(NULL);
    return getTimeStampBefore(initial, timestring);
}

bool getMinAndMaxTime(string timestring, time_t *minTime, time_t *maxTime)
{
    time_t initial = time(NULL);
    return getMinAndMaxTime(initial, timestring, minTime, maxTime);
}

bool getMinAndMaxTime(time_t initial, string timestring, time_t *minTime, time_t *maxTime)
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
std::string &ltrim(std::string &s, const char &c)
{
    size_t pos = s.find_first_not_of(c);
    s = s.substr(pos == string::npos ? s.length() : pos, s.length());
    return s;
}

std::string &rtrim(std::string &s, const char &c)
{
    size_t pos = s.find_last_of(c);
    size_t last = pos == string::npos ? s.length() : pos;
    if (last + 1 < s.length())
    {
        if (s.at(last + 1) != c)
        {
            last = s.length();
        }
    }

    s = s.substr(0, last);
    return s;
}

vector<string> getlistOfWords(char *ptr, bool ignoreTrailingSpaces)
{
    vector<string> words;

    char* wptr;

    // split line into words with quoting and escaping
    for (;; )
    {
        // skip leading blank space
        while (*ptr > 0 && *ptr <= ' ' && (ignoreTrailingSpaces || *(ptr+1)))
        {
            ptr++;
        }

        if (!*ptr)
        {
            break;
        }

        // quoted arg / regular arg
        if (*ptr == '"')
        {
            ptr++;
            wptr = ptr;
            words.push_back(string());

            for (;; )
            {
                if (( *ptr == '"' ) || ( *ptr == '\\' ) || !*ptr)
                {
                    words[words.size() - 1].append(wptr, ptr - wptr);

                    if (!*ptr || ( *ptr++ == '"' ))
                    {
                        break;
                    }

                    wptr = ptr - 1;
                }
                else
                {
                    ptr++;
                }
            }
        }
        else if (*ptr == '\'') // quoted arg / regular arg
        {
            ptr++;
            wptr = ptr;
            words.push_back(string());

            for (;; )
            {
                if (( *ptr == '\'' ) || ( *ptr == '\\' ) || !*ptr)
                {
                    words[words.size() - 1].append(wptr, ptr - wptr);

                    if (!*ptr || ( *ptr++ == '\'' ))
                    {
                        break;
                    }

                    wptr = ptr - 1;
                }
                else
                {
                    ptr++;
                }
            }
        }
        else
        {
            while (*ptr == ' ') ptr++;// only possible if ptr+1 is the end

            wptr = ptr;

            char *prev = ptr;
            //while ((unsigned char)*ptr > ' ')
            while ((*ptr != '\0') && !(*ptr ==' ' && *prev !='\\'))
            {
                if (*ptr == '"')
                {
                    while (*++ptr != '"' && *ptr != '\0')
                    { }
                }
                prev=ptr;
                ptr++;
            }

                words.push_back(string(wptr, ptr - wptr));
        }
    }

    return words;
}

bool stringcontained(const char * s, vector<string> list)
{
    for (int i = 0; i < (int)list.size(); i++)
    {
        if (list[i] == s)
        {
            return true;
        }
    }

    return false;
}

char * dupstr(char* s)
{
    char *r;

    r = (char*)malloc(sizeof( char ) * ( strlen(s) + 1 ));
    strcpy(r, s);
    return( r );
}


bool replace(std::string& str, const std::string& from, const std::string& to)
{
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos)
    {
        return false;
    }
    str.replace(start_pos, from.length(), to);
    return true;
}

void replaceAll(std::string& str, const std::string& from, const std::string& to)
{
    if (from.empty())
    {
        return;
    }
    size_t start_pos = 0;
    while (( start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
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

#elif __cplusplus >= 201103L && !defined(__MINGW32__)
    //TODO??
#endif
    return hasWildCards(what);
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

#endif
    return what;
}

bool megacmdWildcardMatch(const char *pszString, const char *pszMatch)
//  cf. http://www.planet-source-code.com/vb/scripts/ShowCode.asp?txtCodeId=1680&lngWId=3
{
    const char *cp;
    const char *mp;

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
#endif
        LOG_warn << " PCRE not supported";
        return false;
    }

    return megacmdWildcardMatch(what,pattern);
}

int toInteger(string what, int failValue)
{
    if (what.empty())
    {
        return failValue;
    }
    if (!isdigit(what[0]) && !( what[0] != '-' ) && ( what[0] != '+' ))
    {
        return failValue;
    }

    char * p;
    long l = strtol(what.c_str(), &p, 10);

    if (*p != 0)
    {
        return failValue;
    }

    if (( l < INT_MIN ) || ( l > INT_MAX ))
    {
        return failValue;
    }
    return (int)l;
}

string joinStrings(const vector<string>& vec, const char* delim, bool quoted)
{
    stringstream res;
    if (!quoted)
    {
        copy(vec.begin(), vec.end(), ostream_iterator<string>(res, delim));
    }
    else
    {
        for(vector<string>::const_iterator i = vec.begin(); i != vec.end(); ++i)
        {
            res << "\"" << *i << "\"" << delim;
        }
    }
    if (vec.size()>1)
    {
        string toret = res.str();
        return toret.substr(0,toret.size()-strlen(delim));
    }
    return res.str();
}

unsigned int getstringutf8size(const string &str) {
    int c,i,ix,q;
    for (q=0, i=0, ix=int(str.length()); i < ix; i++, q++)
    {
        c = (unsigned char) str[i];

        if (c>=0 && c<=127) i+=0;
        else if ((c & 0xE0) == 0xC0) i+=1;
#ifdef _WIN32
        else if ((c & 0xF0) == 0xE0) i+=2;
#else
        else if ((c & 0xF0) == 0xE0) {i+=2;q++;} //these gliphs may occupy 2 characters! Problem: not always. Let's assume the worse
#endif
        else if ((c & 0xF8) == 0xF0) i+=3;
        else return 0;//invalid utf8
    }
    return q;
}

string getFixLengthString(const string origin, unsigned int size, const char delim, bool alignedright)
{
    string toret;
    size_t printableSize = getstringutf8size(origin);
    size_t bytesSize = origin.size();
    if (printableSize <= size){
        if (alignedright)
        {
            toret.insert(0,size-printableSize,delim);
            toret.insert(size-bytesSize,origin,0,bytesSize);

        }
        else
        {
            toret.insert(0,origin,0,bytesSize);
            toret.insert(bytesSize,size-printableSize,delim);
        }
    }
    else
    {
        toret.insert(0,origin,0,(size+1)/2-2);
        toret.insert((size+1)/2-2,3,'.');
        toret.insert((size+1)/2+1,origin,bytesSize-(size)/2+1,(size)/2-1); //TODO: This could break characters if multibyte!  //alternative: separate in multibyte strings and print one by one?
    }

    return toret;
}

string getRightAlignedString(const string origin, unsigned int minsize)
{
    ostringstream os;
    os << std::setw(minsize) << origin;
    return os.str();
}

int getFlag(map<string, int> *flags, const char * optname)
{
    return flags->count(optname) ? ( *flags )[optname] : 0;
}

string getOption(map<string, string> *cloptions, const char * optname, string defaultValue)
{
    return cloptions->count(optname) ? ( *cloptions )[optname] : defaultValue;
}

int getintOption(map<string, string> *cloptions, const char * optname, int defaultValue)
{
    if (cloptions->count(optname))
    {
        int i = defaultValue;
        istringstream is(( *cloptions )[optname]);
        is >> i;
        return i;
    }
    else
    {
        return defaultValue;
    }
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


string sizeProgressToText(long long partialSize, long long totalSize, bool equalizeUnitsLength, bool humanreadable)
{
    ostringstream os;
    os.precision(2);
    if (humanreadable)
    {
        string unit;
        unit = ( equalizeUnitsLength ? " B" : "B" );
        double reducedPartSize = (double)totalSize;
        double reducedSize = (double)totalSize;

        if ( totalSize > 1099511627776LL *2 )
        {
            reducedPartSize = totalSize / (double) 1099511627776ull;
            reducedSize = totalSize / (double) 1099511627776ull;
            unit = "TB";
        }
        else if ( totalSize > 1073741824LL *2 )
        {
            reducedPartSize = totalSize / (double) 1073741824L;
            reducedSize = totalSize / (double) 1073741824L;
            unit = "GB";
        }
        else if (totalSize > 1048576 * 2)
        {
            reducedPartSize = totalSize / (double) 1048576;
            reducedSize = totalSize / (double) 1048576;
            unit = "MB";
        }
        else if (totalSize > 1024 * 2)
        {
            reducedPartSize = totalSize / (double) 1024;
            reducedSize = totalSize / (double) 1024;
            unit = "KB";
        }
        os << fixed << reducedPartSize << "/" << reducedSize;
        os << " " << unit;
    }
    else
    {
        os << partialSize << "/" << totalSize;
    }

    return os.str();
}

string sizeToText(long long totalSize, bool equalizeUnitsLength, bool humanreadable)
{
    ostringstream os;
    os.precision(2);
    if (humanreadable)
    {
        string unit;
        unit = ( equalizeUnitsLength ? " B" : "B" );
        double reducedSize = (double)totalSize;

        if ( totalSize > 1099511627776LL *2 )
        {
            reducedSize = totalSize / (double) 1099511627776ull;
            unit = "TB";
        }
        else if ( totalSize > 1073741824LL *2 )
        {
            reducedSize = totalSize / (double) 1073741824L;
            unit = "GB";
        }
        else if (totalSize > 1048576 * 2)
        {
            reducedSize = totalSize / (double) 1048576;
            unit = "MB";
        }
        else if (totalSize > 1024 * 2)
        {
            reducedSize = totalSize / (double) 1024;
            unit = "KB";
        }
        os << fixed << reducedSize;
        os << " " << unit;
    }
    else
    {
        os << totalSize;
    }

    return os.str();
}

int64_t textToSize(const char *text)
{
    int64_t sizeinbytes = 0;

    char * ptr = (char *)text;
    char * last = (char *)text;
    while (*ptr != '\0')
    {
        if (( *ptr < '0' ) || ( *ptr > '9' ) || ( *ptr == '.' ) )
        {
            switch (*ptr)
            {
                case 'b': //Bytes
                case 'B':
                    *ptr = '\0';
                    sizeinbytes += int64_t(atof(last));
                    break;

                case 'k': //KiloBytes
                case 'K':
                    *ptr = '\0';
                    sizeinbytes += int64_t(1024.0 * atof(last));
                    break;

                case 'm': //MegaBytes
                case 'M':
                    *ptr = '\0';
                    sizeinbytes += int64_t(1048576.0 * atof(last));
                    break;

                case 'g': //GigaBytes
                case 'G':
                    *ptr = '\0';
                    sizeinbytes += int64_t(1073741824.0 * atof(last));
                    break;

                case 't': //TeraBytes
                case 'T':
                    *ptr = '\0';
                    sizeinbytes += int64_t(1125899906842624.0 * atof(last));
                    break;

                default:
                {
                    return -1;
                }
            }
            last = ptr + 1;
        }
        char *prev = ptr;
        ptr++;
        if (*ptr == '\0' && ( ( *prev == '.' ) || ( ( *prev >= '0' ) && ( *prev <= '9' ) ) ) ) //reach the end with a number or dot
        {
            return -1;
        }
    }
    return sizeinbytes;

}


string secondsToText(::mega::m_time_t seconds, bool humanreadable)
{
    ostringstream os;
    os.precision(2);
    if (humanreadable)
    {
        ::mega::m_time_t reducedSize = ::mega::m_time_t( seconds > 3600 * 2 ? seconds / 3600.0 : ( seconds > 60 * 2 ? seconds / 60.0 : seconds ));
        os << fixed << reducedSize;
        os << ( seconds > 3600 * 2 ? " hours" : ( seconds > 60 * 2 ? " minutes" : " seconds" ));
    }
    else
    {
        os << seconds;
    }

    return os.str();
}

string percentageToText(float percentage)
{
    ostringstream os;
    os.precision(2);
    if (percentage != percentage) //NaN
    {
        os << "----%";
    }
    else
    {
        os << fixed << percentage*100.0 << "%";
    }

    return os.str();
}

unsigned int getNumberOfCols(unsigned int defaultwidth)
{
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    int columns;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    columns = csbi.srWindow.Right - csbi.srWindow.Left - 1;

    return columns;
#else
    struct winsize size;
    if (ioctl(STDOUT_FILENO,TIOCGWINSZ,&size) != -1)
    {
        if (size.ws_col > 2)
        {
            return size.ws_col - 2;
        }
    }
#endif
    return defaultwidth;
}

void sleepSeconds(int seconds)
{
#ifdef _WIN32
    Sleep(1000*seconds);
#else
    sleep(seconds);
#endif
}

void sleepMicroSeconds(long microseconds)
{
#ifdef _WIN32
    Sleep(microseconds);
#else
    usleep(microseconds*1000);
#endif
}

bool isValidEmail(string email)
{
    return !( (email.find("@") == string::npos)
                    || (email.find_last_of(".") == string::npos)
                    || (email.find("@") > email.find_last_of(".")));
}

string &ltrimProperty(string &s, const char &c)
{
    size_t pos = s.find_first_not_of(c);
    s = s.substr(pos == string::npos ? s.length() : pos, s.length());
    return s;
}

string &rtrimProperty(string &s, const char &c)
{
    size_t pos = s.find_last_not_of(c);
    if (pos != string::npos)
    {
        pos++;
    }
    s = s.substr(0, pos);
    return s;
}

string &trimProperty(string &what)
{
    rtrimProperty(what,' ');
    ltrimProperty(what,' ');
    if (what.size() > 1)
    {
        if (what[0] == '\'' || what[0] == '"')
        {
            rtrimProperty(what, what[0]);
            ltrimProperty(what, what[0]);
        }
    }
    return what;
}

string getPropertyFromFile(const char *configFile, const char *propertyName)
{
    ifstream infile(configFile);
    string line;

    while (getline(infile, line))
    {
        if (line.length() > 0 && line[0] != '#')
        {
            if (!strlen(propertyName)) //if empty return first line
            {
                return trimProperty(line);
            }
            string key, value;
            size_t pos = line.find("=");
            if (pos != string::npos && ((pos + 1) < line.size()))
            {
                key = line.substr(0, pos);
                rtrimProperty(key, ' ');

                if (!strcmp(key.c_str(), propertyName))
                {
                    value = line.substr(pos + 1);
                    return trimProperty(value);
                }
            }
        }
    }

    return string();
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
