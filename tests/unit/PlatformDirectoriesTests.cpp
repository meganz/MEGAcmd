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

#include <fstream>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "Instruments.h"
#include "TestUtils.h"

#include "configurationmanager.h"

TEST(PlatformDirectoriesTest, runtimeDirPath)
{
    using megacmd::ConfigurationManager;
    using megacmd::PlatformDirectories;

    auto dirs = PlatformDirectories::getPlatformSpecificDirectories();
    ASSERT_TRUE(dirs != nullptr);

#ifndef __APPLE__
    EXPECT_EQ(dirs->runtimeDirPath(), dirs->configDirPath());
#endif

#ifndef _WIN32
    G_SUBTEST << "Non existing HOME folder";
    {
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", "/non-existent-dir");
        EXPECT_STREQ(dirs->runtimeDirPath().c_str(), megacmd::PosixDirectories::noHomeFallbackFolder().c_str());

        ASSERT_TRUE(ConfigurationManager::lockExecution());
        ASSERT_TRUE(ConfigurationManager::unlockExecution());
    }

    G_SUBTEST << "Empty HOME folder";
    {
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", "");
        EXPECT_STREQ(dirs->runtimeDirPath().c_str(), megacmd::PosixDirectories::noHomeFallbackFolder().c_str());

        ASSERT_TRUE(ConfigurationManager::lockExecution());
        ASSERT_TRUE(ConfigurationManager::unlockExecution());
    }

#ifdef __APPLE__
    G_SUBTEST << "Existing HOME folder (macOS)";
    {
        SelfDeletingTmpFolder tmpFolder;
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", tmpFolder.string());
        fs::create_directories(tmpFolder.path() / "Library" / "Caches");
        EXPECT_STREQ(dirs->runtimeDirPath().c_str(), tmpFolder.string().append("/Library/Caches/megacmd.mac").c_str());

        ASSERT_TRUE(ConfigurationManager::lockExecution());
        ASSERT_TRUE(ConfigurationManager::unlockExecution());
    }
    #else
    G_SUBTEST << "Existing HOME folder";
    {
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", "/tmp");
        EXPECT_EQ(dirs->runtimeDirPath(), "/tmp/.megaCmd");

        ASSERT_TRUE(ConfigurationManager::lockExecution());
        ASSERT_TRUE(ConfigurationManager::unlockExecution());
    }
#endif

    G_SUBTEST << "Existing non-ascii HOME folder";
    {
        SelfDeletingTmpFolder tmpFolder("file_\u5f20\u4e09");

    #ifdef __APPLE__
        fs::path runtimeFolder = tmpFolder.path() / "Library" / "Caches" / "megacmd.mac";
        fs::create_directories(runtimeFolder);
    #else
        fs::path runtimeFolder = tmpFolder.path() / ".megaCmd";
    #endif

        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", tmpFolder.string());
        EXPECT_EQ(dirs->runtimeDirPath().string(), runtimeFolder.string());

        ASSERT_TRUE(ConfigurationManager::lockExecution());
        ASSERT_TRUE(ConfigurationManager::unlockExecution());
    }

#ifdef __APPLE__
    G_SUBTEST << "HOME exists but Library/Caches does not exist";
    {
        SelfDeletingTmpFolder tmpFolder;
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", tmpFolder.string());
        fs::path runtimePath = dirs->runtimeDirPath();
        EXPECT_EQ(runtimePath.string(), megacmd::PosixDirectories::noHomeFallbackFolder());
    }

    G_SUBTEST << "HOME exists but Library/Caches is not a directory";
    {
        SelfDeletingTmpFolder tmpFolder;
        fs::path cachesFile = tmpFolder.path() / "Library" / "Caches";
        fs::create_directories(cachesFile.parent_path());
        std::ofstream(cachesFile) << "not a directory";

        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", tmpFolder.string());
        fs::path runtimePath = dirs->runtimeDirPath();
        EXPECT_EQ(runtimePath.string(), megacmd::PosixDirectories::noHomeFallbackFolder());
    }
#endif

#endif
}

