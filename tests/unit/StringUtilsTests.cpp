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

#include "TestUtils.h"
#include "megacmdcommonutils.h"

namespace StringUtilsTest
{
    static void trimming()
    {
        using megacmd::ltrim;
        using megacmd::rtrim;

        {
            std::string s("123456");
            G_SUBTEST << "Left trimming";
            ASSERT_STREQ(ltrim(s, '2').c_str(), "123456");
            ASSERT_STREQ(ltrim(s, '1').c_str(), "23456");
        }
        {
            std::string s("123456");
            G_SUBTEST << "Right trimming";
            ASSERT_STREQ(rtrim(s, '2').c_str(), "123456");
            ASSERT_STREQ(rtrim(s, '6').c_str(), "12345");
        }
    }
};

TEST(StringUtilsTest, trimming)
{
    StringUtilsTest::trimming();
}
