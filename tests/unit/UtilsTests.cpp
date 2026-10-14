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
#include <sstream>
#include <string_view>
#include <vector>
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
#include "Instruments.h"

namespace fs = std::filesystem;

// Returns a list of path variants for testing:
// 1. Original path
// 2. Kernel format (\\?\ prefix) on Windows
// 3. UNC format on Windows
std::vector<std::string> pathVariants(const std::string& path)
{
    std::vector<std::string> variants{path};

#ifdef _WIN32
    std::string_view pathView{path};
    bool isKernel = pathView.size() >= 4 && pathView.substr(0, 4) == R"(\\?\)";
    bool isKernelWithUnc = pathView.size() >= 8 && pathView.substr(0, 8) == R"(\\?\UNC\)";
    bool isUnc = pathView.size() >= 2 && pathView.substr(0, 2) == R"(\\)" && !isKernel;

    if (isKernelWithUnc)
    {
        std::string uncPath = R"(\\)" + std::string(pathView.substr(8));
        variants.push_back(uncPath);
        variants.push_back(path);
    }
    else if (isUnc)
    {
        std::string kernelUncPath = R"(\\?\UNC\)" + std::string(pathView.substr(2));
        variants.push_back(kernelUncPath);
        variants.push_back(path);
    }
    else if (isKernel)
    {
        std::string localPath = std::string(pathView.substr(4));
        variants.push_back(localPath);

        if (localPath.size() >= 2 && localPath[1] == ':')
        {
            std::string uncPath = R"(\\localhost\)" + localPath.substr(0, 1) + R"($\)" + localPath.substr(3);
            variants.push_back(uncPath);
        }
        else
        {
            variants.push_back(path);
        }
    }
    else
    {
        std::string kernelPath = R"(\\?\)" + path;
        variants.push_back(kernelPath);

        if (pathView.size() >= 2 && pathView[1] == ':')
        {
            std::string uncPath = R"(\\localhost\)" + path.substr(0, 1) + R"($\)";
            if (pathView.size() > 3)
            {
                uncPath += pathView.substr(3);
            }
            variants.push_back(uncPath);
        }
        else
        {
            variants.push_back(kernelPath); // duplicate
        }
    }
#endif

    return variants;
}

