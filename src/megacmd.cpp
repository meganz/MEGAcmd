/**
 * @file src/megacmd.cpp
 * @brief MEGAcmd: Interactive CLI and service application
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

#include "megacmd.h"

#include "megacmdsandbox.h"
#include "megacmdexecuter.h"
#include "megacmdutils.h"
#include "configurationmanager.h"
#include "megacmdlogger.h"
#include "comunicationsmanager.h"
#include "listeners.h"

#include "megacmdplatform.h"
#include "megacmdversion.h"

#define USE_VARARGS
#define PREFER_STDARG

#include <iomanip>
#include <string>

#ifndef _WIN32
#include "signal.h"
#else
#include <fcntl.h>
#include <io.h>
#endif


#if !defined (PARAMS)
#  if defined (__STDC__) || defined (__GNUC__) || defined (__cplusplus)
#    define PARAMS(protos) protos
#  else
#    define PARAMS(protos) ()
#  endif
#endif

typedef char *completionfunction_t PARAMS((const char *, int));

#define SSTR( x ) static_cast< std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()


#ifdef _WIN32
// convert UTF-8 to Windows Unicode wstring
void stringtolocalw(const char* path, std::wstring* local)
{
    // make space for the worst case
    local->resize((strlen(path) + 1) * sizeof(wchar_t));

    int wchars_num = MultiByteToWideChar(CP_UTF8, 0, path,-1, NULL,0);
    local->resize(wchars_num);

    int len = MultiByteToWideChar(CP_UTF8, 0, path,-1, (wchar_t*)local->data(), wchars_num);

    if (len)
    {
        local->resize(len-1);
    }
    else
    {
        local->clear();
    }
}

//widechar to utf8 string
void localwtostring(const std::wstring* wide, std::string *multibyte)
{
    if( !wide->empty() )
    {
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wide->data(), (int)wide->size(), NULL, 0, NULL, NULL);
        multibyte->resize(size_needed);
        WideCharToMultiByte(CP_UTF8, 0, wide->data(), (int)wide->size(), (char*)multibyte->data(), size_needed, NULL, NULL);
    }
}

//override << operators for wostream for string and const char *

std::wostream & operator<< ( std::wostream & ostr, std::string const & str )
{
    std::wstring toout;
    stringtolocalw(str.c_str(),&toout);
    ostr << toout;

return ( ostr );
}

std::wostream & operator<< ( std::wostream & ostr, const char * str )
{
    std::wstring toout;
    stringtolocalw(str,&toout);
    ostr << toout;
    return ( ostr );
}

//override for the log. This is required for compiling, otherwise SimpleLog won't compile.
std::ostringstream & operator<< ( std::ostringstream & ostr, std::wstring const &str)
{
    std::string s;
    localwtostring(&str,&s);
    ostr << s;
    return ( ostr );
}


#endif

#ifdef _WIN32
#ifdef USE_PORT_COMMS
#include "comunicationsmanagerportsockets.h"
#define COMUNICATIONMANAGER ComunicationsManagerPortSockets
#else
#include "comunicationsmanagernamedpipes.h"
#define COMUNICATIONMANAGER ComunicationsManagerNamedPipes
#endif
#else
#include "comunicationsmanagerfilesockets.h"
#define COMUNICATIONMANAGER ComunicationsManagerFileSockets
#include <signal.h>
#endif

using namespace mega;

MegaCmdExecuter *cmdexecuter;
MegaCmdSandbox *sandboxCMD;

MegaSemaphore semaphoreClients; //to limit max parallel petitions

MegaApi *api;

//api objects for folderlinks
std::queue<MegaApi *> apiFolders;
std::vector<MegaApi *> occupiedapiFolders;
MegaSemaphore semaphoreapiFolders;
MegaMutex mutexapiFolders;

MegaCMDLogger *loggerCMD;

MegaMutex mutexEndedPetitionThreads;
std::vector<MegaThread *> petitionThreads;
std::vector<MegaThread *> endedPetitionThreads;


//Comunications Manager
ComunicationsManager * cm;

// global listener
MegaCmdGlobalListener* megaCmdGlobalListener;

MegaCmdMegaListener* megaCmdMegaListener;

bool loginInAtStartup = false;

string validGlobalParameters[] = {"v", "help"};

string alocalremotefolderpatterncommands [] = {"sync"};
vector<string> localremotefolderpatterncommands(alocalremotefolderpatterncommands, alocalremotefolderpatterncommands + sizeof alocalremotefolderpatterncommands / sizeof alocalremotefolderpatterncommands[0]);

string aremotepatterncommands[] = {"export", "attr"};
vector<string> remotepatterncommands(aremotepatterncommands, aremotepatterncommands + sizeof aremotepatterncommands / sizeof aremotepatterncommands[0]);

string aremotefolderspatterncommands[] = {"cd", "share"};
vector<string> remotefolderspatterncommands(aremotefolderspatterncommands, aremotefolderspatterncommands + sizeof aremotefolderspatterncommands / sizeof aremotefolderspatterncommands[0]);

string amultipleremotepatterncommands[] = {"ls", "mkdir", "rm", "du", "find", "mv"};
vector<string> multipleremotepatterncommands(amultipleremotepatterncommands, amultipleremotepatterncommands + sizeof amultipleremotepatterncommands / sizeof amultipleremotepatterncommands[0]);

string aremoteremotepatterncommands[] = {"cp"};
vector<string> remoteremotepatterncommands(aremoteremotepatterncommands, aremoteremotepatterncommands + sizeof aremoteremotepatterncommands / sizeof aremoteremotepatterncommands[0]);

string aremotelocalpatterncommands[] = {"get", "thumbnail", "preview"};
vector<string> remotelocalpatterncommands(aremotelocalpatterncommands, aremotelocalpatterncommands + sizeof aremotelocalpatterncommands / sizeof aremotelocalpatterncommands[0]);

string alocalpatterncommands [] = {"lcd"};
vector<string> localpatterncommands(alocalpatterncommands, alocalpatterncommands + sizeof alocalpatterncommands / sizeof alocalpatterncommands[0]);

string aemailpatterncommands [] = {"invite", "signup", "ipc", "users"};
vector<string> emailpatterncommands(aemailpatterncommands, aemailpatterncommands + sizeof aemailpatterncommands / sizeof aemailpatterncommands[0]);

string avalidCommands [] = { "login", "signup", "confirm", "session", "mount", "ls", "cd", "log", "debug", "pwd", "lcd", "lpwd", "import",
                             "put", "get", "attr", "userattr", "mkdir", "rm", "du", "mv", "cp", "sync", "export", "share", "invite", "ipc",
                             "showpcr", "users", "speedlimit", "killsession", "whoami", "help", "passwd", "reload", "logout", "version", "quit",
                             "thumbnail", "preview", "find", "completion", "clear", "https", "transfers", "exclude", "exit"
#ifdef _WIN32
                             ,"unicode"
#endif
                           };
vector<string> validCommands(avalidCommands, avalidCommands + sizeof avalidCommands / sizeof avalidCommands[0]);

// password change-related state information
string oldpasswd;
string newpasswd;

bool doExit = false;
bool consoleFailed = false;

string dynamicprompt;

static prompttype prompt = COMMAND;

// local console
Console* console;

MegaMutex mutexHistory;

map<int, string> threadline;

void printWelcomeMsg();

string getCurrentThreadLine()
{
    uint64_t currentThread = MegaThread::currentThreadId();
    if (threadline.find(currentThread) == threadline.end())
    { // not found thread
        return string();
    }
    else
    {
        return threadline[currentThread];
    }
}

void setCurrentThreadLine(string s)
{
    threadline[MegaThread::currentThreadId()] = s;
}

void setCurrentThreadLine(const vector<string>& vec)
{
   setCurrentThreadLine(joinStrings(vec));
}

void sigint_handler(int signum)
{
    LOG_verbose << "Received signal: " << signum;
    if (loginInAtStartup)
    {
        exit(-2);
    }

    LOG_debug << "Exiting due to SIGINT";

    doExit = true;
}

#ifdef _WIN32
BOOL CtrlHandler( DWORD fdwCtrlType )
{
  LOG_verbose << "Reached CtrlHandler: " << fdwCtrlType;

  switch( fdwCtrlType )
  {
    // Handle the CTRL-C signal.
    case CTRL_C_EVENT:
       sigint_handler((int)fdwCtrlType);
      return( TRUE );

    default:
      return FALSE;
  }
}
#endif

prompttype getprompt()
{
    return prompt;
}

void setprompt(prompttype p, string arg)
{
    prompt = p;

    if (p == COMMAND)
    {
        console->setecho(true);
    }
    else
    {
        if (arg.size())
        {
            OUTSTREAM << arg << flush;
        }
        else
        {
            OUTSTREAM << prompts[p] << flush;
        }

        console->setecho(false);
    }
}

void changeprompt(const char *newprompt)
{
    dynamicprompt = newprompt;
    string s = "prompt:";
    s+=dynamicprompt;
    cm->informStateListeners(s);
}


void informTransferUpdate(MegaTransfer *transfer, int clientID)
{
    informProgressUpdate(transfer->getTransferredBytes(),transfer->getTotalBytes(), clientID);
}

void informProgressUpdate(long long transferred, long long total, int clientID, string title)
{
    string s = "progress:";
    s+=SSTR(transferred);
    s+=":";
    s+=SSTR(total);

    if (title.size())
    {
        s+=":";
        s+=title;
    }

    cm->informStateListenerByClientId(s, clientID);
}

void insertValidParamsPerCommand(set<string> *validParams, string thecommand, set<string> *validOptValues = NULL)
{
    if (!validOptValues)
    {
        validOptValues = validParams;
    }
    if ("ls" == thecommand)
    {
        validParams->insert("R");
        validParams->insert("r");
        validParams->insert("l");
#ifdef USE_PCRE
        validParams->insert("use-pcre");
#endif
    }
    else if ("du" == thecommand)
    {
        validParams->insert("h");
#ifdef USE_PCRE
        validParams->insert("use-pcre");
#endif
    }
    else if ("help" == thecommand)
    {
        validParams->insert("f");
        validParams->insert("non-interactive");
        validParams->insert("upgrade");
#ifdef _WIN32
        validParams->insert("unicode");
#endif
    }
    else if ("version" == thecommand)
    {
        validParams->insert("l");
        validParams->insert("c");
    }
    else if ("rm" == thecommand)
    {
        validParams->insert("r");
        validParams->insert("f");
#ifdef USE_PCRE
        validParams->insert("use-pcre");
#endif
    }
    else if ("mv" == thecommand)
    {
#ifdef USE_PCRE
        validParams->insert("use-pcre");
#endif
    }
    else if ("speedlimit" == thecommand)
    {
        validParams->insert("u");
        validParams->insert("d");
        validParams->insert("h");
    }
    else if ("whoami" == thecommand)
    {
        validParams->insert("l");
    }
    else if ("log" == thecommand)
    {
        validParams->insert("c");
        validParams->insert("s");
    }
    else if ("exclude" == thecommand)
    {
        validParams->insert("a");
        validParams->insert("d");
        validParams->insert("restart-syncs");
    }
    else if ("sync" == thecommand)
    {
        validParams->insert("d");
        validParams->insert("s");
        validParams->insert("r");
        validOptValues->insert("path-display-size");
    }
    else if ("export" == thecommand)
    {
        validParams->insert("a");
        validParams->insert("d");
        validOptValues->insert("expire");
#ifdef USE_PCRE
        validParams->insert("use-pcre");
#endif
    }
    else if ("share" == thecommand)
    {
        validParams->insert("a");
        validParams->insert("d");
        validParams->insert("p");
        validOptValues->insert("with");
        validOptValues->insert("level");
        validOptValues->insert("personal-representation");
#ifdef USE_PCRE
        validParams->insert("use-pcre");
#endif
    }
    else if ("find" == thecommand)
    {
        validOptValues->insert("pattern");
        validOptValues->insert("l");
#ifdef USE_PCRE
        validParams->insert("use-pcre");
#endif
        validOptValues->insert("mtime");
        validOptValues->insert("size");
    }
    else if ("mkdir" == thecommand)
    {
        validParams->insert("p");
    }
    else if ("users" == thecommand)
    {
        validParams->insert("s");
        validParams->insert("h");
        validParams->insert("d");
        validParams->insert("n");
    }
    else if ("killsession" == thecommand)
    {
        validParams->insert("a");
    }
    else if ("invite" == thecommand)
    {
        validParams->insert("d");
        validParams->insert("r");
        validOptValues->insert("message");
    }
    else if ("signup" == thecommand)
    {
        validParams->insert("name");
    }
    else if ("logout" == thecommand)
    {
        validParams->insert("keep-session");
    }
    else if ("attr" == thecommand)
    {
        validParams->insert("d");
        validParams->insert("s");
    }
    else if ("userattr" == thecommand)
    {
        validOptValues->insert("user");
        validParams->insert("s");
    }
    else if ("ipc" == thecommand)
    {
        validParams->insert("a");
        validParams->insert("d");
        validParams->insert("i");
    }
    else if ("showpcr" == thecommand)
    {
        validParams->insert("in");
        validParams->insert("out");
    }
    else if ("thumbnail" == thecommand)
    {
        validParams->insert("s");
    }
    else if ("preview" == thecommand)
    {
        validParams->insert("s");
    }
    else if ("put" == thecommand)
    {
        validParams->insert("c");
        validParams->insert("q");
        validParams->insert("ignore-quota-warn");
        validOptValues->insert("clientID");
    }
    else if ("get" == thecommand)
    {
        validParams->insert("m");
        validParams->insert("q");
        validParams->insert("ignore-quota-warn");
#ifdef USE_PCRE
        validParams->insert("use-pcre");
#endif
        validOptValues->insert("clientID");
    }
    else if ("login" == thecommand)
    {
        validOptValues->insert("clientID");
    }
    else if ("reload" == thecommand)
    {
        validOptValues->insert("clientID");
    }
    else if ("transfers" == thecommand)
    {
        validParams->insert("show-completed");
        validParams->insert("only-uploads");
        validParams->insert("only-completed");
        validParams->insert("only-downloads");
        validParams->insert("show-syncs");
        validParams->insert("c");
        validParams->insert("a");
        validParams->insert("p");
        validParams->insert("r");
        validOptValues->insert("limit");
        validOptValues->insert("path-display-size");
    }
    else if ("exit" == thecommand || "quit" == thecommand)
    {
        validParams->insert("only-shell");
    }

}

void escapeEspace(string &orig)
{
    replaceAll(orig," ", "\\ ");
}

void unescapeEspace(string &orig)
{
    replaceAll(orig,"\\ ", " ");
}

char* empty_completion(const char* text, int state)
{
    // we offer 2 different options so that it doesn't complete (no space is inserted)
    if (state == 0)
    {
        return strdup(" ");
    }
    if (state == 1)
    {
        return strdup(text);
    }
    return NULL;
}

char* generic_completion(const char* text, int state, vector<string> validOptions)
{
    static size_t list_index, len;
    static bool foundone;
    string name;
    if (!validOptions.size()) // no matches
    {
        return empty_completion(text,state); //dont fall back to filenames
    }
    if (!state)
    {
        list_index = 0;
        foundone = false;
        len = strlen(text);
    }
    while (list_index < validOptions.size())
    {
        name = validOptions.at(list_index);
        //Notice: do not escape options for cmdshell. Plus, we won't filter here, because we don't know if the value of rl_completion_quote_chararcter of cmdshell
        // The filtering and escaping will be performed by the completion function in cmdshell
        if (interactiveThread() && !getCurrentThreadIsCmdShell()) {
            escapeEspace(name);
        }

        list_index++;

        if (!( strcmp(text, ""))
                || (( name.size() >= len ) && ( strlen(text) >= len ) &&  ( name.find(text) == 0 ) )
                || getCurrentThreadIsCmdShell()  //do not filter if cmdshell (it will be filter there)
                )
        {
            foundone = true;
            return dupstr((char*)name.c_str());
        }
    }

    if (!foundone)
    {
        return empty_completion(text,state); //dont fall back to filenames
    }

    return((char*)NULL );
}

char* commands_completion(const char* text, int state)
{
    return generic_completion(text, state, validCommands);
}

char* local_completion(const char* text, int state)
{
    return((char*)NULL );
}

void addGlobalFlags(set<string> *setvalidparams)
{
    for (size_t i = 0; i < sizeof( validGlobalParameters ) / sizeof( *validGlobalParameters ); i++)
    {
        setvalidparams->insert(validGlobalParameters[i]);
    }
}

char * flags_completion(const char*text, int state)
{
    static vector<string> validparams;
    if (state == 0)
    {
        validparams.clear();
        char *saved_line = strdup(getCurrentThreadLine().c_str());
        vector<string> words = getlistOfWords(saved_line);
        free(saved_line);
        if (words.size())
        {
            set<string> setvalidparams;
            set<string> setvalidOptValues;
            addGlobalFlags(&setvalidparams);

            string thecommand = words[0];
            insertValidParamsPerCommand(&setvalidparams, thecommand, &setvalidOptValues);
            set<string>::iterator it;
            for (it = setvalidparams.begin(); it != setvalidparams.end(); it++)
            {
                string param = *it;
                string toinsert;

                if (param.size() > 1)
                {
                    toinsert = "--" + param;
                }
                else
                {
                    toinsert = "-" + param;
                }

                validparams.push_back(toinsert);
            }

            for (it = setvalidOptValues.begin(); it != setvalidOptValues.end(); it++)
            {
                string param = *it;
                string toinsert;

                if (param.size() > 1)
                {
                    toinsert = "--" + param + '=';
                }
                else
                {
                    toinsert = "-" + param + '=';
                }

                validparams.push_back(toinsert);
            }
        }
    }
    char *toret = generic_completion(text, state, validparams);
    return toret;
}

char * flags_value_completion(const char*text, int state)
{
    static vector<string> validValues;

    if (state == 0)
    {
        validValues.clear();

        char *saved_line = strdup(getCurrentThreadLine().c_str());
        vector<string> words = getlistOfWords(saved_line);
        free(saved_line);
        saved_line = NULL;
        if (words.size() > 1)
        {
            string thecommand = words[0];
            string currentFlag = words[words.size() - 1];

            map<string, string> cloptions;
            map<string, int> clflags;

            set<string> validParams;

            insertValidParamsPerCommand(&validParams, thecommand);

            if (setOptionsAndFlags(&cloptions, &clflags, &words, validParams, true))
            {
                // return invalid??
            }

            if (thecommand == "share")
            {
                if (currentFlag.find("--level=") == 0)
                {
                    string prefix = strncmp(text, "--level=", strlen("--level="))?"":"--level=";
                    validValues.push_back(prefix+getShareLevelStr(MegaShare::ACCESS_UNKNOWN));
                    validValues.push_back(prefix+getShareLevelStr(MegaShare::ACCESS_READ));
                    validValues.push_back(prefix+getShareLevelStr(MegaShare::ACCESS_READWRITE));
                    validValues.push_back(prefix+getShareLevelStr(MegaShare::ACCESS_FULL));
                    validValues.push_back(prefix+getShareLevelStr(MegaShare::ACCESS_OWNER));
                    validValues.push_back(prefix+getShareLevelStr(MegaShare::ACCESS_UNKNOWN));
                }
                if (currentFlag.find("--with=") == 0)
                {
                    validValues = cmdexecuter->getlistusers();
                    string prefix = strncmp(text, "--with=", strlen("--with="))?"":"--with=";
                    for (unsigned int i=0;i<validValues.size();i++)
                    {
                        validValues.at(i)=prefix+validValues.at(i);
                    }
                }
            }
            if (( thecommand == "userattr" ) && ( currentFlag.find("--user=") == 0 ))
            {
                validValues = cmdexecuter->getlistusers();
                string prefix = strncmp(text, "--user=", strlen("--user="))?"":"--user=";
                for (unsigned int i=0;i<validValues.size();i++)
                {
                    validValues.at(i)=prefix+validValues.at(i);
                }
            }
        }
    }

    char *toret = generic_completion(text, state, validValues);
    return toret;
}

void unescapeifRequired(string &what)
{
    if (interactiveThread() ) {
        return unescapeEspace(what);
    }
}

char* remotepaths_completion(const char* text, int state)
{
    static vector<string> validpaths;
    if (state == 0)
    {
        string wildtext(text);
        bool usepcre = false; //pcre makes no sense in paths completion
        if (usepcre)
        {
#ifdef USE_PCRE
        wildtext += ".";
#elif __cplusplus >= 201103L
        wildtext += ".";
#endif
        }

        wildtext += "*";

        unescapeEspace(wildtext);

        validpaths = cmdexecuter->listpaths(usepcre, wildtext);
    }
    return generic_completion(text, state, validpaths);
}

char* remotefolders_completion(const char* text, int state)
{
    static vector<string> validpaths;
    if (state == 0)
    {
        string wildtext(text);
        bool usepcre = false; //pcre makes no sense in paths completion
        if (usepcre)
        {
#ifdef USE_PCRE
        wildtext += ".";
#elif __cplusplus >= 201103L
        wildtext += ".";
#endif
        }
        wildtext += "*";

        validpaths = cmdexecuter->listpaths(usepcre, wildtext, true);
    }
    return generic_completion(text, state, validpaths);
}

char* loglevels_completion(const char* text, int state)
{
    static vector<string> validloglevels;
    if (state == 0)
    {
        validloglevels.push_back(getLogLevelStr(MegaApi::LOG_LEVEL_FATAL));
        validloglevels.push_back(getLogLevelStr(MegaApi::LOG_LEVEL_ERROR));
        validloglevels.push_back(getLogLevelStr(MegaApi::LOG_LEVEL_WARNING));
        validloglevels.push_back(getLogLevelStr(MegaApi::LOG_LEVEL_INFO));
        validloglevels.push_back(getLogLevelStr(MegaApi::LOG_LEVEL_DEBUG));
        validloglevels.push_back(getLogLevelStr(MegaApi::LOG_LEVEL_MAX));
    }
    return generic_completion(text, state, validloglevels);
}

char* transfertags_completion(const char* text, int state)
{
    static vector<string> validtransfertags;
    if (state == 0)
    {
        MegaTransferData * transferdata = api->getTransferData();
        if (transferdata)
        {
            for (int i = 0; i < transferdata->getNumUploads(); i++)
            {
                validtransfertags.push_back(SSTR(transferdata->getUploadTag(i)));
            }
            for (int i = 0; i < transferdata->getNumDownloads(); i++)
            {
                validtransfertags.push_back(SSTR(transferdata->getDownloadTag(i)));
            }

            // TODO: reconsider including completed transfers (sth like this:)
//            globalTransferListener->completedTransfersMutex.lock();
//            for (unsigned int i = 0;i < globalTransferListener->completedTransfers.size() && shownCompleted < limit; i++)
//            {
//                MegaTransfer *transfer = globalTransferListener->completedTransfers.at(shownCompleted);
//                if (!transfer->isSyncTransfer())
//                {
//                    validtransfertags.push_back(SSTR(transfer->getTag()));
//                    shownCompleted++;
//                }
//            }
//            globalTransferListener->completedTransfersMutex.unlock();
        }
    }
    return generic_completion(text, state, validtransfertags);
}
char* contacts_completion(const char* text, int state)
{
    static vector<string> validcontacts;
    if (state == 0)
    {
        validcontacts = cmdexecuter->getlistusers();
    }
    return generic_completion(text, state, validcontacts);
}

char* sessions_completion(const char* text, int state)
{
    static vector<string> validSessions;
    if (state == 0)
    {
        validSessions = cmdexecuter->getsessions();
    }

    if (validSessions.size() == 0)
    {
        return empty_completion(text, state);
    }

    return generic_completion(text, state, validSessions);
}

char* nodeattrs_completion(const char* text, int state)
{
    static vector<string> validAttrs;
    if (state == 0)
    {
        validAttrs.clear();
        char *saved_line = strdup(getCurrentThreadLine().c_str());
        vector<string> words = getlistOfWords(saved_line);
        free(saved_line);
        saved_line = NULL;
        if (words.size() > 1)
        {
            validAttrs = cmdexecuter->getNodeAttrs(words[1]);
        }
    }

    if (validAttrs.size() == 0)
    {
        return empty_completion(text, state);
    }

    return generic_completion(text, state, validAttrs);
}

char* userattrs_completion(const char* text, int state)
{
    static vector<string> validAttrs;
    if (state == 0)
    {
        validAttrs.clear();
        validAttrs = cmdexecuter->getUserAttrs();
    }

    if (validAttrs.size() == 0)
    {
        return empty_completion(text, state);
    }

    return generic_completion(text, state, validAttrs);
}

void discardOptionsAndFlags(vector<string> *ws)
{
    for (std::vector<string>::iterator it = ws->begin(); it != ws->end(); )
    {
        /* std::cout << *it; ... */
        string w = ( string ) * it;
        if (w.length() && ( w.at(0) == '-' )) //begins with "-"
        {
            it = ws->erase(it);
        }
        else //not an option/flag
        {
            ++it;
        }
    }
}

