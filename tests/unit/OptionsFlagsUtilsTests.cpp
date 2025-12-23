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

#include <climits>
#include <map>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "TestUtils.h"

TEST(OptionsFlagsUtilsTest, getFlag)
{
    using megacmd::getFlag;

    G_SUBTEST << "Basic case";
    {
        std::map<std::string, int> flags = {{"verbose", 1}, {"debug", 0}};
        EXPECT_EQ(getFlag(&flags, "verbose"), 1);
        EXPECT_EQ(getFlag(&flags, "debug"), 0);
    }

    G_SUBTEST << "Flag does not exist";
    {
        std::map<std::string, int> flags = {{"verbose", 1}};
        EXPECT_EQ(getFlag(&flags, "nonexistent"), 0);
    }

    G_SUBTEST << "Empty map";
    {
        std::map<std::string, int> flags;
        EXPECT_EQ(getFlag(&flags, "verbose"), 0);
    }

    G_SUBTEST << "Negative flag value";
    {
        std::map<std::string, int> flags = {{"flag", -1}};
        EXPECT_EQ(getFlag(&flags, "flag"), -1);
    }

    G_SUBTEST << "Large flag value";
    {
        std::map<std::string, int> flags = {{"flag", 1000}};
        EXPECT_EQ(getFlag(&flags, "flag"), 1000);
    }

    G_SUBTEST << "Multiple flags";
    {
        std::map<std::string, int> flags = {{"a", 1}, {"b", 2}, {"c", 3}};
        EXPECT_EQ(getFlag(&flags, "a"), 1);
        EXPECT_EQ(getFlag(&flags, "b"), 2);
        EXPECT_EQ(getFlag(&flags, "c"), 3);
    }

    G_SUBTEST << "Empty flag name";
    {
        std::map<std::string, int> flags = {{"", 5}};
        EXPECT_EQ(getFlag(&flags, ""), 5);
    }

    G_SUBTEST << "Long flag name";
    {
        std::string longName(1000, 'a');
        std::map<std::string, int> flags = {{longName, 42}};
        EXPECT_EQ(getFlag(&flags, longName.c_str()), 42);
    }
}

TEST(OptionsFlagsUtilsTest, getOption)
{
    using megacmd::getOption;

    G_SUBTEST << "Basic case";
    {
        std::map<std::string, std::string> options = {{"path", "/tmp"}, {"name", "test"}};
        EXPECT_EQ(getOption(&options, "path"), "/tmp");
        EXPECT_EQ(getOption(&options, "name"), "test");
    }

    G_SUBTEST << "Option does not exist";
    {
        std::map<std::string, std::string> options = {{"path", "/tmp"}};
        EXPECT_EQ(getOption(&options, "nonexistent"), "");
    }

    G_SUBTEST << "Custom default value";
    {
        std::map<std::string, std::string> options = {{"path", "/tmp"}};
        EXPECT_EQ(getOption(&options, "nonexistent", "default"), "default");
    }

    G_SUBTEST << "Empty option value";
    {
        std::map<std::string, std::string> options = {{"empty", ""}};
        EXPECT_EQ(getOption(&options, "empty"), "");
    }

    G_SUBTEST << "Empty map";
    {
        std::map<std::string, std::string> options;
        EXPECT_EQ(getOption(&options, "path"), "");
    }

    G_SUBTEST << "Long option value";
    {
        std::string longValue(1000, 'a');
        std::map<std::string, std::string> options = {{"long", longValue}};
        EXPECT_EQ(getOption(&options, "long"), longValue);
    }

    G_SUBTEST << "Option with special characters";
    {
        std::map<std::string, std::string> options = {{"path", "/tmp/file with spaces.txt"}};
        EXPECT_EQ(getOption(&options, "path"), "/tmp/file with spaces.txt");
    }

    G_SUBTEST << "Option with unicode";
    {
        std::string unicodeValue = u8"\u043F\u0440\u0438\u0432\u0435\u0442";
        std::map<std::string, std::string> options = {{"unicode", unicodeValue}};
        EXPECT_EQ(getOption(&options, "unicode"), unicodeValue);
    }

    G_SUBTEST << "Empty option name";
    {
        std::map<std::string, std::string> options = {{"", "value"}};
        EXPECT_EQ(getOption(&options, ""), "value");
    }

    G_SUBTEST << "Multiple options";
    {
        std::map<std::string, std::string> options = {{"a", "1"}, {"b", "2"}, {"c", "3"}};
        EXPECT_EQ(getOption(&options, "a"), "1");
        EXPECT_EQ(getOption(&options, "b"), "2");
        EXPECT_EQ(getOption(&options, "c"), "3");
    }
}

