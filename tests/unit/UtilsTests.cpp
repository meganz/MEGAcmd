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
#include <fstream>
#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
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

TEST(UtilsTest, canWrite)
{
    using megacmd::canWrite;

    G_SUBTEST << "Writable directory";
    {
        SelfDeletingTmpFolder tmpFolder;
        EXPECT_TRUE(canWrite(tmpFolder.string()));
    }

    G_SUBTEST << "Non-existent path";
    {
        EXPECT_FALSE(canWrite("/non/existent/path/12345"));
    }

    G_SUBTEST << "Empty string";
    {
        EXPECT_FALSE(canWrite(""));
    }

    G_SUBTEST << "File path";
    {
        SelfDeletingTmpFolder tmpFolder;
        fs::path filePath = tmpFolder.path() / "test.txt";
        std::ofstream file(filePath);
        file << "test";
        file.close();
        EXPECT_TRUE(canWrite(filePath.string()));
    }

    G_SUBTEST << "Path with trailing slash";
    {
        SelfDeletingTmpFolder tmpFolder;
        std::string pathWithSlash = tmpFolder.string() + "/";
        EXPECT_TRUE(canWrite(pathWithSlash));
    }

    G_SUBTEST << "Path with multiple slashes";
    {
        SelfDeletingTmpFolder tmpFolder;
        fs::path testDir = tmpFolder.path() / "test";
        fs::create_directory(testDir);
        std::string pathWithMultipleSlashes = tmpFolder.string() + "//test//";
        EXPECT_TRUE(canWrite(pathWithMultipleSlashes));
    }

    G_SUBTEST << "Path with relative components";
    {
        SelfDeletingTmpFolder tmpFolder;
        fs::path subDir = tmpFolder.path() / "subdir";
        fs::create_directory(subDir);
        std::string relativePath = (subDir / ".." / "subdir").string();
        EXPECT_TRUE(canWrite(relativePath));
    }

    G_SUBTEST << "Path with only spaces";
    {
        EXPECT_FALSE(canWrite("   "));
    }

    G_SUBTEST << "Path with single dot";
    {
        SelfDeletingTmpFolder tmpFolder;
        fs::path dotPath = tmpFolder.path() / ".";
        EXPECT_TRUE(canWrite(dotPath.string()));
    }

    G_SUBTEST << "Path with double dots";
    {
        SelfDeletingTmpFolder tmpFolder;
        fs::path subDir = tmpFolder.path() / "subdir";
        fs::create_directory(subDir);
        fs::path doubleDotPath = subDir / "..";
        EXPECT_TRUE(canWrite(doubleDotPath.string()));
    }

    G_SUBTEST << "Very long path";
    {
        SelfDeletingTmpFolder tmpFolder;
        std::string longPath = tmpFolder.string();
        longPath += "/" + std::string(200, 'a');
        fs::create_directories(longPath);
        EXPECT_TRUE(canWrite(longPath));
    }

    G_SUBTEST << "Path with special characters";
    {
        SelfDeletingTmpFolder tmpFolder;
        fs::path specialPath = tmpFolder.path() / "test@#$%^&()_+-=[]{};',.";
        fs::create_directory(specialPath);
        EXPECT_TRUE(canWrite(specialPath.string()));
    }

    G_SUBTEST << "Read-only directory";
    {
        SelfDeletingTmpFolder tmpFolder;
        fs::path readOnlyDir = tmpFolder.path() / "readonly";
        fs::create_directory(readOnlyDir);
#ifdef _WIN32
        std::wstring wpath = readOnlyDir.wstring();
        DWORD oldAttrs = GetFileAttributesW(wpath.c_str());
        SetFileAttributesW(wpath.c_str(), FILE_ATTRIBUTE_READONLY);
        EXPECT_FALSE(canWrite(readOnlyDir.string()));
        SetFileAttributesW(wpath.c_str(), oldAttrs);
#else
        chmod(readOnlyDir.c_str(), 0555);
        EXPECT_FALSE(canWrite(readOnlyDir.string()));
        chmod(readOnlyDir.c_str(), 0755);
#endif
    }

    G_SUBTEST << "Read-only file";
    {
        SelfDeletingTmpFolder tmpFolder;
        fs::path readOnlyFile = tmpFolder.path() / "readonly.txt";
        std::ofstream file(readOnlyFile);
        file << "test";
        file.close();
#ifdef _WIN32
        std::wstring wpath = readOnlyFile.wstring();
        DWORD oldAttrs = GetFileAttributesW(wpath.c_str());
        SetFileAttributesW(wpath.c_str(), FILE_ATTRIBUTE_READONLY);
        EXPECT_FALSE(canWrite(readOnlyFile.string()));
        SetFileAttributesW(wpath.c_str(), oldAttrs);
#else
        chmod(readOnlyFile.c_str(), 0444);
        EXPECT_FALSE(canWrite(readOnlyFile.string()));
        chmod(readOnlyFile.c_str(), 0644);
#endif
    }

    G_SUBTEST << "Symlink to writable directory";
    {
        SelfDeletingTmpFolder tmpFolder;
        fs::path targetDir = tmpFolder.path() / "target";
        fs::create_directory(targetDir);
        fs::path symlinkPath = tmpFolder.path() / "symlink";
        fs::create_symlink(targetDir, symlinkPath);
        EXPECT_TRUE(canWrite(symlinkPath.string()));
    }

    G_SUBTEST << "Symlink to read-only directory";
    {
        SelfDeletingTmpFolder tmpFolder;
        fs::path targetDir = tmpFolder.path() / "target";
        fs::create_directory(targetDir);
#ifndef _WIN32
        chmod(targetDir.c_str(), 0555);
        fs::path symlinkPath = tmpFolder.path() / "symlink";
        fs::create_symlink(targetDir, symlinkPath);
        EXPECT_FALSE(canWrite(symlinkPath.string()));
        chmod(targetDir.c_str(), 0755);
#endif
    }

    G_SUBTEST << "Nested directory structure";
    {
        SelfDeletingTmpFolder tmpFolder;
        fs::path nestedPath = tmpFolder.path() / "level1" / "level2" / "level3";
        fs::create_directories(nestedPath);
        EXPECT_TRUE(canWrite(nestedPath.string()));
    }

    G_SUBTEST << "Directory with UTF-8 characters";
    {
        SelfDeletingTmpFolder tmpFolder;
        std::string utf8DirName = u8"\u041f\u0440\u0438\u0432\u0456\u0442"; // Ukrainian
        fs::path utf8Path = tmpFolder.path() / utf8DirName;
        fs::create_directory(utf8Path);
#ifdef _WIN32
        std::string utf8PathStr = megacmd::pathAsUtf8(utf8Path);
#else
        std::string utf8PathStr = utf8Path.string();
#endif
        EXPECT_TRUE(canWrite(utf8PathStr));
    }

    G_SUBTEST << "Nested directory with UTF-8 characters";
    {
        SelfDeletingTmpFolder tmpFolder;
        std::string utf8DirName = u8"\u4e2d\u6587"; // Chinese
        fs::path utf8Path = tmpFolder.path() / utf8DirName / "subdir";
        fs::create_directories(utf8Path);
#ifdef _WIN32
        std::string utf8PathStr = megacmd::pathAsUtf8(utf8Path);
#else
        std::string utf8PathStr = utf8Path.string();
#endif
        EXPECT_TRUE(canWrite(utf8PathStr));
    }
}