completionfunction_t *getCompletionFunction(vector<string> words)
{
    // Strip words without flags
    string thecommand = words[0];

    if (words.size() > 1)
    {
        string lastword = words[words.size() - 1];
        if (lastword.find_first_of("-") == 0)
        {
            if (lastword.find_last_of("=") != string::npos)
            {
                return flags_value_completion;
            }
            else
            {
                return flags_completion;
            }
        }
    }
    discardOptionsAndFlags(&words);

    int currentparameter = words.size() - 1;
    if (stringcontained(thecommand.c_str(), localremotefolderpatterncommands))
    {
        if (currentparameter == 1)
        {
            return local_completion;
        }
        if (currentparameter == 2)
        {
            return remotefolders_completion;
        }
    }
    else if (thecommand == "put")
    {
        if (currentparameter == 1)
        {
            return local_completion;
        }
        else
        {
            return remotepaths_completion;
        }
    }
    else if (stringcontained(thecommand.c_str(), remotepatterncommands))
    {
        if (currentparameter == 1)
        {
            return remotepaths_completion;
        }
    }
    else if (stringcontained(thecommand.c_str(), remotefolderspatterncommands))
    {
        if (currentparameter == 1)
        {
            return remotefolders_completion;
        }
    }
    else if (stringcontained(thecommand.c_str(), multipleremotepatterncommands))
    {
        if (currentparameter >= 1)
        {
            return remotepaths_completion;
        }
    }
    else if (stringcontained(thecommand.c_str(), localpatterncommands))
    {
        if (currentparameter == 1)
        {
            return local_completion;
        }
    }
    else if (stringcontained(thecommand.c_str(), remoteremotepatterncommands))
    {
        if (( currentparameter == 1 ) || ( currentparameter == 2 ))
        {
            return remotepaths_completion;
        }
    }
    else if (stringcontained(thecommand.c_str(), remotelocalpatterncommands))
    {
        if (currentparameter == 1)
        {
            return remotepaths_completion;
        }
        if (currentparameter == 2)
        {
            return local_completion;
        }
    }
    else if (stringcontained(thecommand.c_str(), emailpatterncommands))
    {
        if (currentparameter == 1)
        {
            return contacts_completion;
        }
    }
    else if (thecommand == "import")
    {
        if (currentparameter == 2)
        {
            return remotepaths_completion;
        }
    }
    else if (thecommand == "killsession")
    {
        if (currentparameter == 1)
        {
            return sessions_completion;
        }
    }
    else if (thecommand == "attr")
    {
        if (currentparameter == 1)
        {
            return remotepaths_completion;
        }
        if (currentparameter == 2)
        {
            return nodeattrs_completion;
        }
    }
    else if (thecommand == "userattr")
    {
        if (currentparameter == 1)
        {
            return userattrs_completion;
        }
    }
    else if (thecommand == "log")
    {
        if (currentparameter == 1)
        {
            return loglevels_completion;
        }
    }
    else if (thecommand == "transfers")
    {
        if (currentparameter == 1)
        {
            return transfertags_completion;
        }
    }
    return empty_completion;
}

