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
#include <stdlib.h>

#include "TestUtils.h"
#include "comunicationsmanagerfilesockets.h"

namespace FileSocketsCommunicationsTest
{
    static void getSocketPath()
    {
        using megacmd::ComunicationsManagerFileSockets;

        {
            G_SUBTEST << "With MEGACMD_WORKING_DIR";
            setenv("MEGACMD_WORKING_DIR", "/tmp/foo", 1);
            auto path = ComunicationsManagerFileSockets::getSocketPath(false);
            ASSERT_STREQ(path.c_str(), "/tmp/foo/megaCMD.socket");
        }
        {
            G_SUBTEST << "With XDG_RUNTIME_DIR";
            unsetenv("MEGACMD_WORKING_DIR");
            setenv("XDG_RUNTIME_DIR", "/tmp/xdg-foo", 1);
            auto path = ComunicationsManagerFileSockets::getSocketPath(false);
            ASSERT_STREQ(path.c_str(), "/tmp/xdg-foo/megaCMD.socket");
        }
        {
            G_SUBTEST << "With ~/.megaCMD";
            unsetenv("XDG_RUNTIME_DIR");
            setenv("HOME", "/home/foo", 1);
            auto path = ComunicationsManagerFileSockets::getSocketPath(false);
            ASSERT_STREQ(path.c_str(), "/home/foo/.megaCmd/megaCMD.socket");
        }
        {
            G_SUBTEST << "Fallback in /tmp";
            unsetenv("HOME");
            auto path = ComunicationsManagerFileSockets::getSocketPath(false);
            ASSERT_STREQ(path.c_str(), std::string("/tmp/megaCMD_").append(std::to_string(getuid())).append("/megaCMD.socket").c_str());
        }
    }
}

TEST(FileSocketsCommunicationsTest, getSocketPath)
{
    FileSocketsCommunicationsTest::getSocketPath();
}
