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
#include "../megacmdcommonutils.h"

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

#ifdef __FreeBSD__
#include <netinet/in.h>
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#ifndef ENOTCONN
#define ENOTCONN 107
#endif

using namespace std;

namespace megacmd {

#ifndef _WIN32
string createAndRetrieveConfigFolder()
{
    auto dirs = PlatformDirectories::getPlatformSpecificDirectories();
    auto dir = dirs->configDirPath().string();

    struct stat st = {};
    if (stat(dir.c_str(), &st) == -1)
    {
        mkdir(dir.c_str(), 0700);
    }
    return dir;
}
#endif

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

#ifndef _WIN32
bool MegaCmdShellCommunicationsPosix::isSocketValid(SOCKET socket)
{
    return socket >= 0;
}

SOCKET MegaCmdShellCommunicationsPosix::createSocket(int number, bool initializeserver)
{
    SOCKET thesock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (!isSocketValid(thesock))
    {
        cerr << "ERROR opening socket: " << ERRNO << endl;
        return INVALID_SOCKET;
    }

    bool ok = false;
    ScopeGuard g([this, &thesock, &ok]()
    {
        if (!ok) //something failed
        {
            close(thesock);
        }
    });

    if (fcntl(thesock, F_SETFD, FD_CLOEXEC) == -1)
    {
        cerr << "ERROR setting CLOEXEC to socket: " << errno << endl;
    }
    std::string socketPath = getOrCreateSocketPath(false);
    if (socketPath.empty())
    {
        std::cerr << "Error creating runtime directory for socket file: " << strerror(errno) << std::endl;
        return INVALID_SOCKET;
    }
    struct sockaddr_un addr;

    memset(&addr, 0, sizeof( addr ));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketPath.c_str(), socketPath.size());

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

                // Give an indication of where the logs will be find:
                string pathtolog = createAndRetrieveConfigFolder()+"/megacmdserver.log";
                CERR << "[Initiating MEGAcmd server in background. Log: " << pathtolog << "]" << endl;

                freopen(std::string(pathtolog).append(".out").c_str(),"w",stdout);
                freopen(std::string(pathtolog).append(".err").c_str(),"w",stderr);

#ifndef NDEBUG
                const char executable[] = "./mega-cmd-server";
#else
    #ifdef __MACH__
                const char executable[] = "/Applications/MEGAcmd.app/Contents/MacOS/mega-cmd";
                const char executable2[] = "./mega-cmd";
    #else
                const char executable[] = "mega-cmd-server";
                char executable2[PATH_MAX];
                sprintf(executable2, "%s/mega-cmd-server", getCurrentExecPath().c_str());
    #endif
#endif

                std::vector<const char*> argsVector{
                    executable,
                    nullptr
                };

                auto args = const_cast<char* const*>(argsVector.data());
                int ret = execvp(executable, args);

                if (ret && errno == 2)
                {
                    cerr << "Couln't initiate MEGAcmd server: executable not found: " << executable << endl;

                #ifdef NDEBUG
                    cerr << "Trying to use alternative executable: " << executable2 << endl;

                    argsVector[0] = executable2;
                    ret = execvp(executable2, args);
                    if (ret && errno == 2)
                    {
                        cerr << "Couln't initiate MEGAcmd server: executable not found: " << executable2 << endl;
                    }
                #endif
                }

                if (ret && errno != 2)
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

            if (attempts < 0) //too many attempts
            {

                cerr << "Unable to connect to " << (number?("response socket N "+std::to_string(number)):"service") << ": error=" << ERRNO << endl;
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
                    mServerInitiatedFromShell = true;
                }

                setForRegisterAgain(true);
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
    ok = true;
    return thesock;
}
#endif


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