string getListOfCompletionValues(vector<string> words, char separator = ' ', bool suppressflag = true)
{
    string completionValues;
    completionfunction_t * compfunction = getCompletionFunction(words);
    if (compfunction == local_completion)
    {
        if (!interactiveThread())
        {
            return "MEGACMD_USE_LOCAL_COMPLETION";
        }
        else
        {
            string toret="MEGACMD_USE_LOCAL_COMPLETION";
            toret+=cmdexecuter->getLPWD();
            return toret;
        }
    }
    int state=0;
    if (words.size()>1)
    while (true)
    {
        char *newval;
        string &lastword = words[words.size()-1];
        if (suppressflag && lastword.size()>3 && lastword[0]== '-' && lastword[1]== '-' && lastword.find('=')!=string::npos)
        {
            newval = compfunction(lastword.substr(lastword.find_first_of('=')+1).c_str(), state);
        }
        else
        {
            newval = compfunction(lastword.c_str(), state);
        }

        if (!newval) break;
        if (completionValues.size())
        {
            completionValues+=separator;
        }

        if (strchr(newval,separator))
        {
            completionValues+="\"";
            completionValues+=newval;
            completionValues+="\"";
        }
        else
        {
            completionValues+=newval;
        }
        free(newval);

        state++;
    }
    return completionValues;
}

MegaApi* getFreeApiFolder()
{
    semaphoreapiFolders.wait();
    mutexapiFolders.lock();
    MegaApi* toret = apiFolders.front();
    apiFolders.pop();
    occupiedapiFolders.push_back(toret);
    mutexapiFolders.unlock();
    return toret;
}

void freeApiFolder(MegaApi *apiFolder)
{
    mutexapiFolders.lock();
    occupiedapiFolders.erase(std::remove(occupiedapiFolders.begin(), occupiedapiFolders.end(), apiFolder), occupiedapiFolders.end());
    apiFolders.push(apiFolder);
    semaphoreapiFolders.release();
    mutexapiFolders.unlock();
}

const char * getUsageStr(const char *command)
{
    if (!strcmp(command, "login"))
    {
        if (interactiveThread())
        {
            return "login [email [password]] | exportedfolderurl#key | session";
        }
        else
        {
            return "login email password | exportedfolderurl#key | session";
        }
    }
    if (!strcmp(command, "begin"))
    {
        return "begin [ephemeralhandle#ephemeralpw]";
    }
    if (!strcmp(command, "signup"))
    {
        return "signup email [password] [--name=\"Your Name\"]";
    }
    if (!strcmp(command, "confirm"))
    {
        return "confirm link email [password]";
    }
    if (!strcmp(command, "session"))
    {
        return "session";
    }
    if (!strcmp(command, "mount"))
    {
        return "mount";
    }
    if (!strcmp(command, "unicode"))
    {
        return "unicode";
    }
    if (!strcmp(command, "ls"))
    {
#ifdef USE_PCRE
        return "ls [-lRr] [remotepath] [--use-pcre]";
#else
        return "ls [-lRr] [remotepath]";
#endif
    }
    if (!strcmp(command, "cd"))
    {
        return "cd [remotepath]";
    }
    if (!strcmp(command, "log"))
    {
        return "log [-sc] level";
    }
    if (!strcmp(command, "du"))
    {
#ifdef USE_PCRE
        return "du [-h] [remotepath remotepath2 remotepath3 ... ] [--use-pcre]";
#else
        return "du [-h] [remotepath remotepath2 remotepath3 ... ]";
#endif
    }
    if (!strcmp(command, "pwd"))
    {
        return "pwd";
    }
    if (!strcmp(command, "lcd"))
    {
        return "lcd [localpath]";
    }
    if (!strcmp(command, "lpwd"))
    {
        return "lpwd";
    }
    if (!strcmp(command, "import"))
    {
        return "import exportedfilelink#key [remotepath]";
    }
    if (!strcmp(command, "put"))
    {
        return "put  [-c] [-q] [--ignore-quota-warn] localfile [localfile2 localfile3 ...] [dstremotepath]";
    }
    if (!strcmp(command, "putq"))
    {
        return "putq [cancelslot]";
    }
    if (!strcmp(command, "get"))
    {
#ifdef USE_PCRE
        return "get [-m] [-q] [--ignore-quota-warn] [--use-pcre] exportedlink#key|remotepath [localpath]";
#else
        return "get [-m] [-q] [--ignore-quota-warn] exportedlink#key|remotepath [localpath]";
#endif
    }
    if (!strcmp(command, "getq"))
    {
        return "getq [cancelslot]";
    }
    if (!strcmp(command, "pause"))
    {
        return "pause [get|put] [hard] [status]";
    }
    if (!strcmp(command, "attr"))
    {
        return "attr remotepath [-s attribute value|-d attribute]";
    }
    if (!strcmp(command, "userattr"))
    {
        return "userattr [-s attribute value|attribute] [--user=user@email]";
    }
    if (!strcmp(command, "mkdir"))
    {
        return "mkdir [-p] remotepath";
    }
    if (!strcmp(command, "rm"))
    {
#ifdef USE_PCRE
        return "rm [-r] [-f] [--use-pcre] remotepath";
#else
        return "rm [-r] [-f] remotepath";
#endif
    }
    if (!strcmp(command, "mv"))
    {
#ifdef USE_PCRE
        return "mv srcremotepath [--use-pcre] [srcremotepath2 srcremotepath3 ..] dstremotepath";
#else
        return "mv srcremotepath [srcremotepath2 srcremotepath3 ..] dstremotepath";
#endif
    }
    if (!strcmp(command, "cp"))
    {
        return "cp srcremotepath dstremotepath|dstemail:";
    }
    if (!strcmp(command, "exclude"))
    {
        return "exclude [(-a|-d) pattern1 pattern2 pattern3 [--restart-syncs]]";
    }
    if (!strcmp(command, "sync"))
    {
        return "sync [localpath dstremotepath| [-dsr] [ID|localpath]";
    }
    if (!strcmp(command, "https"))
    {
        return "https [on|off]";
    }
    if (!strcmp(command, "export"))
    {
#ifdef USE_PCRE
        return "export [-d|-a [--expire=TIMEDELAY]] [remotepath] [--use-pcre]";
#else
        return "export [-d|-a [--expire=TIMEDELAY]] [remotepath]";
#endif
    }
    if (!strcmp(command, "share"))
    {
#ifdef USE_PCRE
        return "share [-p] [-d|-a --with=user@email.com [--level=LEVEL]] [remotepath] [--use-pcre]";
#else
        return "share [-p] [-d|-a --with=user@email.com [--level=LEVEL]] [remotepath]";
#endif
    }
    if (!strcmp(command, "invite"))
    {
        return "invite [-d|-r] dstemail [--message=\"MESSAGE\"]";
    }
    if (!strcmp(command, "ipc"))
    {
        return "ipc email|handle -a|-d|-i";
    }
    if (!strcmp(command, "showpcr"))
    {
        return "showpcr [--in | --out]";
    }
    if (!strcmp(command, "users"))
    {
        return "users [-s] [-h] [-n] [-d contact@email]";
    }
    if (!strcmp(command, "getua"))
    {
        return "getua attrname [email]";
    }
    if (!strcmp(command, "putua"))
    {
        return "putua attrname [del|set string|load file]";
    }
    if (!strcmp(command, "speedlimit"))
    {
        return "speedlimit [-u|-d] [-h] [NEWLIMIT]";
    }
    if (!strcmp(command, "killsession"))
    {
        return "killsession [-a|sessionid]";
    }
    if (!strcmp(command, "whoami"))
    {
        return "whoami [-l]";
    }
    if (!strcmp(command, "passwd"))
    {
        return "passwd [oldpassword newpassword]";
    }
    if (!strcmp(command, "retry"))
    {
        return "retry";
    }
    if (!strcmp(command, "recon"))
    {
        return "recon";
    }
    if (!strcmp(command, "reload"))
    {
        return "reload";
    }
    if (!strcmp(command, "logout"))
    {
        return "logout [--keep-session]";
    }
    if (!strcmp(command, "symlink"))
    {
        return "symlink";
    }
    if (!strcmp(command, "version"))
    {
        return "version [-l][-c]";
    }
    if (!strcmp(command, "debug"))
    {
        return "debug";
    }
    if (!strcmp(command, "chatf"))
    {
        return "chatf ";
    }
    if (!strcmp(command, "chatc"))
    {
        return "chatc group [email ro|rw|full|op]*";
    }
    if (!strcmp(command, "chati"))
    {
        return "chati chatid email ro|rw|full|op";
    }
    if (!strcmp(command, "chatr"))
    {
        return "chatr chatid [email]";
    }
    if (!strcmp(command, "chatu"))
    {
        return "chatu chatid";
    }
    if (!strcmp(command, "chatga"))
    {
        return "chatga chatid nodehandle uid";
    }
    if (!strcmp(command, "chatra"))
    {
        return "chatra chatid nodehandle uid";
    }
    if (!strcmp(command, "exit"))
    {
        return "exit [--only-shell]";
    }
    if (!strcmp(command, "quit"))
    {
        return "quit [--only-shell]";
    }
    if (!strcmp(command, "history"))
    {
        return "history";
    }
    if (!strcmp(command, "thumbnail"))
    {
        return "thumbnail [-s] remotepath localpath";
    }
    if (!strcmp(command, "preview"))
    {
        return "preview [-s] remotepath localpath";
    }
    if (!strcmp(command, "find"))
    {
#ifdef USE_PCRE
        return "find [remotepath] [-l] [--pattern=PATTERN] [--mtime=TIMECONSTRAIN] [--size=SIZECONSTRAIN] [--use-pcre]";
#else
        return "find [remotepath] [-l] [--pattern=PATTERN] [--mtime=TIMECONSTRAIN] [--size=SIZECONSTRAIN]";
#endif
    }
    if (!strcmp(command, "help"))
    {
        return "help [-f]";
    }
    if (!strcmp(command, "clear"))
    {
        return "clear";
    }
    if (!strcmp(command, "transfers"))
    {
        return "transfers [-c TAG|-a] | [-r TAG|-a]  | [-p TAG|-a] [--only-downloads | --only-uploads] [SHOWOPTIONS]";
    }
    return "command not found: ";
}

