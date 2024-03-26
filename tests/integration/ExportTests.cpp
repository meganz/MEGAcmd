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

using TI = TestInstruments;

class ExportTest : public NOINTERACTIVELoggedInTest
{
protected:
    void SetUp() override
    {
        NOINTERACTIVELoggedInTest::SetUp();
        auto result = executeInClient({"import", "https://mega.nz/folder/8L80QKyL#glRTp6Zc0gppwp03IG03tA"});
        ASSERT_TRUE(result.ok()) << "could not import testExportFolder";
        ASSERT_TRUE(executeInClient({"ls", "testExportFolder"}).ok()) << "could not find folder testExportFolder";

        result = executeInClient({"import", "https://mega.nz/file/MGk2WKwL#qk9THHhxbakddRmt_tLR8OhInexzVCpPPG6M6feFfZg"});
        ASSERT_TRUE(result.ok()) << "could not import testExportFile01.txt";
        ASSERT_TRUE(executeInClient({"ls", "testExportFile01.txt"}).ok()) << "could not find file testExportFile01.txt";
    }

    void TearDown() override
    {
        ASSERT_TRUE(executeInClient({"rm", "-r", "-f", "testExportFolder"}).ok()) << "could not delete folder testExportFolder";
        ASSERT_TRUE(executeInClient({"rm", "-f", "testExportFile01.txt"}).ok()) << "could not delete file testExportFile01.txt";
        NOINTERACTIVELoggedInTest::TearDown();
    }
};

namespace {
    // We'll use these regex to verify the links and authentication keys are well-formed
    // Links end in <handle>#<key>, whereas tokens end in <handle>#<key>:<auth-key>
    // <handle> is simply alphanumeric, while <auth-key> and <key> can also contain '-' or '_'
    // Since GTest uses a very simple Regex implementation on Windows, we cannot use groups or brackets (see: https://google.github.io/googletest/advanced.html#regular-expression-syntax)
    const std::string megaFileLinkRegex(R"(https:\/\/mega.nz\/file\/\w+\#\S+)");
    const std::string megaFolderLinkRegex(R"(https:\/\/mega.nz\/folder\/\w+\#\S+)");
    const std::string megaPasswordLinkRegex(R"(https:\/\/mega.nz\/\S+)");
    const std::string authTokenRegex(R"(\w+\#\S+\:\S+)");
}

