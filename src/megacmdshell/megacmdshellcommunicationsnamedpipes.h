/**
 * @file examples/megacmd/megacmdshellcommunicationsnamedpipes.h
 * @brief MEGAcmd: Communications module to connect to server using NamedPipes
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

#ifndef MEGACMDSHELLCOMMUNICATIONSNAMEDPIPES_H
#define MEGACMDSHELLCOMMUNICATIONSNAMEDPIPES_H
#ifdef _WIN32

#include "megacmdshellcommunications.h"

#include <windows.h>
#include <Lmcons.h> //getusername

#define ERRNO WSAGetLastError()
#include <string>
#include <iostream>
#include <thread>
#include <mutex>

#include <Shlwapi.h> //PathAppend

typedef struct structListenStateChangesNamedPipe{
    int receiveNamedPipeNum;
    void (*statechangehandle)(std::string);
} sListenStateChangesNamedPipe;


class MegaCmdShellCommunicationsNamedPipes : public MegaCmdShellCommunications
{
public:
    MegaCmdShellCommunicationsNamedPipes();
    ~MegaCmdShellCommunicationsNamedPipes();

    virtual int executeCommand(std::string command, bool (*readconfirmationloop)(const char *) = NULL, OUTSTREAMTYPE &output = COUT, bool interactiveshell = true, std::wstring = L"");
    virtual int executeCommandW(std::wstring command, bool (*readconfirmationloop)(const char *) = NULL, OUTSTREAMTYPE &output = COUT, bool interactiveshell = true);

    virtual int registerForStateChanges(void (*statechangehandle)(std::string) = NULL);

    void setResponseConfirmation(bool confirmation);

    static HANDLE doOpenPipe(std::wstring nameOfPipe);

private:
    static bool namedPipeValid(HANDLE namedPipe);
    static void closeNamedPipe(HANDLE namedPipe);

    static void *listenToStateChangesEntryNamedPipe(void *slsc);
    static int listenToStateChanges(int receiveNamedPipeNum, void (*statechangehandle)(std::string) = NULL);

    static bool confirmResponse;

    static HANDLE newNamedPipe;

    static bool stopListener;
    static mega::Thread *listenerThread;

    static HANDLE createNamedPipe(int number = 0,bool initializeserver = true);

    static bool isFileOwnerCurrentUser(HANDLE hFile);

};

#endif
#endif // MEGACMDSHELLCOMMUNICATIONS_H