bool validCommand(string thecommand)
{
    return stringcontained((char*)thecommand.c_str(), validCommands);
}

string getsupportedregexps()
{
#ifdef USE_PCRE
        return "Perl Compatible Regular Expressions with \"--use-pcre\"\n   or wildcarded expresions with ? or * like f*00?.txt";
#elif __cplusplus >= 201103L
        return "c++11 Regular Expressions";
#else
        return "it accepts wildcards: ? and *. e.g.: f*00?.txt";
#endif
}

string getHelpStr(const char *command)
{
    ostringstream os;

    os << "Usage: " << getUsageStr(command) << endl;
    if (!strcmp(command, "login"))
    {
        os << "Logs into a mega" << endl;
        os << " You can log in either with email and password, with session ID," << endl;
        os << " or into a folder (an exported/public folder)" << endl;
        os << " If loging into a folder indicate url#key" << endl;
    }
    else if (!strcmp(command, "signup"))
    {
        os << "Register as user with a given email" << endl;
        os << endl;
        os << "Options:" << endl;
        os << " --name=\"Your Name\"" << "\t" << "Name to register. e.g. \"John Smith\"" << endl;
        os << endl;
        os << " You will receive an email to confirm your account. " << endl;
        os << " Once you have received the email, please proceed to confirm the link " << endl;
        os << " included in that email with \"confirm\"." << endl;
    }
    else if (!strcmp(command, "clear"))
    {
        os << "Clear screen" << endl;
    }
    else if (!strcmp(command, "help"))
    {
        os << "Prints list of commands" << endl;
        os << endl;
        os << "Options:" << endl;
        os << " -f" << "\t" << "Include a brief description of the commands" << endl;
    }
    else if (!strcmp(command, "history"))
    {
        os << "Prints history of used commands" << endl;
        os << "  Only commands used in interactive mode are registered" << endl;
    }
    else if (!strcmp(command, "confirm"))
    {
        os << "Confirm an account using the link provided after the \"signup\" process." << endl;
        os << " It requires the email and the password used to obtain the link." << endl;
        os << endl;
    }
    else if (!strcmp(command, "session"))
    {
        os << "Prints (secret) session ID" << endl;
    }
    else if (!strcmp(command, "mount"))
    {
        os << "Lists all the main nodes" << endl;
    }
    else if (!strcmp(command, "unicode"))
    {
        os << "Toogle unicode input enabled/disabled in interactive shell" << endl;
        os << endl;
        os << " Unicode mode is experimental, you might experience" << endl;
        os << " some issues interacting with the console" << endl;
        os << " (e.g. history navigation fails)." << endl;
        os << "Type \"help --unicode\" for further info" << endl;

    }
    else if (!strcmp(command, "ls"))
    {
        os << "Lists files in a remote path" << endl;
        os << " remotepath can be a pattern (" << getsupportedregexps() << ") " << endl;
        os << " Also, constructions like /PATTERN1/PATTERN2/PATTERN3 are allowed" << endl;
        os << endl;
        os << "Options:" << endl;
        os << " -R|-r" << "\t" << "list folders recursively" << endl;
        os << " -l" << "\t" << "include extra information" << endl;
#ifdef USE_PCRE
        os << " --use-pcre" << "\t" << "use PCRE expressions" << endl;
#endif
    }
    else if (!strcmp(command, "cd"))
    {
        os << "Changes the current remote folder" << endl;
        os << endl;
        os << "If no folder is provided, it will be changed to the root folder" << endl;
    }
    else if (!strcmp(command, "log"))
    {
        os << "Prints/Modifies the current logs level" << endl;
        os << endl;
        os << "Options:" << endl;
        os << " -c" << "\t" << "CMD log level (higher level messages). " << endl;
        os << "   " << "\t" << " Messages captured by MEGAcmd server." << endl;
        os << " -s" << "\t" << "SDK log level (lower level messages)." << endl;
        os << "   " << "\t" << " Messages captured by the engine and libs" << endl;

        os << endl;
        os << "Regardless of the log level of the" << endl;
        os << " interactive shell, you can increase the amount of information given" <<  endl;
        os << "   by any command by passing \"-v\" (\"-vv\", \"-vvv\", ...)" << endl;


    }
    else if (!strcmp(command, "du"))
    {
        os << "Prints size used by files/folders" << endl;
        os << " remotepath can be a pattern (" << getsupportedregexps() << ") " << endl;
        os << endl;
        os << "Options:" << endl;
        os << " -h" << "\t" << "Human readable" << endl;
#ifdef USE_PCRE
        os << " --use-pcre" << "\t" << "use PCRE expressions" << endl;
#endif
    }
    else if (!strcmp(command, "pwd"))
    {
        os << "Prints the current remote folder" << endl;
    }
    else if (!strcmp(command, "lcd"))
    {
        os << "Changes the current local folder for the interactive console" << endl;
        os << endl;
        os << "It will be used for uploads and downloads" << endl;
        os << endl;
        os << "If not using interactive console, the current local folder will be " << endl;
        os << " that of the shell executing mega comands" << endl;
    }
    else if (!strcmp(command, "lpwd"))
    {
        os << "Prints the current local folder for the interactive console" << endl;
        os << endl;
        os << "It will be used for uploads and downloads" << endl;
        os << endl;
        os << "If not using interactive console, the current local folder will be " << endl;
        os << " that of the shell executing mega comands" << endl;
    }
    else if (!strcmp(command, "logout"))
    {
        os << "Logs out" << endl;
        os << endl;
        os << "Options:" << endl;
        os << " --keep-session" << "\t" << "Keeps the current session." << endl;
    }
    else if (!strcmp(command, "import"))
    {
        os << "Imports the contents of a remote link into user's cloud" << endl;
        os << endl;
        os << "If no remote path is provided, the current local folder will be used" << endl;
    }
    else if (!strcmp(command, "put"))
    {
        os << "Uploads files/folders to a remote folder" << endl;
        os << endl;
        os << "Options:" << endl;
        os << " -c" << "\t" << "Creates remote folder destination in case of not existing." << endl;
        os << " -q" << "\t" << "queue upload: execute in the background. Don't wait for it to end' " << endl;
        os << " --ignore-quota-warn" << "\t" << "ignore quota surpassing warning. " << endl;
        os << "                    " << "\t" << "  The upload will be attempted anyway." << endl;

        os << endl;
        os << "Notice that the dstremotepath can only be omitted when only one local path is provided. " << endl;
        os << " In such case, the current remote working dir will be the destination for the upload." << endl;
        os << " Mind that using wildcards for local paths will result in multiple paths." << endl;
    }
    else if (!strcmp(command, "get"))
    {
        os << "Downloads a remote file/folder or a public link " << endl;
        os << endl;
        os << "In case it is a file, the file will be downloaded at the specified folder " << endl;
        os << "                             (or at the current folder if none specified)." << endl;
        os << "  If the localpath(destiny) already exists and is the same (same contents)" << endl;
        os << "  nothing will be done. If differs, it will create a new file appending \" (NUM)\" " << endl;
        os << "  if the localpath(destiny) is a folder with a file with the same name on it, " << endl;
        os << "         it will preserve the, it will create a new file appending \" (NUM)\" " << endl;
        os << endl;
        os << "For folders, the entire contents (and the root folder itself) will be" << endl;
        os << "                    by default downloaded into the destination folder" << endl;
        os << "Options:" << endl;
        os << " -q" << "\t" << "queue download: execute in the background. Don't wait for it to end' " << endl;
        os << " -m" << "\t" << "if the folder already exists, the contents will be merged with the " << endl;
        os << "                     downloaded one (preserving the existing files)" << endl;
        os << " --ignore-quota-warn" << "\t" << "ignore quota surpassing warning. " << endl;
        os << "                    " << "\t" << "  The download will be attempted anyway." << endl;
#ifdef USE_PCRE
        os << " --use-pcre" << "\t" << "use PCRE expressions" << endl;
#endif

    }
    if (!strcmp(command, "attr"))
    {
        os << "Lists/updates node attributes" << endl;
        os << endl;
        os << "Options:" << endl;
        os << " -s" << "\tattribute value \t" << "sets an attribute to a value" << endl;
        os << " -d" << "\tattribute       \t" << "removes the attribute" << endl;
    }
    if (!strcmp(command, "userattr"))
    {
        os << "Lists/updates user attributes" << endl;
        os << endl;
        os << "Options:" << endl;
        os << " -s" << "\tattribute value \t" << "sets an attribute to a value" << endl;
        os << " --user=user@email" << "\t" << "select the user to query" << endl;
    }
    else if (!strcmp(command, "mkdir"))
    {
        os << "Creates a directory or a directories hierarchy" << endl;
        os << endl;
        os << "Options:" << endl;
        os << " -p" << "\t" << "Allow recursive" << endl;
    }
    else if (!strcmp(command, "rm"))
    {
        os << "Deletes a remote file/folder" << endl;
        os << endl;
        os << "Options:" << endl;
        os << " -r" << "\t" << "Delete recursively (for folders)" << endl;
        os << " -f" << "\t" << "Force (no asking)" << endl;
#ifdef USE_PCRE
        os << " --use-pcre" << "\t" << "use PCRE expressions" << endl;
#endif
    }
    else if (!strcmp(command, "mv"))
    {
        os << "Moves file(s)/folder(s) into a new location (all remotes)" << endl;
        os << endl;
        os << "If the location exists and is a folder, the source will be moved there" << endl;
        os << "If the location doesn't exits, the source will be renamed to the defined destiny" << endl;
#ifdef USE_PCRE
        os << "Options:" << endl;
        os << " --use-pcre" << "\t" << "use PCRE expressions" << endl;
#endif
    }
    else if (!strcmp(command, "cp"))
    {
        os << "Moves a file/folder into a new location (all remotes)" << endl;
        os << endl;
        os << "If the location exists and is a folder, the source will be copied there" << endl;
        os << "If the location doesn't exits, the file/folder will be renamed to the defined destiny" << endl;
        os << endl;
        os << "If \"dstemail:\" provided, the file/folder will be sent to that user's inbox (//in)" << endl;
        os << " Remember the trailing \":\", otherwise a file with the name of that user will be created" << endl;
    }
    else if (!strcmp(command, "https"))
    {
        os << "Shows if HTTPS is used for transfers. Use \"https on\" to enable it." << endl;
        os << endl;
        os << "HTTPS is not necesary since all data is stored and transfered encrypted." << endl;
        os << "Enabling it will increase CPU usage and add network overhead." << endl;
        os << endl;
        os << "Notice that this setting is ephemeral: it will reset for the next time you open MEGAcmd" << endl;
    }
    else if (!strcmp(command, "exclude"))
    {
        os << "Manages exclusions in syncs." << endl;
        os << endl;
        os << "Options:" << endl;
        os << " -a pattern1 pattern2 ..." << "\t" << "adds pattern(s) to the exclusion list" << endl;
        os << "                         " << "\t" << "          (* and ? wildcards allowed)" << endl;
        os << " -d pattern1 pattern2 ..." << "\t" << "deletes pattern(s) from the exclusion list" << endl;
        os << " --restart-syncs" << "\t" << "Try to restart synchronizations." << endl;

        os << endl;
        os << "Changes will not be applied inmediately to actions being performed in active syncs. " << endl;
        os << "After adding/deleting patterns, you might want to: " << endl;
        os << " a) disable/reenable synchronizations manually" << endl;
        os << " b) restart MEGAcmd server" << endl;
        os << " c) use --restart-syncs flag. Caveats:" << endl;
        os << "  This will cause active transfers to be restarted" << endl;
        os << "  In certain cases --restart-syncs might be unable to re-enable a synchronization. " << endl;
        os << "  In such case, you will need to manually resume it or restart MEGAcmd server." << endl;
    }
    else if (!strcmp(command, "sync"))
    {
        os << "Controls synchronizations" << endl;
        os << endl;
        os << "If no argument is provided, it lists current configured synchronizations" << endl;
        os << endl;
        os << "If provided local and remote paths, it will start synchronizing " << endl;
        os << " a local folder into a remote folder" << endl;
        os << endl;
        os << "If an ID/local path is provided, it will list such synchronization " << endl;
        os << " unless an option is specified." << endl;
        os << endl;
        os << "Options:" << endl;
        os << "-d" << " " << "ID|localpath" << "\t" << "deletes a synchronization" << endl;
        os << "-s" << " " << "ID|localpath" << "\t" << "stops(pauses) a synchronization" << endl;
        os << "-r" << " " << "ID|localpath" << "\t" << "resumes a synchronization" << endl;
        os << " --path-display-size=N" << "\t" << "Use a fixed size of N characters for paths" << endl;
    }
    else if (!strcmp(command, "export"))
    {
        os << "Prints/Modifies the status of current exports" << endl;
        os << endl;
        os << "Options:" << endl;
#ifdef USE_PCRE
        os << " --use-pcre" << "\t" << "use PCRE expressions" << endl;
#endif
        os << " -a" << "\t" << "Adds an export (or modifies it if existing)" << endl;
        os << " --expire=TIMEDELAY" << "\t" << "Determines the expiration time of a node." << endl;
        os << "                   " << "\t" << "   It indicates the delay in hours(h), days(d), " << endl;
        os << "                   " << "\t"  << "   minutes(M), seconds(s), months(m) or years(y)" << endl;
        os << "                   " << "\t" << "   e.g. \"1m12d3h\" stablish an expiration time 1 month, " << endl;
        os << "                   " << "\t"  << "   12 days and 3 hours after the current moment" << endl;
        os << " -d" << "\t" << "Deletes an export" << endl;
        os << endl;
        os << "If a remote path is given it'll be used to add/delete or in case of no option selected," << endl;
        os << " it will display all the exports existing in the tree of that path" << endl;
    }
    else if (!strcmp(command, "share"))
    {
        os << "Prints/Modifies the status of current shares" << endl;
        os << endl;
        os << "Options:" << endl;
#ifdef USE_PCRE
        os << " --use-pcre" << "\t" << "use PCRE expressions" << endl;
#endif
        os << " -p" << "\t" << "Show pending shares" << endl;
        os << " --with=email" << "\t" << "Determines the email of the user to [no longer] share with" << endl;
        os << " -d" << "\t" << "Stop sharing with the selected user" << endl;
        os << " -a" << "\t" << "Adds a share (or modifies it if existing)" << endl;
        os << " --level=LEVEL" << "\t" << "Level of acces given to the user" << endl;
        os << "              " << "\t" << "0: " << "Read access" << endl;
        os << "              " << "\t" << "1: " << "Read and write" << endl;
        os << "              " << "\t" << "2: " << "Full access" << endl;
        os << "              " << "\t" << "3: " << "Owner access" << endl;
        os << endl;
        os << "If a remote path is given it'll be used to add/delete or in case " << endl;
        os << " of no option selected, it will display all the shares existing " << endl;
        os << " in the tree of that path" << endl;
        os << endl;
        os << "When sharing a folder with a user that is not a contact (see \"users\" help)" << endl;
        os << "  the share will be in a pending state. You can list pending shares with" << endl;
        os << " \"share -p\". He would need to accept your invitation (see \"ipc\")" << endl;
        os << endl;
        os << "If someone has shared somethin with you, it will be listed as a root folder" << endl;
        os << " Use \"mount\" to list folders shared with you" << endl;
    }
    else if (!strcmp(command, "invite"))
    {
        os << "Invites a contact / deletes an invitation" << endl;
        os << endl;
        os << "Options:" << endl;
        os << " -d" << "\t" << "Deletes invitation" << endl;
        os << " -r" << "\t" << "Sends the invitation again" << endl;
        os << " --message=\"MESSAGE\"" << "\t" << "Sends inviting message" << endl;
        os << endl;
        os << "Use \"showpcr\" to browse invitations" << endl;
        os << "Use \"ipc\" to manage invitations received" << endl;
        os << "Use \"users\" to see contacts" << endl;
    }
    if (!strcmp(command, "ipc"))
    {
        os << "Manages contact incomming invitations." << endl;
        os << endl;
        os << "Options:" << endl;
        os << " -a" << "\t" << "Accepts invitation" << endl;
        os << " -d" << "\t" << "Rejects invitation" << endl;
        os << " -i" << "\t" << "Ignores invitation [WARNING: do not use unless you know what you are doing]" << endl;
        os << endl;
        os << "Use \"invite\" to send/remove invitations to other users" << endl;
        os << "Use \"showpcr\" to browse incoming/outgoing invitations" << endl;
        os << "Use \"users\" to see contacts" << endl;
    }
    if (!strcmp(command, "showpcr"))
    {
        os << "Shows incoming and outgoing contact requests." << endl;
        os << endl;
        os << "Options:" << endl;
        os << " --in" << "\t" << "Shows incoming requests" << endl;
        os << " --out" << "\t" << "Shows outgoing invitations" << endl;
        os << endl;
        os << "Use \"ipc\" to manage invitations received" << endl;
        os << "Use \"users\" to see contacts" << endl;
    }
    else if (!strcmp(command, "users"))
    {
        os << "List contacts" << endl;
        os << endl;
        os << "Options:" << endl;
        os << " -s" << "\t" << "Show shared folders with listed contacts" << endl;
        os << " -h" << "\t" << "Show all contacts (hidden, blocked, ...)" << endl;
        os << " -n" << "\t" << "Show users names" << endl;
        os << " -d" << "\tcontact@email " << "Deletes the specified contact" << endl;
        os << endl;
        os << "Use \"invite\" to send/remove invitations to other users" << endl;
        os << "Use \"showpcr\" to browse incoming/outgoing invitations" << endl;
        os << "Use \"ipc\" to manage invitations received" << endl;
        os << "Use \"users\" to see contacts" << endl;
    }
    else if (!strcmp(command, "speedlimit"))
    {
        os << "Displays/modifies upload/download rate limits" << endl;
        os << " NEWLIMIT stablish the new limit in B/s (0 = no limit)" << endl;
        os << endl;
        os << "Options:" << endl;
        os << " -d" << "\t" << "Download speed limit" << endl;
        os << " -u" << "\t" << "Upload speed limit" << endl;
        os << " -h" << "\t" << "Human readable" << endl;
        os << endl;
        os << "Notice that this limit won't be saved for the next time you execute MEGAcmd server" << endl;

    }
    else if (!strcmp(command, "killsession"))
    {
        os << "Kills a session of current user." << endl;
        os << endl;
        os << "Options:" << endl;
        os << " -a" << "\t" << "kills all sessions except the current one" << endl;
        os << endl;
        os << "To see all sessions use \"whoami -l\"" << endl;
    }
    else if (!strcmp(command, "whoami"))
    {
        os << "Print info of the user" << endl;
        os << endl;
        os << "Options:" << endl;
        os << " -l" << "\t" << "Show extended info: total storage used, storage per main folder " << endl;
        os << "   " << "\t" << "(see mount), pro level, account balance, and also the active sessions" << endl;
    }
    if (!strcmp(command, "passwd"))
    {
        os << "Modifies user password" << endl;
    }
    else if (!strcmp(command, "reload"))
    {
        os << "Forces a reload of the remote files of the user" << endl;
    }
    else if (!strcmp(command, "version"))
    {
        os << "Prints MEGAcmd versioning and extra info" << endl;
        os << endl;
        os << "Options:" << endl;
        os << " -c" << "\t" << "Shows changelog for the current version" << endl;
        os << " -l" << "\t" << "Show extended info: MEGA SDK version and features enabled" << endl;
    }
    else if (!strcmp(command, "thumbnail"))
    {
        os << "To download/upload the thumbnail of a file." << endl;
        os << " If no -s is inidicated, it will download the thumbnail." << endl;
        os << endl;
        os << "Options:" << endl;
        os << " -s" << "\t" << "Sets the thumbnail to the specified file" << endl;
    }
    else if (!strcmp(command, "preview"))
    {
        os << "To download/upload the preview of a file." << endl;
        os << " If no -s is inidicated, it will download the preview." << endl;
        os << endl;
        os << "Options:" << endl;
        os << " -s" << "\t" << "Sets the preview to the specified file" << endl;
    }
    else if (!strcmp(command, "find"))
    {
        os << "Find nodes matching a pattern" << endl;
        os << endl;
        os << "Options:" << endl;
        os << " --pattern=PATTERN" << "\t" << "Pattern to match";
        os << " (" << getsupportedregexps() << ") " << endl;
        os << " --mtime=TIMECONSTRAIN" << "\t" << "Determines time constrains, in the form: [+-]TIMEVALUE" << endl;
        os << "                      " << "\t" << "  TIMEVALUE may include hours(h), days(d), minutes(M)," << endl;
        os << "                      " << "\t" << "   seconds(s), months(m) or years(y)" << endl;
        os << "                      " << "\t" << "  Examples:" << endl;
        os << "                      " << "\t" << "   \"+1m12d3h\" shows files modified before 1 month, " << endl;
        os << "                      " << "\t" << "    12 days and 3 hours the current moment" << endl;
        os << "                      " << "\t" << "   \"-3h\" shows files modified within the last 3 hours" << endl;
        os << "                      " << "\t" << "   \"-3d+1h\" shows files modified in the last 3 days prior to the last hour" << endl;
        os << " --size=SIZECONSTRAIN" << "\t" << "Determines size constrains, in the form: [+-]TIMEVALUE" << endl;
        os << "                      " << "\t" << "  TIMEVALUE may include (B)ytes, (K)ilobytes, (M)egabytes, (G)igabytes & (T)erabytes" << endl;
        os << "                      " << "\t" << "  Examples:" << endl;
        os << "                      " << "\t" << "   \"+1m12k3B\" shows files bigger than 1 Mega, 12 Kbytes and 3Bytes" << endl;
        os << "                      " << "\t" << "   \"-3M\" shows files smaller than 3 Megabytes" << endl;
        os << "                      " << "\t" << "   \"-4M+100K\" shows files smaller than 4 Mbytes and bigger than 100 Kbytes" << endl;
#ifdef USE_PCRE
        os << " --use-pcre" << "\t" << "use PCRE expressions" << endl;
#endif
        os << " -l" << "\t" << "Prints file info" << endl;

    }
    else if(!strcmp(command,"debug") )
    {
        os << "Enters debugging mode (HIGHLY VERBOSE)" << endl;
        os << endl;
        os << "For a finer control of log level see \"log --help\"" << endl;
    }
    else if (!strcmp(command, "quit") || !strcmp(command, "exit"))
    {
        os << "Quits MEGAcmd" << endl;
        os << endl;
        os << "Notice that the session will still be active, and local caches available" << endl;
        os << "The session will be resumed when the service is restarted" << endl;
        if (getCurrentThreadIsCmdShell())
        {
            os << endl;
            os << "Be aware that this will exit both the interactive shell and the server." << endl;
            os << "To only exit current shell and keep server running, use \"exit --only-shell\"" << endl;
        }
    }
    else if (!strcmp(command, "transfers"))
    {
        os << "List or operate with transfers" << endl;
        os << "If executed without option it will list the first 10 tranfers" << endl;
        os << "Options:" << endl;
        os << " -c (TAG|-a)" << "\t" << "Cancel transfer with TAG (or all with -a)" << endl;
        os << " -p (TAG|-a)" << "\t" << "Pause transfer with TAG (or all with -a)" << endl;
        os << " -r (TAG|-a)" << "\t" << "Resume transfer with TAG (or all with -a)" << endl;
        os << " -only-uploads" << "\t" << "Show/Operate only upload transfers" << endl;
        os << " -only-downloads" << "\t" << "Show/Operate only download transfers" << endl;
        os << endl;
        os << "Show options:" << endl;
        os << " -show-syncs" << "\t" << "Show synchronization transfers" << endl;
        os << " -show-completed" << "\t" << "Show completed transfers" << endl;
        os << " -only-completed" << "\t" << "Show only completed download" << endl;
        os << " --limit=N" << "\t" << "Show only first N transfers" << endl;
        os << " --path-display-size=N" << "\t" << "Use a fixed size of N characters for paths" << endl;
    }
    return os.str();
}

