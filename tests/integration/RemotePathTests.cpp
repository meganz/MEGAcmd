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

    return "remote_path_test_dir_" + name;
}
} // namespace

// "." and ".." in remote paths, for the commands that turn path segments
// into the names of folders or files they are about to create.
class RemotePathTests: public NOINTERACTIVELoggedInTest
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

    void createLocalFile(const std::string& filename, const std::string& content = "content")
    {
        std::ofstream file(localPath() / filename, std::ios::binary);
        file << content;
    }

    // Sorted names in a folder of the sandbox (the sandbox itself by default),
    // so a test can pin the whole shape of the folder rather than probe for one entry.
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
};

TEST_F(RemotePathTests, MkdirRejectsParentReference)
{
    auto r = executeInClient({"mkdir", ".."});
    EXPECT_FALSE(r.ok());
    EXPECT_THAT(r.err(), testing::HasSubstr("Invalid folder name"));

    EXPECT_THAT(entriesOf(), testing::IsEmpty());
}

TEST_F(RemotePathTests, MkdirRejectsSelfReference)
{
    auto r = executeInClient({"mkdir", "."});
    EXPECT_FALSE(r.ok());
    EXPECT_THAT(r.err(), testing::HasSubstr("Invalid folder name"));

    EXPECT_THAT(entriesOf(), testing::IsEmpty());
}

TEST_F(RemotePathTests, MkdirRejectsParentReferenceAsLastSegment)
{
    ASSERT_TRUE(executeInClient({"mkdir", "sub"}).ok());

    auto r = executeInClient({"mkdir", "sub/.."});
    EXPECT_FALSE(r.ok());
    EXPECT_THAT(r.err(), testing::HasSubstr("Invalid folder name"));

    EXPECT_THAT(entriesOf(), testing::ElementsAre("sub"));
    EXPECT_THAT(entriesOf("sub"), testing::IsEmpty());
}

TEST_F(RemotePathTests, MkdirRejectsParentReferenceInsideANewTree)
{
    // None of the path exists yet, so no part of it is resolved before creating folders
    auto r = executeInClient({"mkdir", "-p", "new1/../new2"});
    EXPECT_FALSE(r.ok());
    EXPECT_THAT(r.err(), testing::HasSubstr("Invalid folder name"));

    EXPECT_THAT(entriesOf(), testing::IsEmpty());
}

TEST_F(RemotePathTests, MkdirResolvesParentReferenceThroughExistingFolders)
{
    ASSERT_TRUE(executeInClient({"mkdir", "sub"}).ok());

    auto r = executeInClient({"mkdir", "-p", "sub/../other"});
    ASSERT_TRUE(r.ok()) << r.err();

    EXPECT_THAT(entriesOf(), testing::ElementsAre("other", "sub"));
}

TEST_F(RemotePathTests, PutWithCreateFlagRejectsParentReferenceAsName)
{
    const std::string filename = "file01.txt";
    createLocalFile(filename);

    auto r = executeInClient({"put", "-c", (localPath() / filename).string(), "new1/.."});
    EXPECT_FALSE(r.ok());
    EXPECT_THAT(r.err(), testing::HasSubstr("Invalid destination name"));

    EXPECT_THAT(entriesOf(), testing::IsEmpty());
}

TEST_F(RemotePathTests, PutWithCreateFlagRejectsSelfReferenceAsName)
{
    const std::string filename = "file01.txt";
    createLocalFile(filename);

    auto r = executeInClient({"put", "-c", (localPath() / filename).string(), "new1/."});
    EXPECT_FALSE(r.ok());
    EXPECT_THAT(r.err(), testing::HasSubstr("Invalid destination name"));

    EXPECT_THAT(entriesOf(), testing::IsEmpty());
}

TEST_F(RemotePathTests, PutWithCreateFlagRejectsParentReferenceInThePath)
{
    const std::string filename = "file01.txt";
    createLocalFile(filename);

    auto r = executeInClient({"put", "-c", (localPath() / filename).string(), "new1/../" + filename});
    EXPECT_FALSE(r.ok());
    EXPECT_THAT(r.err(), testing::HasSubstr("Invalid folder name"));

    EXPECT_THAT(entriesOf(), testing::IsEmpty());
}