#ifndef _WIN32
int MegaCmdShellCommunicationsPosix::executeCommand(string command, std::string (*readresponse)(const char *), OUTSTREAMTYPE &output, bool interactiveshell, wstring /*wcommand*/)
{
    SOCKET thesock = createSocket(0, command.compare(0,4,"exit") && command.compare(0,4,"quit") && command.compare(0,10,"completion"));
    if (!isSocketValid(thesock))
    {
        return -1;
    }

    ScopeGuard g([this, &thesock]()
    {
        close(thesock);
    });

    if (interactiveshell)
    {
        command="X"+command;
    }

    auto n = send(thesock,command.data(),command.size(), MSG_NOSIGNAL);
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
                std::lock_guard<std::mutex> stdOutLockGuard(getStdoutLockGuard());
                do{
                    char *buffer = new char[partialoutsize+1];
                    n = recv(thesock, (char *)buffer, partialoutsize, MSG_NOSIGNAL);

                    if (n)
                    {
                        output << string(buffer,partialoutsize) << flush;
                        partialoutsize-=n;
                    }
                    delete[] buffer;
                } while(n != 0 && partialoutsize && n !=SOCKET_ERROR);
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
                n = recv(thesock, buffer, BUFFERSIZE, MSG_NOSIGNAL);
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
            if (n != 1 || buffer[0] != 0) //To avoid outputing 0 char in binary outputs
            {
                std::lock_guard<std::mutex> stdOutLockGuard(getStdoutLockGuard());
                output << string(buffer,n) << flush;
            }
        }
    } while(n != 0 && n !=SOCKET_ERROR);

    if (n == SOCKET_ERROR)
    {
        cerr << "ERROR reading output: " << ERRNO << endl;
        return -1;
    }

    return outcode;
}

int MegaCmdShellCommunicationsPosix::listenToStateChanges(int receiveSocket, StateChangedCb_t statechangehandle)
{
    assert(isSocketValid(receiveSocket));

    ScopeGuard g([this, receiveSocket]()
    {
        mStateListenerSocket = -1; // we don't want shutdowns after close!
        close(receiveSocket);
    });

    while (!mStopListener)
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
            return -1;
        }

        if (!n) // server closed the connection
        {
            return -1;
        }

        if (statechangehandle)
        {
            statechangehandle(newstate, *this);
        }
    }
    return 0;
}
#endif

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

        if (response == "yes" || response == "y" || response == "YES" || response == "Y" || response == "Yes")
        {
            return MCMDCONFIRM_YES;
        }
        if (response == "no" || response == "n" || response == "NO" || response == "N" || response == "No")
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

bool MegaCmdShellCommunications::waitForServerReadyOrRegistrationFailed(std::optional<std::chrono::milliseconds> timeout)
{
    return timeout ? mPromiseServerReadyOrRegistrationFailed.get_future().wait_for(*timeout) != std::future_status::timeout
            : (mPromiseServerReadyOrRegistrationFailed.get_future().wait(), true);
}

void MegaCmdShellCommunications::markServerIsUpdating()
{
    mUpdating = true;
}

void MegaCmdShellCommunications::unmarkServerIsUpdating()
{
    mUpdating = false;
}

bool MegaCmdShellCommunications::isServerUpdating()
{
    return mUpdating;
}

void MegaCmdShellCommunications::markServerReadyOrRegistrationFailed(bool readyOrFailed)
{
    if (!mPromiseServerReadyOrRegistrationFailedAttended.test_and_set()) // only once
    {
        mPromiseServerReadyOrRegistrationFailed.set_value(readyOrFailed);
    }
}

