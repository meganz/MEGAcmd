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

std::vector<std::string> splitByNewline(const std::string& str)
{
    std::vector<std::string> result;
    std::string line;
    std::istringstream iss(str);

    while (std::getline(iss, line))
    {
        result.push_back(line);
    }
    return result;
}

ClientResponse executeInClient(const std::vector<std::string>& command, bool /*nonInteractive: TODO: give support to shell execs*/)
{
    // To manage the memory of the first arg
    const std::string firstArg("args0_test_client");

    std::vector<char*> args{const_cast<char*>(firstArg.c_str())};
    for (const auto& word : command)
    {
        args.push_back(const_cast<char*>(word.c_str()));
    }
    args.push_back(nullptr);

    OUTSTRINGSTREAM stream;
    auto code = megacmd::executeClient(static_cast<int>(args.size() - 1), args.data(), stream);
    return {code, stream};
}

bool isServerLogged()
{
    return executeInClient({"whoami"}).ok();
}

void ensureLoggedIn()
{
    if (!isServerLogged())
    {
        const char* user = getenv("MEGACMD_TEST_USER");
        const char* pass = getenv("MEGACMD_TEST_PASS");

        if (!user || !pass)
        {
            std::cerr << "ENSURE MEGACMD_TEST_USER and MEGACMD_TEST_PASS are set!" << std::endl;
            ASSERT_FALSE("MISSING USER/PASS env variables");
        }

        auto result = executeInClient({"login", user, pass});
        ASSERT_EQ(0, result.status());
    }
}

bool hasReadStructure()
{
    return executeInClient({"ls", "testReadingFolder01"}).ok();
}

void ensureReadStructure()
{
    if (!hasReadStructure())
    {
        auto result = executeInClient({"import", "https://mega.nz/folder/gflVFLhC#6neMkeJrt4dWboRTc1NLUg"});
        ASSERT_EQ(0, result.status());
    }
}