TEST(OptionsFlagsUtilsTest, getOptionAsOptional)
{
    using megacmd::getOptionAsOptional;

    G_SUBTEST << "Basic case";
    {
        std::map<std::string, std::string> options = {{"path", "/tmp"}};
        auto result = getOptionAsOptional(options, "path");
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, "/tmp");
    }

    G_SUBTEST << "Option does not exist";
    {
        std::map<std::string, std::string> options = {{"path", "/tmp"}};
        auto result = getOptionAsOptional(options, "nonexistent");
        EXPECT_FALSE(result.has_value());
    }

    G_SUBTEST << "Empty map";
    {
        std::map<std::string, std::string> options;
        auto result = getOptionAsOptional(options, "path");
        EXPECT_FALSE(result.has_value());
    }

    G_SUBTEST << "Empty option value";
    {
        std::map<std::string, std::string> options = {{"empty", ""}};
        auto result = getOptionAsOptional(options, "empty");
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, "");
    }

    G_SUBTEST << "Long option value";
    {
        std::string longValue(1000, 'a');
        std::map<std::string, std::string> options = {{"long", longValue}};
        auto result = getOptionAsOptional(options, "long");
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, longValue);
    }

    G_SUBTEST << "Option with special characters";
    {
        std::map<std::string, std::string> options = {{"path", "/tmp/file.txt"}};
        auto result = getOptionAsOptional(options, "path");
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, "/tmp/file.txt");
    }

    G_SUBTEST << "Empty option name";
    {
        std::map<std::string, std::string> options = {{"", "value"}};
        auto result = getOptionAsOptional(options, "");
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, "value");
    }
}

