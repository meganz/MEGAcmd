/**
 * @file src/megacmd.h
 * @brief MEGAcmd: Interactive CLI and service application
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

#ifndef MEGACMD_H
#define MEGACMD_H

#include <iostream>
#include <iomanip>
#ifdef _WIN32
#include <algorithm>
#endif
using std::cout;
using std::endl;
using std::max;
using std::min;
using std::flush;
using std::left;
using std::cerr;
using std::istringstream;
using std::locale;
using std::stringstream;
using std::exception;



#include "megaapi_impl.h"

#define PROGRESS_COMPLETE -2
namespace megacmd {

struct StatsManager {

    /** MEGAcmd events **/
    // Allocated ranges:
    //    - regular: [98'900,98'999]
    //    - extended [860'000,879'999]
    // Params:
    //   - event name
    //   - number
    //   - Default message

    #define GENERATE_FROM_MEGACMD_EVENTS(GENERATOR_MACRO) \
        GENERATOR_MACRO(UPDATE                                          , 98900, "MEGAcmd update") \
        GENERATOR_MACRO(UPDATE_START                                    , 98901, "MEGAcmd auto-update start") \
        GENERATOR_MACRO(UPDATE_RESTART                                  , 98902, "MEGAcmd updated requiring restart") \
        GENERATOR_MACRO(FIRST_CONFIGURED_SYNC                           , 98903, "MEGAcmd first sync configured") \
        GENERATOR_MACRO(WAITED_TOO_LONG_FOR_NODES_CURRENT               , 98904, "MEGAcmd nodes current wait timed out")

    static constexpr auto FIRST_EVENT_NUMBER = 98900u;

    enum class MegacmdEvent
    {
    #define SOME_GENERATOR_MACRO(name, num, __) name = num,
      GENERATE_FROM_MEGACMD_EVENTS(SOME_GENERATOR_MACRO)
    #undef SOME_GENERATOR_MACRO
        LastEvent
    };

    static const char *defaultEventMsg(MegacmdEvent ev);
    static const char *eventName(MegacmdEvent ev);
};

typedef struct sync_struct
{
    mega::MegaHandle handle;
    bool active;
    std::string localpath;
    long long fingerprint;
    bool loadedok; //ephimeral data
} sync_struct;


typedef struct backup_struct
{
    mega::MegaHandle handle;
    bool active;
    std::string localpath; //TODO: review wether this is local or utf-8 representation and be consistent
    int64_t period;
    std::string speriod;
    int numBackups;
    bool failed; //This should mark the failure upon resuming. It shall not be persisted
    int tag; //This is depends on execution. should not be persisted
    int id; //Internal id for megacmd. Depends on execution should not be persisted
} backup_istruct;


enum prompttype
{
    COMMAND, LOGINPASSWORD, NEWPASSWORD, PASSWORDCONFIRM, AREYOUSURETODELETE
};

static const char* const prompts[] =
{
    "MEGA CMD> ", "Password:", "New Password:", "Retype New Password:", "Are you sure to delete? "
};

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

    MCMD_REQRESTART = -71,     ///< Restart required

};


enum confirmresponse
{
    MCMDCONFIRM_NO=0,
    MCMDCONFIRM_YES,
    MCMDCONFIRM_ALL,
    MCMDCONFIRM_NONE
};

void changeprompt(const char *newprompt);

void informStateListener(std::string message, int clientID);
void broadcastMessage(std::string message, bool keepIfNoListeners = false);
void informStateListeners(std::string s);


void removeDelayedBroadcastMatching(const std::string &toMatch);
void broadcastDelayedMessage(std::string message, bool keepIfNoListeners);

void appendGreetingStatusFirstListener(const std::string &msj);
void removeGreetingStatusFirstListener(const std::string &msj);
void appendGreetingStatusAllListener(const std::string &msj);
void clearGreetingStatusAllListener();
void clearGreetingStatusFirstListener();
void removeGreetingStatusAllListener(const std::string &msj);
void removeGreetingMatching(const std::string &toMatch);
void removeDelayedBroadcastMatching(const std::string &toMatch);

void setloginInAtStartup(bool value);
void setBlocked(int value);
int getBlocked();
void unblock();
bool getloginInAtStartup();
void updatevalidCommands();
void reset();

/**
 * @brief A class to ensure clients are properly informed of login in situations
 */
class LoginGuard {
public:
    LoginGuard()
    {
        appendGreetingStatusAllListener(std::string("login:"));
        setloginInAtStartup(true);
    }

    ~LoginGuard()
    {
        removeGreetingStatusAllListener(std::string("login:"));
        informStateListeners("loged:"); //send this even when failed!
        setloginInAtStartup(false);
    }
};


mega::MegaApi* getFreeApiFolder();
void freeApiFolder(mega::MegaApi *apiFolder);

const char * getUsageStr(const char *command);

void unescapeifRequired(std::string &what);

void setprompt(prompttype p, std::string arg = "");

prompttype getprompt();

void printHistory();

int askforConfirmation(std::string message);
bool booleanAskForConfirmation(std::string messageHeading);

std::string askforUserResponse(std::string message);

void* checkForUpdates(void *param);

void stopcheckingForUpdates();
void startcheckingForUpdates();

void informTransferUpdate(mega::MegaTransfer *transfer, int clientID);
void informStateListenerByClientId(int clientID, std::string s);


void informProgressUpdate(long long transferred, long long total, int clientID, std::string title = "");

void sendEvent(StatsManager::MegacmdEvent event, mega::MegaApi *megaApi, bool wait = true);
void sendEvent(StatsManager::MegacmdEvent event, const char *msg, mega::MegaApi *megaApi, bool wait = true);

int executeServer(int argc, char* argv[]);
void stopServer();

}//end namespace
#endif
