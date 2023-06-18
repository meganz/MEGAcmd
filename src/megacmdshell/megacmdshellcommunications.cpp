/**
 * @file src/megacmdshellcommunications.cpp
 * @brief MEGAcmd: Communications module to connect to server
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
 *
 * This file is also distributed under the terms of the GNU General
 * Public License, see http://www.gnu.org/copyleft/gpl.txt for details.
 */

#include "megacmdshellcommunications.h"

#include <iostream>
#include <sstream>
#include <string.h>

#include <assert.h>

#ifdef _WIN32
#include <shlobj.h> //SHGetFolderPath
#include <Shlwapi.h> //PathAppend

#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#ifndef _O_U16TEXT
#define _O_U16TEXT 0x00020000
#endif
#ifndef _O_U8TEXT
#define _O_U8TEXT 0x00040000
#endif

#else
#include <fcntl.h>

#include <sys/stat.h>

#include <pwd.h>  //getpwuid_r
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <limits.h>
#endif

#if defined(__FreeBSD__) || defined(__OpenBSD__)
#include <netinet/in.h>
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#ifndef ENOTCONN
#define ENOTCONN 107
#endif

#ifndef SSTR
    #define SSTR( x ) static_cast< const std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()
#endif

namespace megacmd {
using namespace std;

bool MegaCmdShellCommunications::serverinitiatedfromshell;
bool MegaCmdShellCommunications::registerAgainRequired;
bool MegaCmdShellCommunications::confirmResponse;
bool MegaCmdShellCommunications::stopListener;
bool MegaCmdShellCommunications::updating;
MegaThread *MegaCmdShellCommunications::listenerThread;

//state changes socket:
SOCKET MegaCmdShellCommunications::newsockfd = INVALID_SOCKET;

std::mutex MegaCmdShellCommunications::megaCmdStdoutputing;

bool MegaCmdShellCommunications::socketValid(SOCKET socket)
{
#ifdef _WIN32
    return socket != INVALID_SOCKET;
#else
    return socket >= 0;
#endif
}

void MegaCmdShellCommunications::closeSocket(SOCKET socket){
#ifdef _WIN32
    closesocket(socket);
#else
    close(socket);
#endif
}

string createAndRetrieveConfigFolder()
{
    string configFolder;

#ifdef _WIN32

   TCHAR szPath[MAX_PATH];
    if (!SUCCEEDED(GetModuleFileName(NULL, szPath , MAX_PATH)))
    {
        cerr << "Couldnt get EXECUTABLE folder" << endl;
    }
    else
    {
        if (SUCCEEDED(PathRemoveFileSpec(szPath)))
        {
            if (PathAppend(szPath,TEXT(".megaCmd")))
            {
                utf16ToUtf8(szPath, lstrlen(szPath), &configFolder);
            }
        }
    }
    //TODO: create folder (not required currently)
#else
    const char *homedir = NULL;

    homedir = getenv("HOME");
    if (!homedir)
    {
        struct passwd pd;
        struct passwd* pwdptr = &pd;
        struct passwd* tempPwdPtr;
        char pwdbuffer[200];
        int pwdlinelen = sizeof( pwdbuffer );

        if (( getpwuid_r(22, pwdptr, pwdbuffer, pwdlinelen, &tempPwdPtr)) != 0)
        {
            cerr << "Couldnt get HOME folder" << endl;
            return "/tmp";
        }
        else
        {
            homedir = pwdptr->pw_dir;
        }
    }
    stringstream sconfigDir;
    sconfigDir << homedir << "/" << ".megaCmd";
    configFolder = sconfigDir.str();


    struct stat st;
    if (stat(configFolder.c_str(), &st) == -1) {
        mkdir(configFolder.c_str(), 0700);
    }

#endif

    return configFolder;
}


#ifndef _WIN32
#include <sys/wait.h>
bool is_pid_running(pid_t pid) {

    while(waitpid(-1, 0, WNOHANG) > 0) {
        // Wait for defunct....
    }

    if (0 == kill(pid, 0))
        return 1; // Process exists

    return 0;
}
#endif

SOCKET MegaCmdShellCommunications::createSocket(int number, bool initializeserver, bool net)
{
    if (net)
    {
        SOCKET thesock = socket(AF_INET, SOCK_STREAM, 0);
        if (!socketValid(thesock))
        {
            cerr << "ERROR opening socket: " << ERRNO << endl;
            return INVALID_SOCKET;
        }
#ifndef _WIN32
        if (fcntl(thesock, F_SETFD, FD_CLOEXEC) == -1)
        {
            cerr << "ERROR setting CLOEXEC to socket: " << errno << endl;
        }
#endif
        int portno=MEGACMDINITIALPORTNUMBER+number;

        struct sockaddr_in addr;

        memset(&addr, 0, sizeof( addr ));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
        addr.sin_port = (unsigned short)htons((unsigned short)portno);

        if (::connect(thesock, (struct sockaddr*)&addr, sizeof( addr )) == SOCKET_ERROR)
        {
            if (!number && initializeserver)
            {
                //launch server
                cerr << "[MEGAcmd Server not running. Initiating in the background]"<< endl;
#ifdef _WIN32
                STARTUPINFO si;
                PROCESS_INFORMATION pi;
                ZeroMemory( &si, sizeof(si) );
                ZeroMemory( &pi, sizeof(pi) );

#ifndef NDEBUG
                LPCWSTR t = TEXT("..\\MEGAcmdServer\\debug\\MEGAcmdServer.exe");
                if (true)
                {
#else

                wchar_t foldercontainingexec[MAX_PATH+1];
                bool okgetcontaningfolder = false;
                if (S_OK != SHGetFolderPathW(NULL,CSIDL_LOCAL_APPDATA,NULL,0,(LPWSTR)foldercontainingexec))
                {
                    if(S_OK != SHGetFolderPathW(NULL,CSIDL_COMMON_APPDATA,NULL,0,(LPWSTR)foldercontainingexec))
                    {
                        cerr << " Could not get LOCAL nor COMMON App Folder : " << ERRNO << endl;
                    }
                    else
                    {
                        okgetcontaningfolder = true;
                    }
                }
                else
                {
                    okgetcontaningfolder = true;
                }

                if (okgetcontaningfolder)
                {
                    wstring fullpathtoexec(foldercontainingexec);
                    fullpathtoexec+=L"\\MEGAcmd\\MEGAcmdServer.exe";

                    LPCWSTR t = fullpathtoexec.c_str();
#endif

                    LPWSTR t2 = (LPWSTR) t;
                    si.cb = sizeof(si);
                    if (!CreateProcess( t,t2,NULL,NULL,TRUE,
                                        CREATE_NEW_CONSOLE,
                                        NULL,NULL,
                                        &si,&pi) )
                    {
                        COUT << "Unable to execute: " << t << " errno = : " << ERRNO << endl;
                    }
                    Sleep(2000); // Give it a initial while to start.
                }

                //try again:
                int attempts = 0; //TODO: if >0, connect will cause a SOCKET_ERROR in first recv in the server (not happening in the next petition)
                int waitimet = 1500;
                while ( attempts && ::connect(thesock, (struct sockaddr*)&addr, sizeof( addr )) == SOCKET_ERROR)
                {
                    Sleep(waitimet/1000);
                    waitimet=waitimet*2;
                    attempts--;
                }
                if (attempts < 0) //TODO: check this whenever attempts is > 0
                {
                    cerr << "Unable to connect to " << (number?("response socket N "+SSTR(number)):"MEGAcmd server") << ": error=" << ERRNO << endl;
#ifdef __linux__
                    cerr << "Please ensure mega-cmd-server is running" << endl;
#else
                    cerr << "Please ensure MEGAcmdServer is running" << endl;
#endif
                    return INVALID_SOCKET;
                }
                else
                {
                    serverinitiatedfromshell = true;
                    registerAgainRequired = true;
                }
#else
                //TODO: implement linux part (see !net option)
#endif
            }
            return INVALID_SOCKET;
        }
        return thesock;
    }

#ifndef _WIN32
    else
    {
        SOCKET thesock = socket(AF_UNIX, SOCK_STREAM, 0);
        char socket_path[60];
        if (!socketValid(thesock))
        {
            cerr << "ERROR opening socket: " << ERRNO << endl;
            return INVALID_SOCKET;
        }
        if (fcntl(thesock, F_SETFD, FD_CLOEXEC) == -1)
        {
            cerr << "ERROR setting CLOEXEC to socket: " << errno << endl;
        }

        bzero(socket_path, sizeof( socket_path ) * sizeof( *socket_path ));
        sprintf(socket_path, "/tmp/megaCMD_%d/srv", getuid() );

        struct sockaddr_un addr;

        memset(&addr, 0, sizeof( addr ));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, socket_path, sizeof( addr.sun_path ) - 1);


        if (::connect(thesock, (struct sockaddr*)&addr, sizeof( addr )) == SOCKET_ERROR)
        {
            if (!number && initializeserver)
            {
                //launch server
                int forkret = fork();
//                if (forkret) //-> child is megacmdshell (debug megacmd server)
                if (!forkret) //-> child is server. (debug megacmdshell)
                {
                    signal(SIGINT, SIG_IGN); //ignore Ctrl+C in the server
                    setsid(); //create new session so as not to receive parent's Ctrl+C

                    string pathtolog = createAndRetrieveConfigFolder()+"/megacmdserver.log";
                    CERR << "[Initiating MEGAcmd server in background. Log: " << pathtolog << "]" << endl; //TODO: try this in windows with non unicode user name?

                    freopen(pathtolog.c_str(),"w",stdout);
                    dup2(fileno(stdout), fileno(stderr));  //redirects stderr to stdout below this line.

#ifndef NDEBUG

#ifdef __MACH__
                    const char executable[] = "../../../../MEGAcmdServer/MEGAcmd.app/Contents/MacOS/MEGAcmd";
#else
                    const char executable[] = "../MEGAcmdServer/MEGAcmd";
#endif
                    const char executable2[] = "./mega-cmd-server";

#else
    #ifdef __MACH__
                    const char executable[] = "/Applications/MEGAcmd.app/Contents/MacOS/MEGAcmdLoader";
                    const char executable2[] = "./MEGAcmdLoader";
    #else
                    const char executable[] = "mega-cmd-server";
        #ifdef __linux__
                    char executable2[PATH_MAX];
                    sprintf(executable2, "%s/mega-cmd-server", getCurrentExecPath().c_str());
        #else
                    const char executable2[] = "./mega-cmd-server";
        #endif
    #endif
#endif

                    char **args = new char*[2];
                    args[0]=(char *)executable;
                    args[1] = NULL;

                    int ret = execvp(executable,args);

                    if (ret && errno == 2 )
                    {
                        cerr << "Couln't initiate MEGAcmd server: executable not found: " << executable << endl;
                        cerr << "Trying to use alternative executable: " << executable2 << endl;
                        args[0]=(char *)executable2;
                        ret = execvp(executable2,args);
                        if (ret && errno == 2 )
                        {
                            cerr << "Couln't initiate MEGAcmd server: executable not found: " << executable2 << endl;
                        }
                    }

                    if (ret && errno !=2 )
                    {
                        cerr << "MEGAcmd server exit with code " << ret << " . errno = " << errno << endl;
                    }
                    exit(0);
                }


                //try again:
                int attempts = 12;
#ifdef __MACH__
                int waitimet = 15000; // Give a longer while for the user to insert password to unblock fsevents. TODO: this should only be required the first time using megacmd
#else
                int waitimet = 1500;
                static int relaunchnumber = 1;
                waitimet=waitimet*(relaunchnumber++);
#endif

                usleep(waitimet*100);
                while ( ::connect(thesock, (struct sockaddr*)&addr, sizeof( addr )) == SOCKET_ERROR && attempts--)
                {
                    usleep(waitimet);
                    waitimet=waitimet*2;
                }
                if (attempts<0)
                {

                    cerr << "Unable to connect to " << (number?("response socket N "+SSTR(number)):"service") << ": error=" << ERRNO << endl;
#ifdef __linux__
                    cerr << "Please ensure mega-cmd-server is running" << endl;
#else
                    cerr << "Please ensure MEGAcmdServer is running" << endl;
#endif
                    return INVALID_SOCKET;
                }
                else
                {
                    if (forkret && is_pid_running(forkret)) // server pid is alive (most likely because I initiated the server)
                    {
                        serverinitiatedfromshell = true;
                    }
                    registerAgainRequired = true;
                }
            }
            else
            {
#ifdef ECONNREFUSED
                if (!initializeserver && ERRNO == ECONNREFUSED)
                {
                    cerr << "MEGAcmd Server is not responding" << endl;
                }
                else
#endif
                {
                    cerr << "Unable to connect to socket  " << number <<  " : " << ERRNO << endl;
                }
                return INVALID_SOCKET;
            }
        }

        return thesock;
    }
#endif
    return INVALID_SOCKET;
}

MegaCmdShellCommunications::MegaCmdShellCommunications()
{
#ifdef _WIN32
    setlocale(LC_ALL, "en-US");
#endif


#if _WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    /* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        cerr << "ERROR initializing WSA" << endl;
    }
#endif