TEST_F(ExportTest, Basic)
{
    {
        G_SUBTEST << "File";
        const std::string file_path = "testExportFile01.txt";

        auto rExport = executeInClient({"export", file_path});
        ASSERT_FALSE(rExport.ok());
        EXPECT_THAT(rExport.out(), testing::HasSubstr(file_path + " is not exported"));
        EXPECT_THAT(rExport.out(), testing::HasSubstr("Use -a to export it"));

        auto rCreate = executeInClient({"export", "-a", "-f", file_path});
        ASSERT_TRUE(rCreate.ok());
        EXPECT_THAT(rCreate.out(), testing::HasSubstr("Exported /" + file_path));
        EXPECT_THAT(rCreate.out(), testing::ContainsStdRegex(megaFileLinkRegex));

        // Verify it shows up as exported
        rExport = executeInClient({"export", file_path});
        ASSERT_TRUE(rExport.ok());
        EXPECT_THAT(rExport.out(), testing::HasSubstr(file_path));
        EXPECT_THAT(rExport.out(), testing::HasSubstr("shared as exported permanent file link"));
        EXPECT_THAT(rExport.out(), testing::ContainsStdRegex(megaFileLinkRegex));

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
        EXPECT_THAT(rCreate.out(), testing::ContainsStdRegex(megaFolderLinkRegex));

        // Verify it shows up as exported
        rExport = executeInClient({"export", dir_path});
        ASSERT_TRUE(rExport.ok());
        EXPECT_THAT(rExport.out(), testing::HasSubstr(dir_path));
        EXPECT_THAT(rExport.out(), testing::HasSubstr("shared as exported permanent folder link"));
        EXPECT_THAT(rExport.out(), testing::ContainsStdRegex(megaFolderLinkRegex));

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
    const std::string file_path = "testExportFile01.txt";
    const std::vector<std::string> createCommand{"export", "-a", "-f", file_path};

    auto rCreate = executeInClient(createCommand);
    ASSERT_TRUE(rCreate.ok());
    EXPECT_THAT(rCreate.out(), testing::HasSubstr("Exported /" + file_path));
    EXPECT_THAT(rCreate.out(), testing::ContainsStdRegex(megaFileLinkRegex));

    rCreate = executeInClient(createCommand);
    ASSERT_FALSE(rCreate.ok());
    EXPECT_THAT(rCreate.out(), testing::HasSubstr(file_path + " is already exported"));

    auto rDisable = executeInClient({"export", "-d", file_path});
    ASSERT_TRUE(rDisable.ok());
    EXPECT_THAT(rDisable.out(), testing::StartsWith("Disabled export: /" + file_path));
}

TEST_F(ExportTest, Writable)
{
    const std::string dir_path = "testExportFolder";

    auto rOnlyWritable = executeInClient({"export", "--writable", dir_path});
    ASSERT_FALSE(rOnlyWritable.ok());
    EXPECT_THAT(rOnlyWritable.out(), testing::HasSubstr("Option can only be used when adding an export"));

    auto rCreate = executeInClient({"export", "-a", "-f", "--writable", dir_path});
    ASSERT_TRUE(rCreate.ok());
    EXPECT_THAT(rCreate.out(), testing::HasSubstr("Exported /" + dir_path));
    EXPECT_THAT(rCreate.out(), testing::ContainsStdRegex(megaFolderLinkRegex));
    EXPECT_THAT(rCreate.out(), testing::ContainsStdRegex("AuthToken = " + authTokenRegex));

    // Verify the authToken is also present when checking the export (not just when creating it)
    auto rCheck = executeInClient({"export", dir_path});
    ASSERT_TRUE(rCheck.ok());
    EXPECT_THAT(rCheck.out(), testing::HasSubstr(dir_path));
    EXPECT_THAT(rCheck.out(), testing::HasSubstr("shared as exported permanent folder link"));
    EXPECT_THAT(rCheck.out(), testing::ContainsStdRegex(megaFolderLinkRegex));
    EXPECT_THAT(rCheck.out(), testing::ContainsStdRegex("AuthToken=" + authTokenRegex));

    auto rDisable = executeInClient({"export", "-d", dir_path});
    ASSERT_TRUE(rDisable.ok());
    EXPECT_THAT(rDisable.out(), testing::StartsWith("Disabled export: /" + dir_path));
}

TEST_F(ExportTest, NestedDirectoryStructure)
{
    {
        G_SUBTEST << "Directory";
        const std::string dir_path = "testExportFolder/subDirectoryExport";

        auto rCreate = executeInClient({"export", "-a", "-f", dir_path});
        ASSERT_TRUE(rCreate.ok());
        EXPECT_THAT(rCreate.out(), testing::HasSubstr("Exported /" + dir_path));
        EXPECT_THAT(rCreate.out(), testing::ContainsStdRegex(megaFolderLinkRegex));

        auto rExport = executeInClient({"export", dir_path});
        ASSERT_TRUE(rExport.ok());
        EXPECT_THAT(rExport.out(), testing::HasSubstr(dir_path));
        EXPECT_THAT(rExport.out(), testing::HasSubstr("shared as exported permanent folder link"));
        EXPECT_THAT(rExport.out(), testing::ContainsStdRegex(megaFolderLinkRegex));

        auto rDisable = executeInClient({"export", "-d", dir_path});
        ASSERT_TRUE(rDisable.ok());
        EXPECT_THAT(rDisable.out(), testing::StartsWith("Disabled export: /" + dir_path));
    }

    {
        G_SUBTEST << "File";
        const std::string file_path = "testExportFolder/subDirectoryExport/file01.txt";

        auto rCreate = executeInClient({"export", "-a", "-f", file_path});
        ASSERT_TRUE(rCreate.ok());
        EXPECT_THAT(rCreate.out(), testing::HasSubstr("Exported /" + file_path));
        EXPECT_THAT(rCreate.out(), testing::ContainsStdRegex(megaFileLinkRegex));

        auto rExport = executeInClient({"export", file_path});
        ASSERT_TRUE(rExport.ok());
        EXPECT_THAT(rExport.out(), testing::HasSubstr(file_path));
        EXPECT_THAT(rExport.out(), testing::HasSubstr("shared as exported permanent file link"));
        EXPECT_THAT(rExport.out(), testing::ContainsStdRegex(megaFileLinkRegex));

        auto rDisable = executeInClient({"export", "-d", file_path});
        ASSERT_TRUE(rDisable.ok());
        EXPECT_THAT(rDisable.out(), testing::StartsWith("Disabled export: /" + file_path));
    }
}

TEST_F(ExportTest, PasswordProtected)
{
    const std::string file_path = "testExportFile01.txt";
    const std::vector<std::string> createCommand{"export", "-a", "-f", "--password=SomePassword", file_path};
    const std::vector<std::string> disableCommand{"export", "-d", file_path};

    {
        G_SUBTEST << "Non-pro account";
        TestInstrumentsTestValueGuard proLevelValueGuard(TI::TestValue::AMIPRO_LEVEL, int64_t(0));

        auto rCreate = executeInClient(createCommand);
        ASSERT_TRUE(rCreate.ok());
        EXPECT_THAT(rCreate.out(), testing::HasSubstr("Only PRO users can protect links with passwords"));
        EXPECT_THAT(rCreate.out(), testing::HasSubstr("Showing UNPROTECTED link"));
        EXPECT_THAT(rCreate.out(), testing::HasSubstr("Exported /" + file_path));
        EXPECT_THAT(rCreate.out(), testing::ContainsStdRegex(megaFileLinkRegex));

        auto rDisable = executeInClient(disableCommand);
        ASSERT_TRUE(rDisable.ok());
        EXPECT_THAT(rDisable.out(), testing::StartsWith("Disabled export: /" + file_path));
    }

    {
        G_SUBTEST << "Pro account";
        TestInstrumentsTestValueGuard proLevelValueGuard(TI::TestValue::AMIPRO_LEVEL, int64_t(1));

        auto rCreate = executeInClient(createCommand);
        ASSERT_TRUE(rCreate.ok());
        EXPECT_THAT(rCreate.out(), testing::Not(testing::HasSubstr("Only PRO users can protect links with passwords")));
        EXPECT_THAT(rCreate.out(), testing::Not(testing::HasSubstr("Showing UNPROTECTED link")));
        EXPECT_THAT(rCreate.out(), testing::HasSubstr("Exported /" + file_path));
        EXPECT_THAT(rCreate.out(), testing::ContainsStdRegex(megaPasswordLinkRegex));

        auto rDisable = executeInClient(disableCommand);
        ASSERT_TRUE(rDisable.ok());
        EXPECT_THAT(rDisable.out(), testing::StartsWith("Disabled export: /" + file_path));
    }
}