TEST(OptionsFlagsUtilsTest, getintOption)
{
    using megacmd::getintOption;

    G_SUBTEST << "Basic case";
    {
        std::map<std::string, std::string> options = {{"timeout", "30"}, {"port", "8080"}};
        EXPECT_EQ(getintOption(&options, "timeout"), 30);
        EXPECT_EQ(getintOption(&options, "port"), 8080);
    }

    G_SUBTEST << "Invalid integer option";
    {
        std::map<std::string, std::string> options = {{"timeout", "abc"}};
        EXPECT_EQ(getintOption(&options, "timeout"), 0);
    }

    G_SUBTEST << "Option does not exist";
    {
        std::map<std::string, std::string> options = {{"timeout", "30"}};
        EXPECT_EQ(getintOption(&options, "nonexistent"), 0);
    }

    G_SUBTEST << "Custom default value";
    {
        std::map<std::string, std::string> options = {{"timeout", "30"}};
        EXPECT_EQ(getintOption(&options, "nonexistent", 42), 42);
    }

    G_SUBTEST << "Negative integer";
    {
        std::map<std::string, std::string> options = {{"value", "-10"}};
        EXPECT_EQ(getintOption(&options, "value"), -10);
    }

    G_SUBTEST << "Zero value";
    {
        std::map<std::string, std::string> options = {{"value", "0"}};
        EXPECT_EQ(getintOption(&options, "value"), 0);
    }

    G_SUBTEST << "INT_MAX value";
    {
        std::map<std::string, std::string> options = {{"value", "2147483647"}};
        EXPECT_EQ(getintOption(&options, "value"), INT_MAX);
    }

    G_SUBTEST << "INT_MIN value";
    {
        std::map<std::string, std::string> options = {{"value", "-2147483648"}};
        EXPECT_EQ(getintOption(&options, "value"), INT_MIN);
    }

    G_SUBTEST << "Number exceeding INT_MAX";
    {
        std::map<std::string, std::string> options = {{"value", "2147483648"}};
        EXPECT_EQ(getintOption(&options, "value"), 0);
    }

    G_SUBTEST << "Number below INT_MIN";
    {
        std::map<std::string, std::string> options = {{"value", "-2147483649"}};
        EXPECT_EQ(getintOption(&options, "value"), 0);
    }

    G_SUBTEST << "Integer with leading spaces";
    {
        std::map<std::string, std::string> options = {{"value", "  123"}};
        EXPECT_EQ(getintOption(&options, "value"), 123);
    }

    G_SUBTEST << "Integer with trailing spaces";
    {
        std::map<std::string, std::string> options = {{"value", "123  "}};
        EXPECT_EQ(getintOption(&options, "value"), 123);
    }

    G_SUBTEST << "Integer with spaces on both sides";
    {
        std::map<std::string, std::string> options = {{"value", "  123  "}};
        EXPECT_EQ(getintOption(&options, "value"), 123);
    }

    G_SUBTEST << "Partial integer string";
    {
        std::map<std::string, std::string> options = {{"value", "123abc"}};
        EXPECT_EQ(getintOption(&options, "value"), 123);
    }

    G_SUBTEST << "Empty string";
    {
        std::map<std::string, std::string> options = {{"value", ""}};
        EXPECT_EQ(getintOption(&options, "value"), 0);
    }

    G_SUBTEST << "Empty map";
    {
        std::map<std::string, std::string> options;
        EXPECT_EQ(getintOption(&options, "value"), 0);
    }

    G_SUBTEST << "Custom default with invalid option";
    {
        std::map<std::string, std::string> options = {{"value", "invalid"}};
        EXPECT_EQ(getintOption(&options, "value", 99), 99);
    }

    G_SUBTEST << "Custom default with empty string";
    {
        std::map<std::string, std::string> options = {{"value", ""}};
        EXPECT_EQ(getintOption(&options, "value", 99), 99);
    }

    G_SUBTEST << "Custom default with spaces";
    {
        std::map<std::string, std::string> options = {{"value", "  123  "}};
        EXPECT_EQ(getintOption(&options, "value", 99), 123);
    }

    G_SUBTEST << "Very large int number string";
    {
        std::map<std::string, std::string> options = {{"value", "999999999999999999999"}};
        EXPECT_EQ(getintOption(&options, "value"), 0);
    }

    G_SUBTEST << "Very large negative int number string";
    {
        std::map<std::string, std::string> options = {{"value", "-999999999999999999999"}};
        EXPECT_EQ(getintOption(&options, "value"), 0);
    }

    G_SUBTEST << "String with only spaces";
    {
        std::map<std::string, std::string> options = {{"value", "   "}};
        EXPECT_EQ(getintOption(&options, "value"), 0);
    }

    G_SUBTEST << "String starting with non-digit";
    {
        std::map<std::string, std::string> options = {{"value", "abc123"}};
        EXPECT_EQ(getintOption(&options, "value"), 0);
    }

    G_SUBTEST << "String with special characters";
    {
        std::map<std::string, std::string> options = {{"value", "12@34"}};
        EXPECT_EQ(getintOption(&options, "value"), 12);
    }
}

