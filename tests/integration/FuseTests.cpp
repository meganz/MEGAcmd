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
#ifdef _WIN32
    TestInstrumentsEnvVarGuard mEnvGuard{"MEGACMD_FUSE_ALLOW_LOCAL_PATHS", "1"};
#endif
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

protected:
    void removeAllMounts()
    {
        auto result = executeInClient({"fuse-show"});
        ASSERT_TRUE(result.ok());

        auto lines = splitByNewline(result.out());
        for (size_t i = 1; i < lines.size(); ++i)
        {
            if (lines[i].empty() || megacmd::startsWith(lines[i], "Use " /* skip detail usage line */))
            {
                continue;
            }

            auto words = megacmd::split(lines[i], " ");
            ASSERT_TRUE(!words.empty());

            const std::string mountId = words[0];

            auto result = executeInClient({"fuse-disable", "--", mountId});
            ASSERT_TRUE(result.ok()) << "Could not disable mount " << mountId;

            result = executeInClient({"fuse-remove", "--", mountId});
            ASSERT_TRUE(result.ok()) << "Could not remove mount " << mountId;
        }
    }

    fs::path mountDirLocalPath() const
    {
        return mTmpDir.path() / "tests_integration_mount_dir";
    }

    std::string mountDirLocal() const
    {
        return mountDirLocalPath().string();
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
#ifdef WIN32
    // Windows expects mount dir to not exist
    fs::remove_all(mountDirLocal());
#endif
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
#ifdef WIN32
    // Windows expects mount dir to not exist
    fs::remove_all(mountDirLocal());
#endif
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

TEST_F(FuseTests, EnsureUniqueId)
{
    const std::string dir1 = (mountDirLocalPath() / "dir1").string();
    const std::string dir2 = (mountDirLocalPath() / "dir2").string();
#ifndef WIN32
    fs::create_directory(dir1);
    fs::create_directory(dir2);
#endif

    auto result = executeInClient({"fuse-add", dir1, mountDirCloud(), "--name=name1"});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Added a new mount from " + qw(dir1) + " to " + qw(mountDirCloud())));
    EXPECT_THAT(result.out(), testing::HasSubstr("Enabled mount"));

    result = executeInClient({"fuse-add", dir2, mountDirCloud(), "--name=name2"});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Added a new mount from " + qw(dir2) + " to " + qw(mountDirCloud())));
    EXPECT_THAT(result.out(), testing::HasSubstr("Enabled mount"));

    result = executeInClient({"fuse-show"});
    ASSERT_TRUE(result.ok());

    auto lines = splitByNewline(result.out());
    ASSERT_EQ(lines.size(), 5); // Column names + 2 mounts + newline + detail usage

    const std::string firstMountId = megacmd::split(lines[1], " ")[0];
    const std::string secondMountId = megacmd::split(lines[2], " ")[0];
    EXPECT_NE(firstMountId, secondMountId);

    // change names
    result = executeInClient({"fuse-config", "name2",  "--name=name1"});
    ASSERT_FALSE(result.ok()); // not allowed: name clash
    result = executeInClient({"fuse-config", "name2",  "--name=name3"});
    ASSERT_TRUE(result.ok()); // allowed

    {
        result = executeInClient({"fuse-show"});
        ASSERT_TRUE(result.ok());

        auto lines = splitByNewline(result.out());
        ASSERT_EQ(lines.size(), 5); // Column names + 2 mounts + newline + detail usage

        const std::string secondMountId = megacmd::split(lines[2], " ")[0];
        EXPECT_EQ(secondMountId, "name3");
    }

    removeAllMounts();
}

#endif // WITH_FUSE
