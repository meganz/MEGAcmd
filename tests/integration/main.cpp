/**
 * (c) 2013 by Mega Limited, Auckland, New Zealand
 *
 * This file is part of MEGAcmd.
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

#include <gtest/gtest.h>

#ifdef __linux__
#include <sched.h>
#endif

bool extractargvalue(std::vector<const char*>& args, const char *what, std::string& param)
{
    auto len = strlen(what);
    for (int i = int(args.size()); i--; )
    {
        if (strlen(args[i]) > (len + 1) && !strncmp(args[i], what, len) && args[i][len] == '=')
        {
            param=&args[i][len+1];

            args.erase(args.begin() + i);
            return true;
        }
    }
    return false;
}

#ifdef __linux__
static void setUpUnixSignals()
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
}
#endif

int main (int argc, char *argv[])
{
#ifdef __linux__
    if (getenv("MEGA_INTEGRATION_TEST_ENFORCE_SINGLE_CPU"))
    {
        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(1, &set);
        if (sched_setaffinity(getpid(), sizeof(set), &set) == -1)
        {
            std::cerr << "sched_setaffinity error: " << errno << std::endl;
            exit(errno);
        }
    }

    setUpUnixSignals();
#endif

    testing::InitGoogleTest(&argc, argv);

    auto exitCode = RUN_ALL_TESTS();

    return exitCode;
}
