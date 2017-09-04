/**
 * @file examples/megacmd/megacmdshell.h
 * @brief MEGAcmd: Interactive CLI and service application
 * This is the shell application
 *
 * (c) 2013-2017 by Mega Limited, Auckland, New Zealand
 *
 * This file is distributed under the terms of the GNU General Public
 * License, see http://www.gnu.org/copyleft/gpl.txt
 * for details.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef MEGACMDSHELL_H
#define MEGACMDSHELL_H

#include <iostream>
#include <string>

#define OUTSTREAM COUT

enum prompttype
{
    COMMAND, LOGINPASSWORD, OLDPASSWORD, NEWPASSWORD, PASSWORDCONFIRM, AREYOUSURE
};

static const char* const prompts[] =
{
    "MEGA CMD> ", "Password:", "Old Password:", "New Password:", "Retype New Password:", "Are you sure to delete? "
};

void sleepSeconds(int seconds);

void sleepMicroSeconds(long microseconds);

void restoreprompt();

void printprogress(long long completed, long long total, const char *title = "TRANSFERING");

void changeprompt(const char *newprompt, bool redisplay = false);

const char * getUsageStr(const char *command);

void unescapeifRequired(std::string &what);

void setprompt(prompttype p, std::string arg = "");

prompttype getprompt();

void printHistory();

bool readconfirmationloop(const char *question);

#ifdef _WIN32
void stringtolocalw(const char* path, std::wstring* local);
void localwtostring(const std::wstring* wide, std::string *multibyte);

void utf16ToUtf8(const wchar_t* utf16data, int utf16size, std::string* utf8string);
#endif


#endif // MEGACMDSHELL_H

