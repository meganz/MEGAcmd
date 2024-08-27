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

#include <filesystem>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "TestUtils.h"
#include "MegaCmdTestingTools.h"

using namespace megacmd;
namespace fs = std::filesystem;
using TI = TestInstruments;

class SyncIgnoreTests : public NOINTERACTIVELoggedInTest
{
    void SetUp() override
    {
        NOINTERACTIVELoggedInTest::SetUp();
        TearDown();
    }

    void TearDown() override
    {
        removeAllSyncs();
        writeToDefaultFile();
    }

protected:
    SelfDeletingTmpFolder mTmpDir;

    void writeToDefaultFile(const std::vector<std::string>& lines = {})
    {
        auto platformDirs = PlatformDirectories::getPlatformSpecificDirectories();
        auto configDirPath = platformDirs->configDirPath();
        std::string megaIgnorePath = configDirPath + "/.megaignore.default";

        std::ofstream file(megaIgnorePath);
        ASSERT_TRUE(file.is_open());

        for (const std::string& line : lines)
        {
            file << line << '\n';
        }
        ASSERT_FALSE(file.fail());
    }

    void readFromDefaultFile(std::string& contents) // cannot return string or asserts won't work
    {
        auto platformDirs = PlatformDirectories::getPlatformSpecificDirectories();
        auto configDirPath = platformDirs->configDirPath();
        std::string megaIgnorePath = configDirPath + "/.megaignore.default";

        std::ifstream file(megaIgnorePath);
        ASSERT_TRUE(file.is_open());

        for (std::string line; getline(file, line);)
        {
            contents += line + "\n";
        }
        ASSERT_TRUE(file.eof());
    }

    std::string w(const std::string& str) // wrap for filters to allow '-' and other special chars
    {
        return "`" + str + "`";
    }

    std::string qw(const std::string& str) // quote wrap
    {
        return "\"" + str + "\"";
    }
};

TEST_F(SyncIgnoreTests, DefaultIgnoreFile)
{
    auto result = executeInClient({"sync-ignore"});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Using default .megaignore file"));
}

TEST_F(SyncIgnoreTests, AddAndShow)
{
    std::string filter1 = "-:some_file.txt";
    std::string filter2 = "-fp:video/*.avi";
    std::string filter3 = "+fng:family*.jpg";
    std::string filter4 = "-d:private";
    std::string filter5 = "-:*";

    auto result = executeInClient({"sync-ignore", "--add", w(filter1), w(filter2), w(filter3), w(filter4), w(filter5)});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Added filter " + qw(filter1)));
    EXPECT_THAT(result.out(), testing::HasSubstr("Added filter " + qw(filter2)));
    EXPECT_THAT(result.out(), testing::HasSubstr("Added filter " + qw(filter3)));
    EXPECT_THAT(result.out(), testing::HasSubstr("Added filter " + qw(filter4)));
    EXPECT_THAT(result.out(), testing::HasSubstr("Added filter " + qw(filter5)));

    result = executeInClient({"sync-ignore", "--show"});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr(filter1));
    EXPECT_THAT(result.out(), testing::HasSubstr(filter2));
    EXPECT_THAT(result.out(), testing::HasSubstr(filter3));
    EXPECT_THAT(result.out(), testing::HasSubstr(filter4));
    EXPECT_THAT(result.out(), testing::HasSubstr(filter5));
}

TEST_F(SyncIgnoreTests, AddAndRemove)
{
    std::string filter = "-N:*.avi";

    auto result = executeInClient({"sync-ignore", "--add", w(filter)});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Added filter " + qw(filter)));

    result = executeInClient({"sync-ignore", "--remove", w(filter)});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Removed filter " + qw(filter)));

    result = executeInClient({"sync-ignore", "--show"});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::Not(testing::HasSubstr(filter)));
}

