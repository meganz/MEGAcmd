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
#include <gmock/gmock.h>

#include "MegaCmdTestingTools.h"
#include "Instruments.h"
#include "TestUtils.h"

using TI = TestInstruments;

TEST_F(NOINTERACTIVEBasicTest, Version)
{
    executeInClient({"version"});
}

TEST_F(NOINTERACTIVEBasicTest, Help)
{
    executeInClient({"help"});
}

TEST_F(NOINTERACTIVENotLoggedTest, Folderlogin)
{
    {
        auto rLoging = executeInClient({"login", LINK_TESTEXPORTFOLDER});
        ASSERT_TRUE(rLoging.ok());
    }

    {
        auto rLogout = executeInClient({"logout", "--keep-session"});
        ASSERT_TRUE(rLogout.ok());
    }

    {
        EventTracker evTracker({TestInstruments::Event::FETCH_NODES_REQ_UPDATE});
        auto rLoging = executeInClient({"login", "--resume", LINK_TESTEXPORTFOLDER});
        ASSERT_TRUE(rLoging.ok());
        ASSERT_STREQ(rLoging.out().c_str(), "");
        ASSERT_STREQ(rLoging.err().c_str(), "");
        ASSERT_FALSE(evTracker.eventHappened(TestInstruments::Event::FETCH_NODES_REQ_UPDATE));
    }
}

TEST_F(NOINTERACTIVEReadTest, Find)
{
    auto r = executeInClient({"find"});
    ASSERT_TRUE(r.ok());

    std::vector<std::string> result_paths = splitByNewline(r.out());
    ASSERT_THAT(result_paths, testing::Not(testing::IsEmpty()));

    EXPECT_THAT(result_paths, testing::Contains("."));
    EXPECT_THAT(result_paths, testing::Contains("testReadingFolder01"));
    EXPECT_THAT(result_paths, testing::Contains("testReadingFolder01/file03.txt"));
    EXPECT_THAT(result_paths, testing::Contains("testReadingFolder01/folder01/file03.txt"));
    EXPECT_THAT(result_paths, testing::Contains("testReadingFolder01/folder02/subfolder03/file02.txt"));
}

TEST_F(NOINTERACTIVELoggedInTest, Whoami)
{

    {
        G_SUBTEST << "Basic";

        auto r = executeInClient({"whoami"});
        ASSERT_TRUE(r.ok());

        auto out = r.out();
        ASSERT_THAT(out, testing::Not(testing::IsEmpty()));
        ASSERT_THAT(getenv("MEGACMD_TEST_USER"), testing::NotNull());
        EXPECT_EQ(out, std::string("Account e-mail: ").append(getenv("MEGACMD_TEST_USER")).append("\n"));
    }

    {
        G_SUBTEST << "Extended";

        {
           // Ensure there's at least a file around contributing to ROOT usage:
           auto result = executeInClient({"import", LINK_TESTEXPORTFILE01TXT});
           ASSERT_TRUE(result.ok()) << "could not import testExportFile01.txt";
        }

        auto r = executeInClient({"whoami", "-l"});
        ASSERT_TRUE(r.ok());

        std::vector<std::string> details_out = splitByNewline(r.out());

        ASSERT_THAT(details_out, testing::Not(testing::IsEmpty()));
        ASSERT_THAT(details_out, testing::Contains(testing::HasSubstr("Available storage:")));
        ASSERT_THAT(details_out, testing::Contains(testing::HasSubstr("Pro level:")));
        ASSERT_THAT(details_out, testing::Contains(testing::HasSubstr("Current Session")));
        ASSERT_THAT(details_out, testing::Contains(testing::HasSubstr("In RUBBISH:")));

        EXPECT_THAT(details_out, testing::Not(testing::Contains(ContainsStdRegex("Available storage:\\s+0\\.00\\s+Bytes"))));
        EXPECT_THAT(details_out, testing::Not(testing::Contains(ContainsStdRegex("In ROOT:\\s+0\\.00\\s+Bytes"))));
    }
}

TEST_F(NOINTERACTIVEBasicTest, EchoInvalidUtf8)
{
    const std::string validUtf8 = u8"\uc548\uc548\ub155\ud558\uc138\uc694\uc138\uacc4";

#ifdef _WIN32
    // Required because windows converts the args to UTF-16 before sending them to the server,
    // so we can't pass an invalid UTF-8 string all the way to the executer; we need to escape it
    const std::string invalidUtf8 = "<win-invalid-utf8>";
#else
    const std::string invalidUtf8 = "\xf0\x8f\xbf\xbf";
#endif

    auto result = executeInClient({"echo", validUtf8});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr(validUtf8));

    result = executeInClient({"echo", invalidUtf8});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::Not(testing::HasSubstr(invalidUtf8)));

    result = executeInClient({"echo", "--log-as-err", validUtf8});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.err(), testing::HasSubstr(validUtf8));

    result = executeInClient({"echo", "--log-as-err", invalidUtf8});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.err(), testing::HasSubstr("<invalid utf8>"));
    EXPECT_THAT(result.err(), testing::Not(testing::HasSubstr(invalidUtf8)));
    EXPECT_THAT(result.out(), testing::Not(testing::HasSubstr(invalidUtf8)));
}
