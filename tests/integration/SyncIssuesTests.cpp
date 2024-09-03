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

namespace fs = std::filesystem;
using TI = TestInstruments;

class SyncIssueListGuard
{
    TestInstrumentsWaitForEventGuard mGuard;
    const uint64_t mExpectedListSize;

public:
    SyncIssueListGuard(uint64_t expectedListSize) :
        mGuard(TI::Event::SYNC_ISSUES_LIST_UPDATED),
        mExpectedListSize(expectedListSize)
    {
    }

    ~SyncIssueListGuard()
    {
        EXPECT_TRUE(mGuard.waitForEvent(std::chrono::seconds(10)));

        auto syncIssueListSizeOpt = TI::Instance().testValue(TI::TestValue::SYNC_ISSUES_LIST_SIZE);
        EXPECT_TRUE(syncIssueListSizeOpt.has_value());

        auto syncIssueListSize = std::get<uint64_t>(*syncIssueListSizeOpt);
        EXPECT_EQ(syncIssueListSize, mExpectedListSize);
    }
};

class SyncIssuesTests : public NOINTERACTIVELoggedInTest
{
    SelfDeletingTmpFolder mTmpDir;

    void removeAllSyncs()
    {
        auto result = executeInClient({"sync"});
        ASSERT_TRUE(result.ok());

        auto lines = splitByNewline(result.out());
        for (size_t i = 1; i < lines.size(); ++i)
        {
            auto words = megacmd::split(lines[i], " ");
            ASSERT_TRUE(!words.empty());

            std::string syncId = words[0];
            auto result = executeInClient({"sync", "--remove", "--", syncId}).ok();
            ASSERT_TRUE(result);
        }
    }

    void SetUp() override
    {
        NOINTERACTIVELoggedInTest::SetUp();
        TearDown();

        auto result = executeInClient({"mkdir", "-p", syncDirCloud()}).ok();
        ASSERT_TRUE(result);

        result = fs::create_directory(syncDirLocal());
        ASSERT_TRUE(result);

        result = executeInClient({"sync", syncDirLocal(), syncDirCloud()}).ok();
        ASSERT_TRUE(result);
    }

    void TearDown() override
    {
        removeAllSyncs();

        fs::remove_all(syncDirLocal());
        executeInClient({"rm", "-r", "-f", syncDirCloud()});
    }

protected:
    std::string syncDirLocal() const
    {
        return mTmpDir.string() + "/sync_dir/";
    }

    std::string syncDirCloud() const
    {
        return "sync_dir/";
    }
};

TEST_F(SyncIssuesTests, NoIssues)
{
    auto result = executeInClient({"sync-issues"});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("There are no sync issues"));
}

TEST_F(SyncIssuesTests, NameConflict)
{
#ifdef _WIN32
    const char* conflictingName = "F01";
#else
    const char* conflictingName = "f%301";
#endif

    auto rMkdir = executeInClient({"mkdir", syncDirCloud() + "f01"});
    ASSERT_TRUE(rMkdir.ok());

    {
        // Register the event callback *before* causing the sync issue
        SyncIssueListGuard guard(1);

        // Cause the name conclict
        rMkdir = executeInClient({"mkdir", syncDirCloud() + conflictingName});
        ASSERT_TRUE(rMkdir.ok());
    }

    // Check the name conclict appears in the list of sync issues
    auto result = executeInClient({"sync-issues"});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::Not(testing::HasSubstr("There are no sync issues")));
    EXPECT_THAT(result.out(), testing::AnyOf(testing::HasSubstr(syncDirCloud() + "f01"), testing::HasSubstr(syncDirCloud() + conflictingName)));
}

TEST_F(SyncIssuesTests, SymLink)
{
    const std::string dirPath = syncDirLocal() + "some_dir";
    ASSERT_TRUE(fs::create_directory(dirPath));

    std::string linkPath = syncDirLocal() + "some_link";

#ifdef _WIN32
    megacmd::replaceAll(linkPath, "/", "\\");
#endif

    {
        SyncIssueListGuard guard(1);
        fs::create_directory_symlink(dirPath, linkPath);
    }

    // Check the symlink in the list of sync issues
    auto result = executeInClient({"sync-issues"});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::Not(testing::HasSubstr("There are no sync-issues issues")));
    EXPECT_THAT(result.out(), testing::HasSubstr(linkPath));

    {
        SyncIssueListGuard guard(0);
        fs::remove(linkPath);
    }

    result = executeInClient({"sync-issues"});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("There are no sync issues"));
}

TEST_F(SyncIssuesTests, IncorrectSyncIssueListSizeOnSecondSymlink)
{
    // This tests against an internal issue that caused the sync issue list to have incorrect
    // size for a brief period of time after creating a second symlink. This is unlikely to
    // affect how users interact with MEGAcmd, since it happened over a short timespan. But the test
    // is still useful to prove the internal correctness of our sync issues is not broken.
    // Note: required sdk fix (+our changes) to pass (see: SDK-4016)

    const std::string dirPath = syncDirLocal() + "some_dir";
    ASSERT_TRUE(fs::create_directory(dirPath));

    // The first link doesn't cause an issue, but we still need to wait for it
    {
        SyncIssueListGuard guard(1);
        fs::create_directory_symlink(dirPath, syncDirLocal() + "link1");
    }

    // The second link causes the issue
    {
        SyncIssueListGuard guard(2);
        fs::create_directory_symlink(dirPath, syncDirLocal() + "link2");
    }
}

TEST_F(SyncIssuesTests, LimitedSyncIssueList)
{
    const std::string dirPath = syncDirLocal() + "some_dir";
    ASSERT_TRUE(fs::create_directory(dirPath));

    // Create 5 sync issues
    {
        SyncIssueListGuard guard(5);
        for (int i = 0; i < 5; ++i)
        {
            fs::create_directory_symlink(dirPath, syncDirLocal() + "link" + std::to_string(i));
        }
    }

    auto result = executeInClient({"sync-issues", "--limit=" + std::to_string(3)});
    ASSERT_TRUE(result.ok());

    // There should be 6 lines (column header + limit=3 + newline + limit-specific note)
    auto lines = splitByNewline(result.out());
    EXPECT_THAT(lines, testing::SizeIs(6));
    EXPECT_THAT(lines.back(), testing::HasSubstr("showing 3 out of 5 issues"));
}
