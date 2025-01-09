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
        for (const std::string& line : lines)
        {
            constexpr std::string_view namePrefix = "Name: ";
            if (!megacmd::startsWith(line, namePrefix))
            {
                continue;
            }

            const std::string name = line.substr(namePrefix.size());

            result = executeInClient({"fuse-disable", "--by-name=" + name});
            ASSERT_TRUE(result.ok());

            result = executeInClient({"fuse-remove", "--by-name=" + name});
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
        return "tests_integration_mount_dir";
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
    auto result = executeInClient({"fuse-add", mountDirLocal(), mountDirCloud()});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Successfully added a new mount from " + qw(mountDirCloud()) + " to " + qw(mountDirLocal())));

    result = executeInClient({"fuse-show"});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::Not(testing::HasSubstr("There are no mounts")));
    EXPECT_THAT(result.out(), testing::HasSubstr(mountDirLocal()));
    EXPECT_THAT(result.out(), testing::HasSubstr(mountDirCloud()));
    EXPECT_THAT(result.out(), testing::HasSubstr("Enabled: NO"));

    result = executeInClient({"fuse-remove", "--by-path=" + mountDirLocal()});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Successfully removed the mount on " + qw(mountDirLocal())));
}

TEST_F(FuseTests, EnableAndDisableMount)
{
    auto result = executeInClient({"fuse-add", mountDirLocal(), mountDirCloud()});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Successfully added a new mount from " + qw(mountDirCloud()) + " to " + qw(mountDirLocal())));

    result = executeInClient({"fuse-enable", "--by-path=" + mountDirLocal()});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Successfully enabled the mount on " + qw(mountDirLocal())));

    result = executeInClient({"fuse-show"});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::Not(testing::HasSubstr("There are no mounts")));
    EXPECT_THAT(result.out(), testing::HasSubstr(mountDirLocal()));
    EXPECT_THAT(result.out(), testing::HasSubstr(mountDirCloud()));
    EXPECT_THAT(result.out(), testing::HasSubstr("Enabled: YES"));

    result = executeInClient({"fuse-disable", "--by-path=" + mountDirLocal()});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Successfully disabled the mount on " + qw(mountDirLocal())));

    result = executeInClient({"fuse-show"});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::Not(testing::HasSubstr("There are no mounts")));
    EXPECT_THAT(result.out(), testing::HasSubstr(mountDirLocal()));
    EXPECT_THAT(result.out(), testing::HasSubstr(mountDirCloud()));

    result = executeInClient({"fuse-remove", "--by-path=" + mountDirLocal()});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Successfully removed the mount on " + qw(mountDirLocal())));
}

#endif // WITH_FUSE