TEST(PlatformDirectoriesTest, configDirPath)
{
    using megacmd::ConfigurationManager;
    using megacmd::PlatformDirectories;

    auto dirs = PlatformDirectories::getPlatformSpecificDirectories();
    ASSERT_TRUE(dirs != nullptr);

#ifdef _WIN32
    G_SUBTEST << "With $MEGACMD_WORKING_FOLDER_SUFFIX";
    {
        auto guard = TestInstrumentsEnvVarGuard("MEGACMD_WORKING_FOLDER_SUFFIX", "foobar");
        EXPECT_THAT(dirs->configDirPath().wstring(), testing::EndsWith(L".megaCmd_foobar"));

        ConfigurationManager::saveSession("test_session_data");
        EXPECT_TRUE(fs::is_regular_file(dirs->configDirPath() / "session"));
    }

    G_SUBTEST << "Without $MEGACMD_WORKING_FOLDER_SUFFIX";
    {
        auto guard = TestInstrumentsUnsetEnvVarGuard("MEGACMD_WORKING_FOLDER_SUFFIX");
        EXPECT_THAT(dirs->configDirPath().wstring(), testing::EndsWith(L".megaCmd"));

        ConfigurationManager::saveSession("test_session_data");
        EXPECT_TRUE(fs::is_regular_file(dirs->configDirPath() / "session"));
    }

    G_SUBTEST << "With non-ascii $MEGACMD_WORKING_FOLDER_SUFFIX";
    {
        auto guard = TestInstrumentsEnvVarGuardW(L"MEGACMD_WORKING_FOLDER_SUFFIX", L"file_\u5f20\u4e09");
        EXPECT_THAT(dirs->configDirPath().wstring(), testing::EndsWith(L"file_\u5f20\u4e09"));

        ConfigurationManager::saveSession("test_session_data");
        EXPECT_TRUE(fs::is_regular_file(dirs->configDirPath() / "session"));
    }

    G_SUBTEST << "With path length exceeding MAX_PATH";
    {
        // To reach maximum length for the folder path; since len(".megaCmd_")=9, and 9+246=255
        // Full path can exceed MAX_PATH=260, but each individual file or folder must have length < 255
        constexpr int suffixLength = 246;

        const std::string suffix = megacmd::generateRandomAlphaNumericString(suffixLength);
        auto guard = TestInstrumentsEnvVarGuard("MEGACMD_WORKING_FOLDER_SUFFIX", suffix);

        const fs::path configDir = dirs->configDirPath();
        EXPECT_THAT(configDir.string(), testing::EndsWith(suffix));
        EXPECT_THAT(configDir.string(), testing::StartsWith(R"(\\?\)"));
        ConfigurationManager::saveSession("test_session_data");
        EXPECT_TRUE(fs::is_regular_file(configDir / "session"));

        // This would throw an exception without the \\?\ prefix
        SelfDeletingTmpFolder tmpFolder(configDir);
    }
#else
    G_SUBTEST << "With alternative existing HOME";
    {
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", "/tmp");
        EXPECT_EQ(dirs->configDirPath().string(), "/tmp/.megaCmd");

        ConfigurationManager::saveSession("test_session_data");
        EXPECT_TRUE(fs::is_regular_file(dirs->configDirPath() / "session"));
    }

    G_SUBTEST << "With alternative existing non-ascii HOME";
    {
        SelfDeletingTmpFolder tmpFolder("file_\u5f20\u4e09");
        fs::path configFolder = tmpFolder.path() / ".megaCmd";

        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", tmpFolder.string());
        EXPECT_EQ(dirs->configDirPath().string(), configFolder.string());

        ConfigurationManager::saveSession("test_session_data");
        EXPECT_TRUE(fs::is_regular_file(dirs->configDirPath() / "session"));
    }

    G_SUBTEST << "With alternative NON existing HOME";
    {
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", "/non-existent-dir");
        EXPECT_EQ(dirs->configDirPath().string(), megacmd::PosixDirectories::noHomeFallbackFolder());

        ConfigurationManager::saveSession("test_session_data");
        EXPECT_TRUE(fs::is_regular_file(dirs->configDirPath() / "session"));
    }

    G_SUBTEST << "HOME exists but is not a directory";
    {
        SelfDeletingTmpFolder tmpFolder;
        fs::path homeFile = tmpFolder.path() / "home_file";
        std::ofstream(homeFile) << "not a directory";

        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", homeFile.string());
        fs::path configPath = dirs->configDirPath();
        EXPECT_EQ(configPath.string(), megacmd::PosixDirectories::noHomeFallbackFolder());
    }

    G_SUBTEST << "Correct 0700 permissions";
    {
        using megacmd::ConfigurationManager;
        using megacmd::ScopeGuard;

        const mode_t oldUmask = umask(0);
        ScopeGuard g([oldUmask] { umask(oldUmask); });

        SelfDeletingTmpFolder tmpFolder;
        const fs::path dirPath = tmpFolder.path() / "some_folder";
        ConfigurationManager::createFolderIfNotExisting(dirPath);

        struct stat info;
        EXPECT_EQ(stat(dirPath.c_str(), &info), 0);
        EXPECT_TRUE(S_ISDIR(info.st_mode));

        const mode_t actualPerms = info.st_mode & 0777;
        const mode_t expectedPerms = 0700;
        EXPECT_EQ(actualPerms, expectedPerms);
    }

    G_SUBTEST << "Very long path";
    {
        SelfDeletingTmpFolder tmpFolder(std::string(250, 'a'));
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", tmpFolder.string());
        fs::path configPath = dirs->configDirPath();
        EXPECT_FALSE(configPath.empty());

        ConfigurationManager::saveSession("test_session_data");
        EXPECT_TRUE(fs::is_regular_file(configPath / "session"));
    }

    G_SUBTEST << "Path with special characters";
    {
#ifdef _WIN32
        SelfDeletingTmpFolder tmpFolder("test!@#$%^&()_+-=[]{};',.");
#else
        SelfDeletingTmpFolder tmpFolder("test@#$%^&*()_+-=[]{}|;':\",.<>?");
#endif
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", tmpFolder.string());
        fs::path configPath = dirs->configDirPath();
        EXPECT_FALSE(configPath.empty());

        ConfigurationManager::saveSession("test_session_data");
        EXPECT_TRUE(fs::is_regular_file(configPath / "session"));
    }

    G_SUBTEST << "Path with spaces";
    {
        SelfDeletingTmpFolder tmpFolder("test folder with spaces");
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", tmpFolder.string());
        fs::path configPath = dirs->configDirPath();
        EXPECT_FALSE(configPath.empty());

        ConfigurationManager::saveSession("test_session_data");
        EXPECT_TRUE(fs::is_regular_file(configPath / "session"));
    }

#ifndef _WIN32
    G_SUBTEST << "Path with newlines and tabs";
    {
        SelfDeletingTmpFolder tmpFolder("test\n\tfolder");
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", tmpFolder.string());
        fs::path configPath = dirs->configDirPath();
        EXPECT_FALSE(configPath.empty());

        ConfigurationManager::saveSession("test_session_data");
        EXPECT_TRUE(fs::is_regular_file(configPath / "session"));
    }
#endif
#endif
}

