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

#include "MegaCmdTestingTools.h"
#include "TestUtils.h"

class ExportTest : public NOINTERACTIVELoggedInTest {};

TEST_F(ExportTest, Basic)
{
    {
        G_SUBTEST << "File";
        const std::string file_path = "file01.txt";

        auto rExport = executeInClient({"export", file_path});
        ASSERT_FALSE(rExport.ok());
        EXPECT_THAT(rExport.out(), testing::HasSubstr(file_path + " is not exported"));
        EXPECT_THAT(rExport.out(), testing::HasSubstr("Use -a to export it"));

        auto rCreate = executeInClient({"export", "-a", "-f", file_path});
        ASSERT_TRUE(rCreate.ok());
        EXPECT_THAT(rCreate.out(), testing::HasSubstr("Exported /" + file_path));

        // Verify it shows up as exported
        rExport = executeInClient({"export", file_path});
        ASSERT_TRUE(rExport.ok());
        EXPECT_THAT(rExport.out(), testing::HasSubstr(file_path));
        EXPECT_THAT(rExport.out(), testing::HasSubstr("shared as exported permanent file link"));

        auto rDisable = executeInClient({"export", "-d", file_path});
        ASSERT_TRUE(rDisable.ok());
        EXPECT_THAT(rDisable.out(), testing::StartsWith("Disabled export: /" + file_path));

        // Again, verify it's not exported
        rExport = executeInClient({"export", file_path});
        ASSERT_FALSE(rExport.ok());
        EXPECT_THAT(rExport.out(), testing::HasSubstr(file_path + " is not exported"));
        EXPECT_THAT(rExport.out(), testing::HasSubstr("Use -a to export it"));
    }

    {
        G_SUBTEST << "Directory";
        const std::string dir_path = "testExportFolder";

        auto rExport = executeInClient({"export", dir_path});
        ASSERT_FALSE(rExport.ok());
        EXPECT_THAT(rExport.out(), testing::HasSubstr("Couldn't find anything exported below"));
        EXPECT_THAT(rExport.out(), testing::HasSubstr(dir_path));
        EXPECT_THAT(rExport.out(), testing::HasSubstr("Use -a to export it"));

        auto rCreate = executeInClient({"export", "-a", "-f", dir_path});
        ASSERT_TRUE(rCreate.ok());
        EXPECT_THAT(rCreate.out(), testing::HasSubstr("Exported /" + dir_path));

        // Verify it shows up as exported
        rExport = executeInClient({"export", dir_path});
        ASSERT_TRUE(rExport.ok());
        EXPECT_THAT(rExport.out(), testing::HasSubstr(dir_path));
        EXPECT_THAT(rExport.out(), testing::HasSubstr("shared as exported permanent folder link"));

        auto rDisable = executeInClient({"export", "-d", dir_path});
        ASSERT_TRUE(rDisable.ok());
        EXPECT_THAT(rDisable.out(), testing::StartsWith("Disabled export: /" + dir_path));

        // Again, verify it's not exported
        rExport = executeInClient({"export", dir_path});
        ASSERT_FALSE(rExport.ok());
        EXPECT_THAT(rExport.out(), testing::HasSubstr("Couldn't find anything exported below"));
        EXPECT_THAT(rExport.out(), testing::HasSubstr(dir_path));
        EXPECT_THAT(rExport.out(), testing::HasSubstr("Use -a to export it"));
    }
}

TEST_F(ExportTest, FailedRecreation)
{
    const std::string file_path = "file01.txt";
    const std::vector<std::string> createCommand{"export", "-a", "-f", file_path};

    auto rCreate = executeInClient(createCommand);
    ASSERT_TRUE(rCreate.ok());
    EXPECT_THAT(rCreate.out(), testing::HasSubstr("Exported /" + file_path));

    rCreate = executeInClient(createCommand);
    ASSERT_FALSE(rCreate.ok());
    EXPECT_THAT(rCreate.out(), testing::HasSubstr(file_path + " is already exported"));

    auto rDisable = executeInClient({"export", "-d", file_path});
    ASSERT_TRUE(rDisable.ok());
    EXPECT_THAT(rDisable.out(), testing::StartsWith("Disabled export: /" + file_path));
}