#define SSTR( x ) static_cast< std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()

void printAvailableCommands(int extensive = 0)
{
    vector<string> validCommandsOrdered = validCommands;
    sort(validCommandsOrdered.begin(), validCommandsOrdered.end());
    if (!extensive)
    {
        size_t i = 0;
        size_t j = (validCommandsOrdered.size()/3)+((validCommandsOrdered.size()%3>0)?1:0);
        size_t k = 2*(validCommandsOrdered.size()/3)+validCommandsOrdered.size()%3;
        for (i = 0; i < validCommandsOrdered.size() && j < validCommandsOrdered.size()  && k < validCommandsOrdered.size(); i++, j++, k++)
        {
            OUTSTREAM << "      " << setw(20) << left << validCommandsOrdered.at(i) <<  setw(20) << validCommandsOrdered.at(j)  <<  "      " << validCommandsOrdered.at(k) << endl;
        }
        if (validCommandsOrdered.size()%3)
        {
            OUTSTREAM << "      " << setw(20) <<  validCommandsOrdered.at(i);
            if (validCommandsOrdered.size()%3 > 1 )
            {
                OUTSTREAM << setw(20) <<  validCommandsOrdered.at(j);
            }
            OUTSTREAM << endl;
        }
    }
    else
    {
        for (size_t i = 0; i < validCommandsOrdered.size(); i++)
        {
            if (validCommandsOrdered.at(i)!="completion")
            {
                if (extensive > 1)
                {
                    unsigned int width = getNumberOfCols();

                    OUTSTREAM <<  "<" << validCommandsOrdered.at(i) << ">" << endl;
                    OUTSTREAM <<  getHelpStr(validCommandsOrdered.at(i).c_str());
                    for (unsigned int j = 0; j< width; j++) OUTSTREAM << "-";
                    OUTSTREAM << endl;
                }
                else
                {
                    OUTSTREAM << "      " << getUsageStr(validCommandsOrdered.at(i).c_str());
                    string helpstr = getHelpStr(validCommandsOrdered.at(i).c_str());
                    helpstr=string(helpstr,helpstr.find_first_of("\n")+1);
                    OUTSTREAM << ": " << string(helpstr,0,helpstr.find_first_of("\n"));

                    OUTSTREAM << endl;
                }
            }
        }
    }
}