TEST(PlatformDirectoriesTest, lockExecution)
{
    using megacmd::ConfigurationManager;
    using megacmd::PlatformDirectories;

    G_SUBTEST << "With default paths";
    {
        ASSERT_TRUE(ConfigurationManager::lockExecution());
        ASSERT_TRUE(ConfigurationManager::unlockExecution());
    }

#ifndef _WIN32
    auto dirs = PlatformDirectories::getPlatformSpecificDirectories();

    G_SUBTEST << "With legacy one";
    {
        auto legacyLockFolder = ConfigurationManager::getConfigFolder();
        if (dirs->runtimeDirPath() != legacyLockFolder)
        {
            EXPECT_STRNE(dirs->runtimeDirPath().c_str(), legacyLockFolder.c_str());

            ASSERT_TRUE(ConfigurationManager::lockExecution(legacyLockFolder));
            ASSERT_FALSE(ConfigurationManager::lockExecution());
            ASSERT_TRUE(ConfigurationManager::unlockExecution(legacyLockFolder));

            // All good after that
            ASSERT_TRUE(ConfigurationManager::lockExecution());
            ASSERT_TRUE(ConfigurationManager::unlockExecution());
        }
        else
        {
            G_TEST_INFO << "LEGACY path is the same as runtime one. All good.";
        }
    }

    G_SUBTEST << "Double lock fails";
    {
        ASSERT_TRUE(ConfigurationManager::lockExecution());
        ASSERT_FALSE(ConfigurationManager::lockExecution());
        ASSERT_TRUE(ConfigurationManager::unlockExecution());
    }

    G_SUBTEST << "Unlock without lock";
    {
        ConfigurationManager::unlockExecution();
        ASSERT_FALSE(ConfigurationManager::unlockExecution());
    }

    G_SUBTEST << "Lock after unlock";
    {
        ASSERT_TRUE(ConfigurationManager::lockExecution());
        ASSERT_TRUE(ConfigurationManager::unlockExecution());
        ASSERT_TRUE(ConfigurationManager::lockExecution());
        ASSERT_TRUE(ConfigurationManager::unlockExecution());
    }

    G_SUBTEST << "Lock with very long path";
    {
        SelfDeletingTmpFolder tmpFolder("this_is_a_very_very_very_lengthy_folder_name_meant_to_test_lock_with_long_path");
#ifdef __APPLE__
        fs::create_directories(tmpFolder.path() / "Library" / "Caches");
#endif
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", tmpFolder.string());
        ASSERT_TRUE(ConfigurationManager::lockExecution());
        ASSERT_TRUE(ConfigurationManager::unlockExecution());
    }

    G_SUBTEST << "Lock with special characters in path";
    {
#ifdef _WIN32
        SelfDeletingTmpFolder tmpFolder("test!@#$%^&()_+-=[]{};',.");
#else
        SelfDeletingTmpFolder tmpFolder("test@#$%^&*()_+-=[]{}|;':\",.<>?");
#endif
#ifdef __APPLE__
        fs::create_directories(tmpFolder.path() / "Library" / "Caches");
#endif
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", tmpFolder.string());
        ASSERT_TRUE(ConfigurationManager::lockExecution());
        ASSERT_TRUE(ConfigurationManager::unlockExecution());
    }
#endif
}

