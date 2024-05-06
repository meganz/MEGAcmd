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
#endif

TEST(PlatformDirectoriesTest, runtimeDirPath)
{
    using megacmd::PlatformDirectories;

    auto dirs = PlatformDirectories::getPlatformSpecificDirectories();
    ASSERT_TRUE(dirs != nullptr);

#if !defined(_WIN32) && !defined(__APPLE__)
    {
        G_SUBTEST << "With $XDG_RUNTIME_DIR";
        auto guard = TestInstrumentsEnvVarGuard("XDG_RUNTIME_DIR", "/tmp/test");
        EXPECT_EQ(dirs->runtimeDirPath(), "/tmp/test/megacmd");
    }
    {
        G_SUBTEST << "Without $XDG_RUNTIME_DIR";
        auto guard = TestInstrumentsUnsetEnvVarGuard("XDG_RUNTIME_DIR");
        {
            auto homeGuard = TestInstrumentsEnvVarGuard("HOME", "/non-existent-dir");
            EXPECT_EQ(dirs->runtimeDirPath(), std::string("/tmp/megacmd-").append(std::to_string(getuid())));
        }
        {
            auto homeGuard = TestInstrumentsEnvVarGuard("HOME", "/tmp");
            EXPECT_EQ(dirs->runtimeDirPath(), "/tmp/.megaCmd");
        }
    }
#endif
}

TEST(PlatformDirectoriesTest, configDirPath)
{
    using megacmd::PlatformDirectories;

    auto dirs = PlatformDirectories::getPlatformSpecificDirectories();
    ASSERT_TRUE(dirs != nullptr);
#if !defined(_WIN32) && !defined(__APPLE__)
    {
        G_SUBTEST << "With alternative existing HOME";
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", "/tmp");
        EXPECT_EQ(dirs->configDirPath(), "/tmp/.megaCmd");
    }
    {
        G_SUBTEST << "With alternative NON existing HOME";
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", "/non-existent-dir");
        EXPECT_EQ(dirs->configDirPath(), std::string("/tmp/megacmd-").append(std::to_string(getuid())));
    }
#endif
#ifdef _WIN32
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

#ifndef _WIN32

    using megacmd::PlatformDirectories;
    auto dirs = PlatformDirectories::getPlatformSpecificDirectories();
    {
        G_SUBTEST << "Another HOME";

        std::string tmpFolder = std::tmpnam(nullptr);
        fs::path tempDir = fs::path(tmpFolder);
        fs::create_directory(tempDir);

        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", tmpFolder);

#ifdef __APPLE__
        fs::path subDir = tempDir / "Library" / "Caches" / "megacmd.mac";
        EXPECT_STREQ(dirs->runtimeDirPath().c_str(), subDir.string().c_str());

        fs::create_directories(subDir);
#endif
        ASSERT_TRUE(ConfigurationManager::lockExecution());
        ASSERT_TRUE(ConfigurationManager::unlockExecution());

        fs::remove_all(tempDir);
    }

    {
        G_SUBTEST << "With legacy one";

#ifndef __APPLE__
        auto xdgruntime = getenv("XDG_RUNTIME_DIR");

        std::unique_ptr<TestInstrumentsEnvVarGuard> guardRuntimeDir;
        std::unique_ptr<fs::path> tempDir;

        if (!xdgruntime)
        {
            std::string tmpFolder = std::tmpnam(nullptr);
            tempDir.reset(new fs::path(tmpFolder));
            fs::create_directory(*tempDir);
            guardRuntimeDir.reset(new TestInstrumentsEnvVarGuard("XDG_RUNTIME_DIR", tempDir->string()));
            G_TEST_INFO << "Missing XDG_RUNTIME_DIR, set to " << tempDir->string();
        }
#endif

        auto legacyLockFolder = ConfigurationManager::getConfigFolder();
        EXPECT_STRNE(dirs->runtimeDirPath().c_str(), legacyLockFolder.c_str());

        ASSERT_TRUE(ConfigurationManager::lockExecution(legacyLockFolder));

        ASSERT_FALSE(ConfigurationManager::lockExecution());

        ASSERT_TRUE(ConfigurationManager::unlockExecution(legacyLockFolder));

        // All good after that
        ASSERT_TRUE(ConfigurationManager::lockExecution());
        ASSERT_TRUE(ConfigurationManager::unlockExecution());

#ifndef __APPLE__
        if (tempDir)
        {
            fs::remove_all(*tempDir);
        }
#endif

    }
#endif
}

#ifndef _WIN32
TEST(PlatformDirectoriesTest, getOrCreateSocketPath)
{
    using megacmd::getOrCreateSocketPath;
    using megacmd::PlatformDirectories;

    auto dirs = PlatformDirectories::getPlatformSpecificDirectories();
    auto runtimeDir = dirs->runtimeDirPath();

    {
        G_SUBTEST << "With $MEGACMD_SOCKET_NAME";
        auto guard = TestInstrumentsEnvVarGuard("MEGACMD_SOCKET_NAME", "test.sock");
        EXPECT_EQ(getOrCreateSocketPath(false), runtimeDir + "/test.sock");
    }
    {
        G_SUBTEST << "Without $MEGACMD_SOCKET_NAME";
        auto guard = TestInstrumentsUnsetEnvVarGuard("MEGACMD_SOCKET_NAME");
        EXPECT_EQ(getOrCreateSocketPath(false, true), runtimeDir + "/megacmd.socket");
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
