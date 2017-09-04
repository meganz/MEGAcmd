/**
 * @file examples/megacmd/megacmdutils.cpp
 * @brief MEGAcmd: Auxiliary methods
 *
 * (c) 2013-2016 by Mega Limited, Auckland, New Zealand
 *
 * This file is part of the MEGA SDK - Client Access Engine.
 *
 * Applications using the MEGA API must present a valid application key
 * and comply with the the rules set forth in the Terms of Service.
 *
 * The MEGA SDK is distributed in the hope that it will be useful,
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

using namespace std;
using namespace mega;

int * getNumFolderFiles(MegaNode *n, MegaApi *api)
{
    int * nFolderFiles = new int[2]();
    MegaNodeList *totalnodes = api->getChildren(n);
    for (int i = 0; i < totalnodes->size(); i++)
    {
        if (totalnodes->get(i)->getType() == MegaNode::TYPE_FILE)
        {
            nFolderFiles[1]++;
        }
        else
        {
            nFolderFiles[0]++; //folder
        }
    }

    int nfolders = nFolderFiles[0];
    for (int i = 0; i < nfolders; i++)
    {
        int * nFolderFilesSub = getNumFolderFiles(totalnodes->get(i), api);

        nFolderFiles[0] += nFolderFilesSub[0];
        nFolderFiles[1] += nFolderFilesSub[1];
        delete []nFolderFilesSub;
    }

    delete totalnodes;
    return nFolderFiles;
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

const char* getSyncStateStr(int state)
{
    switch (state)
    {
        case 0:
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
    if (( link.find_first_of("http") == 0 ) && ( link.find_first_of("#") != string::npos ))
    {
        return true;
    }
    return false;
}

bool hasWildCards(string &what)
{
    return what.find('*') != string::npos || what.find('?') != string::npos;
}

std::string getReadableTime(const time_t rawtime)
{
    struct tm * dt;
    char buffer [40];
    dt = localtime(&rawtime);
    strftime(buffer, sizeof( buffer ), "%a, %d %b %Y %T %z", dt); // Following RFC 2822 (as in date -R)
    return std::string(buffer);
}

time_t getTimeStampAfter(time_t initial, string timestring)
{
    char *buffer = new char[timestring.size() + 1];
    strcpy(buffer, timestring.c_str());

    time_t days = 0, hours = 0, minutes = 0, seconds = 0, months = 0, years = 0;

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
        ptr++;
    }

    struct tm * dt;
    dt = localtime(&initial);

    dt->tm_mday += days;
    dt->tm_hour += hours;
    dt->tm_min += minutes;
    dt->tm_sec += seconds;
    dt->tm_mon += months;
    dt->tm_year += years;

    delete [] buffer;
    return mktime(dt);
}

time_t getTimeStampAfter(string timestring)
{
    time_t initial = time(NULL);
    return getTimeStampAfter(initial, timestring);
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

string getFixLengthString(const string origin, unsigned int size, const char delim, bool alignedright)
{
    string toret;
    unsigned int origsize = origin.size();
    if (origsize <= size){
        if (alignedright)
        {
            toret.insert(0,size-origsize,delim);
            toret.insert(size-origsize,origin,0,origsize);

        }
        else
        {
            toret.insert(0,origin,0,origsize);
            toret.insert(origsize,size-origsize,delim);
        }
    }
    else
    {
        toret.insert(0,origin,0,(size+1)/2-2);
        toret.insert((size+1)/2-2,3,'.');
        toret.insert((size+1)/2+1,origin,origsize-(size)/2+1,(size)/2-1);
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
        int i;
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

string sizeToText(long long totalSize, bool equalizeUnitsLength, bool humanreadable)
{
    ostringstream os;
    os.precision(2);
    if (humanreadable)
    {
        double reducedSize = ( totalSize > 1048576 * 2 ? totalSize / 1048576.0 : ( totalSize > 1024 * 2 ? totalSize / 1024.0 : totalSize ));
        os << fixed << reducedSize;
        os << ( totalSize > 1048576 * 2 ? " MB" : ( totalSize > 1024 * 2 ? " KB" : ( equalizeUnitsLength ? "  B" : " B" )));
    }
    else
    {
        os << totalSize;
    }

    return os.str();
}

string secondsToText(time_t seconds, bool humanreadable)
{
    ostringstream os;
    os.precision(2);
    if (humanreadable)
    {
        time_t reducedSize = ( seconds > 3600 * 2 ? seconds / 3600.0 : ( seconds > 60 * 2 ? seconds / 60.0 : seconds ));
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

