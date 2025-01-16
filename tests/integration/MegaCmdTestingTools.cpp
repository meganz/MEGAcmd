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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MegaCmdTestingTools.h"

#include "TestUtils.h"
#include <numeric>
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
    G_TEST_MSG << " Executing in client: " << std::accumulate(command.begin(), command.end(), std::string(), [](auto&& acc, auto&& v) { return acc + (acc.empty() ? "" : ",") + v; });
    auto code = megacmd::executeClient(static_cast<int>(args.size() - 1), args.data(), stream);
    G_TEST_MSG << " Response (code=" << code << ", size= " << stream.str().size() << "):\n" << stream.str();
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

        ASSERT_THAT(user, testing::NotNull()) << "Missing testing user env variable. Ensure that MEGACMD_TEST_USER is set!";
        ASSERT_STRNE(user, "") << "Missing testing user env variable. Ensure that MEGACMD_TEST_USER is set!";

        ASSERT_THAT(pass, testing::NotNull()) << "Missing testing user password env variable. Ensure that MEGACMD_TEST_PASS is set!";
        ASSERT_STRNE(pass, "") << "Missing testing user password env variable. Ensure that MEGACMD_TEST_PASS is set!";

        auto result = executeInClient({"login", user, pass});
        ASSERT_TRUE(result.ok());

        result = executeInClient({"whoami"});
        ASSERT_TRUE(result.ok());
        ASSERT_THAT(result.out(), testing::HasSubstr(user));
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
        auto result = executeInClient({"import", LINK_TESTREADINGFOLDER01});
        ASSERT_TRUE(result.ok());
    }
}