TEST(OptionsFlagsUtilsTest, getIntOptional)
{
    using megacmd::getIntOptional;

    G_SUBTEST << "Basic case";
    {
        std::map<std::string, std::string> options = {{"timeout", "30"}, {"port", "8080"}};
        auto result = getIntOptional(options, "timeout");
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, 30);
        result = getIntOptional(options, "port");
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, 8080);
    }

    G_SUBTEST << "Invalid integer option";
    {
        std::map<std::string, std::string> options = {{"timeout", "abc"}};
        auto result = getIntOptional(options, "timeout");
        EXPECT_FALSE(result.has_value());
    }

    G_SUBTEST << "Option does not exist";
    {
        std::map<std::string, std::string> options = {{"timeout", "30"}};
        auto result = getIntOptional(options, "nonexistent");
        EXPECT_FALSE(result.has_value());
    }

    G_SUBTEST << "Empty map";
    {
        std::map<std::string, std::string> options;
        auto result = getIntOptional(options, "timeout");
        EXPECT_FALSE(result.has_value());
    }

    G_SUBTEST << "Negative integer";
    {
        std::map<std::string, std::string> options = {{"value", "-10"}};
        auto result = getIntOptional(options, "value");
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, -10);
    }

    G_SUBTEST << "Zero value";
    {
        std::map<std::string, std::string> options = {{"value", "0"}};
        auto result = getIntOptional(options, "value");
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, 0);
    }

    G_SUBTEST << "INT_MAX value";
    {
        std::map<std::string, std::string> options = {{"value", "2147483647"}};
        auto result = getIntOptional(options, "value");
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, INT_MAX);
    }

    G_SUBTEST << "INT_MIN value";
    {
        std::map<std::string, std::string> options = {{"value", "-2147483648"}};
        auto result = getIntOptional(options, "value");
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, INT_MIN);
    }

    G_SUBTEST << "Number exceeding INT_MAX";
    {
        std::map<std::string, std::string> options = {{"value", "2147483648"}};
        auto result = getIntOptional(options, "value");
        EXPECT_FALSE(result.has_value());
    }

    G_SUBTEST << "Number below INT_MIN";
    {
        std::map<std::string, std::string> options = {{"value", "-2147483649"}};
        auto result = getIntOptional(options, "value");
        EXPECT_FALSE(result.has_value());
    }

    G_SUBTEST << "Partial integer string";
    {
        std::map<std::string, std::string> options = {{"value", "123abc"}};
        auto result = getIntOptional(options, "value");
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, 123);
    }

    G_SUBTEST << "Empty string";
    {
        std::map<std::string, std::string> options = {{"value", ""}};
        auto result = getIntOptional(options, "value");
        EXPECT_FALSE(result.has_value());
    }

    G_SUBTEST << "Integer with leading spaces";
    {
        std::map<std::string, std::string> options = {{"value", "  123"}};
        auto result = getIntOptional(options, "value");
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, 123);
    }

    G_SUBTEST << "Integer with trailing spaces";
    {
        std::map<std::string, std::string> options = {{"value", "123  "}};
        auto result = getIntOptional(options, "value");
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, 123);
    }

    G_SUBTEST << "Integer with spaces on both sides";
    {
        std::map<std::string, std::string> options = {{"value", "  123  "}};
        auto result = getIntOptional(options, "value");
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, 123);
    }

    G_SUBTEST << "Very large number string";
    {
        std::map<std::string, std::string> options = {{"value", "999999999999999999999"}};
        auto result = getIntOptional(options, "value");
        EXPECT_FALSE(result.has_value());
    }

    G_SUBTEST << "Very large negative number string";
    {
        std::map<std::string, std::string> options = {{"value", "-999999999999999999999"}};
        auto result = getIntOptional(options, "value");
        EXPECT_FALSE(result.has_value());
    }

    G_SUBTEST << "String with only spaces";
    {
        std::map<std::string, std::string> options = {{"value", "   "}};
        auto result = getIntOptional(options, "value");
        EXPECT_FALSE(result.has_value());
    }

    G_SUBTEST << "String starting with non-digit";
    {
        std::map<std::string, std::string> options = {{"value", "abc123"}};
        auto result = getIntOptional(options, "value");
        EXPECT_FALSE(result.has_value());
    }

    G_SUBTEST << "String with special characters";
    {
        std::map<std::string, std::string> options = {{"value", "12@34"}};
        auto result = getIntOptional(options, "value");
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, 12);
    }

    G_SUBTEST << "Positive number with plus sign";
    {
        std::map<std::string, std::string> options = {{"value", "+123"}};
        auto result = getIntOptional(options, "value");
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, 123);
    }

    G_SUBTEST << "Single digit";
    {
        std::map<std::string, std::string> options = {{"value", "5"}};
        auto result = getIntOptional(options, "value");
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, 5);
    }

    G_SUBTEST << "Large positive number within range";
    {
        std::map<std::string, std::string> options = {{"value", "1000000"}};
        auto result = getIntOptional(options, "value");
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, 1000000);
    }

    G_SUBTEST << "Large negative number within range";
    {
        std::map<std::string, std::string> options = {{"value", "-1000000"}};
        auto result = getIntOptional(options, "value");
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, -1000000);
    }
}