bool MegaCmdShellCommunications::registerForStateChanges(bool interactive, StateChangedCb_t statechangehandle, bool initiateServer)
{
    if (mStopListener || mUpdating) // finished
    {
        return false;
    }

    {
        std::lock_guard<std::mutex> g(mRegistrationMutex);
        if (mLastFailedRegistration && ( (std::chrono::steady_clock::now() - *mLastFailedRegistration) < std::chrono::seconds(30)))
        {
            // defer registration attempt
            return false;
        }
    }

    // if there's a listener thread running
    if (mListenerThread)
    {
        mStopListener = true;
        triggerListenerThreadShutdown();
        mListenerThread->join();
    }

    auto resultRegistration = registerForStateChangesImpl(interactive, initiateServer);
    if (!resultRegistration)
    {
        markServerRegistrationFailed();
        return false;
    }

    mRegisterRequired = false;

    mStopListener = false;

    mListenerThread.reset(new std::thread(
        [this, fd{resultRegistration.value()}, statechangeCb{std::move(statechangehandle)}]()
        {
            bool everSucceeded = false;
            auto stateChangedWrapped = [&everSucceeded, statechangeCb{std::move(statechangeCb)}](std::string state, MegaCmdShellCommunications &comsManager)
            {
                everSucceeded = true;
                statechangeCb(std::move(state), comsManager);
            };

            auto r = listenToStateChanges(fd, stateChangedWrapped);

            // Logic to consider registration again:
            if (r < 0 && !mStopListener && !mUpdating)
            {
                auto errorLine = !everSucceeded ? "\nWarning: Unable to register to state changes.\n" // This could happen if for instance, the server rejects registering a listener because max descriptors allowed for it has been depleted
#ifdef WIN32
                   : "\n[MEGAcmdServer.exe process seems to have stopped. Type to respawn or reconnect to it]\n";
#else
                    : "\n[mega-cmd-server process seems to have stopped. Type to respawn or reconnect to it]\n";
#endif

                setForRegisterAgain();
                std::cerr << errorLine << std::flush;
            }

            // In either case the above may have failed before receiving server readyness.
            // This will ensure we don't halt main thread execution if that's the case:
            markServerRegistrationFailed();
        }
    ));
    return true;
}

#ifndef _WIN32
std::optional<int> MegaCmdShellCommunicationsPosix::registerForStateChangesImpl(bool interactive, bool initiateServer)
{
    SOCKET thesock = createSocket(0, initiateServer);

    if (thesock == INVALID_SOCKET)
    {
        cerr << "Failed to create socket for registering for state changes" << endl;
        return {};
    }

    bool ok = false;
    ScopeGuard g([this, &thesock, &ok]()
    {
        if (!ok) //something failed
        {
            close(thesock);
        }
    });

    string command=interactive?"Xregisterstatelistener":"registerstatelistener";

    auto n = send(thesock,command.data(),command.size(), MSG_NOSIGNAL);

    if (n == SOCKET_ERROR)
    {
        cerr << "ERROR writing output Code to socket: " << ERRNO << endl;
        return {};
    }

    ok = true;

    mStateListenerSocket = thesock; // to be able to trigger shutdown
    return thesock;
}

void MegaCmdShellCommunicationsPosix::triggerListenerThreadShutdown()
{
    if (auto socket = mStateListenerSocket.exchange(-1); socket != -1)
    {
        // enforce socket shutdown: this will wake listenerThread
        ::shutdown(socket, SHUT_RDWR);
    }
}
#endif

void MegaCmdShellCommunications::setResponseConfirmation(bool /*confirmation*/)
{
}

void MegaCmdShellCommunications::shutdown()
{
    if (mListenerThread)
    {
        mStopListener = true;
        triggerListenerThreadShutdown();
        mListenerThread->join();
        mListenerThread.reset();
    }
}

bool MegaCmdShellCommunications::registerRequired()
{
    std::lock_guard<std::mutex> g(mRegistrationMutex);
    return mRegisterRequired;
}

void MegaCmdShellCommunications::setForRegisterAgain(bool dontWait)
{
    std::lock_guard<std::mutex> g(mRegistrationMutex);
    mRegisterRequired = true;
    if (dontWait)
    {
        mLastFailedRegistration = {};
    }
    else
    {
        mLastFailedRegistration = std::chrono::steady_clock::now();
    }
}

std::lock_guard<std::mutex> MegaCmdShellCommunications::getStdoutLockGuard()
{
    return std::lock_guard<std::mutex>(mStdoutMutex);
}

MegaCmdShellCommunications::~MegaCmdShellCommunications()
{
    assert(!mListenerThread);
}
} //end namespace
