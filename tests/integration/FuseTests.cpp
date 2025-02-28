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
#ifdef WITH_FUSE

#include <filesystem>

#include "TestUtils.h"
#include "MegaCmdTestingTools.h"

namespace fs = std::filesystem;

class FuseTests : public NOINTERACTIVELoggedInTest
{
    SelfDeletingTmpFolder mTmpDir;

    void SetUp() override
    {
        NOINTERACTIVELoggedInTest::SetUp();
        TearDown();

        auto result = executeInClient({"mkdir", "-p", mountDirCloud() + "/"}).ok();
        ASSERT_TRUE(result);

        result = fs::create_directory(mountDirLocal());
        ASSERT_TRUE(result);
    }

    void TearDown() override
    {
        removeAllMounts();
    }

    void removeAllMounts()
    {
        auto result = executeInClient({"fuse-show"});
        ASSERT_TRUE(result.ok());

        auto lines = splitByNewline(result.out());
        for (size_t i = 1; i < lines.size(); ++i)
        {
            if (lines[i].empty() || megacmd::startsWith(lines[i], "Use fuse-show"))
            {
                continue;
            }

            auto words = megacmd::split(lines[i], " ");
            ASSERT_TRUE(!words.empty());

            if (megacmd::stringToTimestamp(words[0].substr(1))) // discard log lines
            {
                continue;
            }

            const std::string mountId = words[0];

            auto result = executeInClient({"fuse-disable", mountId});
            ASSERT_TRUE(result.ok());

            result = executeInClient({"fuse-remove", mountId});
            ASSERT_TRUE(result.ok());
        }
    }

protected:
    std::string mountDirLocal() const
    {
        return mTmpDir.string() + "/tests_integration_mount_dir";
    }

    std::string mountDirCloud() const
    {
        return "/tests_integration_mount_dir";
    }

    std::string qw(const std::string& str) // quote wrap
    {
        return "\"" + str + "\"";
    }
};

TEST_F(FuseTests, NoMounts)
{
    auto result = executeInClient({"fuse-show"});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("There are no mounts"));
}

TEST_F(FuseTests, AddAndRemoveMount)
{
    auto result = executeInClient({"fuse-add", "--disabled", mountDirLocal(), mountDirCloud()});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Added a new mount from " + qw(mountDirLocal()) + " to " + qw(mountDirCloud())));

    result = executeInClient({"fuse-show", "--disable-path-collapse"});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::Not(testing::HasSubstr("There are no mounts")));
    EXPECT_THAT(result.out(), testing::HasSubstr(mountDirLocal()));
    EXPECT_THAT(result.out(), testing::HasSubstr(mountDirCloud()));

    result = executeInClient({"fuse-show", "--only-enabled"});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("There are no mounts"));

    result = executeInClient({"fuse-show", mountDirLocal()});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::ContainsRegex("Enabled:\\s*NO"));

    result = executeInClient({"fuse-remove", mountDirLocal()});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Removed mount"));
    EXPECT_THAT(result.out(), testing::HasSubstr("on " + qw(mountDirLocal())));
}

TEST_F(FuseTests, AddMountEnabled)
{
    auto result = executeInClient({"fuse-add", mountDirLocal(), mountDirCloud()});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Added a new mount from " + qw(mountDirLocal()) + " to " + qw(mountDirCloud())));
    EXPECT_THAT(result.out(), testing::HasSubstr("Enabled mount"));

    result = executeInClient({"fuse-show", mountDirLocal()});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::ContainsRegex("Enabled:\\s*YES"));

    result = executeInClient({"fuse-disable", mountDirLocal()});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Disabled mount"));
    EXPECT_THAT(result.out(), testing::HasSubstr("on " + qw(mountDirLocal())));

    result = executeInClient({"fuse-remove", mountDirLocal()});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Removed mount"));
    EXPECT_THAT(result.out(), testing::HasSubstr("on " + qw(mountDirLocal())));
}

#endif // WITH_FUSE