void executecommand(char* ptr)
{
    vector<string> words = getlistOfWords(ptr);
    if (!words.size())
    {
        return;
    }

    string thecommand = words[0];

    if (( thecommand == "?" ) || ( thecommand == "h" ))
    {
        printAvailableCommands();
        return;
    }

    if (words[0] == "completion")
    {
        if (words.size() < 3) words.push_back("");
        vector<string> wordstocomplete(words.begin()+1,words.end());
        setCurrentThreadLine(wordstocomplete);
        OUTSTREAM << getListOfCompletionValues(wordstocomplete);
        return;
    }
    if (words[0] == "retrycons")
    {
        api->retryPendingConnections();
        return;
    }
    if (words[0] == "loggedin")
    {
        if (!api->isFilesystemAvailable())
        {
            setCurrentOutCode(MCMD_NOTLOGGEDIN);
        }
        return;
    }
    if (words[0] == "completionshell")
    {
        if (words.size() == 2)
        {
            vector<string> validCommandsOrdered = validCommands;
            sort(validCommandsOrdered.begin(), validCommandsOrdered.end());
            for (size_t i = 0; i < validCommandsOrdered.size(); i++)
            {
                if (validCommandsOrdered.at(i)!="completion")
                {
                    OUTSTREAM << validCommandsOrdered.at(i);
                    if (i != validCommandsOrdered.size() -1)
                    {
                        OUTSTREAM << (char)0x1F;
                    }
                }
            }
        }
        else
        {
            if (words.size() < 3) words.push_back("");
            vector<string> wordstocomplete(words.begin()+1,words.end());
            setCurrentThreadLine(wordstocomplete);
            OUTSTREAM << getListOfCompletionValues(wordstocomplete,(char)0x1F, false);
        }

        return;
    }

    words = getlistOfWords(ptr,true); //Get words again ignoring trailing spaces (only reasonable for completion)

    map<string, string> cloptions;
    map<string, int> clflags;

    set<string> validParams;
    addGlobalFlags(&validParams);

    if (setOptionsAndFlags(&cloptions, &clflags, &words, validParams, true))
    {
        setCurrentOutCode(MCMD_EARGS);
        LOG_err << "      " << getUsageStr(thecommand.c_str());
        return;
    }

    insertValidParamsPerCommand(&validParams, thecommand);

    if (!validCommand(thecommand))   //unknown command
    {
        setCurrentOutCode(MCMD_EARGS);
        LOG_err << "Command not found: " << thecommand;
        return;
    }

    if (setOptionsAndFlags(&cloptions, &clflags, &words, validParams))
    {
        setCurrentOutCode(MCMD_EARGS);
        LOG_err << "      " << getUsageStr(thecommand.c_str());
        return;
    }
    setCurrentThreadLogLevel(MegaApi::LOG_LEVEL_ERROR + (getFlag(&clflags, "v")?(1+getFlag(&clflags, "v")):0));

    if (getFlag(&clflags, "help"))
    {
        string h = getHelpStr(thecommand.c_str());
        OUTSTREAM << h << endl;
        return;
    }


    if ( thecommand == "help" )
    {
        if (getFlag(&clflags,"upgrade"))
        {

             const char *userAgent = api->getUserAgent();
             char* url = new char[strlen(userAgent)+10];

             sprintf(url, "pro/uao=%s",userAgent);

             string theurl;

             if (api->isLoggedIn())
             {

                 MegaCmdListener *megaCmdListener = new MegaCmdListener(api, NULL);
                 api->getSessionTransferURL(url, megaCmdListener);
                 megaCmdListener->wait();
                 if (megaCmdListener->getError() && megaCmdListener->getError()->getErrorCode() == MegaError::API_OK)
                 {
                     theurl = megaCmdListener->getRequest()->getLink();
                 }
                 else
                 {
                     setCurrentOutCode(MCMD_EUNEXPECTED);
                     LOG_warn << "Unable to get session transfer url: " << megaCmdListener->getError()->getErrorString();
                 }
                 delete megaCmdListener;
             }

             if (!theurl.size())
             {
                 theurl = url;
             }

             OUTSTREAM << "MEGA offers different PRO plans to increase your allowed transfer quota and user storage." << endl;
             OUTSTREAM << "Open the following link in your browser to obtain a PRO account: " << endl;
             OUTSTREAM << "  " << theurl << endl;

             delete [] url;
        }
        else if (getFlag(&clflags,"non-interactive"))
        {
            OUTSTREAM << "MEGAcmd features two modes of interaction:" << endl;
            OUTSTREAM << " - interactive: entering commands in this shell. Enter \"help\" to list available commands" << endl;
            OUTSTREAM << " - non-interactive: MEGAcmd is also listening to outside petitions" << endl;
            OUTSTREAM << "For the non-interactive mode, there are client commands you can use. " << endl;
#ifdef _WIN32

            OUTSTREAM << "Along with the interactive shell, there should be several mega-*.bat scripts" << endl;
            OUTSTREAM << "installed with MEGAcmd. You can use them writting their absolute paths, " << endl;
            OUTSTREAM << "or including their location into your environment PATH and execute simply with mega-*" << endl;
            OUTSTREAM << "If you use PowerShell, you can add the the location of the scripts to the PATH with:" << endl;
            OUTSTREAM << "  $env:PATH += \";$env:LOCALAPPDATA\\MEGAcmd\"" << endl;
            OUTSTREAM << "Client commands completion requires bash, hence, it is not available for Windows. " << endl;
            OUTSTREAM << "You can add \" -o outputfile\" to save the output into a file instead of to standard output." << endl;
            OUTSTREAM << endl;

#elif __MACH__
            OUTSTREAM << "After installing the dmg, along with the interactive shell, client commands" << endl;
            OUTSTREAM << "should be located at /Applications/MEGAcmd.app/Contents/MacOS" << endl;
            OUTSTREAM << "If you wish to use the client commands from MacOS Terminal, open the Terminal and " << endl;
            OUTSTREAM << "include the installation folder in the PATH. Typically:" << endl;
            OUTSTREAM << endl;
            OUTSTREAM << " export PATH=/Applications/MEGAcmd.app/Contents/MacOS:$PATH" << endl;
            OUTSTREAM << endl;
            OUTSTREAM << "And for bash completion, source megacmd_completion.sh:" << endl;
            OUTSTREAM << " source /Applications/MEGAcmd.app/Contents/MacOS/megacmd_completion.sh" << endl;
#else
            OUTSTREAM << "If you have installed MEGAcmd using one of the available packages" << endl;
            OUTSTREAM << "both the interactive shell (mega-cmd) and the different client commands (mega-*) " << endl;
            OUTSTREAM << "will be in your PATH (you might need to open your shell again). " << endl;
            OUTSTREAM << "If you are using bash, you should also have autocompletion for client commands working. " << endl;

#endif
        }
#ifdef _WIN32
        else if (getFlag(&clflags,"unicode"))
        {
            OUTSTREAM << "A great effort has been done so as to have MEGAcmd support non-ASCII characters." << endl;
            OUTSTREAM << "However, it might still be consider in an experimantal state. You might experiment some issues." << endl;
            OUTSTREAM << "If that is the case, do not hesistate to contact us so as to improve our support." << endl;
            OUTSTREAM << endl;
            OUTSTREAM << "Known issues: " << endl;
            OUTSTREAM << endl;
            OUTSTREAM << "In Windows, when executing a client command in non-interactive mode or the interactive shell " << endl;
            OUTSTREAM << "Some symbols might not be printed. This is something expected, since your terminal (PowerShell/Command Prompt)" << endl;
            OUTSTREAM << "is not able to draw those symbols. However you can use the non-interactive mode to have the output " << endl;
            OUTSTREAM << "written into a file and open it with a graphic editor that supports them. The file will be UTF-8 encoded." << endl;
            OUTSTREAM << "To do that, use \"-o outputfile\" with your mega-*.bat commands. (See \"help --non-interactive\")." << endl;
            OUTSTREAM << "Please, restrain using \"> outputfile\" or piping the output into another command if you require unicode support" << endl;
            OUTSTREAM << "because for instance, when piping, your terminal does not treat the output as binary; " << endl;
            OUTSTREAM << "it will meddle with the encoding, resulting in unusable output." << endl;
            OUTSTREAM << endl;
            OUTSTREAM << "In the interactive shell, the library used for reading the inputs is not able to capture unicode inputs by default" << endl;
            OUTSTREAM << "There's a workaround to activate an alternative way to read input. You can activate it using \"unicode\" command. " << endl;
            OUTSTREAM << "However, if you do so, arrow keys and hotkeys combinations will be disabled. You can disable this input mode again. " << endl;
            OUTSTREAM << "See \"unicode --help\" for further info." << endl;
        }
#endif
        else
        {
            OUTSTREAM << "Here is the list of available commands and their usage" << endl;
            OUTSTREAM << "Use \"help -f\" to get a brief description of the commands" << endl;
            OUTSTREAM << "You can get further help on a specific command with \"command\" --help " << endl;
            OUTSTREAM << "Alternatively, you can use \"help\" -ff to get a complete description of all commands" << endl;
            OUTSTREAM << "Use \"help --non-interactive\" to learn how to use MEGAcmd with scripts" << endl;
            OUTSTREAM << "Use \"help --upgrade\" to learn about the limitations and obtaining PRO accounts" << endl;

            OUTSTREAM << endl << "Commands:" << endl;

            printAvailableCommands(getFlag(&clflags,"f"));
            OUTSTREAM << endl << "Verbosity: You can increase the amount of information given by any command by passing \"-v\" (\"-vv\", \"-vvv\", ...)" << endl;
        }
        return;
    }

    cmdexecuter->executecommand(words, &clflags, &cloptions);
}


