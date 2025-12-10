/**
 * (c) 2025 by Mega Limited, Auckland, New Zealand
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

#include "TestUtils.h"
#include "megacmdcommonutils.h"

TEST(MegaCmdCommonUtilsTest, split)
{
    using megacmd::split;

    G_SUBTEST << "Basic split";
    {
        std::vector<std::string> result = split("a,b,c", ",");
        EXPECT_THAT(result, testing::ElementsAre("a", "b", "c"));
    }

    G_SUBTEST << "Empty string";
    {
        std::vector<std::string> result = split("", ",");
        EXPECT_TRUE(result.empty());
    }

    G_SUBTEST << "No delimiter found";
    {
        std::vector<std::string> result = split("abc", ",");
        EXPECT_THAT(result, testing::ElementsAre("abc"));
    }

    G_SUBTEST << "Multiple consecutive delimiters";
    {
        std::vector<std::string> result = split("a,,b,,c", ",");
        EXPECT_THAT(result, testing::ElementsAre("a", "b", "c"));
    }

    G_SUBTEST << "Delimiter at start";
    {
        std::vector<std::string> result = split(",a,b", ",");
        EXPECT_THAT(result, testing::ElementsAre("a", "b"));
    }

    G_SUBTEST << "Delimiter at end";
    {
        std::vector<std::string> result = split("a,b,", ",");
        EXPECT_THAT(result, testing::ElementsAre("a", "b"));
    }

    G_SUBTEST << "Multi-character delimiter";
    {
        std::vector<std::string> result = split("a::b::c", "::");
        EXPECT_THAT(result, testing::ElementsAre("a", "b", "c"));
    }

    G_SUBTEST << "Single character";
    {
        std::vector<std::string> result = split("a", ",");
        EXPECT_THAT(result, testing::ElementsAre("a"));
    }
}
