/**
 * @file examples/megacmd/client/megacmdclient.cpp
 * @brief MEGAcmdClient: Client application of MEGAcmd
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

using namespace std;

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
                                  local->size() / sizeof(wchar_t) + 1);
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
                                     local->size() / sizeof(wchar_t),
                                     (char*)path->data(),
                                     path->size() + 1,
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

bool readconfirmationloop(const char *question)
{
    bool firstime = true;
    for (;; )
    {
        string response;

        if (firstime)
        {
            cout << question << " " << flush;
            getline(cin, response);
        }
        else
        {
            cout << "Please enter [y]es/[n]o:" << " " << flush;
            getline(cin, response);
        }

        firstime = false;

        if (response == "yes" || response == "y" || response == "YES" || response == "Y")
        {
            return true;
        }
        if (response == "no" || response == "n" || response == "NO" || response == "N")
        {
            return false;
        }
    }
}


int main(int argc, char* argv[])
{
#ifdef _WIN32
    setlocale(LC_ALL, ""); // en_US.utf8 could do?

    //Redirect stdout to a file
    for (int i=1;i<argc;i++)
    {
        if (i<(argc-1) && !strcmp(argv[i],"-o"))
        {
            freopen(argv[i+1],"w",stdout);
        }
    }

#endif
    if (argc < 2)
    {
        cerr << "Too few arguments" << endl;
        return -1;
    }
#ifdef _WIN32
    MegaCmdShellCommunications *comms = new MegaCmdShellCommunicationsNamedPipes();
#else
    MegaCmdShellCommunications *comms = new MegaCmdShellCommunications();
#endif
#ifdef _WIN32
    int wargc;
    LPWSTR *szArglist = CommandLineToArgvW(GetCommandLineW(),&wargc);
    wstring wcommand = parsewArgs(wargc,szArglist);
    int outcode = comms->executeCommandW(wcommand, readconfirmationloop, COUT, false);
#else
    string parsedArgs = parseArgs(argc,argv);
    int outcode = comms->executeCommand(parsedArgs, readconfirmationloop, COUT, false);
#endif

    delete comms;

    return outcode;
}
