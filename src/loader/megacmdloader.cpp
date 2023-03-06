/**
 * @file src/loader/megacmdloader.cpp
 * @brief MEGAcmdClient: Loader application of MEGAcmd for MAC
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
#include <unistd.h>

int main(int argc, char *argv[])
{
    execl("/Applications/MEGAcmd.app/Contents/MacOS/mega-cmd", NULL);
    return 0;
}

