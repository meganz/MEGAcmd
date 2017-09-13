/**
 * @file src/loader/megacmdloader.cpp
 * @brief MEGAcmdClient: Loader application of MEGAcmd for MAC
 *
 * (c) 2013-2016 by Mega Limited, Auckland, New Zealand
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{
    char buf[16];
    int fd = open("/dev/fsevents", O_RDONLY);
    seteuid(getuid());
    snprintf(buf, sizeof(buf), "%d", fd);
    execl("/Applications/MEGAcmd.app/Contents/MacOS/mega-cmd", buf, NULL);
    return 0;
}