void testCanWriteAllPathVariants(const std::string& path, bool expectedResult)
{
    using megacmd::canWrite;
    auto variants = pathVariants(path);

#ifdef _WIN32
    ASSERT_EQ(variants.size(), 3);
#else
    ASSERT_EQ(variants.size(), 1);
#endif

    for (const auto& variant : variants)
    {
        EXPECT_EQ(canWrite(variant), expectedResult) << "Path: " << variant;
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

TEST(UtilsTest, buildProgressBarLine)
{
    using megacmd::buildProgressBarLine;

    const char *title = "TRANSFERRING ||";
    const char *suffix = "||(512/1024 KB:  50.00 %) ";
    const size_t titleLen = strlen(title);
    const size_t suffixLen = strlen(suffix);

    G_SUBTEST << "Normal terminal width";
    {
        const auto line = buildProgressBarLine(title, suffix, 50.0f, 80);
        EXPECT_EQ(line.size(), 80u);
        EXPECT_EQ(line.compare(0, titleLen, title), 0);
        EXPECT_EQ(line.compare(80 - suffixLen, suffixLen, suffix), 0);
        EXPECT_NE(line.find('#'), std::string::npos);
    }

    G_SUBTEST << "Zero percent draws no bar";
    {
        const auto line = buildProgressBarLine(title, suffix, 0.0f, 80);
        EXPECT_EQ(line.size(), 80u);
        EXPECT_EQ(line.find('#'), std::string::npos);
    }

    G_SUBTEST << "Percentages outside [0, 100] are clamped";
    {
        const auto full  = buildProgressBarLine(title, suffix, 100.0f, 80);
        const auto over  = buildProgressBarLine(title, suffix, 1000.0f, 80);
        const auto under = buildProgressBarLine(title, suffix, -50.0f, 80);
        EXPECT_EQ(full, over);
        EXPECT_EQ(under.find('#'), std::string::npos);
    }

    G_SUBTEST << "A full bar does not encroach on the suffix";
    {
        // The old loop bound was inclusive, so at 100% it wrote one cell past the
        // bar and clobbered the first character of the suffix.
        const auto line = buildProgressBarLine(title, suffix, 100.0f, 80);
        EXPECT_EQ(line.compare(80 - suffixLen, suffixLen, suffix), 0);
        EXPECT_EQ(line.find('#'), titleLen);
        EXPECT_EQ(line.rfind('#'), 80 - suffixLen - 1);
    }

    G_SUBTEST << "CMD-894: terminals narrower than title and suffix";
    {
        // getNumberOfCols() returns ws_col - 2, so a 3 column terminal yields 1.
        // Every one of these used to write outside the buffer.
        for (unsigned int cols : {0u, 1u, 2u, 3u, 5u, 10u, 15u, 20u, 38u})
        {
            const auto line = buildProgressBarLine(title, suffix, 50.0f, cols);
            EXPECT_EQ(line.size(), cols) << "cols=" << cols;
        }
    }

    G_SUBTEST << "A too-narrow line keeps the tail of the suffix";
    {
        const auto line = buildProgressBarLine(title, suffix, 50.0f, 4);
        EXPECT_EQ(line.size(), 4u);
        EXPECT_EQ(line, std::string(suffix).substr(suffixLen - 4));
    }

    G_SUBTEST << "Exactly enough room for title and suffix leaves no bar";
    {
        const unsigned int cols = static_cast<unsigned int>(titleLen + suffixLen);
        const auto line = buildProgressBarLine(title, suffix, 100.0f, cols);
        EXPECT_EQ(line.size(), cols);
        EXPECT_EQ(line, std::string(title) + suffix);
    }

    G_SUBTEST << "Empty title and suffix";
    {
        // With neither title nor suffix the whole width is available to the bar.
        const auto line = buildProgressBarLine("", "", 50.0f, 10);
        EXPECT_EQ(line.size(), 10u);
        EXPECT_EQ(line, "#####.....");
    }
}

TEST(UtilsTest, canWrite)
{
    using megacmd::canWrite;

    G_SUBTEST << "Writable directory";
    {
        SelfDeletingTmpFolder tmpFolder;
        testCanWriteAllPathVariants(tmpFolder.string(), true);
    }

    G_SUBTEST << "Non-existent path";
    {
#ifdef _WIN32
        testCanWriteAllPathVariants(R"(C:\non\existent\path\12345)", false);
#else
        testCanWriteAllPathVariants("/non/existent/path/12345", false);
#endif
    }

    G_SUBTEST << "Empty string";
    {
        testCanWriteAllPathVariants("", false);
    }

    G_SUBTEST << "File path";
    {
        SelfDeletingTmpFolder tmpFolder;
        fs::path filePath = tmpFolder.path() / "test.txt";
        std::ofstream file(filePath);
        file << "test";
        file.close();
#ifdef _WIN32
        std::string pathStr = filePath.string();
#else
        std::string pathStr = filePath.string();
#endif
        testCanWriteAllPathVariants(pathStr, true);
    }

    G_SUBTEST << "Path with trailing slash";
    {
        SelfDeletingTmpFolder tmpFolder;
#ifdef _WIN32
        std::string pathWithSlash = tmpFolder.string() + R"(\)";
#else
        std::string pathWithSlash = tmpFolder.string() + "/";
#endif
        testCanWriteAllPathVariants(pathWithSlash, true);
    }

    G_SUBTEST << "Path with only spaces";
    {
#ifdef _WIN32
        testCanWriteAllPathVariants("   ", true); // Edge case on Windows, see canWrite() implementation
#else
        testCanWriteAllPathVariants("   ", false);
#endif
    }

    G_SUBTEST << "Very long path";
    {
        SelfDeletingTmpFolder tmpFolder;
        std::string longPath = tmpFolder.string();
#ifdef _WIN32
        longPath += R"(\)" + std::string(200, 'a');
#else
        longPath += "/" + std::string(200, 'a');
#endif
        EXPECT_TRUE(fs::create_directories(longPath));
        testCanWriteAllPathVariants(longPath, true);
    }

    G_SUBTEST << "Very long path exceeding 255 characters and MAX_PATH";
    {
        SelfDeletingTmpFolder tmpFolder;
        std::string longPath = tmpFolder.string();
#ifdef _WIN32
        longPath += R"(\)" + std::string(300, 'a');
#else
        longPath += "/" + std::string(300, 'a');
#endif
        EXPECT_THROW(fs::create_directories(longPath), std::filesystem::filesystem_error);
        testCanWriteAllPathVariants(longPath, false);
    }

    G_SUBTEST << "Path with special characters";
    {
        SelfDeletingTmpFolder tmpFolder;
        fs::path specialPath = tmpFolder.path() / "test@#$%^&()_+-=[]{};',.";
        EXPECT_TRUE(fs::create_directory(specialPath));
        testCanWriteAllPathVariants(specialPath.string(), true);
    }

    G_SUBTEST << "Read-only directory";
    {
        SelfDeletingTmpFolder tmpFolder;
        fs::path readOnlyDir = tmpFolder.path() / "readonly";
        EXPECT_TRUE(fs::create_directory(readOnlyDir));
#ifdef _WIN32
        // On Windows, FILE_ATTRIBUTE_READONLY on directories is an Explorer
        // folder-customization signal (desktop.ini), not an access control flag.
        // canWrite must ignore it and rely on _waccess() / ACLs instead.
        // See Microsoft KB326549.
        std::wstring wpath = readOnlyDir.wstring();
        DWORD oldAttrs = GetFileAttributesW(wpath.c_str());
        SetFileAttributesW(wpath.c_str(), oldAttrs | FILE_ATTRIBUTE_READONLY);
        testCanWriteAllPathVariants(readOnlyDir.string(), true);
        SetFileAttributesW(wpath.c_str(), oldAttrs);
#else
        chmod(readOnlyDir.c_str(), 0555);
        testCanWriteAllPathVariants(readOnlyDir.string(), false);
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
        testCanWriteAllPathVariants(readOnlyFile.string(), false);
        SetFileAttributesW(wpath.c_str(), oldAttrs);
#else
        chmod(readOnlyFile.c_str(), 0444);
        testCanWriteAllPathVariants(readOnlyFile.string(), false);
        chmod(readOnlyFile.c_str(), 0644);
#endif
    }

    G_SUBTEST << "Symlink to writable directory";
    {
        SelfDeletingTmpFolder tmpFolder;
        fs::path targetDir = tmpFolder.path() / "target";
        EXPECT_TRUE(fs::create_directory(targetDir));
        fs::path symlinkPath = tmpFolder.path() / "symlink";
        fs::create_symlink(targetDir, symlinkPath);
        testCanWriteAllPathVariants(symlinkPath.string(), true);
    }

    G_SUBTEST << "Symlink to read-only directory";
    {
        SelfDeletingTmpFolder tmpFolder;
        fs::path targetDir = tmpFolder.path() / "target";
        EXPECT_TRUE(fs::create_directory(targetDir));
#ifndef _WIN32
        chmod(targetDir.c_str(), 0555);
        fs::path symlinkPath = tmpFolder.path() / "symlink";
        fs::create_symlink(targetDir, symlinkPath);
        testCanWriteAllPathVariants(symlinkPath.string(), false);
        chmod(targetDir.c_str(), 0755);
#endif
    }

    G_SUBTEST << "Nested directory structure";
    {
        SelfDeletingTmpFolder tmpFolder;
        fs::path nestedPath = tmpFolder.path() / "level1" / "level2" / "level3";
        EXPECT_TRUE(fs::create_directories(nestedPath));
        testCanWriteAllPathVariants(nestedPath.string(), true);
    }

    G_SUBTEST << "Directory with UTF-8 characters";
    {
        SelfDeletingTmpFolder tmpFolder;
        std::string utf8DirName = u8"\u041f\u0440\u0438\u0432\u0456\u0442"; // Ukrainian
        fs::path utf8Path = tmpFolder.path() / utf8DirName;
        EXPECT_TRUE(fs::create_directory(utf8Path));
#ifdef _WIN32
        std::string utf8PathStr = megacmd::pathAsUtf8(utf8Path);
#else
        std::string utf8PathStr = utf8Path.string();
#endif
        testCanWriteAllPathVariants(utf8PathStr, true);
    }

    G_SUBTEST << "Nested directory with UTF-8 characters";
    {
        SelfDeletingTmpFolder tmpFolder;
        std::string utf8DirName = u8"\u4e2d\u6587"; // Chinese
        fs::path utf8Path = tmpFolder.path() / utf8DirName / "subdir";
        EXPECT_TRUE(fs::create_directories(utf8Path));
#ifdef _WIN32
        std::string utf8PathStr = megacmd::pathAsUtf8(utf8Path);
#else
        std::string utf8PathStr = utf8Path.string();
#endif
        testCanWriteAllPathVariants(utf8PathStr, true);
    }
}

#ifdef _WIN32
// Regression: FILE_ATTRIBUTE_READONLY on directories is an Explorer
// folder-customization hint (desktop.ini), not an access restriction.
// canWrite() used to check this attribute and return false for directories
// that were perfectly writable. See Microsoft KB326549.
TEST(UtilsTest, canWriteIgnoresReadOnlyAttributeOnDirectories)
{
    using megacmd::canWrite;

    G_SUBTEST << "Directory with FILE_ATTRIBUTE_READONLY is still writable";
    {
        SelfDeletingTmpFolder tmpFolder;
        fs::path dir = tmpFolder.path() / "customized_folder";
        EXPECT_TRUE(fs::create_directory(dir));

        std::wstring wpath = dir.wstring();
        DWORD oldAttrs = GetFileAttributesW(wpath.c_str());

        // Simulate what Explorer does for folder customization
        SetFileAttributesW(wpath.c_str(), oldAttrs | FILE_ATTRIBUTE_READONLY);

        // Verify the attribute is actually set
        DWORD newAttrs = GetFileAttributesW(wpath.c_str());
        EXPECT_TRUE(newAttrs & FILE_ATTRIBUTE_READONLY);

        // canWrite must return true — the directory is writable despite the attribute
        testCanWriteAllPathVariants(dir.string(), true);

        SetFileAttributesW(wpath.c_str(), oldAttrs);
    }

    G_SUBTEST << "Directory with FILE_ATTRIBUTE_READONLY and trailing separator";
    {
        SelfDeletingTmpFolder tmpFolder;
        fs::path dir = tmpFolder.path() / "customized_folder2";
        EXPECT_TRUE(fs::create_directory(dir));

        std::wstring wpath = dir.wstring();
        DWORD oldAttrs = GetFileAttributesW(wpath.c_str());
        SetFileAttributesW(wpath.c_str(), oldAttrs | FILE_ATTRIBUTE_READONLY);

        testCanWriteAllPathVariants(dir.string() + R"(\)", true);

        SetFileAttributesW(wpath.c_str(), oldAttrs);
    }

    G_SUBTEST << "Read-only file is still correctly rejected";
    {
        SelfDeletingTmpFolder tmpFolder;
        fs::path file = tmpFolder.path() / "readonly.txt";
        std::ofstream ofs(file);
        ofs << "test";
        ofs.close();

        std::wstring wpath = file.wstring();
        DWORD oldAttrs = GetFileAttributesW(wpath.c_str());
        SetFileAttributesW(wpath.c_str(), oldAttrs | FILE_ATTRIBUTE_READONLY);

        // For files, FILE_ATTRIBUTE_READONLY *does* mean read-only,
        // and _waccess should return EACCES
        EXPECT_FALSE(canWrite(file.string()));

        SetFileAttributesW(wpath.c_str(), oldAttrs);
    }
}
#endif

#ifdef _WIN32
TEST(UtilsTest, canWriteWithBypassEnvVar)
{
    using megacmd::canWrite;

    G_SUBTEST << "MEGACMD_BYPASS_CAN_WRITE=1 makes canWrite always return true";
    {
        TestInstrumentsEnvVarGuard bypassGuard("MEGACMD_BYPASS_CAN_WRITE", "1");

        G_SUBSUBTEST << "Non-existent path should return true";
        {
            EXPECT_TRUE(canWrite(R"(C:\non\existent\path\12345)"));
        }

        G_SUBSUBTEST << "Empty string should return true";
        {
            EXPECT_TRUE(canWrite(""));
        }

        G_SUBSUBTEST << "Read-only directory should return true";
        {
            SelfDeletingTmpFolder tmpFolder;
            fs::path readOnlyDir = tmpFolder.path() / "readonly";
            EXPECT_TRUE(fs::create_directory(readOnlyDir));
            std::wstring wpath = readOnlyDir.wstring();
            SetFileAttributesW(wpath.c_str(), FILE_ATTRIBUTE_READONLY);
            EXPECT_TRUE(canWrite(readOnlyDir.string()));
        }

        G_SUBSUBTEST << "Read-only file should return true";
        {
            SelfDeletingTmpFolder tmpFolder;
            fs::path readOnlyFile = tmpFolder.path() / "readonly.txt";
            std::ofstream file(readOnlyFile);
            file << "test";
            file.close();
            std::wstring wpath = readOnlyFile.wstring();
            SetFileAttributesW(wpath.c_str(), FILE_ATTRIBUTE_READONLY);
            EXPECT_TRUE(canWrite(readOnlyFile.string()));
        }

        G_SUBSUBTEST << "Writable directory should return true";
        {
            SelfDeletingTmpFolder tmpFolder;
            EXPECT_TRUE(canWrite(tmpFolder.string()));
        }
    }

    G_SUBTEST << "MEGACMD_BYPASS_CAN_WRITE not set or not equal to 1 should work normally";
    {
        TestInstrumentsUnsetEnvVarGuard unsetGuard("MEGACMD_BYPASS_CAN_WRITE");

        G_SUBSUBTEST << "Non-existent path should return false";
        {
            EXPECT_FALSE(canWrite(R"(C:\non\existent\path\12345)"));
        }

        G_SUBSUBTEST << "Empty string should return false";
        {
            EXPECT_FALSE(canWrite(""));
        }

        G_SUBSUBTEST << "Writable directory should return true";
        {
            SelfDeletingTmpFolder tmpFolder;
            EXPECT_TRUE(canWrite(tmpFolder.string()));
        }
    }

    G_SUBTEST << "MEGACMD_BYPASS_CAN_WRITE set to value other than 1 should work normally";
    {
        TestInstrumentsEnvVarGuard bypassGuard("MEGACMD_BYPASS_CAN_WRITE", "0");

        G_SUBSUBTEST << "Non-existent path should return false";
        {
            EXPECT_FALSE(canWrite(R"(C:\non\existent\path\12345)"));
        }

        G_SUBSUBTEST << "Writable directory should return true";
        {
            SelfDeletingTmpFolder tmpFolder;
            EXPECT_TRUE(canWrite(tmpFolder.string()));
        }
    }
}
#endif

namespace
{
void writeUtf8File(const fs::path& filePath, const std::string& utf8Content)
{
    std::ofstream f(filePath, std::ios::binary);
    ASSERT_TRUE(f.is_open());
    f.write(utf8Content.data(), static_cast<std::streamsize>(utf8Content.size()));
    ASSERT_TRUE(f.good());
}
}

TEST(PropertyFileTest, getPropertyFromFileScenarios)
{
    using megacmd::getPropertyFromFile;

    G_SUBTEST << "Missing file returns empty";
    {
        SelfDeletingTmpFolder tmp;
        const fs::path missing = tmp.path() / "nope.cfg";
        EXPECT_TRUE(getPropertyFromFile(missing, "key").empty());
    }

    G_SUBTEST << "Key value and trimmed value";
    {
        SelfDeletingTmpFolder tmp;
        const fs::path cfg = tmp.path() / "a.cfg";
        writeUtf8File(cfg, "alpha =  hello  \n");
        EXPECT_EQ(getPropertyFromFile(cfg, "alpha"), "hello");
    }

    G_SUBTEST << "Comment and blank lines skipped before match";
    {
        SelfDeletingTmpFolder tmp;
        const fs::path cfg = tmp.path() / "b.cfg";
        writeUtf8File(cfg, "# ignored\n\nbeta=y\n");
        EXPECT_EQ(getPropertyFromFile(cfg, "beta"), "y");
    }

    G_SUBTEST << "Empty property name returns first non comment line trimmed";
    {
        SelfDeletingTmpFolder tmp;
        const fs::path cfg = tmp.path() / "c.cfg";
        writeUtf8File(cfg, "# x\n  first  \n");
        EXPECT_EQ(getPropertyFromFile(cfg, ""), "first");
    }

    G_SUBTEST << "Quoted value trimmed";
    {
        SelfDeletingTmpFolder tmp;
        const fs::path cfg = tmp.path() / "d.cfg";
        writeUtf8File(cfg, "q=\"quoted\"\n");
        EXPECT_EQ(getPropertyFromFile(cfg, "q"), "quoted");
    }

    G_SUBTEST << "First occurrence wins";
    {
        SelfDeletingTmpFolder tmp;
        const fs::path cfg = tmp.path() / "e.cfg";
        writeUtf8File(cfg, "k=1\nk=2\n");
        EXPECT_EQ(getPropertyFromFile(cfg, "k"), "1");
    }

    G_SUBTEST << "Unknown key returns empty";
    {
        SelfDeletingTmpFolder tmp;
        const fs::path cfg = tmp.path() / "f.cfg";
        writeUtf8File(cfg, "only=here\n");
        EXPECT_TRUE(getPropertyFromFile(cfg, "missing").empty());
    }

    G_SUBTEST << "Line without equals is ignored for named lookup";
    {
        SelfDeletingTmpFolder tmp;
        const fs::path cfg = tmp.path() / "g.cfg";
        writeUtf8File(cfg, "not_a_pair\nkey=ok\n");
        EXPECT_EQ(getPropertyFromFile(cfg, "key"), "ok");
    }
}

TEST(PropertyFileTest, getValueFromFileScenarios)
{
    using megacmd::getValueFromFile;

    G_SUBTEST << "Integer parsed";
    {
        SelfDeletingTmpFolder tmp;
        const fs::path cfg = tmp.path() / "i.cfg";
        writeUtf8File(cfg, "n=42\n");
        EXPECT_EQ(getValueFromFile(cfg, "n", -1), 42);
    }

    G_SUBTEST << "Integer default when missing";
    {
        SelfDeletingTmpFolder tmp;
        const fs::path cfg = tmp.path() / "j.cfg";
        writeUtf8File(cfg, "other=1\n");
        EXPECT_EQ(getValueFromFile(cfg, "n", 99), 99);
    }

    G_SUBTEST << "Double parsed";
    {
        SelfDeletingTmpFolder tmp;
        const fs::path cfg = tmp.path() / "k.cfg";
        writeUtf8File(cfg, "x=3.5\n");
        EXPECT_DOUBLE_EQ(getValueFromFile(cfg, "x", 0.0), 3.5);
    }

    G_SUBTEST << "Default when property empty";
    {
        SelfDeletingTmpFolder tmp;
        const fs::path cfg = tmp.path() / "l.cfg";
        writeUtf8File(cfg, "e=  \n");
        EXPECT_EQ(getValueFromFile(cfg, "e", 7), 7);
    }
}

TEST(PropertyFileTest, StressTestManyLinesBeforeKey)
{
    using megacmd::getPropertyFromFile;

    G_SUBTEST << "Scan until key near end";
    {
        SelfDeletingTmpFolder tmp;
        const fs::path cfg = tmp.path() / "big.cfg";
        std::ostringstream body;
        for (int i = 0; i < 4000; i++)
        {
            body << "pad=" << i << "\n";
        }
        body << "target=found\n";
        writeUtf8File(cfg, body.str());
        EXPECT_EQ(getPropertyFromFile(cfg, "target"), "found");
    }
}
