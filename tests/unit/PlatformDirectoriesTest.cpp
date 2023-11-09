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
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", "/home/test");
        EXPECT_EQ(dirs->runtimeDirPath(),
                  std::string("/tmp/megacmd-").append(std::to_string(getuid())));
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
        auto homeGuard= TestInstrumentsEnvVarGuard("HOME", "/home/test");
        EXPECT_EQ(dirs->configDirPath(), "/home/test/.megaCmd");
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
        auto homeGuard = TestInstrumentsEnvVarGuard("HOME", "/home/test");
        auto guard = TestInstrumentsUnsetEnvVarGuard("XDG_CACHE_HOME");
        EXPECT_EQ(dirs->cacheDirPath(), "/home/test/.megaCmd");
    }
#endif
}

#ifndef _WIN32
TEST(PlatformDirectoriesTest, getSocketPath)
{
    using megacmd::getSocketPath;
    using megacmd::PlatformDirectories;

    auto dirs = PlatformDirectories::getPlatformSpecificDirectories();
    auto runtimeDir = dirs->runtimeDirPath();

    {
        G_SUBTEST << "With $MEGACMD_SOCKET_NAME";
        auto guard = TestInstrumentsEnvVarGuard("MEGACMD_SOCKET_NAME", "megacmd-test.socket");
        EXPECT_EQ(getSocketPath(false), runtimeDir + "/megacmd-test.socket");
    }
    {
        G_SUBTEST << "With $MEGACMD_SOCKET_NAME";
        auto guard = TestInstrumentsUnsetEnvVarGuard("MEGACMD_SOCKET_NAME");
        EXPECT_EQ(getSocketPath(false), runtimeDir + "/megacmd.socket");
    }
}
#endif
