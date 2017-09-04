/**
 * @file examples/megacmd/megacmdshellcommunications.h
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

#ifndef MEGACMDSHELLCOMMUNICATIONS_H
#define MEGACMDSHELLCOMMUNICATIONS_H

#include "mega/thread.h"

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


#ifdef _WIN32

#define OUTSTREAMTYPE std::wostream
#define OUTSTRINGSTREAM std::wostringstream
#define OUTSTRING std::wstring
#define COUT std::wcout

std::wostream & operator<< ( std::wostream & ostr, std::string const & str );
std::wostream & operator<< ( std::wostream & ostr, const char * str );
std::ostringstream & operator<< ( std::ostringstream & ostr, std::wstring const &str);

#else
#define OUTSTREAMTYPE std::ostream
#define OUTSTRINGSTREAM std::ostringstream
#define OUTSTRING std::string
#define COUT std::cout
#endif

#define OUTSTREAM COUT

#ifdef _WIN32
void stringtolocalw(const char* path, std::wstring* local);
void localwtostring(const std::wstring* wide, std::string *multibyte);
void utf16ToUtf8(const wchar_t* utf16data, int utf16size, std::string* utf8string);
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

typedef struct structListenStateChanges{
    int receiveSocket;
    void (*statechangehandle)(std::string);
} sListenStateChanges;

class MegaCmdShellCommunications
{
public:
    MegaCmdShellCommunications();
    virtual ~MegaCmdShellCommunications();

    virtual int executeCommand(std::string command, bool (*readconfirmationloop)(const char *) = NULL, OUTSTREAMTYPE &output = COUT, bool interactiveshell = true, std::wstring = L"");
    virtual int executeCommandW(std::wstring command, bool (*readconfirmationloop)(const char *) = NULL, OUTSTREAMTYPE &output = COUT, bool interactiveshell = true);

    virtual int registerForStateChanges(void (*statechangehandle)(std::string) = NULL);

    virtual void setResponseConfirmation(bool confirmation);

    static bool serverinitiatedfromshell;
    static bool registerAgainRequired;

private:
    static int newsockfd;
    static bool socketValid(int socket);
    static void closeSocket(int socket);

    static void *listenToStateChangesEntry(void *slsc);
    static int listenToStateChanges(int receiveSocket, void (*statechangehandle)(std::string) = NULL);

    static bool confirmResponse;

    static bool stopListener;
    static mega::Thread *listenerThread;

#ifdef _WIN32
static int createSocket(int number = 0, bool initializeserver = true, bool net = true);
#else
static int createSocket(int number = 0, bool initializeserver = true, bool net = false);
#endif


};

#endif // MEGACMDSHELLCOMMUNICATIONS_H