TEST(OptionsFlagsUtilsTest, discardOptionsAndFlags)
{
    using megacmd::discardOptionsAndFlags;

    G_SUBTEST << "Basic case";
    {
        std::vector<std::string> words = {"cmd", "-v", "--path=/tmp", "arg1"};
        discardOptionsAndFlags(&words);
        std::vector<std::string> expected = {"cmd", "arg1"};
        EXPECT_EQ(words, expected);
    }

    G_SUBTEST << "No options or flags";
    {
        std::vector<std::string> words = {"cmd", "arg1", "arg2"};
        discardOptionsAndFlags(&words);
        std::vector<std::string> expected = {"cmd", "arg1", "arg2"};
        EXPECT_EQ(words, expected);
    }

    G_SUBTEST << "Only options and flags";
    {
        std::vector<std::string> words = {"-v", "--path=/tmp"};
        discardOptionsAndFlags(&words);
        std::vector<std::string> expected = {};
        EXPECT_EQ(words, expected);
    }

    G_SUBTEST << "Empty vector";
    {
        std::vector<std::string> words;
        discardOptionsAndFlags(&words);
        std::vector<std::string> expected = {};
        EXPECT_EQ(words, expected);
    }

    G_SUBTEST << "Single dash";
    {
        std::vector<std::string> words = {"cmd", "-", "arg1"};
        discardOptionsAndFlags(&words);
        std::vector<std::string> expected = {"cmd", "arg1"};
        EXPECT_EQ(words, expected);
    }

    G_SUBTEST << "Double dash flag";
    {
        std::vector<std::string> words = {"cmd", "--flag", "arg1"};
        discardOptionsAndFlags(&words);
        std::vector<std::string> expected = {"cmd", "arg1"};
        EXPECT_EQ(words, expected);
    }

    G_SUBTEST << "Multiple flags";
    {
        std::vector<std::string> words = {"cmd", "-a", "-b", "-c", "arg1"};
        discardOptionsAndFlags(&words);
        std::vector<std::string> expected = {"cmd", "arg1"};
        EXPECT_EQ(words, expected);
    }

    G_SUBTEST << "Mixed flags and options";
    {
        std::vector<std::string> words = {"cmd", "-v", "--path=/tmp", "--name=test", "arg1", "arg2"};
        discardOptionsAndFlags(&words);
        std::vector<std::string> expected = {"cmd", "arg1", "arg2"};
        EXPECT_EQ(words, expected);
    }

    G_SUBTEST << "Flags at the end";
    {
        std::vector<std::string> words = {"cmd", "arg1", "-v"};
        discardOptionsAndFlags(&words);
        std::vector<std::string> expected = {"cmd", "arg1"};
        EXPECT_EQ(words, expected);
    }

    G_SUBTEST << "Flags at the beginning";
    {
        std::vector<std::string> words = {"-v", "--path=/tmp", "cmd", "arg1"};
        discardOptionsAndFlags(&words);
        std::vector<std::string> expected = {"cmd", "arg1"};
        EXPECT_EQ(words, expected);
    }

    G_SUBTEST << "Consecutive flags";
    {
        std::vector<std::string> words = {"cmd", "-a", "-b", "-c", "arg1"};
        discardOptionsAndFlags(&words);
        std::vector<std::string> expected = {"cmd", "arg1"};
        EXPECT_EQ(words, expected);
    }

    G_SUBTEST << "Long flag name";
    {
        std::vector<std::string> words = {"cmd", "--very-long-flag-name", "arg1"};
        discardOptionsAndFlags(&words);
        std::vector<std::string> expected = {"cmd", "arg1"};
        EXPECT_EQ(words, expected);
    }

    G_SUBTEST << "Option with empty value";
    {
        std::vector<std::string> words = {"cmd", "--option=", "arg1"};
        discardOptionsAndFlags(&words);
        std::vector<std::string> expected = {"cmd", "arg1"};
        EXPECT_EQ(words, expected);
    }

    G_SUBTEST << "Only command";
    {
        std::vector<std::string> words = {"cmd"};
        discardOptionsAndFlags(&words);
        std::vector<std::string> expected = {"cmd"};
        EXPECT_EQ(words, expected);
    }
}