    serverinitiatedfromshell = false;
    registerAgainRequired = false;

    stopListener = false;
    updating = false;
    listenerThread = NULL;
}


#ifdef _WIN32
std::string to_utf8(uint32_t cp) //TODO: move this to a common place
{
//    // c++11
//    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
//    return conv.to_bytes( (char32_t)cp );

    std::string result;

    int count;
    if (cp < 0x0080)
        count = 1;
    else if (cp < 0x0800)
        count = 2;
    else if (cp < 0x10000)
        count = 3;
    else if (cp <= 0x10FFFF)
        count = 4;
    else
        return result; // or throw an exception

    result.resize(count);

    for (int i = count-1; i > 0; --i)
    {
        result[i] = (char) (0x80 | (cp & 0x3F));
        cp >>= 6;
    }

    for (int i = 0; i < count; ++i)
        cp |= (1 << (7-i));

    result[0] = (char) cp;

    return result;
}

string unescapeutf16escapedseqs(const char *what)
{
    //    string toret;
    //    size_t len = strlen(what);
    //    for (int i=0;i<len;)
    //    {
    //        if (i<(len-5) && what[i]=='\\' && what[i+1]=='u')
    //        {
    //            toret+="?"; //TODO: translate \uXXXX to utf8 char *
    //            // TODO: ideally, if first \uXXXX between [D800,DBFF] and there is a second between [DC00,DFFF] -> that's only one gliph
    //            i+=6;
    //        }
    //        else
    //        {
    //            toret+=what[i];
    //            i++;
    //        }
    //    }
    //    return toret;

    std::string str = what;
    std::string::size_type startIdx = 0;
    do
    {
        startIdx = str.find("\\u", startIdx);
        if (startIdx == std::string::npos) break;

        std::string::size_type endIdx = str.find_first_not_of("0123456789abcdefABCDEF", startIdx+2);
        if (endIdx == std::string::npos) break;

        std::string tmpStr = str.substr(startIdx+2, endIdx-(startIdx+2));
        std::istringstream iss(tmpStr);

        uint32_t cp;
        if (iss >> std::hex >> cp)
        {
            std::string utf8 = to_utf8(cp);
            str.replace(startIdx, 2+tmpStr.length(), utf8);
            startIdx += utf8.length();
        }
        else
            startIdx += 2;
    }
    while (true);

    return str;
}

