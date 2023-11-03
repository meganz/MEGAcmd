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

#include "MegaCmdTestingTools.h"

ClientResponse executeInClient(std::list<std::string> command, bool /*nonInteractive: TODO: give support to shell execs*/)
{
    char **args = new char*[2 + command.size()];
    int i = 0;
    args[i++] = (char*) "args0_test_client";
    for (auto & word :command)
    {
        args[i++] = (char*) word.c_str();
    }
    args[i] = nullptr;
    OUTSTRINGSTREAM stream;
    auto code = megacmd::executeClient(i, args, stream);
    ClientResponse r;
    r.mStatus = code;
    r.mOut = stream.str();
    //TODO: have output handled
    return r;
}

bool isServerLogged()
{
    return !executeInClient({"whoami"}).mStatus;
}

void ensureLoggedIn()
{
    if (!isServerLogged())
    {
        auto user = getenv("MEGACMD_TEST_USER");
        auto pass = getenv("MEGACMD_TEST_PASS");

        if (!user || !pass)
        {
            std::cerr << "ENSURE MEGACMD_TEST_USER and MEGACMD_TEST_PASS are set!" << std::endl;
            ASSERT_FALSE("MISSING USER/PASS env variables");
        }

        auto result = executeInClient({"login",user,pass});

        ASSERT_EQ(0, result.mStatus);
    }
}

bool hasReadStructure()
{
    return !executeInClient({"ls testReadingFolder01"}).mStatus;
}

void ensureReadStructure()
{
    if (!hasReadStructure())
    {
        auto result = executeInClient({"import","https://mega.nz/folder/1NYSDSDZ#slJ2xyjgDMqJ5rHFM-YCSw", "testReadingFolder01"}); //TODO: procure the means to generate this one.
        ASSERT_EQ(0, result.mStatus);
    }
}

