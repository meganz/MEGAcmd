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

#include "MegaCmdTestingTools.h"

namespace fs = std::filesystem;
using TI = TestInstruments;

namespace {
    const std::string syncDir("sync_dir");
}

class StalledIssuesTests : public NOINTERACTIVELoggedInTest
{
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

        auto result = executeInClient({"mkdir", syncDir}).ok();
        ASSERT_TRUE(result);

        result = fs::create_directory(syncDir);
        ASSERT_TRUE(result);

        result = executeInClient({"sync", syncDir, syncDir}).ok();
        ASSERT_TRUE(result);
    }

    void TearDown() override
    {
        removeAllSyncs();

        fs::remove_all(syncDir);
        executeInClient({"rm", "-r", "-f", syncDir});
    }
};

TEST_F(StalledIssuesTests, NoIssues)
{
    auto result = executeInClient({"stalled"});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::HasSubstr("There are no stalled issues"));
}

TEST_F(StalledIssuesTests, NameConflict)
{
#ifdef _WIN32
    const char* conflictingName = "/F01";
#else
    const char* conflictingName = "/f%301";
#endif

    // Register the event callback *before* causing the stalled issue
    TestInstrumentsWaitForEventGuard stallUpdatedGuard(TI::Event::STALLED_ISSUES_LIST_UPDATED);

    // Cause the name conclict
    auto rMkdir = executeInClient({"mkdir", syncDir + "/f01"});
    ASSERT_TRUE(rMkdir.ok());
    rMkdir = executeInClient({"mkdir", syncDir + conflictingName});
    ASSERT_TRUE(rMkdir.ok());

    stallUpdatedGuard.waitForEvent(std::chrono::seconds(10));

    // Check the name conclict appears in the list of stalled issues
    auto result = executeInClient({"stalled"});
    ASSERT_TRUE(result.ok());
    EXPECT_THAT(result.out(), testing::Not(testing::HasSubstr("There are no stalled issues")));
    EXPECT_THAT(result.out(), testing::AnyOf(testing::HasSubstr(syncDir + "/f01"), testing::HasSubstr(syncDir + conflictingName)));
}
