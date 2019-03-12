/**
 * @file src/megacmdshellcommunications.h
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

#ifndef MEGACMDSHELLCOMMUNICATIONS_H
#define MEGACMDSHELLCOMMUNICATIONS_H

#include "../megacmdcommonutils.h"

#include <string>
#include <iostream>

#ifdef _WIN32
#include <WinSock2.h>
#include <Shlwapi.h> //PathAppend
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/un.h>
#endif

#if defined(_WIN32) && !defined(WINDOWS_PHONE) && !defined(USE_CPPTHREAD)
#include "mega/thread/win32thread.h"
class MegaMutex : public ::mega::Win32Mutex {};
class MegaThread : public ::mega::Win32Thread {};
#elif defined(USE_CPPTHREAD)
#include "mega/thread/cppthread.h"
class MegaMutex : public ::mega::CppMutex {};
class MegaThread : public ::mega::CppThread {};
#else
#include "mega/thread/posixthread.h"
class MegaMutex : public ::mega::PosixMutex {};
class MegaThread : public ::mega::PosixThread {};
#endif

#ifdef _WIN32
#else
typedef int SOCKET;
#endif

#ifdef _WIN32
#include <windows.h>
#define ERRNO WSAGetLastError()
#else
#define ERRNO errno
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

#ifdef __MACH__
#define MSG_NOSIGNAL 0
#elif _WIN32
#define MSG_NOSIGNAL 0
#endif

#define MEGACMDINITIALPORTNUMBER 12300

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
    MCMD_REQSTRING = -61,     ///< String required
    MCMD_PARTIALOUT = -62,     ///< Partial output provided

#if defined(_WIN32) || defined(__APPLE__)
    MCMD_REQRESTART = -71,     ///< Restart required
#endif
};

enum confirmresponse
{
    MCMDCONFIRM_NO=0,
    MCMDCONFIRM_YES,
    MCMDCONFIRM_ALL,
    MCMDCONFIRM_NONE
};

typedef struct structListenStateChanges{
    int receiveSocket;
    void (*statechangehandle)(std::string);
} sListenStateChanges;

class MegaCmdShellCommunications
{
public:
    MegaCmdShellCommunications();
    virtual ~MegaCmdShellCommunications();

    static MegaMutex megaCmdStdoutputing;
    virtual int executeCommand(std::string command, std::string (*readresponse)(const char *) = NULL, OUTSTREAMTYPE &output = COUT, bool interactiveshell = true, std::wstring = L"");
    virtual int executeCommandW(std::wstring command, std::string (*readresponse)(const char *) = NULL, OUTSTREAMTYPE &output = COUT, bool interactiveshell = true);

    virtual int registerForStateChanges(void (*statechangehandle)(std::string) = NULL);

    virtual void setResponseConfirmation(bool confirmation);

    static bool serverinitiatedfromshell;
    static bool registerAgainRequired;
    int readconfirmationloop(const char *question, std::string (*readresponse)(const char *));
    static bool updating;

private:
    static SOCKET newsockfd;
    static bool socketValid(SOCKET socket);
    static void closeSocket(SOCKET socket);

    static void *listenToStateChangesEntry(void *slsc);
    static int listenToStateChanges(int receiveSocket, void (*statechangehandle)(std::string) = NULL);


    static bool confirmResponse;

    static bool stopListener;
    static mega::Thread *listenerThread;

#ifdef _WIN32
static SOCKET createSocket(int number = 0, bool initializeserver = true, bool net = true);
#else
static SOCKET createSocket(int number = 0, bool initializeserver = true, bool net = false);
#endif


};

#endif // MEGACMDSHELLCOMMUNICATIONS_H
