/**
 * @file src/megacmdshell.h
 * @brief MEGAcmd: Interactive CLI and service application
 * This is the shell application
 *
 * (c) 2013 by Mega Limited, Auckland, New Zealand
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
    COMMAND, LOGINPASSWORD, NEWPASSWORD, PASSWORDCONFIRM, AREYOUSURE
};

static const char* const prompts[] =
{
    "MEGA CMD> ", "Password:", "New Password:", "Retype New Password:", "Are you sure to delete? "
};

void sleepSeconds(int seconds);

void sleepMilliSeconds(long milliseconds);

void restoreprompt();

void printprogress(long long completed, long long total, const char *title = "TRANSFERRING");

void changeprompt(const char *newprompt, bool redisplay = false, const char *newpreffix = NULL);

const char * getUsageStr(const char *command);

void unescapeifRequired(std::string &what);

void setprompt(prompttype p, std::string arg = "");

prompttype getprompt();

void printHistory();

std::string readresponse(const char *question);

#endif // MEGACMDSHELL_H

