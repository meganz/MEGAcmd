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

#pragma once

#include <gtest/gtest.h>

#include "megacmdcommonutils.h"
#include "megacmd.h"
#include "client/megacmdclient.h"
#include "megacmdlogger.h"

#include <gtest/gtest.h>
#include "Instruments.h"

#include <chrono>
#include <future>
#include <regex>

constexpr const char* LINK_TESTEXPORTFILE01TXT = "https://mega.nz/file/YfNngDKR#qk9THHhxbakddRmt_tLR8OhInexzVCpPPG6M6feFfZg";
constexpr const char* LINK_TESTEXPORTFOLDER = "https://mega.nz/folder/saMRXBYL#9GETCO4E-Po45d3qSjZhbQ";
constexpr const char* LINK_TESTREADINGFOLDER01 = "https://mega.nz/folder/YPV0nCKS#bSruKSPPubdCmm5harBJOQ";

std::vector<std::string> splitByNewline(const std::string& str);

class ClientResponse
{
    int mStatus = -1;
    OUTSTRING mErr;
    OUTSTRING mOut;
#ifdef _WIN32
    std::string mUtf8String;
    std::string mUtf8ErrString;
#endif

public:
    ClientResponse(int status, OUTSTRINGSTREAM &streamOut, OUTSTRINGSTREAM &streamErr)
        : mStatus(status)
        , mOut (streamOut.str())
        , mErr (streamErr.str())
    #ifdef _WIN32
        , mUtf8String(megacmd::utf16ToUtf8(mOut))
        , mUtf8ErrString(megacmd::utf16ToUtf8(mErr))
    #endif
    {}

    int status() { return mStatus; }
    bool ok() { return !mStatus; }

    // returns an utf8 std::string with the response from the server
    std::string &out()
    {
#ifdef _WIN32
        return mUtf8String;
#else
        return mOut;
#endif
    }

    //returns an utf8 std::string with the response from the server
    std::string &err()
    {
#ifdef _WIN32
        return mUtf8ErrString;
#else
        return mErr;
#endif
    }
};

std::string joinString(const std::vector<std::string> &vec, std::string_view sep = " ");
ClientResponse executeInClient(const std::vector<std::string>& command, bool nonInteractive = true);
void ensureLoggedIn();
void ensureReadStructure();

class BasicGenericTest : public ::testing::Test
{
};

class NotLoggedTest : public BasicGenericTest
{
protected:

    void SetUp() override
    {
        BasicGenericTest::SetUp();
        auto result = executeInClient({"logout"}).ok();
        ASSERT_TRUE(result);
    }

    void TearDown() override
    {
        BasicGenericTest::SetUp();
        auto result = executeInClient({"logout"}).ok();
        ASSERT_TRUE(result);
    }
};

class LoggedInTest : public BasicGenericTest
{
protected:
    void removeAllSyncs()
    {
        auto result = executeInClient({"sync"});
        ASSERT_TRUE(result.ok());

        auto lines = splitByNewline(result.out());
        for (size_t i = 1; i < lines.size(); ++i)
        {
            // After an empty line, there are only user-facing messages (if any)
            if (lines[i].empty())
            {
                return;
            }

            auto words = megacmd::split(lines[i], " ");
            ASSERT_FALSE(words.empty());

            std::string syncId = words[0];
            auto result = executeInClient({"sync", "--remove", "--", syncId}).ok();
            ASSERT_TRUE(result);
        }
    }

    void SetUp() override
    {
        BasicGenericTest::SetUp();
        ensureLoggedIn();
    }
};

class ReadTest : public LoggedInTest
{
protected:
    void SetUp() override
    {
        LoggedInTest::SetUp();
        ensureReadStructure();
    }
};

class NOINTERACTIVEBasicTest : public BasicGenericTest{};
class NOINTERACTIVELoggedInTest : public LoggedInTest{};
class NOINTERACTIVENotLoggedTest : public NotLoggedTest{};
class NOINTERACTIVEReadTest : public ReadTest{};
