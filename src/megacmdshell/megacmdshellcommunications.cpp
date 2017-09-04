/**
 * @file examples/megacmd/megacmdshellcommunications.cpp
 * @brief MEGAcmd: Communications module to connect to server
 *
 * (c) 2013-2017 by Mega Limited, Auckland, New Zealand
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
 *
 * This file is also distributed under the terms of the GNU General
 * Public License, see http://www.gnu.org/copyleft/gpl.txt for details.
 */

#include "megacmdshellcommunications.h"

#include <iostream>
#include <sstream>

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
#endif

#ifdef __FreeBSD__
#include <netinet/in.h>
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#ifndef ENOTCONN
#define ENOTCONN 107
#endif

#ifndef SSTR
    #define SSTR( x ) static_cast< std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()
#endif

#if defined(_WIN32) && !defined(WINDOWS_PHONE)
#include "mega/thread/win32thread.h"
class MegaThread : public mega::Win32Thread {};
#elif defined(USE_CPPTHREAD)
#include "mega/thread/cppthread.h"
class MegaThread : public mega::CppThread {};
#else
#include "mega/thread/posixthread.h"
class MegaThread : public mega::PosixThread {};
#endif


using namespace std;

enum
{
    MCMD_OK = 0,              ///< Everything OK

    MCMD_EARGS = -51,         ///< Wrong arguments
    MCMD_INVALIDEMAIL = -52,  ///< Invalid email
    MCMD_NOTFOUND = -53,      ///< Resource not found
    MCMD_INVALIDSTATE = -54,  ///< Invalid state
    MCMD_INVALIDTYPE = -55,   ///< Invalid type
    MCMD_NOTPERMITTED = -56,  ///< Operation not allowed
    MCMD_NOTLOGGEDIN = -57,   ///< Needs loging in
    MCMD_NOFETCH = -58,       ///< Nodes not fetched
    MCMD_EUNEXPECTED = -59,   ///< Unexpected failure

    MCMD_REQCONFIRM = -60,     ///< Confirmation required

};


bool MegaCmdShellCommunications::serverinitiatedfromshell;
bool MegaCmdShellCommunications::registerAgainRequired;
bool MegaCmdShellCommunications::confirmResponse;
bool MegaCmdShellCommunications::stopListener;
::mega::Thread *MegaCmdShellCommunications::listenerThread;
int MegaCmdShellCommunications::newsockfd;

#ifdef _WIN32
// UNICODE SUPPORT FOR WINDOWS

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

//override for the log. This is required for compiling, otherwise SimpleLog won't compile. FIXME
std::ostringstream & operator<< ( std::ostringstream & ostr, std::wstring const &str)
{
    std::string s;
    localwtostring(&str,&s);
    ostr << s;
    return ( ostr );
}

// convert Windows Unicode to UTF-8
void utf16ToUtf8(const wchar_t* utf16data, int utf16size, string* utf8string)
{
    if(!utf16size)
    {
        utf8string->clear();
        return;
    }

    utf8string->resize((utf16size + 1) * 4);

    utf8string->resize(WideCharToMultiByte(CP_UTF8, 0, utf16data,
        utf16size,
        (char*)utf8string->data(),
        utf8string->size() + 1,
        NULL, NULL));
}
#endif

bool MegaCmdShellCommunications::socketValid(int socket)
{
#ifdef _WIN32
    return socket != INVALID_SOCKET;
#else
    return socket >= 0;
#endif
}