#ifndef _WIN32
TEST(PlatformDirectoriesTest, getOrCreateSocketPath)
{
    using megacmd::getOrCreateSocketPath;
    using megacmd::PlatformDirectories;

    auto dirs = PlatformDirectories::getPlatformSpecificDirectories();

    G_SUBTEST << "With $MEGACMD_SOCKET_NAME (normal or fallback case)";
    {
        auto guard = TestInstrumentsEnvVarGuard("MEGACMD_SOCKET_NAME", "test.sock");
        auto socketPath = getOrCreateSocketPath(false);

        auto expectedNormalFile = (dirs->runtimeDirPath() / "test.sock").string(); // normal case
        auto expectedFallbackFile = megacmd::PosixDirectories::noHomeFallbackFolder().append("/test.sock"); // too length case
        EXPECT_THAT(socketPath, testing::AnyOf(expectedNormalFile, expectedFallbackFile));
    }

    G_SUBTEST << "Socket path with very long HOME";
    {
        SelfDeletingTmpFolder tmpFolder(std::string(150, 'x'));
#ifdef __APPLE__
        fs::create_directories(tmpFolder.path() / "Library" / "Caches");
#endif
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", tmpFolder.string());
        auto guard = TestInstrumentsEnvVarGuard("MEGACMD_SOCKET_NAME", "test.sock");
        auto socketPath = getOrCreateSocketPath(false);
        EXPECT_FALSE(socketPath.empty());
    }

    G_SUBTEST << "Socket path with special characters in HOME";
    {
        SelfDeletingTmpFolder tmpFolder("test@#$%");
#ifdef __APPLE__
        fs::create_directories(tmpFolder.path() / "Library" / "Caches");
#endif
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", tmpFolder.string());
        auto guard = TestInstrumentsEnvVarGuard("MEGACMD_SOCKET_NAME", "test.sock");
        auto socketPath = getOrCreateSocketPath(false);
        EXPECT_FALSE(socketPath.empty());
    }

    G_SUBTEST << "With non-ascii $MEGACMD_SOCKET_NAME (normal or fallback case)";
    {
        auto guard = TestInstrumentsEnvVarGuard("MEGACMD_SOCKET_NAME", "file_\u5f20\u4e09");
        auto socketPath = getOrCreateSocketPath(false);

        auto expectedNormalFile = (dirs->runtimeDirPath() / "file_\u5f20\u4e09").string(); // normal case
        auto expectedFallbackFile = megacmd::PosixDirectories::noHomeFallbackFolder().append("/file_\u5f20\u4e09"); // too length case
        EXPECT_THAT(socketPath, testing::AnyOf(expectedNormalFile, expectedFallbackFile));
    }

    G_SUBTEST << "Without $MEGACMD_SOCKET_NAME (normal or fallback case)";
    {
        auto socketPath = getOrCreateSocketPath(false);

        auto expectedNormalFile = (dirs->runtimeDirPath() / "megacmd.socket").string(); // normal case
        auto expectedFallbackFile = megacmd::PosixDirectories::noHomeFallbackFolder().append("/megacmd.socket"); // too length case
        EXPECT_THAT(socketPath, testing::AnyOf(expectedNormalFile, expectedFallbackFile));
    }

    G_SUBTEST << "Without $MEGACMD_SOCKET_NAME, short path: no /tmp/megacmd-UID fallback";
    {
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", "/tmp");
        auto guard = TestInstrumentsUnsetEnvVarGuard("MEGACMD_SOCKET_NAME");
        auto runtimeDir = dirs->runtimeDirPath();
        auto socketPath = getOrCreateSocketPath(false);

        EXPECT_STREQ(socketPath.c_str(), (dirs->runtimeDirPath() / "megacmd.socket").string().c_str());
    }

    G_SUBTEST << "Without $MEGACMD_SOCKET_NAME, longth path: /tmp/megacmd-UID fallback";
    {
        SelfDeletingTmpFolder tmpFolder("this_is_a_very_very_very_lengthy_folder_name_meant_to_make_socket_path_exceed_max_unix_socket_path_allowance");
#ifdef __APPLE__
        fs::create_directories(tmpFolder.path() / "Library" / "Caches");
#endif
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", tmpFolder.string());
        auto guard = TestInstrumentsUnsetEnvVarGuard("MEGACMD_SOCKET_NAME");
        auto runtimeDir = dirs->runtimeDirPath();
        auto socketPath = getOrCreateSocketPath(false);

        EXPECT_STRNE(socketPath.c_str(), (dirs->runtimeDirPath() / "megacmd.socket").string().c_str());
        EXPECT_STREQ(socketPath.c_str(), megacmd::PosixDirectories::noHomeFallbackFolder().append("/megacmd.socket").c_str());
    }

    G_SUBTEST << "With createDirectory=true, directory created";
    {
        SelfDeletingTmpFolder tmpFolder;
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", tmpFolder.string());
        auto socketPath = getOrCreateSocketPath(true);
        EXPECT_FALSE(socketPath.empty());
        fs::path socketDir = fs::path(socketPath).parent_path();
        EXPECT_TRUE(fs::is_directory(socketDir));
    }

    G_SUBTEST << "With createDirectory=true, directory exists";
    {
        SelfDeletingTmpFolder tmpFolder;
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", tmpFolder.string());
        auto dirs = PlatformDirectories::getPlatformSpecificDirectories();
        fs::path runtimeDir = dirs->runtimeDirPath();
        fs::create_directories(runtimeDir);

        auto socketPath = getOrCreateSocketPath(true);
        EXPECT_FALSE(socketPath.empty());
        EXPECT_TRUE(fs::is_directory(runtimeDir));
    }

    G_SUBTEST << "With createDirectory=true and long path fallback";
    {
        SelfDeletingTmpFolder tmpFolder("this_is_a_very_very_very_lengthy_folder_name_meant_to_make_socket_path_exceed_max_unix_socket_path_allowance");
#ifdef __APPLE__
        fs::create_directories(tmpFolder.path() / "Library" / "Caches");
#endif
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", tmpFolder.string());
        auto socketPath = getOrCreateSocketPath(true);
        EXPECT_FALSE(socketPath.empty());
        fs::path socketDir = fs::path(socketPath).parent_path();
        EXPECT_TRUE(fs::is_directory(socketDir));
        EXPECT_EQ(socketDir.string(), megacmd::PosixDirectories::noHomeFallbackFolder());
    }
}
#else
TEST(PlatformDirectoriesTest, getNamedPipeName)
{
    using megacmd::getNamedPipeName;

    G_SUBTEST << "With $MEGACMD_PIPE_SUFFIX";
    {
        auto guard = TestInstrumentsEnvVarGuard("MEGACMD_PIPE_SUFFIX", "foobar");
        auto name = getNamedPipeName();
        EXPECT_THAT(name, testing::EndsWith(L"foobar"));
    }

    G_SUBTEST << "Without $MEGACMD_PIPE_SUFFIX";
    {
        auto guard = TestInstrumentsUnsetEnvVarGuard("MEGACMD_PIPE_SUFFIX");
        auto name = getNamedPipeName();
        EXPECT_THAT(name, testing::Not(testing::EndsWith(L"foobar")));
    }

    G_SUBTEST << "With non-ascii $MEGACMD_PIPE_SUFFIX";
    {
        auto guard = TestInstrumentsEnvVarGuardW(L"MEGACMD_PIPE_SUFFIX", L"file_\u5f20\u4e09");
        auto name = getNamedPipeName();
        EXPECT_THAT(name, testing::EndsWith(L"file_\u5f20\u4e09"));
    }
}
#endif
