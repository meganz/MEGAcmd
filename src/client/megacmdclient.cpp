/**
 * @file src/client/megacmdclient.cpp
 * @brief MEGAcmdClient: Client application of MEGAcmd
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

#include "../megacmdcommonutils.h"
#include "../megacmdshell/megacmdshellcommunications.h"
#include "../megacmdshell/megacmdshellcommunicationsnamedpipes.h"

#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <memory.h>
#include <limits.h>

#ifdef _WIN32
#include <Shlwapi.h> //PathAppend
#include <Shellapi.h> //CommandLineToArgvW

#include <fcntl.h>
#include <io.h>
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/un.h>
#endif

#ifdef _WIN32
#else
#include <sys/ioctl.h> // console size
#endif
#include <sstream>


#define PROGRESS_COMPLETE -2
#define SPROGRESS_COMPLETE "-2"

using namespace std;

#define SSTR( x ) static_cast< const std::ostringstream & >( \
        (  std::ostringstream() << std::dec << x ) ).str()

void printprogress(long long completed, long long total, const char *title = "TRANSFERRING");

#ifdef _WIN32
// convert UTF-8 to Windows Unicode
void path2local(string* path, string* local)
{
    // make space for the worst case
    local->resize((path->size() + 1) * sizeof(wchar_t));

    int len = MultiByteToWideChar(CP_UTF8, 0,
                                  path->c_str(),
                                  -1,
                                  (wchar_t*)local->data(),
                                  int(local->size() / sizeof(wchar_t) + 1));
    if (len)
    {
        // resize to actual result
        local->resize(sizeof(wchar_t) * (len - 1));
    }
    else
    {
        local->clear();
    }
}

// convert to Windows Unicode Utf8
void local2path(string* local, string* path)
{
    path->resize((local->size() + 1) * 4 / sizeof(wchar_t));

    path->resize(WideCharToMultiByte(CP_UTF8, 0, (wchar_t*)local->data(),
                                     int(local->size() / sizeof(wchar_t)),
                                     (char*)path->data(),
                                     int(path->size() + 1),
                                     NULL, NULL));
    //normalize(path);
}
//TODO: delete the 2 former??

wstring getWAbsPath(wstring localpath)
{
    if (!localpath.size())
    {
        return localpath;
    }

    wstring utf8absolutepath;

    wstring absolutelocalpath;

   if (!PathIsRelativeW((LPCWSTR)localpath.data()))
   {
       if (localpath.find(L"\\\\?\\") != 0)
       {
           localpath.insert(0, L"\\\\?\\");
       }
       return localpath;
   }

   int len = GetFullPathNameW((LPCWSTR)localpath.data(), 0, NULL, NULL);
   if (len <= 0)
   {
      return localpath;
   }

   absolutelocalpath.resize(len);
   int newlen = GetFullPathNameW((LPCWSTR)localpath.data(), len, (LPWSTR)absolutelocalpath.data(), NULL);
   if (newlen <= 0 || newlen >= len)
   {
       cerr << " failed to get CWD" << endl;
       return localpath;
   }
   absolutelocalpath.resize(newlen);

   if (absolutelocalpath.find(L"\\\\?\\") != 0)
   {
       absolutelocalpath.insert(0, L"\\\\?\\");
   }

   return absolutelocalpath;

}
#endif

string clientID; //identifier for a registered state listener

string getAbsPath(string relativePath)
{
    if (!relativePath.size())
    {
        return relativePath;
    }

#ifdef _WIN32
    string utf8absolutepath;
    string localpath;
    path2local(&relativePath, &localpath);

    string absolutelocalpath;
    localpath.append("", 1);

   if (!PathIsRelativeW((LPCWSTR)localpath.data()))
   {
       utf8absolutepath = relativePath;
       if (utf8absolutepath.find("\\\\?\\") != 0)
       {
           utf8absolutepath.insert(0, "\\\\?\\");
       }
       return utf8absolutepath;
   }

   int len = GetFullPathNameW((LPCWSTR)localpath.data(), 0, NULL, NULL);
   if (len <= 0)
   {
      return relativePath;
   }

   absolutelocalpath.resize(len * sizeof(wchar_t));
   int newlen = GetFullPathNameW((LPCWSTR)localpath.data(), len, (LPWSTR)absolutelocalpath.data(), NULL);
   if (newlen <= 0 || newlen >= len)
   {
       cerr << " failed to get CWD" << endl;
       return relativePath;
   }
   absolutelocalpath.resize(newlen* sizeof(wchar_t));

   local2path(&absolutelocalpath, &utf8absolutepath);

   if (utf8absolutepath.find("\\\\?\\") != 0)
   {
       utf8absolutepath.insert(0, "\\\\?\\");
   }

   return utf8absolutepath;

#else
    if (relativePath.size() && relativePath.at(0) == '/')
    {
        return relativePath;
    }
    else
    {
        char cCurrentPath[PATH_MAX];
        if (!getcwd(cCurrentPath, sizeof(cCurrentPath)))
        {
            cerr << " failed to get CWD" << endl;
            return relativePath;
        }

        string absolutepath = cCurrentPath;
        absolutepath.append("/");
        absolutepath.append(relativePath);
        return absolutepath;
    }

    return relativePath;
#endif

}

string parseArgs(int argc, char* argv[])
{
    vector<string> absolutedargs;
    int totalRealArgs = 0;
    if (argc>1)
    {
        absolutedargs.push_back(argv[1]);

        if (!strcmp(argv[1],"get")
                || !strcmp(argv[1],"put")
                || !strcmp(argv[1],"login")
                || !strcmp(argv[1],"reload") )
        {
            int waittime = 15000;
            while (waittime > 0 && !clientID.size())
            {
                sleepMilliSeconds(100);
                waittime -= 100;
            }
            if (clientID.size())
            {
                string sclientID = "--clientID=";
                sclientID+=clientID;
                absolutedargs.push_back(sclientID);
            }
        }

        if (!strcmp(argv[1],"backup")
                || !strcmp(argv[1],"du")
                || !strcmp(argv[1],"mediainfo")
                || !strcmp(argv[1],"sync")
                || !strcmp(argv[1],"transfers") )
        {
            unsigned int width = getNumberOfCols(80);
            int pathSize = width/2;

            if ( !strcmp(argv[1],"transfers") )
            {
                pathSize = int((width-46)/2);
            }
            else if ( !strcmp(argv[1],"du") )
            {
                pathSize = int(width-13);
                for (int i = 1; i < argc; i++)
                {
                    if (strstr(argv[i], "--versions"))
                    {
                        pathSize -= 11;
                        break;
                    }
                }
            }
            else if ( !strcmp(argv[1],"sync") )
            {
                pathSize = int((width-46)/2);
            }
            else if ( !strcmp(argv[1],"mediainfo") )
            {
                pathSize = int(width - 28);
            }
            else if ( !strcmp(argv[1],"backup") )
            {
                pathSize = int((width-21)/2);
            }

            string spathSize = "--path-display-size=";
            spathSize+=SSTR(pathSize);
            absolutedargs.push_back(spathSize);
        }

        if (!strcmp(argv[1],"sync"))
        {
            for (int i = 2; i < argc; i++)
            {
                if (strlen(argv[i]) && argv[i][0] !='-' )
                {
                    totalRealArgs++;
                }
            }
            bool firstrealArg = true;
            for (int i = 2; i < argc; i++)
            {
                if (strlen(argv[i]) && argv[i][0] !='-' )
                {
                    if (totalRealArgs >=2 && firstrealArg)
                    {
                        absolutedargs.push_back(getAbsPath(argv[i]));
                        firstrealArg=false;
                    }
                    else
                    {
                        absolutedargs.push_back(argv[i]);
                    }
                }
                else
                {
                    absolutedargs.push_back(argv[i]);
                }
            }
        }
        else if (!strcmp(argv[1],"lcd")) //localpath args
        {
            for (int i = 2; i < argc; i++)
            {
                if (strlen(argv[i]) && argv[i][0] !='-' )
                {
                    absolutedargs.push_back(getAbsPath(argv[i]));
                }
                else
                {
                    absolutedargs.push_back(argv[i]);
                }
            }
        }
        else if (!strcmp(argv[1],"get") || !strcmp(argv[1],"preview") || !strcmp(argv[1],"thumbnail"))
        {
            for (int i = 2; i < argc; i++)
            {
                if (strlen(argv[i]) && argv[i][0] != '-' )
                {
                    totalRealArgs++;
                    if (totalRealArgs>1)
                    {
                        absolutedargs.push_back(getAbsPath(argv[i]));
                    }
                    else
                    {
                        absolutedargs.push_back(argv[i]);
                    }
                }
                else
                {
                    absolutedargs.push_back(argv[i]);
                }
            }
            if (totalRealArgs == 1)
            {
                absolutedargs.push_back(getAbsPath("."));

            }
        }
        else if (!strcmp(argv[1],"put"))
        {
            int lastRealArg = 0;
            for (int i = 2; i < argc; i++)
            {
                if (strlen(argv[i]) && argv[i][0] !='-' )
                {
                    lastRealArg = i;
                }
            }
            bool firstRealArg = true;
            for  (int i = 2; i < argc; i++)
            {
                if (strlen(argv[i]) && argv[i][0] !='-')
                {
                    if (firstRealArg || i <lastRealArg)
                    {
                        absolutedargs.push_back(getAbsPath(argv[i]));
                        firstRealArg = false;
                    }
                    else
                    {
                        absolutedargs.push_back(argv[i]);
                    }
                }
                else
                {
                    absolutedargs.push_back(argv[i]);
                }
            }
        }
        else
        {
            for (int i = 2; i < argc; i++)
            {
                absolutedargs.push_back(argv[i]);
            }
        }
    }

    string toret="";
    for (u_int i=0; i < absolutedargs.size(); i++)
    {
        if (absolutedargs.at(i).find(" ") != string::npos || !absolutedargs.at(i).size())
        {
            toret += "\"";
        }
        toret+=absolutedargs.at(i);
        if (absolutedargs.at(i).find(" ") != string::npos || !absolutedargs.at(i).size())
        {
            toret += "\"";
        }

        if (i != (absolutedargs.size()-1))
        {
            toret += " ";
        }
    }

    return toret;
}


#ifdef _WIN32

wstring parsewArgs(int argc, wchar_t* argv[])
{
    for (int i=1;i<argc;i++)
    {
        if (i<(argc-1) && !wcscmp(argv[i],L"-o"))
        {
            if (i < (argc-2))
                argv[i]=argv[i+2];
            argc=argc-2;
        }
    }


    vector<wstring> absolutedargs;
    int totalRealArgs = 0;
    if (argc>1)
    {
        absolutedargs.push_back(argv[1]);

        if (!wcscmp(argv[1],L"get")
                || !wcscmp(argv[1],L"put")
                || !wcscmp(argv[1],L"login")
                || !wcscmp(argv[1],L"reload") )
        {
            int waittime = 5000;
            while (waittime > 0 && !clientID.size())
            {
                sleepMilliSeconds(100);
                waittime -= 100;
            }
            if (clientID.size())
            {
                wstring sclientID = L"--clientID=";
                std::wstring wclientID(clientID.begin(), clientID.end());
                sclientID+=wclientID;
                absolutedargs.push_back(sclientID);
            }
        }

        if (!wcscmp(argv[1],L"backup")
                || !wcscmp(argv[1],L"du")
                || !wcscmp(argv[1],L"mediainfo")
                || !wcscmp(argv[1],L"sync")
                || !wcscmp(argv[1],L"transfers") )
        {
            unsigned int width = getNumberOfCols(80);
            int pathSize = width/2;

            if ( !wcscmp(argv[1],L"transfers") )
            {
                pathSize = int((width-46)/2);
            }
            else if ( !wcscmp(argv[1],L"du") )
            {
                pathSize = int(width-13);

                for (int i = 1; i < argc; i++)
                {
                    if (wcsstr(argv[i], L"--versions"))
                    {
                        pathSize -= 11;
                        break;
                    }
                }

            }
            else if ( !wcscmp(argv[1],L"sync") )
            {
                pathSize = int((width-46)/2);
            }
            else if ( !wcscmp(argv[1],L"mediainfo") )
            {
                pathSize = int(width - 28);
            }
            else if ( !wcscmp(argv[1],L"backup") )
            {
                pathSize = int((width-21)/2);
            }

            wstring spathSize = L"--path-display-size=";
            string sps = SSTR(pathSize);
            std::wstring wspathSize(sps.begin(), sps.end());
            spathSize+=wspathSize;
            absolutedargs.push_back(spathSize);
        }

        if (!wcscmp(argv[1],L"sync"))
        {
            for (int i = 2; i < argc; i++)
            {
                if (wcslen(argv[i]) && argv[i][0] !='-' )
                {
                    totalRealArgs++;
                }
            }
            bool firstrealArg = true;
            for (int i = 2; i < argc; i++)
            {
                if (wcslen(argv[i]) && argv[i][0] !='-' )
                {
                    if (totalRealArgs >=2 && firstrealArg)
                    {
                        absolutedargs.push_back(getWAbsPath(argv[i]));
                        firstrealArg=false;
                    }
                    else
                    {
                        absolutedargs.push_back(argv[i]);
                    }
                }
                else
                {
                    absolutedargs.push_back(argv[i]);
                }
            }
        }
        else if (!wcscmp(argv[1],L"lcd")) //localpath args
        {
            for (int i = 2; i < argc; i++)
            {
                if (wcslen(argv[i]) && argv[i][0] !='-' )
                {
                    absolutedargs.push_back(getWAbsPath(argv[i]));
                }
                else
                {
                    absolutedargs.push_back(argv[i]);
                }
            }
        }
        else if (!wcscmp(argv[1],L"get") || !wcscmp(argv[1],L"preview") || !wcscmp(argv[1],L"thumbnail"))
        {
            for (int i = 2; i < argc; i++)
            {
                if (wcslen(argv[i]) && argv[i][0] != '-' )
                {
                    totalRealArgs++;
                    if (totalRealArgs>1)
                    {
                        absolutedargs.push_back(getWAbsPath(argv[i]));
                    }
                    else
                    {
                        absolutedargs.push_back(argv[i]);
                    }
                }
                else
                {
                    absolutedargs.push_back(argv[i]);
                }
            }
            if (totalRealArgs == 1)
            {
                absolutedargs.push_back(getWAbsPath(L"."));

            }
        }
        else if (!wcscmp(argv[1],L"put"))
        {
            int lastRealArg = 0;
            for (int i = 2; i < argc; i++)
            {
                if (wcslen(argv[i]) && argv[i][0] !='-' )
                {
                    lastRealArg = i;
                }
            }
            bool firstRealArg = true;
            for  (int i = 2; i < argc; i++)
            {
                if (wcslen(argv[i]) && argv[i][0] !='-')
                {
                    if (firstRealArg || i <lastRealArg)
                    {
                        absolutedargs.push_back(getWAbsPath(argv[i]));
                        firstRealArg = false;
                    }
                    else
                    {
                        absolutedargs.push_back(argv[i]);
                    }
                }
                else
                {
                    absolutedargs.push_back(argv[i]);
                }
            }
        }
        else
        {
            for (int i = 2; i < argc; i++)
            {
                absolutedargs.push_back(argv[i]);
            }
        }
    }

    wstring toret=L"";
    for (u_int i=0; i < absolutedargs.size(); i++)
    {
        if (absolutedargs.at(i).find(L" ") != wstring::npos || !absolutedargs.at(i).size())
        {
            toret += L"\"";
        }
        toret+=absolutedargs.at(i);
        if (absolutedargs.at(i).find(L" ") != wstring::npos || !absolutedargs.at(i).size())
        {
            toret += L"\"";
        }

        if (i != (absolutedargs.size()-1))
        {
            toret += L" ";
        }
    }

    return toret;
}
#endif

std::string readresponse(const char *question)
{
    string response;
    cout << question << " " << flush;
    getline(cin, response);
    return response;
}

void printprogress(long long completed, long long total, const char *title)
{
    static bool alreadyFinished = false; //flag to show progress
    static float percentDowloaded = 0.0;

    float oldpercent = percentDowloaded;
    if (total == 0)
    {
        percentDowloaded = 0;
    }
    else
    {
        percentDowloaded = float(completed * 1.0 / total * 100.0);
    }
    if (completed != PROGRESS_COMPLETE && (alreadyFinished || ( ( percentDowloaded == oldpercent ) && ( oldpercent != 0 ) ) ))
    {
        return;
    }
    if (percentDowloaded < 0)
    {
        percentDowloaded = 0;
    }

    if (total < 0)
    {
        return; // after a 100% this happens
    }
    if (completed != PROGRESS_COMPLETE  && completed < 0.001 * total)
    {
        return; // after a 100% this happens
    }
    if (completed == PROGRESS_COMPLETE)
    {
        alreadyFinished = true;
        completed = total;
        percentDowloaded = 100;
    }

    printPercentageLineCerr(title, completed, total, percentDowloaded, !alreadyFinished);
}

void statechangehandle(string statestring)
{
    char statedelim[2]={(char)0x1F,'\0'};
    size_t nextstatedelimitpos = statestring.find(statedelim);
    static bool shown_partial_progress = false;

    while (nextstatedelimitpos!=string::npos && statestring.size())
    {
        string newstate = statestring.substr(0,nextstatedelimitpos);
        statestring=statestring.substr(nextstatedelimitpos+1);
        nextstatedelimitpos = statestring.find(statedelim);
        if (newstate.compare(0, strlen("prompt:"), "prompt:") == 0)
        {
            //ignore prompt state
        }
        else if (newstate.compare(0, strlen("endtransfer:"), "endtransfer:") == 0)
        {
            string rest = newstate.substr(strlen("endtransfer:"));
            if (rest.size() >=3)
            {
                bool isdown = rest.at(0) == 'D';
                string path = rest.substr(2);

                stringstream os;
                if (shown_partial_progress)
                    os << endl;

                os << (isdown?"Download":"Upload") << " finished: " << path << endl;

#ifdef _WIN32
                wstring wbuffer;
                stringtolocalw((const char*)os.str().data(),&wbuffer);
                int oldmode;
                MegaCmdShellCommunicationsNamedPipes::megaCmdStdoutputing.lock();
                oldmode = _setmode(_fileno(stdout), _O_U8TEXT);
                OUTSTREAM << wbuffer << flush;
                _setmode(_fileno(stdout), oldmode);
                MegaCmdShellCommunicationsNamedPipes::megaCmdStdoutputing.unlock();

#else
                OUTSTREAM << os.str();
#endif
            }
        }
        else if (newstate.compare(0, strlen("message:"), "message:") == 0)
        {
            string contents = newstate.substr(strlen("message:"));
            unsigned int width = getNumberOfCols(80);
            if (width > 1 ) width--;
            MegaCmdShellCommunicationsNamedPipes::megaCmdStdoutputing.lock();
            if (contents.find("-----") != 0)
            {
                printCenteredContentsCerr(contents, width);
            }
            else
            {
                cerr << endl <<  contents << endl;
            }
            MegaCmdShellCommunicationsNamedPipes::megaCmdStdoutputing.unlock();
        }
        else if (newstate.compare(0, strlen("clientID:"), "clientID:") == 0)
        {
            clientID = newstate.substr(strlen("clientID:")).c_str();
        }
        else if (newstate.compare(0, strlen("progress:"), "progress:") == 0)
        {
            string rest = newstate.substr(strlen("progress:"));

            size_t nexdel = rest.find(":");
            string received = rest.substr(0,nexdel);

            rest = rest.substr(nexdel+1);
            nexdel = rest.find(":");
            string total = rest.substr(0,nexdel);

            string title;
            if ( (nexdel != string::npos) && (nexdel < rest.size() ) )
            {
                rest = rest.substr(nexdel+1);
                nexdel = rest.find(":");
                title = rest.substr(0,nexdel);
            }

            if (received!=SPROGRESS_COMPLETE)
            {
                shown_partial_progress = true;
            }
            else
            {
                shown_partial_progress = false;
            }
            MegaCmdShellCommunicationsNamedPipes::megaCmdStdoutputing.lock();

            if (title.size())
            {
                if (received==SPROGRESS_COMPLETE)
                {
                    printprogress(PROGRESS_COMPLETE, charstoll(total.c_str()),title.c_str());

                }
                else
                {
                    printprogress(charstoll(received.c_str()), charstoll(total.c_str()),title.c_str());
                }
            }
            else
            {
                if (received==SPROGRESS_COMPLETE)
                {
                    printprogress(PROGRESS_COMPLETE, charstoll(total.c_str()));
                }
                else
                {
                    printprogress(charstoll(received.c_str()), charstoll(total.c_str()));
                }
            }
            MegaCmdShellCommunicationsNamedPipes::megaCmdStdoutputing.unlock();

        }
        else if (newstate == "ack")
        {
            // do nothing, all good
        }
        else if (newstate == "restart")
        {
            // ignore restart command
        }
        else
        {
            //received unrecognized state change. sleep a while to avoid continuous looping
            sleepMilliSeconds(1000);
        }

        if (newstate.compare(0, strlen("progress:"), "progress:") != 0)
        {
            shown_partial_progress = false;
        }
    }
}



int main(int argc, char* argv[])
{
#ifdef _WIN32
    setlocale(LC_ALL, "en-US");

    bool redirectedoutput = false;

    //Redirect stdout to a file
    for (int i=1;i<argc;i++)
    {
        if (i<(argc-1) && !strcmp(argv[i],"-o"))
        {
            freopen(argv[i+1],"w",stdout);
            redirectedoutput = true;
        }
    }

#endif
    if (argc < 2)
    {
        cerr << "Too few arguments" << endl;
        return -1;
    }
#ifdef _WIN32
    MegaCmdShellCommunications *comms = new MegaCmdShellCommunicationsNamedPipes(redirectedoutput);
#else
    MegaCmdShellCommunications *comms = new MegaCmdShellCommunications();
#endif

    comms->registerForStateChanges(statechangehandle);

#ifdef _WIN32
    int wargc;
    LPWSTR *szArglist = CommandLineToArgvW(GetCommandLineW(),&wargc);
    wstring wcommand = parsewArgs(wargc,szArglist);
    int outcode = comms->executeCommandW(wcommand, readresponse, COUT, false);
#else
    string parsedArgs = parseArgs(argc,argv);
    int outcode = comms->executeCommand(parsedArgs, readresponse, COUT, false);
#endif

    delete comms;

    // do always return positive error codes (POSIX compliant)
    if (outcode < 0)
    {
        outcode = - outcode;
    }

    return outcode;
}