static bool process_line(char* l)
{
    switch (prompt)
    {
        case AREYOUSURETODELETE:
            if (!strcmp(l,"yes") || !strcmp(l,"YES") || !strcmp(l,"y") || !strcmp(l,"Y"))
            {
                cmdexecuter->confirmDelete();
            }
            else if (!strcmp(l,"no") || !strcmp(l,"NO") || !strcmp(l,"n") || !strcmp(l,"N"))
            {
                cmdexecuter->discardDelete();
            }
            else
            {
                //Do nth, ask again
                OUTSTREAM << "Please enter [y]es/[n]o: " << flush;
            }
        break;
        case LOGINPASSWORD:
        {
            if (!strlen(l))
            {
                break;
            }
            if (!cmdexecuter->confirming)
            {
                cmdexecuter->loginWithPassword(l);
            }
            else
            {
                cmdexecuter->confirmWithPassword(l);
                cmdexecuter->confirming = false;
            }
            setprompt(COMMAND);
            break;
        }

        case OLDPASSWORD:
        {
            if (!strlen(l))
            {
                break;
            }
            oldpasswd = l;
            OUTSTREAM << endl;
            setprompt(NEWPASSWORD);
            break;
        }

        case NEWPASSWORD:
        {
            if (!strlen(l))
            {
                break;
            }
            newpasswd = l;
            OUTSTREAM << endl;
            setprompt(PASSWORDCONFIRM);
        }
        break;

        case PASSWORDCONFIRM:
        {
            if (!strlen(l))
            {
                break;
            }
            if (l != newpasswd)
            {
                OUTSTREAM << endl << "New passwords differ, please try again" << endl;
            }
            else
            {
                OUTSTREAM << endl;
                if (!cmdexecuter->signingup)
                {
                    cmdexecuter->changePassword(oldpasswd.c_str(), newpasswd.c_str());
                }
                else
                {
                    cmdexecuter->signupWithPassword(l);
                    cmdexecuter->signingup = false;
                }
            }

            setprompt(COMMAND);
            break;
        }

        case COMMAND:
        {
            if (!l || !strcmp(l, "q") || !strcmp(l, "quit") || !strcmp(l, "exit") || !strcmp(l, "exit ") || !strcmp(l, "quit "))
            {
                //                store_line(NULL);
                return true; // exit
            }

            else  if (!strncmp(l,"sendack",strlen("sendack")) ||
                      !strncmp(l,"Xsendack",strlen("Xsendack")))
            {
                string sack="ack";
                cm->informStateListeners(sack);
                break;
            }
            executecommand(l);
            break;
        }
    }
    return false; //Do not exit
}

void * doProcessLine(void *pointer)
{
    CmdPetition *inf = (CmdPetition*)pointer;

    OUTSTRINGSTREAM s;
    setCurrentThreadOutStream(&s);
    setCurrentThreadLogLevel(MegaApi::LOG_LEVEL_ERROR);
    setCurrentOutCode(MCMD_OK);
    setCurrentPetition(inf);

    if (inf->getLine() && *(inf->getLine())=='X')
    {
        setCurrentThreadIsCmdShell(true);
        char * aux = inf->line;
        inf->line=strdup(inf->line+1);
        free(aux);
    }
    else
    {
        setCurrentThreadIsCmdShell(false);
    }

    LOG_verbose << " Processing " << *inf << " in thread: " << MegaThread::currentThreadId() << " " << cm->get_petition_details(inf);

    doExit = process_line(inf->getLine());

    if (doExit)
    {
        LOG_verbose << " Exit registered upon process_line: " ;
    }

    LOG_verbose << " Procesed " << *inf << " in thread: " << MegaThread::currentThreadId() << " " << cm->get_petition_details(inf);

    MegaThread * petitionThread = inf->getPetitionThread();
    cm->returnAndClosePetition(inf, &s, getCurrentOutCode());

    semaphoreClients.release();

    if (doExit && (!interactiveThread() || getCurrentThreadIsCmdShell() ))
    {
        cm->stopWaiting();
    }

    mutexEndedPetitionThreads.lock();
    endedPetitionThreads.push_back(petitionThread);
    mutexEndedPetitionThreads.unlock();

    return NULL;
}


bool askforConfirmation(string message)
{
    CmdPetition *inf = getCurrentPetition();
    if (inf)
    {
        return cm->getConfirmation(inf,message);
    }
    else
    {
        LOG_err << "Unable to get current petition to ask for confirmation";
    }

    return false;
}


void delete_finished_threads()
{
    mutexEndedPetitionThreads.lock();
    for (std::vector<MegaThread *>::iterator it = endedPetitionThreads.begin(); it != endedPetitionThreads.end(); )
    {
        MegaThread *mt = (MegaThread*)*it;
        for (std::vector<MegaThread *>::iterator it2 = petitionThreads.begin(); it2 != petitionThreads.end(); )
        {
            if (mt == (MegaThread*)*it2)
            {
                it2 = petitionThreads.erase(it2);
            }
            else
            {
                ++it2;
            }
        }

        mt->join();
        delete mt;
        it = endedPetitionThreads.erase(it);
    }
    mutexEndedPetitionThreads.unlock();
}



void finalize()
{
    static bool alreadyfinalized = false;
    if (alreadyfinalized)
        return;
    alreadyfinalized = true;
    LOG_info << "closing application ...";
    delete_finished_threads();
    delete cm;
    if (!consoleFailed)
    {
        delete console;
    }

    delete megaCmdMegaListener;
    delete api;

    while (!apiFolders.empty())
    {
        delete apiFolders.front();
        apiFolders.pop();
    }

    for (std::vector< MegaApi * >::iterator it = occupiedapiFolders.begin(); it != occupiedapiFolders.end(); ++it)
    {
        delete ( *it );
    }

    occupiedapiFolders.clear();

    delete megaCmdGlobalListener;
    delete cmdexecuter;

    LOG_debug << "resources have been cleaned ...";
    delete loggerCMD;
    ConfigurationManager::unloadConfiguration();

}

int currentclientID = 1;

