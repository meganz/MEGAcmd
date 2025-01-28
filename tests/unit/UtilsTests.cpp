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

#include "megacmdutils.h"
#include <cstring>
#include <cerrno>
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

namespace UtilsTest
{
    static void numberOfDigits()
    {
        {
            G_SUBTEST << "Positive integers";
            ASSERT_EQ(megacmd::numberOfDigits(1), 1);
            ASSERT_EQ(megacmd::numberOfDigits(10), 2);
            ASSERT_EQ(megacmd::numberOfDigits(100), 3);
        }
        {
            G_SUBTEST << "Negative integers";
            ASSERT_EQ(megacmd::numberOfDigits(-1), 2);
            ASSERT_EQ(megacmd::numberOfDigits(-10), 3);
            ASSERT_EQ(megacmd::numberOfDigits(-100), 4);
        }
    }
}

TEST(UtilsTest, numberOfDigits)
{
    UtilsTest::numberOfDigits();
}

TEST(UtilsTest, getListOfWords)
{
    {
            G_SUBTEST << "Simple";
            std::vector<std::string> words = megacmd::getlistOfWords("mega-share -a --with=some-email@mega.co.nz some_dir/some_file.txt");
            ASSERT_THAT(words, testing::ElementsAre("mega-share", "-a", "--with=some-email@mega.co.nz", "some_dir/some_file.txt"));
    }

    {
            G_SUBTEST << "Double quotes";
            std::vector<std::string> words = megacmd::getlistOfWords("mkdir \"some dir\" \"another dir\"");
            ASSERT_THAT(words, testing::ElementsAre("mkdir", "some dir", "another dir"));
    }

    {
            G_SUBTEST << "Double quotes without matching";
            std::vector<std::string> words = megacmd::getlistOfWords("find \"something with quotes\" odd/file\"01.txt");
            ASSERT_THAT(words, testing::ElementsAre("find", "something with quotes", "odd/file\"01.txt"));
    }

    {
            G_SUBTEST << "Single quotes";
            std::vector<std::string> words = megacmd::getlistOfWords("mkdir 'some dir' 'another dir'");
            ASSERT_THAT(words, testing::ElementsAre("mkdir", "some dir", "another dir"));
    }

    {
            G_SUBTEST << "Single quotes without matching";
            std::vector<std::string> words = megacmd::getlistOfWords("find 'something with quotes' odd/file'01.txt");
            ASSERT_THAT(words, testing::ElementsAre("find", "something with quotes", "odd/file'01.txt"));
    }

    {
        G_SUBTEST << "Escape backslash in completion";
        const char* str = "get root_dir\\some_dir\\some_file.jpg another_file.txt";
        const char* completion_str = "completion get root_dir\\some_dir\\some_file.jpg another_file.txt";

        // '\\' won't be replaced since the first word is not `completion`
        std::vector<std::string> words = megacmd::getlistOfWords(str, true);
        EXPECT_THAT(words, testing::ElementsAre("get", "root_dir\\some_dir\\some_file.jpg", "another_file.txt"));

        // '\\' will be replaced with '\\\\' since the first word is `completion`
        words = megacmd::getlistOfWords(completion_str, true);
        EXPECT_THAT(words, testing::ElementsAre("completion", "get", "root_dir\\\\some_dir\\\\some_file.jpg", "another_file.txt"));

        // '\\' won't be replaced since we're setting `escapeBackSlashInCompletion=false`
        words = megacmd::getlistOfWords(str, false);
        EXPECT_THAT(words, testing::ElementsAre("get", "root_dir\\some_dir\\some_file.jpg", "another_file.txt"));
    }

    {
        G_SUBTEST << "Ignore trailing spaces";
        const char* str = "export -f --writable some_dir/some_file.txt    ";

        // Do not ignore trailing spaces
        std::vector<std::string> words = megacmd::getlistOfWords(str, false, false);
        EXPECT_THAT(words, testing::ElementsAre("export", "-f", "--writable", "some_dir/some_file.txt", ""));

        // Ignore trailing spaces
        words = megacmd::getlistOfWords(str, false, true);
        EXPECT_THAT(words, testing::ElementsAre("export", "-f", "--writable", "some_dir/some_file.txt"));
    }

    {
        G_SUBTEST << "Detect matching quotes";
        for (const auto& [line, expectedWords] : std::vector<std::pair<std::string, std::vector<std::string>>>
            {
                {"some \"case\" here", {"some", "case", "here"}},
                {"--another=\"here\"", {"--another=\"here\""}},
                {"--another=\"here\" with extra bits", {"--another=\"here\"", "with", "extra", "bits"}},
                {"some \"other case with spaces\" here", {"some", "other case with spaces", "here"}},
                {"--something=\"quote missing", {"--something=\"quote missing"}},
                {"--nothing=\"", {"--nothing=\""}},
                {"--arg1=\"a b\" word --arg2=\"c d\"", {"--arg1=\"a b\"", "word", "--arg2=\"c d\""}}
            })
        {
            std::vector<std::string> words = megacmd::getlistOfWords(const_cast<char*>(line.c_str()));
            EXPECT_EQ(words, expectedWords);
        }
    }
}

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
