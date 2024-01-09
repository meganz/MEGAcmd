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
#include "TestUtils.h"

TEST_F(NOINTERACTIVEBasicTest, Version)
{
    executeInClient({"version"});
}

TEST_F(NOINTERACTIVEBasicTest, Help)
{
    executeInClient({"help"});
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
        EXPECT_EQ(out, std::string("Account e-mail: ").append(getenv("MEGACMD_TEST_USER")).append("\n"));
    }

    {
        G_SUBTEST << "Extended";

        auto r = executeInClient({"whoami", "-l"});
        ASSERT_TRUE(r.ok());

        std::vector<std::string> details_out = splitByNewline(r.out());

        ASSERT_THAT(details_out, testing::Not(testing::IsEmpty()));
        ASSERT_THAT(details_out, testing::Contains(testing::HasSubstr("Available storage:")));
        ASSERT_THAT(details_out, testing::Contains(testing::HasSubstr("Pro level:")));
        ASSERT_THAT(details_out, testing::Contains(testing::HasSubstr("Current Session")));
        ASSERT_THAT(details_out, testing::Contains(testing::HasSubstr("In RUBBISH:")));

        EXPECT_THAT(details_out, testing::Not(testing::Contains(testing::ContainsRegex("Available storage:\\s+0\\.00\\s+Bytes"))));
        EXPECT_THAT(details_out, testing::Not(testing::Contains(testing::ContainsRegex("In ROOT:\\s+0\\.00\\s+Bytes"))));
    }
}