#endif


int MegaCmdShellCommunications::executeCommandW(wstring wcommand, std::string (*readresponse)(const char *), OUTSTREAMTYPE &output, bool interactiveshell)
{
    return executeCommand("", readresponse, output, interactiveshell, wcommand);
}

int MegaCmdShellCommunications::executeCommand(string command, std::string (*readresponse)(const char *), OUTSTREAMTYPE &output, bool interactiveshell, wstring wcommand)
{
    SOCKET thesock = createSocket(0, command.compare(0,4,"exit") && command.compare(0,4,"quit") && command.compare(0,10,"completion"));
    if (!socketValid(thesock))
    {
        return -1;
    }

    if (interactiveshell)
    {
        command="X"+command;
    }

#ifdef _WIN32
//    //unescape \uXXXX sequences
//    command=unescapeutf16escapedseqs(command.c_str());

    //get local wide chars string (utf8 -> utf16)
    if (!wcommand.size())
    {
        stringtolocalw(command.c_str(),&wcommand);
    }
    else if (interactiveshell)
    {
        wcommand=L"X"+wcommand;
    }
    int n = send(thesock,(char *)wcommand.data(),int(wcslen(wcommand.c_str())*sizeof(wchar_t)), MSG_NOSIGNAL);

#else
    auto n = send(thesock,command.data(),command.size(), MSG_NOSIGNAL);
#endif
    if (n == SOCKET_ERROR)
    {
        if ( (!command.compare(0,5,"Xexit") || !command.compare(0,5,"Xquit") ) && (ERRNO == ENOTCONN) )
        {
             cerr << "Could not send exit command to MEGAcmd server (probably already down)" << endl;
        }
        else
        {
            cerr << "ERROR writing command to socket: " << ERRNO << endl;
        }
        return -1;
    }

    int outcode = -1;

    n = recv(thesock, (char *)&outcode, sizeof(outcode), MSG_NOSIGNAL);
    if (n == SOCKET_ERROR)
    {
        cerr << "ERROR reading output code: " << ERRNO << endl;
        return -1;
    }

    while (outcode == MCMD_REQCONFIRM || outcode == MCMD_REQSTRING || outcode == MCMD_PARTIALOUT)
    {
        if (outcode == MCMD_PARTIALOUT)
        {
            size_t partialoutsize;

            n = recv(thesock, (char *)&partialoutsize, sizeof(partialoutsize), MSG_NOSIGNAL);
            if (n && partialoutsize > 0)
            {
                megaCmdStdoutputing.lock();

                do{
                    char *buffer = new char[partialoutsize+1];
#ifdef _WIN32
                    n = recv(thesock, (char *)buffer, (int)partialoutsize, MSG_NOSIGNAL);
#else
                    n = recv(thesock, (char *)buffer, partialoutsize, MSG_NOSIGNAL);
#endif

                    if (n)
                    {
#ifdef _WIN32
                        assert(0 && "This is deprecated/untested. Using named pipes required!");
                        buffer[n]='\0';

                        wstring wbuffer;
                        stringtolocalw((const char*)&buffer,&wbuffer);
                        int oldmode = _setmode(_fileno(stdout), _O_U16TEXT);
                        output << wbuffer << flush;
                        _setmode(_fileno(stdout), oldmode);
#else
                        output << string(buffer,partialoutsize) << flush;
#endif
                        partialoutsize-=n;
                    }
                    delete[] buffer;
                } while(n != 0 && partialoutsize && n !=SOCKET_ERROR);
                megaCmdStdoutputing.unlock();
            }
            else
            {
                std::cerr << "Error reading size of partial output: " << ERRNO << std::endl;
                return -1;
            }
        }
        else { //REQCONFIRM|REQSTRING
            size_t BUFFERSIZE = 1024;
            string confirmQuestion;
            char buffer[1025];
            do{
#ifdef _WIN32
                n = recv(thesock, buffer, (int)BUFFERSIZE, MSG_NOSIGNAL);
#else
                n = recv(thesock, buffer, BUFFERSIZE, MSG_NOSIGNAL);
#endif
                if (n)
                {
                    buffer[n]='\0';
                    confirmQuestion.append(buffer);
                }
            } while(n == BUFFERSIZE && n != SOCKET_ERROR);

            if (outcode == MCMD_REQCONFIRM)
            {
                int response = MCMDCONFIRM_NO;

                if (readresponse != NULL)
                {
                    response = readconfirmationloop(confirmQuestion.c_str(), readresponse);
                }

                n = send(thesock, (const char *) &response, sizeof(response), MSG_NOSIGNAL);
            }
            else // MCMD_REQSTRING
            {
                string response = "FAILED";

                if (readresponse != NULL)
                {
                    response = readresponse(confirmQuestion.c_str());
                }

                n = send(thesock, (const char *) response.data(), sizeof(response), MSG_NOSIGNAL);
            }
            if (n == SOCKET_ERROR)
            {
                cerr << "ERROR writing confirm response to socket: " << ERRNO << endl;
                return -1;
            }

        }

        n = recv(thesock, (char *)&outcode, sizeof(outcode), MSG_NOSIGNAL);
        if (n == SOCKET_ERROR)
        {
            cerr << "ERROR reading output code: " << ERRNO << endl;
            return -1;
        }
    }

    int BUFFERSIZE = 1024;
    char buffer[1025];
    do{
        n = recv(thesock, buffer, BUFFERSIZE, MSG_NOSIGNAL);
        if (n)
        {
            megaCmdStdoutputing.lock();

#ifdef _WIN32
            buffer[n]='\0';

            wstring wbuffer;
            stringtolocalw((const char*)&buffer,&wbuffer);
            int oldmode = _setmode(_fileno(stdout), _O_U16TEXT);
            output << wbuffer;
            _setmode(_fileno(stdout), oldmode);
#else
            if (n != 1 || buffer[0] != 0) //To avoid outputing 0 char in binary outputs
            {
                output << string(buffer,n) << flush;
            }
#endif
            megaCmdStdoutputing.unlock();
        }
    } while(n != 0 && n !=SOCKET_ERROR);

    if (n == SOCKET_ERROR)
    {
        cerr << "ERROR reading output: " << ERRNO << endl;
        return -1;
    }

    closeSocket(thesock);
    return outcode;
}


