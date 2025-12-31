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

#include <cctype>
#include <cerrno>
#include <cstring>
#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "TestUtils.h"
#include "megacmdcommonutils.h"

TEST(UtilsTest, nonAsciiConsolePrint)
{
    // No need to check the output, we just want to ensure
    // the test doesn't crash and no asserts are triggered

    const char* char_str = u8"\uc548\uc548\ub155\ud558\uc138\uc694\uc138\uacc4";
    std::cout << "something before" << char_str << std::endl;
    std::cout << char_str << std::endl;
    std::cout << char_str << "something after" << std::endl;
    std::cerr << "something before" << char_str << std::endl;
    std::cerr << char_str << std::endl;
    std::cerr << char_str << "something after" << std::endl;

    const std::string str = u8"\u3053\u3093\u306b\u3061\u306f\u4e16\u754c";
    std::cout << "something before" << str << std::endl;
    std::cout << str << std::endl;
    std::cout << str << "something after" << std::endl;
    std::cerr << "something before" << str << std::endl;
    std::cerr << str << std::endl;
    std::cerr << str << "something after" << std::endl;

#ifdef _WIN32
    const wchar_t* wchar_str = L"\uc548\uc548\ub155\ud558\uc138\uc694\uc138\uacc4";
    std::wcout << "something before" << wchar_str << std::endl;
    std::wcout << wchar_str << std::endl;
    std::wcout << wchar_str << "something after" << std::endl;
    std::wcerr << "something before" << wchar_str << std::endl;
    std::wcerr << wchar_str << std::endl;
    std::wcerr << wchar_str << "something after" << std::endl;

    const std::wstring wstr = L"\u3053\u3093\u306b\u3061\u306f\u4e16\u754c";
    std::wcout << "something before" << wstr << std::endl;
    std::wcout << wstr << std::endl;
    std::wcout << wstr << "something after" << std::endl;
    std::wcerr << "something before" << wstr << std::endl;
    std::wcerr << wstr << std::endl;
    std::wcerr << wstr << "something after" << std::endl;

    std::wcout << megacmd::utf8StringToUtf16WString(char_str) << std::endl;
    std::wcerr << megacmd::utf8StringToUtf16WString(char_str) << std::endl;
    std::wcout << megacmd::utf8StringToUtf16WString(str.data(), str.size()) << std::endl;
    std::wcerr << megacmd::utf8StringToUtf16WString(str.data(), str.size()) << std::endl;

    std::cout << megacmd::utf16ToUtf8(wchar_str) << std::endl;
    std::cerr << megacmd::utf16ToUtf8(wchar_str) << std::endl;
    std::cout << megacmd::utf16ToUtf8(wstr) << std::endl;
    std::cerr << megacmd::utf16ToUtf8(wstr) << std::endl;
#endif
}

TEST(StringUtilsTest, generateRandomAlphaNumericString)
{
    using megacmd::generateRandomAlphaNumericString;

    G_SUBTEST << "Generate string of specific length";
    {
        std::string result = generateRandomAlphaNumericString(10);
        EXPECT_EQ(result.length(), 10);
    }

    G_SUBTEST << "Generate empty string";
    {
        std::string result = generateRandomAlphaNumericString(0);
        EXPECT_EQ(result.length(), 0);
    }

    G_SUBTEST << "Generate long string";
    {
        std::string result = generateRandomAlphaNumericString(100);
        EXPECT_EQ(result.length(), 100);
    }

    G_SUBTEST << "Generated strings are different";
    {
        std::string result1 = generateRandomAlphaNumericString(20);
        std::string result2 = generateRandomAlphaNumericString(20);
        EXPECT_NE(result1, result2);
    }

    G_SUBTEST << "Generated string contains alphanumeric characters";
    {
        std::string result = generateRandomAlphaNumericString(1000);
        for (char c : result)
        {
            EXPECT_TRUE(std::isalnum(c));
        }
    }
}

TEST(UtilsTest, getNumberOfCols)
{
    using megacmd::getNumberOfCols;

    G_SUBTEST << "Base case";
    {
        auto cols = getNumberOfCols();
        EXPECT_GT(cols, 0u);
    }

    G_SUBTEST << "With default value";
    {
        auto cols = getNumberOfCols(80);
        EXPECT_GT(cols, 0u);
    }

    G_SUBTEST << "Multiple calls return consistent results";
    {
        auto cols1 = getNumberOfCols(80);
        auto cols2 = getNumberOfCols(80);
        EXPECT_EQ(cols1, cols2);
    }

    G_SUBTEST << "Result is reasonable size";
    {
        unsigned int cols = getNumberOfCols();
        EXPECT_LE(cols, 10000u);
    }
}