// main loop
void megacmd()
{
    LOG_info << "Listening to petitions ... ";

    for (;; )
    {
        cm->waitForPetition();

        api->retryPendingConnections();

        if (doExit)
        {
            LOG_verbose << "closing after wait ..." ;
            return;
        }

        if (cm->receivedPetition())
        {

            LOG_verbose << "Client connected ";

            CmdPetition *inf = cm->getPetition();

            LOG_verbose << "petition registered: " << *inf;

            delete_finished_threads();

            if (!inf || !strcmp(inf->getLine(),"ERROR"))
            {
                LOG_warn << "Petition couldn't be registered. Dismissing it.";
                delete inf;
            }
            // if state register petition
            else  if (!strncmp(inf->getLine(),"registerstatelistener",strlen("registerstatelistener")) ||
                      !strncmp(inf->getLine(),"Xregisterstatelistener",strlen("Xregisterstatelistener")))
            {

                cm->registerStateListener(inf);

                // communicate client ID
                string s = "clientID:";
                s+=SSTR(currentclientID);
                s+=(char)0x1F;
                inf->clientID = currentclientID;
                currentclientID++;

                cm->informStateListener(inf,s);

                // communicate status info
                s = "prompt:";
                s+=dynamicprompt;
                s+=(char)0x1F;

#if defined(_WIN32) || defined(__APPLE__)
                string message="";
                ostringstream os;
                MegaCmdListener *megaCmdListener = new MegaCmdListener(NULL);
                api->getLastAvailableVersion("BdARkQSQ",megaCmdListener);
                if (!megaCmdListener->trywait(2000))
                {
                    if (!megaCmdListener->getError())
                    {
                        LOG_fatal << "No MegaError at getLastAvailableVersion: ";
                    }
                    else if (megaCmdListener->getError()->getErrorCode() != MegaError::API_OK)
                    {
                        LOG_debug << "Couldn't get latests available version: " << megaCmdListener->getError()->getErrorString();
                    }
                    else
                    {
                        if (megaCmdListener->getRequest()->getNumber() != MEGACMD_CODE_VERSION)
                        {
                            os << "---------------------------------------------------------------------" << endl;
                            os << "--        There is a new version available of megacmd: " << setw(12) << left << megaCmdListener->getRequest()->getName() << "--" << endl;
                            os << "--        Please, download it from https://mega.nz/cmd             --" << endl;
#if defined(__APPLE__)
                            os << "--        Before installing enter \"exit\" to close MEGAcmd          --" << endl;
#endif
                            os << "---------------------------------------------------------------------" << endl;
                        }
                    }
                    message=os.str();
                    delete megaCmdListener;
                }
                else
                {
                    LOG_debug << "Couldn't get latests available version (petition timed out)";

                    api->removeRequestListener(megaCmdListener);
                    delete megaCmdListener;
                }

                if (message.size())
                {
                    s += "message:";
                    s+=message;
                    s+=(char)0x1F;
                }
#endif
                cm->informStateListener(inf,s);

            }
            else
            { // normal petition

                semaphoreClients.wait();

                //append new one
                MegaThread * petitionThread = new MegaThread();
                petitionThreads.push_back(petitionThread);
                inf->setPetitionThread(petitionThread);

                LOG_verbose << "starting processing: <" << *inf << ">";

                petitionThread->start(doProcessLine, (void*)inf);
            }
        }
    }
}

class NullBuffer : public std::streambuf
{
public:
    int overflow(int c)
    {
        return c;
    }
};

void printCenteredLine(string msj, unsigned int width, bool encapsulated = true)
{
    if (msj.size()>width)
    {
        width = msj.size();
    }
    if (encapsulated)
        COUT << "|";
    for (unsigned int i = 0; i < (width-msj.size())/2; i++)
        COUT << " ";
    COUT << msj;
    for (unsigned int i = 0; i < (width-msj.size())/2 + (width-msj.size())%2 ; i++)
        COUT << " ";
    if (encapsulated)
        COUT << "|";
    COUT << endl;
}

void printWelcomeMsg()
{
    unsigned int width = getNumberOfCols(75);

#ifdef _WIN32
        width--;
#endif

    COUT << endl;
    COUT << ".";
    for (unsigned int i = 0; i < width; i++)
        COUT << "=" ;
    COUT << ".";
    COUT << endl;
    printCenteredLine(" __  __ _____ ____    _                      _ ",width);
    printCenteredLine("|  \\/  | ___|/ ___|  / \\   ___ _ __ ___   __| |",width);
    printCenteredLine("| |\\/| | \\  / |  _  / _ \\ / __| '_ ` _ \\ / _` |",width);
    printCenteredLine("| |  | | /__\\ |_| |/ ___ \\ (__| | | | | | (_| |",width);
    printCenteredLine("|_|  |_|____|\\____/_/   \\_\\___|_| |_| |_|\\__,_|",width);

    COUT << "|";
    for (unsigned int i = 0; i < width; i++)
        COUT << " " ;
    COUT << "|";
    COUT << endl;
    printCenteredLine("SERVER",width);

    COUT << "`";
    for (unsigned int i = 0; i < width; i++)
        COUT << "=" ;
    COUT << "´";
    COUT << endl;

}

int quote_detector(char *line, int index)
{
    return (
        index > 0 &&
        line[index - 1] == '\\' &&
        !quote_detector(line, index - 1)
    );
}


#ifdef __MACH__


bool enableSetuidBit()
{
    char *response = runWithRootPrivileges("do shell script \"chown root /Applications/MEGAcmd.app/Contents/MacOS/MEGAcmdLoader && chmod 4755 /Applications/MEGAcmd.app/Contents/MacOS/MEGAcmdLoader && echo true\"");
    if (!response)
    {
        return NULL;
    }
    bool result = strlen(response) >= 4 && !strncmp(response, "true", 4);
    delete response;
    return result;
}


void initializeMacOSStuff(int argc, char* argv[])
{
#ifndef NDEBUG
        return;
#endif

    int fd = -1;
    if (argc)
    {
        long int value = strtol(argv[argc-1], NULL, 10);
        if (value > 0 && value < INT_MAX)
        {
            fd = value;
        }
    }

    if (fd < 0)
    {
        if (!enableSetuidBit())
        {
            ::exit(0);
        }

        //Reboot
        if (fork() )
        {
            execv("/Applications/MEGAcmd.app/Contents/MacOS/MEGAcmdLoader",argv);
        }
        sleep(10);
        ::exit(0);
    }
}

#endif

bool runningInBackground()
{
#ifndef _WIN32
    pid_t fg = tcgetpgrp(STDIN_FILENO);
    if(fg == -1) {
        // Piped:
        return false;
    }  else if (fg == getpgrp()) {
        // foreground
        return false;
    } else {
        // background
        return true;
    }
#endif
    return false;
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
    // Set Environment's default locale
    setlocale(LC_ALL, "en-US");
#endif

#ifdef __MACH__
    initializeMacOSStuff(argc,argv);
#endif

    NullBuffer null_buffer;
    std::ostream null_stream(&null_buffer);
    SimpleLogger::setAllOutputs(&null_stream);
    SimpleLogger::setLogLevel(logMax); // do not filter anything here, log level checking is done by loggerCMD

    loggerCMD = new MegaCMDLogger(&COUT);

    loggerCMD->setApiLoggerLevel(MegaApi::LOG_LEVEL_ERROR);
    loggerCMD->setCmdLoggerLevel(MegaApi::LOG_LEVEL_INFO);
    loggerCMD->setCmdLoggerLevel(MegaApi::LOG_LEVEL_DEBUG);

    string loglevelenv;
#ifndef _WIN32
    loglevelenv = (getenv ("MEGACMD_LOGLEVEL") == NULL)?"":getenv ("MEGACMD_LOGLEVEL");
#endif

    if (!loglevelenv.compare("DEBUG") || (( argc > 1 ) && !( strcmp(argv[1], "--debug"))) )
    {
        loggerCMD->setCmdLoggerLevel(MegaApi::LOG_LEVEL_DEBUG);
    }
    if (!loglevelenv.compare("FULLDEBUG") || (( argc > 1 ) && !( strcmp(argv[1], "--debug-full"))) )
    {
        loggerCMD->setApiLoggerLevel(MegaApi::LOG_LEVEL_DEBUG);
        loggerCMD->setCmdLoggerLevel(MegaApi::LOG_LEVEL_DEBUG);
    }

    mutexHistory.init(false);

    mutexEndedPetitionThreads.init(false);

    ConfigurationManager::loadConfiguration(( argc > 1 ) && !( strcmp(argv[1], "--debug")));

    char userAgent[30];
    sprintf(userAgent, "MEGAcmd/%d.%d.%d.0", MEGACMD_MAJOR_VERSION,MEGACMD_MINOR_VERSION,MEGACMD_MICRO_VERSION);

#if defined(__MACH__) && defined(ENABLE_SYNC)
    int fd = -1;
    if (argc)
    {
        long int value = strtol(argv[argc-1], NULL, 10);
        if (value > 0 && value < INT_MAX)
        {
            fd = value;
        }
    }

    if (fd >= 0)
    {
        api = new MegaApi("BdARkQSQ", ConfigurationManager::getConfigFolder().c_str(), userAgent, fd);
    }
    else
    {
        api = new MegaApi("BdARkQSQ", ConfigurationManager::getConfigFolder().c_str(), userAgent);
    }
#else
    api = new MegaApi("BdARkQSQ", ConfigurationManager::getConfigFolder().c_str(), userAgent);
#endif

    for (int i = 0; i < 5; i++)
    {
        MegaApi *apiFolder = new MegaApi("BdARkQSQ", (const char*)NULL, userAgent);
        apiFolders.push(apiFolder);
        apiFolder->addLoggerObject(loggerCMD);
        apiFolder->setLogLevel(MegaApi::LOG_LEVEL_MAX);
        semaphoreapiFolders.release();
    }

    for (int i = 0; i < 100; i++)
    {
        semaphoreClients.release();
    }

    mutexapiFolders.init(false);

    api->addLoggerObject(loggerCMD);
    api->setLogLevel(MegaApi::LOG_LEVEL_MAX);

    sandboxCMD = new MegaCmdSandbox();
    cmdexecuter = new MegaCmdExecuter(api, loggerCMD, sandboxCMD);

    megaCmdGlobalListener = new MegaCmdGlobalListener(loggerCMD, sandboxCMD);
    megaCmdMegaListener = new MegaCmdMegaListener(api, NULL);
    api->addGlobalListener(megaCmdGlobalListener);
    api->addListener(megaCmdMegaListener);

    // set up the console
#ifdef _WIN32
    console = new CONSOLE_CLASS;
#else
    struct termios term;
    if ( ( tcgetattr(STDIN_FILENO, &term) < 0 ) || runningInBackground() ) //try console
    {
        consoleFailed = true;
        console = NULL;
    }
    else
    {
        console = new CONSOLE_CLASS;
    }
#endif
    cm = new COMUNICATIONMANAGER();

#if _WIN32
    if( SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE ) )
     {
        LOG_debug << "Control handler set";
     }
     else
     {
        LOG_warn << "Control handler set";
     }
#else
    // prevent CTRL+C exit
    if (!consoleFailed){
        signal(SIGINT, sigint_handler);
    }
#endif

    atexit(finalize);

    printWelcomeMsg();

    if (!ConfigurationManager::session.empty())
    {
        loginInAtStartup = true;
        stringstream logLine;
        logLine << "login " << ConfigurationManager::session;
        LOG_debug << "Executing ... " << logLine.str();
        process_line((char*)logLine.str().c_str());
        loginInAtStartup = false;
    }

    megacmd();
    finalize();
}