void *MegaCmdShellCommunications::listenToStateChangesEntry(void *slsc)
{
    listenToStateChanges(((sListenStateChanges *)slsc)->receiveSocket,((sListenStateChanges *)slsc)->statechangehandle);
    delete ((sListenStateChanges *)slsc);
    return NULL;
}

int MegaCmdShellCommunications::listenToStateChanges(int receiveSocket, void (*statechangehandle)(string))
{
    if (!socketValid(receiveSocket))
    {
        return -1;
    }

    int timeout_notified_server_might_be_down = 0;
    while (!stopListener)
    {
        string newstate;

        int BUFFERSIZE = 1024;
        char buffer[1025];
        int n = SOCKET_ERROR;
        do{
            n = recv(receiveSocket, buffer, BUFFERSIZE, MSG_NOSIGNAL);
            if (n)
            {
                buffer[n]='\0';
                newstate += buffer;
            }
        } while(n == BUFFERSIZE && n !=SOCKET_ERROR);

        if (n == SOCKET_ERROR)
        {
            cerr << "ERROR reading state from MEGAcmd server: " << ERRNO << endl;
            closeSocket(receiveSocket);
            return -1;
        }

        if (!n)
        {
            if (!timeout_notified_server_might_be_down)
            {
                timeout_notified_server_might_be_down = 30;
                if (!stopListener && !updating)
                {
                    cerr << endl << "[mega-cmd-server is probably down. Type to respawn or reconnect to it]" << endl;
                }
                else
                {
                    closeSocket(receiveSocket);
                    return 0;
                }
            }
            timeout_notified_server_might_be_down--;
            if (!timeout_notified_server_might_be_down)
            {
                registerAgainRequired = true;
                closeSocket(receiveSocket);
                return -1;
            }
#ifdef _WIN32
            Sleep(1000);
#else
            sleep(1);
#endif
            continue;
        }

        if (statechangehandle != NULL)
        {
            statechangehandle(newstate);
        }
    }

    closeSocket(receiveSocket);
    return 0;
}