TEST_F(SyncIgnoreTests, CannotAddIfAlreadyExists)
{
    std::string filter = "+fg:work*.txt";

    auto result = executeInClient({"sync-ignore", "--add", w(filter)});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Added filter " + qw(filter)));

    result = executeInClient({"sync-ignore", "--add", w(filter)});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Cannot add filter " + qw(filter)));
    EXPECT_THAT(result.out(), testing::HasSubstr("already in the .megaignore file"));
}

TEST_F(SyncIgnoreTests, CannotRemoveIfDoesntExist)
{
    std::string filter = "+fnpg:family*.jpg";

    auto result = executeInClient({"sync-ignore", "--remove", w(filter)});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Cannot remove filter " + qw(filter)));
    EXPECT_THAT(result.out(), testing::HasSubstr("it's not in the .megaignore file"));
}

TEST_F(SyncIgnoreTests, NonDefaultIgnoreFile)
{
    std::string localDir = mTmpDir.string();

#ifdef __APPLE__
    localDir = "/private" + localDir;
#endif

    std::string cloudDir = "cloud_dir";
    TI::ScopeGuard cloudRmGuard([&cloudDir] { executeInClient({"rm", "-r", "-f", cloudDir}); });
    executeInClient({"rm", "-r", "-f", cloudDir});

    auto result = executeInClient({"mkdir", "-p", cloudDir});
    ASSERT_TRUE(result.ok());

    result = executeInClient({"sync", localDir, cloudDir});
    ASSERT_TRUE(result.ok());

    result = executeInClient({"sync-ignore", "--show", localDir});
    ASSERT_TRUE(result.ok());

    std::string filter = "-d:.vscode";

    result = executeInClient({"sync-ignore", "--add", w(filter), localDir});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Added filter " + qw(filter)));

    result = executeInClient({"sync-ignore", "--show", localDir});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr(filter));

    result = executeInClient({"sync-ignore", "--remove", w(filter), localDir});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Removed filter " + qw(filter)));
}

TEST_F(SyncIgnoreTests, NonExistingIgnoreFile)
{
    std::string invalidSyncDir = "some_sync_dir";

    auto result = executeInClient({"sync-ignore", "--show", invalidSyncDir});
    ASSERT_FALSE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Sync " + invalidSyncDir + " was not found"));

    // Must also work with implicit "--show"
    result = executeInClient({"sync-ignore", invalidSyncDir});
    ASSERT_FALSE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Sync " + invalidSyncDir + " was not found"));
}

TEST_F(SyncIgnoreTests, IncorrectFilterFormat)
{
    std::string validFilter = "-fp:video/*.avi";
    std::string invalidFilter = "video/family/*.avi";

    auto result = executeInClient({"sync-ignore", "--add", w(validFilter), w(invalidFilter)});
    ASSERT_FALSE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("The filter " + qw(invalidFilter) + " does not have valid format"));
}

TEST_F(SyncIgnoreTests, CommentsAndBOMNotRemoved)
{
    std::string BOM = "\xEF\xBB\xBF";
    std::string comment = "# This is a comment";
    std::string filter = "-nr:.*foo";
    writeToDefaultFile({BOM, comment, filter});

    auto result = executeInClient({"sync-ignore", "--remove", w(filter)});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Removed filter " + qw(filter)));

    std::string contents;
    readFromDefaultFile(contents);
    EXPECT_THAT(contents, testing::HasSubstr(BOM));
    EXPECT_THAT(contents, testing::HasSubstr(comment));
}

TEST_F(SyncIgnoreTests, UseExcludeInterface)
{
    std::string pattern = "*.pdf";

    auto result = executeInClient({"sync-ignore", "--show"});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::Not(testing::HasSubstr(pattern)));

    result = executeInClient({"exclude", "-a", pattern});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Added filter"));
    EXPECT_THAT(result.out(), testing::HasSubstr(pattern));

    result = executeInClient({"sync-ignore", "--show"});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr(pattern));

    result = executeInClient({"exclude", "-d", pattern});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("Removed filter"));
    EXPECT_THAT(result.out(), testing::HasSubstr(pattern));

    result = executeInClient({"sync-ignore", "--show"});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::Not(testing::HasSubstr(pattern)));
}