void MegaCmdShellCommunications::closeSocket(int socket){
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

int MegaCmdShellCommunications::createSocket(int number, bool initializeserver, bool net)
{
    if (net)
    {
        int thesock = socket(AF_INET, SOCK_STREAM, 0);
        if (!socketValid(thesock))
        {
            cerr << "ERROR opening socket: " << ERRNO << endl;
            return INVALID_SOCKET;
        }
        int portno=MEGACMDINITIALPORTNUMBER+number;

        struct sockaddr_in addr;

        memset(&addr, 0, sizeof( addr ));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
        addr.sin_port = htons(portno);

        if (::connect(thesock, (struct sockaddr*)&addr, sizeof( addr )) == SOCKET_ERROR)
        {
            if (!number && initializeserver)
            {
                //launch server
                cerr << "[Server not running. Initiating in the background]"<< endl;
#ifdef _WIN32
                STARTUPINFO si;
                PROCESS_INFORMATION pi;
                ZeroMemory( &si, sizeof(si) );
                ZeroMemory( &pi, sizeof(pi) );

#ifndef NDEBUG
                LPCWSTR t = TEXT("..\\MEGAcmdServer\\debug\\MEGAcmd.exe");
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
                    fullpathtoexec+=L"\\MEGAcmd\\MEGAcmd.exe";

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
                    cerr << "Unable to connect to " << (number?("response socket N "+SSTR(number)):"server") << ": error=" << ERRNO << endl;
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
        int thesock = socket(AF_UNIX, SOCK_STREAM, 0);
        char socket_path[60];
        if (!socketValid(thesock))
        {
            cerr << "ERROR opening socket: " << ERRNO << endl;
            return INVALID_SOCKET;
        }

        bzero(socket_path, sizeof( socket_path ) * sizeof( *socket_path ));
        if (number)
        {
            sprintf(socket_path, "/tmp/megaCMD_%d/srv_%d", getuid(), number);
        }
        else
        {
            sprintf(socket_path, "/tmp/megaCMD_%d/srv", getuid() );
        }

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
                    OUTSTREAM << "[Initiating server in background. Log: " << pathtolog << "]" << endl;

                    dup2(fileno(stdout), fileno(stderr));  //redirects stderr to stdout below this line.
                    freopen(pathtolog.c_str(),"w",stdout);

#ifndef NDEBUG

#ifdef __MACH__
                    const char executable[] = "../../../../MEGAcmdServer/MEGAcmd.app/Contents/MacOS/MEGAcmd";
#else
                const char executable[] = "../MEGAcmdServer/MEGAcmd";
#endif

#else
    #ifdef __MACH__
                    const char executable[] = "/Applications/MEGAcmd.app/Contents/MacOS/MEGAcmdLoader";
    #else
                    const char executable[] = "mega-cmd-server";
    #endif
#endif
                    char * args[] = {NULL};

                    int ret = execvp(executable,args);
                    if (ret)
                    {
                        if (errno == 2 )
                        {
                            cerr << "Couln't initiate MEGAcmd server: executable not found: " << executable << endl;

                        }
                        else
                        {
                            cerr << "MEGAcmd server exit with code " << ret << " . errno = " << errno << endl;
                        }
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
                cerr << "Unable to connect to socket  " << number <<  " : " << ERRNO << endl;
                return INVALID_SOCKET;
            }
        }

        return thesock;
    }
    return INVALID_SOCKET;

#endif
    return INVALID_SOCKET;
}

MegaCmdShellCommunications::MegaCmdShellCommunications()
{
#ifdef _WIN32
    setlocale(LC_ALL, ""); // en_US.utf8 could do?
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


int MegaCmdShellCommunications::executeCommandW(wstring wcommand, bool (*readconfirmationloop)(const char *), OUTSTREAMTYPE &output, bool interactiveshell)
{
    return executeCommand("", readconfirmationloop, output, interactiveshell, wcommand);
}

int MegaCmdShellCommunications::executeCommand(string command, bool (*readconfirmationloop)(const char *), OUTSTREAMTYPE &output, bool interactiveshell, wstring wcommand)
{
    int thesock = createSocket(0, command.compare(0,4,"exit") && command.compare(0,4,"quit"));
    if (thesock == INVALID_SOCKET)
    {
        return INVALID_SOCKET;
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
    int n = send(thesock,(char *)wcommand.data(),wcslen(wcommand.c_str())*sizeof(wchar_t), MSG_NOSIGNAL);

#else
    int n = send(thesock,command.data(),command.size(), MSG_NOSIGNAL);
#endif
    if (n == SOCKET_ERROR)
    {
        if ( (!command.compare(0,5,"Xexit") || !command.compare(0,5,"Xquit") ) && (ERRNO == ENOTCONN) )
        {
             cerr << "Could not send exit command to server (probably already down)" << endl;
        }
        else
        {
            cerr << "ERROR writing command to socket: " << ERRNO << endl;
        }
        return -1;
    }

    int receiveSocket = SOCKET_ERROR ;

    n = recv(thesock, (char *)&receiveSocket, sizeof(receiveSocket), MSG_NOSIGNAL);
    if (n == SOCKET_ERROR)
    {
        cerr << "ERROR reading output socket" << endl;
        return -1;
    }

    int newsockfd = createSocket(receiveSocket);
    if (newsockfd == INVALID_SOCKET)
        return INVALID_SOCKET;

    int outcode = -1;

    n = recv(newsockfd, (char *)&outcode, sizeof(outcode), MSG_NOSIGNAL);
    if (n == SOCKET_ERROR)
    {
        cerr << "ERROR reading output code: " << ERRNO << endl;
        return -1;
    }

    while (outcode == MCMD_REQCONFIRM)
    {
        int BUFFERSIZE = 1024;
        string confirmQuestion;
        char buffer[1025];
        do{
            n = recv(newsockfd, buffer, BUFFERSIZE, MSG_NOSIGNAL);
            if (n)
            {
                buffer[n]='\0';
                confirmQuestion.append(buffer);
            }
        } while(n == BUFFERSIZE && n !=SOCKET_ERROR);

        bool response = false;

        if (readconfirmationloop != NULL)
        {
            response = readconfirmationloop(confirmQuestion.c_str());
        }

        n = send(newsockfd, (const char *) &response, sizeof(response), MSG_NOSIGNAL);
        if (n == SOCKET_ERROR)
        {
            cerr << "ERROR writing confirm response to socket: " << ERRNO << endl;
            return -1;
        }

        n = recv(newsockfd, (char *)&outcode, sizeof(outcode), MSG_NOSIGNAL);
        if (n == SOCKET_ERROR)
        {
            cerr << "ERROR reading output code: " << ERRNO << endl;
            return -1;
        }
    }

    int BUFFERSIZE = 1024;
    char buffer[1025];
    do{
        n = recv(newsockfd, buffer, BUFFERSIZE, MSG_NOSIGNAL);
        if (n)
        {
#ifdef _WIN32
            buffer[n]='\0';

            wstring wbuffer;
            stringtolocalw((const char*)&buffer,&wbuffer);
            int oldmode = _setmode(_fileno(stdout), _O_U16TEXT);
            output << wbuffer;
            _setmode(_fileno(stdout), oldmode);
#else
            buffer[n]='\0';
            output << buffer;
#endif
        }
    } while(n == BUFFERSIZE && n !=SOCKET_ERROR);

    if (n == SOCKET_ERROR)
    {
        cerr << "ERROR reading output: " << ERRNO << endl;
        return -1;;
    }

    closeSocket(newsockfd);
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
    newsockfd = createSocket(receiveSocket);

    int timeout_notified_server_might_be_down = 0;
    while (!stopListener)
    {
        if (newsockfd == INVALID_SOCKET)
            return INVALID_SOCKET;

        string newstate;

        int BUFFERSIZE = 1024;
        char buffer[1025];
        int n = SOCKET_ERROR;
        do{
            n = recv(newsockfd, buffer, BUFFERSIZE, MSG_NOSIGNAL);
            if (n)
            {
                buffer[n]='\0';
                newstate += buffer;
            }
        } while(n == BUFFERSIZE && n !=SOCKET_ERROR);

        if (n == SOCKET_ERROR)
        {
            cerr << "ERROR reading state from server: " << ERRNO << endl;
            closeSocket(newsockfd);
            return -1;;
        }

        if (!n)
        {
            if (!timeout_notified_server_might_be_down)
            {
                timeout_notified_server_might_be_down = 30;
                if (!stopListener)
                {
                    cerr << endl << "[Server is probably down. Type to respawn or reconnect to it]" << endl;
                }
                else
                {
                    closeSocket(newsockfd);
                    return 0;
                }
            }
            timeout_notified_server_might_be_down--;
            if (!timeout_notified_server_might_be_down)
            {
                registerAgainRequired = true;
                closeSocket(newsockfd);
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

    closeSocket(newsockfd);
    return 0;
}

int MegaCmdShellCommunications::registerForStateChanges(void (*statechangehandle)(string))
{
    if (statechangehandle == NULL)
    {
        cerr << "Not registering for state changes since statechangehandle is NULL" << endl; //TODO: delete
        registerAgainRequired = false;
        return 0; //Do nth
    }
    int thesock = createSocket();
    if (thesock == INVALID_SOCKET)
    {
        cerr << "Failed to create socket for registering for state changes" << endl;
        registerAgainRequired = true;
        return INVALID_SOCKET;
    }

#ifdef _WIN32
    wstring wcommand=L"registerstatelistener";
    int n = send(thesock,(char*)wcommand.data(),wcslen(wcommand.c_str())*sizeof(wchar_t), MSG_NOSIGNAL);
#else
    string command="registerstatelistener";
    int n = send(thesock,command.data(),command.size(), MSG_NOSIGNAL);
#endif

    if (n == SOCKET_ERROR)
    {
        cerr << "ERROR writing output Code to socket: " << ERRNO << endl;
        registerAgainRequired = true;
        return -1;
    }

    int receiveSocket = SOCKET_ERROR ;

    n = recv(thesock, (char *)&receiveSocket, sizeof(receiveSocket), MSG_NOSIGNAL);
    if (n == SOCKET_ERROR)
    {
        cerr << "ERROR reading output socket" << endl;
        registerAgainRequired = true;
        return -1;
    }

    if (listenerThread != NULL)
    {
        stopListener = true;
        listenerThread->join();
    }

    stopListener = false;

    sListenStateChanges * slsc = new sListenStateChanges();
    slsc->receiveSocket = receiveSocket;
    slsc->statechangehandle = statechangehandle;
    listenerThread = new MegaThread();
    listenerThread->start(listenToStateChangesEntry,slsc);


    registerAgainRequired = false;

    closeSocket(thesock);
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
