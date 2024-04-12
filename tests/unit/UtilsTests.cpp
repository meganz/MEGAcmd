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
            std::vector<std::string> words = megacmd::getlistOfWords(const_cast<char*>("mega-share -a --with=some-email@mega.co.nz some_dir/some_file.txt"));
            ASSERT_THAT(words, testing::ElementsAre("mega-share", "-a", "--with=some-email@mega.co.nz", "some_dir/some_file.txt"));
    }

    {
            G_SUBTEST << "Double quotes";
            std::vector<std::string> words = megacmd::getlistOfWords(const_cast<char*>("mkdir \"some dir\" \"another dir\""));
            ASSERT_THAT(words, testing::ElementsAre("mkdir", "some dir", "another dir"));
    }

    {
            G_SUBTEST << "Double quotes without matching";
            std::vector<std::string> words = megacmd::getlistOfWords(const_cast<char*>("find \"something with quotes\" odd/file\"01.txt"));
            ASSERT_THAT(words, testing::ElementsAre("find", "something with quotes", "odd/file\"01.txt"));
    }

    {
            G_SUBTEST << "Single quotes";
            std::vector<std::string> words = megacmd::getlistOfWords(const_cast<char*>("mkdir 'some dir' 'another dir'"));
            ASSERT_THAT(words, testing::ElementsAre("mkdir", "some dir", "another dir"));
    }

    {
            G_SUBTEST << "Single quotes without matching";
            std::vector<std::string> words = megacmd::getlistOfWords(const_cast<char*>("find 'something with quotes' odd/file'01.txt"));
            ASSERT_THAT(words, testing::ElementsAre("find", "something with quotes", "odd/file'01.txt"));
    }

    {
        G_SUBTEST << "Escape backslash in completion";
        auto str = const_cast<char*>("get root_dir\\some_dir\\some_file.jpg another_file.txt");
        auto completion_str = const_cast<char*>("completion get root_dir\\some_dir\\some_file.jpg another_file.txt");

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
        auto str = const_cast<char*>("export -f --writable some_dir/some_file.txt    ");

        // Do not ignore trailing spaces
        std::vector<std::string> words = megacmd::getlistOfWords(str, false, false);
        EXPECT_THAT(words, testing::ElementsAre("export", "-f", "--writable", "some_dir/some_file.txt", ""));

        // Ignore trailing spaces
        words = megacmd::getlistOfWords(str, false, true);
        EXPECT_THAT(words, testing::ElementsAre("export", "-f", "--writable", "some_dir/some_file.txt"));
    }
}

TEST(UtilsTest, pathIsExistingDirValidDirPath)
{
    std::unique_ptr<char[]> buf;
#ifdef _WIN32
    buf.reset(_getcwd(nullptr, 0));
#else
    buf.reset(getcwd(nullptr, 0));
#endif
    ASSERT_THAT(buf, testing::NotNull()) << "could not get current working directory: " << std::strerror(errno);

    EXPECT_THAT(buf.get(), testing::ResultOf(::megacmd::pathIsExistingDir, testing::IsTrue()));
}

TEST(UtilsTest, pathIsExistingDirInvalidPath)
{
    EXPECT_FALSE(::megacmd::pathIsExistingDir("/path/to/invalid/dir"));
}

TEST(UtilsTest, pathIsExistingDirFilePath)
{
#ifdef _WIN32
    TCHAR u16Path[MAX_PATH];
    std::string u8Path;

    ASSERT_TRUE(SUCCEEDED(GetModuleFileName(nullptr, u16Path, MAX_PATH)));
    megacmd::utf16ToUtf8(u16Path, lstrlen(u16Path), &u8Path);

    ASSERT_THAT(u8Path, testing::Not(testing::IsEmpty()));
    EXPECT_THAT(u8Path, testing::ResultOf(::megacmd::pathIsExistingDir, testing::IsTrue()));
#else
    EXPECT_FALSE(::megacmd::pathIsExistingDir("/dev/null"));
#endif
}
