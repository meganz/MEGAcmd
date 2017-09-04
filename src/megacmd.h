/**
 * @file examples/megacmd/megacmd.h
 * @brief MEGAcmd: Interactive CLI and service application
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

#ifndef MEGACMD_H
#define MEGACMD_H

#ifdef _WIN32

#define OUTSTREAMTYPE std::wostream
#define OUTSTRINGSTREAM std::wostringstream
#define OUTSTRING std::wstring
#define COUT wcout



#include <string>
std::wostream & operator<< ( std::wostream & ostr, std::string const & str );
std::wostream & operator<< ( std::wostream & ostr, const char * str );
std::ostringstream & operator<< ( std::ostringstream & ostr, std::wstring const &str);

void localwtostring(const std::wstring* wide, std::string *multibyte);

#else
#define OUTSTREAMTYPE std::ostream
#define OUTSTRINGSTREAM std::ostringstream
#define OUTSTRING std::string
#define COUT cout
#endif

#include "megaapi_impl.h"

typedef struct sync_struct
{
    mega::MegaHandle handle;
    bool active;
    std::string localpath;
    long long fingerprint;
} sync_struct;


enum prompttype
{
    COMMAND, LOGINPASSWORD, OLDPASSWORD, NEWPASSWORD, PASSWORDCONFIRM, AREYOUSURETODELETE
};

static const char* const prompts[] =
{
    "MEGA CMD> ", "Password:", "Old Password:", "New Password:", "Retype New Password:", "Are you sure to delete? "
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

};


void changeprompt(const char *newprompt);

mega::MegaApi* getFreeApiFolder();
void freeApiFolder(mega::MegaApi *apiFolder);

const char * getUsageStr(const char *command);

void unescapeifRequired(std::string &what);

void setprompt(prompttype p, std::string arg = "");

prompttype getprompt();

void printHistory();

bool askforConfirmation(std::string message);

void informTransferUpdate(mega::MegaTransfer *transfer, int clientID);

void informProgressUpdate(long long transferred, long long total, int clientID, std::string title = "");



#endif
