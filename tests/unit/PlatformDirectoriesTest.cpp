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

#include "Instruments.h"
#include "TestUtils.h"
#include "megacmdcommonutils.h"

#include "configurationmanager.h"

#ifndef WIN32
#include <filesystem>
namespace fs = std::filesystem;

class SelfDeletingTmpFolder
{
    fs::path mTempDir;

public:
    SelfDeletingTmpFolder()
    {
        mTempDir = fs::temp_directory_path() / fs::path(megacmd::generateRandomAlphaNumericString(10));
        fs::create_directory(mTempDir);
    }

    ~SelfDeletingTmpFolder()
    {
        fs::remove_all(mTempDir);
    }

    std::string string(){
        return mTempDir.string();
    }

    fs::path path()
    {
        return mTempDir;
    }
};
#endif


TEST(PlatformDirectoriesTest, runtimeDirPath)
{
    using megacmd::PlatformDirectories;

    auto dirs = PlatformDirectories::getPlatformSpecificDirectories();
    ASSERT_TRUE(dirs != nullptr);

#ifndef __APPLE__
    EXPECT_EQ(dirs->runtimeDirPath(), dirs->configDirPath());
#endif

#ifndef WIN32
    {
        G_SUBTEST << "Non existing HOME folder";
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", "/non-existent-dir");
        EXPECT_STREQ(dirs->runtimeDirPath().c_str(), megacmd::PosixDirectories::noHomeFallbackFolder().c_str());
    }
    {
        G_SUBTEST << "Empty HOME folder";
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", "");
        EXPECT_STREQ(dirs->runtimeDirPath().c_str(), megacmd::PosixDirectories::noHomeFallbackFolder().c_str());
    }

    #ifdef __APPLE__
    {
        SelfDeletingTmpFolder tmpFolder;
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", tmpFolder.string());
        fs::create_directories(tmpFolder.path() / "Library" / "Caches");
        EXPECT_STREQ(dirs->runtimeDirPath().c_str(), tmpFolder.string().append("/Library/Caches/megacmd.mac").c_str());
    }
    #else
    {
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", "/tmp");
        EXPECT_EQ(dirs->runtimeDirPath(), "/tmp/.megaCmd");
    }
    #endif
#endif
}

TEST(PlatformDirectoriesTest, configDirPath)
{
    using megacmd::PlatformDirectories;

    auto dirs = PlatformDirectories::getPlatformSpecificDirectories();
    ASSERT_TRUE(dirs != nullptr);

#ifdef WIN32
    {
        G_SUBTEST << "With $MEGACMD_WORKING_FOLDER_SUFFIX";
        auto guard = TestInstrumentsEnvVarGuard("MEGACMD_WORKING_FOLDER_SUFFIX", "foobar");
        EXPECT_THAT(dirs->configDirPath(), testing::EndsWith(".megaCmd_foobar"));
    }
    {
        G_SUBTEST << "Without $MEGACMD_WORKING_FOLDER_SUFFIX";
        auto guard = TestInstrumentsUnsetEnvVarGuard("MEGACMD_WORKING_FOLDER_SUFFIX");
        EXPECT_THAT(dirs->configDirPath(), testing::EndsWith(".megaCmd"));
    }
#else
    {
        G_SUBTEST << "With alternative existing HOME";
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", "/tmp");
        EXPECT_EQ(dirs->configDirPath(), "/tmp/.megaCmd");
    }
    {
        G_SUBTEST << "With alternative NON existing HOME";
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", "/non-existent-dir");
        EXPECT_EQ(dirs->configDirPath(), megacmd::PosixDirectories::noHomeFallbackFolder());
    }
#endif
}

