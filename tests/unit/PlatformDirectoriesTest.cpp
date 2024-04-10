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
        G_SUBTEST << "With $XDG_CONFIG_HOME";
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", "/home/test");
        auto guard = TestInstrumentsEnvVarGuard("XDG_CONFIG_HOME", "/tmp/test");
        EXPECT_EQ(dirs->configDirPath(), "/tmp/test/megacmd");
    }
    {
        G_SUBTEST << "Without $XDG_CONFIG_HOME";
        auto guard = TestInstrumentsUnsetEnvVarGuard("XDG_CONFIG_HOME");
        {
            auto homeGuard = TestInstrumentsEnvVarGuard("HOME", "/tmp");
            EXPECT_EQ(dirs->configDirPath(), "/tmp/.megaCmd");
        }
        {
            auto homeGuard = TestInstrumentsEnvVarGuard("HOME", "/non-existent-dir");
            EXPECT_EQ(dirs->configDirPath(), std::string("/tmp/megacmd-").append(std::to_string(getuid())));
        }
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

TEST(PlatformDirectoriesTest, cacheDirPath)
{
    using megacmd::PlatformDirectories;

    auto dirs = PlatformDirectories::getPlatformSpecificDirectories();
    ASSERT_TRUE(dirs != nullptr);

#if !defined(_WIN32) && !defined(__APPLE__)
    {
        G_SUBTEST << "With $XDG_CACHE_HOME";
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", "/home/test");
        auto guard = TestInstrumentsEnvVarGuard("XDG_CACHE_HOME", "/tmp/test");
        EXPECT_EQ(dirs->cacheDirPath(), "/tmp/test/megacmd");
    }
    {
        G_SUBTEST << "Without $XDG_CACHE_HOME";
        auto guard = TestInstrumentsUnsetEnvVarGuard("XDG_CACHE_HOME");
        {
            auto homeGuard = TestInstrumentsEnvVarGuard("HOME", "/tmp");
            EXPECT_EQ(dirs->cacheDirPath(), "/tmp/.megaCmd");
        }
        {
            auto homeGuard = TestInstrumentsEnvVarGuard("HOME", "/non-existent-dir");
            EXPECT_EQ(dirs->configDirPath(), std::string("/tmp/megacmd-").append(std::to_string(getuid())));
        }
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
        auto guard = TestInstrumentsEnvVarGuard("MEGACMD_SOCKET_NAME", "megacmd-test.socket");
        EXPECT_EQ(getOrCreateSocketPath(false), runtimeDir + "/megacmd-test.socket");
    }
    {
        G_SUBTEST << "Without $MEGACMD_SOCKET_NAME";
        auto guard = TestInstrumentsUnsetEnvVarGuard("MEGACMD_SOCKET_NAME");
        EXPECT_EQ(getOrCreateSocketPath(false), runtimeDir + "/megacmd.socket");
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