int MegaCmdShellCommunications::readconfirmationloop(const char *question, string (*readresponse)(const char *))
{
    bool firstime = true;
    for (;; )
    {
        string response;

        if (firstime)
        {
            response = readresponse(question);
        }
        else
        {
            response = readresponse("Please enter [y]es/[n]o/[a]ll/none:");
        }

        firstime = false;

        if (response == "yes" || response == "y" || response == "YES" || response == "Y")
        {
            return MCMDCONFIRM_YES;
        }
        if (response == "no" || response == "n" || response == "NO" || response == "N")
        {
            return MCMDCONFIRM_NO;
        }
        if (response == "All" || response == "ALL" || response == "a" || response == "A" || response == "all")
        {
            return MCMDCONFIRM_ALL;
        }
        if (response == "none" || response == "NONE" || response == "None")
        {
            return MCMDCONFIRM_NONE;
        }
    }

}

int MegaCmdShellCommunications::registerForStateChanges(bool interactive, void (*statechangehandle)(string), bool initiateServer)
{
    if (statechangehandle == NULL)
    {
        cerr << "Not registering for state changes since statechangehandle is NULL" << endl; //TODO: delete
        registerAgainRequired = false;
        return 0; //Do nth
    }
    SOCKET thesock = createSocket(0, initiateServer);

    if (thesock == INVALID_SOCKET)
    {
        cerr << "Failed to create socket for registering for state changes" << endl;
        registerAgainRequired = true;
        return -1;
    }

#ifdef _WIN32
    wstring wcommand=interactive?L"Xregisterstatelistener":L"registerstatelistener";
    int n = send(thesock,(char*)wcommand.data(),int(wcslen(wcommand.c_str())*sizeof(wchar_t)), MSG_NOSIGNAL);
#else
    string command=interactive?"Xregisterstatelistener":"registerstatelistener";

    auto n = send(thesock,command.data(),command.size(), MSG_NOSIGNAL);
#endif

    if (n == SOCKET_ERROR)
    {
        cerr << "ERROR writing output Code to socket: " << ERRNO << endl;
        registerAgainRequired = true;
        closeSocket(thesock);
        return -1;
    }

    if (listenerThread != NULL)
    {
        stopListener = true;
        listenerThread->join();
    }

    stopListener = false;

    sListenStateChanges * slsc = new sListenStateChanges();
#ifdef _WIN32
    slsc->receiveSocket = (int)thesock;
#else
    slsc->receiveSocket = thesock;
#endif
    slsc->statechangehandle = statechangehandle;

    //store the socket to close connections when closing
    newsockfd = thesock;

    listenerThread = new MegaThread();
    listenerThread->start(listenToStateChangesEntry,slsc);

    registerAgainRequired = false;
    return 0;
}

void MegaCmdShellCommunications::setResponseConfirmation(bool confirmation)
{
    confirmResponse = confirmation;
}

MegaCmdShellCommunications::~MegaCmdShellCommunications()
{
#if _WIN32
    WSACleanup();
#endif

    if (listenerThread != NULL)
    {
        stopListener = true;
#ifdef _WIN32
    shutdown(newsockfd,SD_BOTH);
#else
    shutdown(newsockfd,SHUT_RDWR);
#endif
        listenerThread->join();
    }
    delete (MegaThread *)listenerThread;
}
} //end namespace