TEST(PlatformDirectoriesTest, lockExecution)
{
    using megacmd::ConfigurationManager;
    {
        G_SUBTEST << "With default paths:";
        ASSERT_TRUE(ConfigurationManager::lockExecution());
        ASSERT_TRUE(ConfigurationManager::unlockExecution());
    }

#ifndef WIN32

    using megacmd::PlatformDirectories;
    auto dirs = PlatformDirectories::getPlatformSpecificDirectories();
    {
        G_SUBTEST << "Another HOME";

        SelfDeletingTmpFolder tmpFolder;
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", tmpFolder.string());

#ifdef __APPLE__
        fs::create_directories(tmpFolder.path() / "Library" / "Caches");
        EXPECT_STREQ(dirs->runtimeDirPath().c_str(), tmpFolder.string().append("/Library/Caches/megacmd.mac").c_str());
#endif
        ASSERT_TRUE(ConfigurationManager::lockExecution());
        ASSERT_TRUE(ConfigurationManager::unlockExecution());

    }

    {
        G_SUBTEST << "With legacy one";

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
#endif
}

#ifndef WIN32
TEST(PlatformDirectoriesTest, getOrCreateSocketPath)
{
    using megacmd::getOrCreateSocketPath;
    using megacmd::PlatformDirectories;

    auto dirs = PlatformDirectories::getPlatformSpecificDirectories();

    {
        G_SUBTEST << "With $MEGACMD_SOCKET_NAME (normal or fallback case)";
        auto guard = TestInstrumentsEnvVarGuard("MEGACMD_SOCKET_NAME", "test.sock");
        auto socketPath = getOrCreateSocketPath(false);

        ASSERT_TRUE(socketPath == dirs->runtimeDirPath() + "/test.sock" // normal case
                    || socketPath == megacmd::PosixDirectories::noHomeFallbackFolder().append("/test.sock")); // too lengthy case
    }

    {
        G_SUBTEST << "Without $MEGACMD_SOCKET_NAME (normal or fallback case)";
        auto socketPath = getOrCreateSocketPath(false);

        ASSERT_TRUE(socketPath == dirs->runtimeDirPath() + "/megacmd.socket" // normal case
                    || socketPath == megacmd::PosixDirectories::noHomeFallbackFolder().append("/megacmd.socket")); // too lengthy case
    }

    {
        G_SUBTEST << "Without $MEGACMD_SOCKET_NAME, short path: no /tmp/megacmd-UID fallback";
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", "/tmp");
        auto guard = TestInstrumentsUnsetEnvVarGuard("MEGACMD_SOCKET_NAME");
        auto runtimeDir = dirs->runtimeDirPath();
        auto socketPath = getOrCreateSocketPath(false);
        ASSERT_STREQ(socketPath.c_str(), std::string(dirs->runtimeDirPath()).append("/megacmd.socket").c_str());
    }

    {
        G_SUBTEST << "Without $MEGACMD_SOCKET_NAME, longth path: /tmp/megacmd-UID fallback";

        SelfDeletingTmpFolder tmpFolder;
        fs::path lengthyHome = tmpFolder.path() / "this_is_a_very_very_very_lengthy_folder_name_meant_to_make_socket_path_exceed_max_unix_socket_path_allowance";
        fs::create_directories(lengthyHome);
#ifdef __APPLE__
        fs::create_directories(lengthyHome / "Library" / "Caches");
#endif
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", lengthyHome.string());
        auto guard = TestInstrumentsUnsetEnvVarGuard("MEGACMD_SOCKET_NAME");
        auto runtimeDir = dirs->runtimeDirPath();
        auto socketPath = getOrCreateSocketPath(false);
        ASSERT_STRNE(socketPath.c_str(), std::string(dirs->runtimeDirPath()).append("/megacmd.socket").c_str());
        ASSERT_STREQ(socketPath.c_str(), megacmd::PosixDirectories::noHomeFallbackFolder().append("/megacmd.socket").c_str());
    }
}
#else
TEST(PlatformDirectoriesTest, getNamedPipeName)
{
    using megacmd::getNamedPipeName;

    {
        G_SUBTEST << "With $MEGACMD_PIPE_SUFFIX";
        auto guard = TestInstrumentsEnvVarGuard("MEGACMD_PIPE_SUFFIX", "foobar");
        auto name = getNamedPipeName();
        EXPECT_THAT(name, testing::EndsWith(L"foobar"));
    }

    {
        G_SUBTEST << "Without $MEGACMD_PIPE_SUFFIX";
        auto guard = TestInstrumentsUnsetEnvVarGuard("MEGACMD_PIPE_SUFFIX");
        auto name = getNamedPipeName();
        EXPECT_THAT(name, testing::Not(testing::EndsWith(L"foobar")));
    }
}
#endif
