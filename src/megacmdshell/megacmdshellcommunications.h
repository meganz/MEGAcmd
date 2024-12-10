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

// In the server, OUTSTREAM is defined in megacmdlogger.h: #define OUTSTREAM getCurrentOut()
// However in the exec and cmd apps:
#define OUTSTREAM COUT

#include <string>
#include <iostream>
#include <mutex>
#include <future>

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
#include "mega/thread/cppthread.h"
class MegaThread : public ::mega::CppThread {};
#else
#include "mega/thread/posixthread.h"
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
namespace megacmd {

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

class MegaCmdShellCommunications
{
public:
    using StateChangedCb_t = std::function<void(std::string /*state string*/, MegaCmdShellCommunications &)>;
    MegaCmdShellCommunications();
    virtual ~MegaCmdShellCommunications();

    static inline std::mutex megaCmdStdoutputing;
    virtual int executeCommand(std::string command, std::string (*readresponse)(const char *) = NULL, OUTSTREAMTYPE &output = COUT, bool interactiveshell = true, std::wstring = L"") = 0;
    virtual int executeCommandW(std::wstring command, std::string (*readresponse)(const char *) = NULL, OUTSTREAMTYPE &output = COUT, bool interactiveshell = true);

    virtual bool registerForStateChanges(bool interactive, StateChangedCb_t statechangehandle, bool initiateServer = true);

    virtual void setResponseConfirmation(bool confirmation);

    bool mServerInitiatedFromShell = false;

    int readconfirmationloop(const char *question, std::string (*readresponse)(const char *));

    // returns true if did not timeout
    bool waitForServerReadyOrRegistrationFailed(std::optional<std::chrono::milliseconds> timeout = {});

    void markServerReady() { markServerReadyOrRegistrationFailed(true); }
    void markServerRegistrationFailed() { markServerReadyOrRegistrationFailed(false); }

    void markServerIsUpdating();
    bool isServerUpdating();

    void shutdown();
    bool registerRequired();
    void setForRegisterAgain(bool dontWait = false);

private:
    virtual void triggerListenerThreadShutdown() {};
    virtual std::optional<int> registerForStateChangesImpl(bool interactive, bool initiateServer = true) = 0;
    virtual int listenToStateChanges(int receiveSocket, StateChangedCb_t statechangehandle) = 0;

    static inline bool confirmResponse = false;

    std::unique_ptr<std::thread> mListenerThread;

    std::mutex mRegistrationMutex;
    std::optional<std::chrono::steady_clock::time_point> mLastFailedRegistration;
    bool mRegisterRequired = true;

    void markServerReadyOrRegistrationFailed(bool readyOrFailed);

protected:
    std::promise<bool> mPromiseServerReadyOrRegistrationFailed;
    std::atomic_flag mPromiseServerReadyOrRegistrationFailedAttended = ATOMIC_FLAG_INIT;
    std::atomic_bool mStopListener = false;
    std::atomic_bool mUpdating = false;

};

#ifndef _WIN32
class MegaCmdShellCommunicationsPosix : public MegaCmdShellCommunications
{
public:
    int executeCommand(std::string command, std::string (*readresponse)(const char *) = NULL, OUTSTREAMTYPE &output = COUT, bool interactiveshell = true, std::wstring = L"") override;
private:

    bool isSocketValid(SOCKET socket);
    SOCKET createSocket(int number = 0, bool initializeserver = true);

    std::atomic_int mStateListenerSocket = -1;
    std::optional<int> registerForStateChangesImpl(bool interactive, bool initiateServer = true) override;
    int listenToStateChanges(int receiveSocket, StateChangedCb_t statechangehandle) override;
    void triggerListenerThreadShutdown() override;
};
#endif

}//end namespace
#endif // MEGACMDSHELLCOMMUNICATIONS_H
