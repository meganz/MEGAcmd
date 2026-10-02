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

#include "MegaCmdTestingTools.h"
#include "TestUtils.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace
{
std::string stripTrailingNewlines(std::string s)
{
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r'))
    {
        s.pop_back();
    }
    return s;
}

std::string makeUniqueRemoteBase()
{
    const auto* ti = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string name = std::string(ti->test_suite_name()) + "_" + std::string(ti->name());

    for (auto& ch: name)
    {
        if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '-'))
        {
            ch = '_';
        }
    }

    return "move_test_dir_" + name;
}
} // namespace

// mv onto a path that already exists
class MoveTests: public NOINTERACTIVELoggedInTest
{
    SelfDeletingTmpFolder mTmpDir;
    std::string mRemoteBase;

    void SetUp() override
    {
        NOINTERACTIVELoggedInTest::SetUp();

        mRemoteBase = makeUniqueRemoteBase();

        (void)executeInClient({"cd", "/"});
        (void)executeInClient({"rm", "-rf", mRemoteBase});

        auto r = executeInClient({"mkdir", mRemoteBase});
        ASSERT_TRUE(r.ok()) << r.err();

        r = executeInClient({"cd", mRemoteBase});
        ASSERT_TRUE(r.ok()) << r.err();
    }

    void TearDown() override
    {
        (void)executeInClient({"cd", "/"});
        (void)executeInClient({"rm", "-rf", mRemoteBase});
        NOINTERACTIVELoggedInTest::TearDown();
    }

protected:
    fs::path localPath() const
    {
        return mTmpDir.path();
    }

    // Writes a local file (and its folders) and uploads its top level item to the sandbox
    void uploadFile(const std::string& relativePath, const std::string& content)
    {
        const fs::path path = localPath() / relativePath;
        fs::create_directories(path.parent_path());
        {
            std::ofstream file(path, std::ios::binary);
            file << content;
        }

        const fs::path topLevel = localPath() / fs::path(relativePath).begin()->string();
        auto r = executeInClient({"put", topLevel.string()});
        ASSERT_TRUE(r.ok()) << r.err();
    }

    std::vector<std::string> entriesOf(const std::string& relativePath = "")
    {
        const std::string path = "/" + mRemoteBase + (relativePath.empty() ? "" : "/" + relativePath);
        auto r = executeInClient({"ls", path});
        EXPECT_TRUE(r.ok()) << r.err();

        auto entries = splitByNewline(r.out());
        entries.erase(std::remove(entries.begin(), entries.end(), std::string()), entries.end());
        std::sort(entries.begin(), entries.end());
        return entries;
    }

    std::string contentOf(const std::string& remotePath)
    {
        auto r = executeInClient({"cat", remotePath});
        EXPECT_TRUE(r.ok()) << r.err();
        return stripTrailingNewlines(r.out());
    }
};

TEST_F(MoveTests, FolderOntoFileFails)
{
    ASSERT_NO_FATAL_FAILURE(uploadFile("file.txt", "file content"));
    ASSERT_NO_FATAL_FAILURE(uploadFile("dir/child.txt", "child content"));

    auto r = executeInClient({"mv", "dir", "file.txt"});
    EXPECT_FALSE(r.ok());
    EXPECT_THAT(r.err(), testing::HasSubstr("Cannot overwrite file with folder"));

    EXPECT_THAT(entriesOf(), testing::ElementsAre("dir", "file.txt"));
    EXPECT_THAT(entriesOf("dir"), testing::ElementsAre("child.txt"));
    EXPECT_EQ(contentOf("file.txt"), "file content");
}

TEST_F(MoveTests, FileOntoFileReplacesIt)
{
    ASSERT_NO_FATAL_FAILURE(uploadFile("source.txt", "source content"));
    ASSERT_NO_FATAL_FAILURE(uploadFile("target.txt", "target content"));

    auto r = executeInClient({"mv", "source.txt", "target.txt"});
    ASSERT_TRUE(r.ok()) << r.err();

    EXPECT_THAT(entriesOf(), testing::ElementsAre("target.txt"));
    EXPECT_EQ(contentOf("target.txt"), "source content");
}
