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

    // Keep it filesystem/MEGA-path friendly
    for (auto& ch: name)
    {
        if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '-'))
        {
            ch = '_';
        }
    }

    return "put_test_dir_" + name;
}

std::string readLocalFile(const fs::path& p)
{
    std::ifstream in(p, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}
} // namespace

class PutTests: public NOINTERACTIVELoggedInTest
{
    SelfDeletingTmpFolder mTmpDir;
    std::string mRemoteBase;

    void SetUp() override
    {
        NOINTERACTIVELoggedInTest::SetUp();

        mRemoteBase = makeUniqueRemoteBase();

        // Ensure a clean remote sandbox
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

    void createLocalFile(const std::string& filename, const std::string& content = "")
    {
        fs::create_directories((localPath() / filename).parent_path());
        std::ofstream file(localPath() / filename, std::ios::binary);
        file << content;
    }

    void createLocalDir(const std::string& dirname)
    {
        fs::create_directories(localPath() / dirname);
    }

    std::string remoteBase() const
    {
        return mRemoteBase;
    }
};

TEST_F(PutTests, SingleFileUpload)
{
    const std::string filename = "test_file.txt";
    const std::string content = "Hello, MEGAcmd!";

    createLocalFile(filename, content);

    // Upload into current remote cwd (mRemoteBase)
    auto result = executeInClient({"put", (localPath() / filename).string()});
    ASSERT_TRUE(result.ok()) << result.err();

    // Verify file was uploaded + content matches (normalize trailing newline)
    result = executeInClient({"cat", filename});
    ASSERT_TRUE(result.ok()) << result.err();
    EXPECT_EQ(content, stripTrailingNewlines(result.out()));
}

TEST_F(PutTests, SingleFileUploadToDestination)
{
    const std::string filename = "test_file.txt";
    const std::string content = "Hello, MEGAcmd!";
    const std::string remoteDir = "dest_dir";

    createLocalFile(filename, content);

    auto result = executeInClient({"mkdir", remoteDir});
    ASSERT_TRUE(result.ok()) << result.err();

    result = executeInClient({"put", (localPath() / filename).string(), remoteDir});
    ASSERT_TRUE(result.ok()) << result.err();

    result = executeInClient({"cat", remoteDir + "/" + filename});
    ASSERT_TRUE(result.ok()) << result.err();
    EXPECT_EQ(content, stripTrailingNewlines(result.out()));
}

TEST_F(PutTests, EmptyFileUpload)
{
    const std::string filename = "empty_file.txt";
    createLocalFile(filename);

    auto result = executeInClient({"put", (localPath() / filename).string()});
    ASSERT_TRUE(result.ok()) << result.err();

    result = executeInClient({"ls", filename});
    ASSERT_TRUE(result.ok()) << result.err();
    EXPECT_THAT(result.out(), testing::HasSubstr(filename));
}

TEST_F(PutTests, MultipleFileUpload)
{
    const std::string remoteDir = "multi_dest";

    auto result = executeInClient({"mkdir", remoteDir});
    ASSERT_TRUE(result.ok()) << result.err();

    const std::vector<std::string> files = {"file1.txt", "file2.txt", "file3.txt"};
    for (const auto& f: files)
    {
        createLocalFile(f, "Content of " + f);
    }

    std::vector<std::string> command = {"put"};
    for (const auto& f: files)
    {
        command.push_back((localPath() / f).string());
    }
    command.push_back(remoteDir);

    result = executeInClient(command);
    ASSERT_TRUE(result.ok()) << " command = " << joinString(command) << " err=" << result.err();

    for (const auto& f: files)
    {
        result = executeInClient({"cat", remoteDir + "/" + f});
        ASSERT_TRUE(result.ok()) << "Failed to verify file: " << f << " err=" << result.err();
        EXPECT_EQ("Content of " + f, stripTrailingNewlines(result.out()));
    }
}

TEST_F(PutTests, DirectoryUpload)
{
    const std::string dirname = "test_dir";
    const std::string subfile = "subfile.txt";
    const std::string content = "Content in subdirectory";

    createLocalDir(dirname);
    createLocalFile(dirname + "/" + subfile, content);

    auto result = executeInClient({"put", (localPath() / dirname).string()});
    ASSERT_TRUE(result.ok()) << result.err();

    result = executeInClient({"ls", dirname});
    ASSERT_TRUE(result.ok()) << result.err();
    EXPECT_THAT(result.out(), testing::HasSubstr(subfile));

    result = executeInClient({"cat", dirname + "/" + subfile});
    ASSERT_TRUE(result.ok()) << result.err();
    EXPECT_EQ(content, stripTrailingNewlines(result.out()));
}

TEST_F(PutTests, DirectoryUploadToDestination)
{
    const std::string dirname = "test_dir";
    const std::string subfile = "subfile.txt";
    const std::string content = "Content in subdirectory";
    const std::string remoteDest = "dest_dir";

    createLocalDir(dirname);
    createLocalFile(dirname + "/" + subfile, content);

    auto result = executeInClient({"mkdir", remoteDest});
    ASSERT_TRUE(result.ok()) << result.err();

    result = executeInClient({"put", (localPath() / dirname).string(), remoteDest});
    ASSERT_TRUE(result.ok()) << result.err();

    result = executeInClient({"ls", remoteDest + "/" + dirname});
    ASSERT_TRUE(result.ok()) << result.err();
    EXPECT_THAT(result.out(), testing::HasSubstr(subfile));

    result = executeInClient({"cat", remoteDest + "/" + dirname + "/" + subfile});
    ASSERT_TRUE(result.ok()) << result.err();
    EXPECT_EQ(content, stripTrailingNewlines(result.out()));
}

TEST_F(PutTests, UploadWithCreateFlag)
{
    const std::string filename = "test_file.txt";
    const std::string content = "Hello, MEGAcmd!";
    const std::string remotePath = "new_dir/test_file.txt";

    createLocalFile(filename, content);

    auto result = executeInClient({"put", "-c", (localPath() / filename).string(), remotePath});
    ASSERT_TRUE(result.ok()) << result.err();

    result = executeInClient({"cat", remotePath});
    ASSERT_TRUE(result.ok()) << result.err();
    EXPECT_EQ(content, stripTrailingNewlines(result.out()));
}

TEST_F(PutTests, UploadNonAsciiFilename)
{
#ifdef _WIN32
  // Skip on Windows due to encoding issues in test environment
    GTEST_SKIP() << "Skipping non-ASCII filename test on Windows due to encoding limitations";
    return;
#endif
    const std::string filename = "\xE3\x83\x86\xE3\x82\xB9\xE3\x83\x88\xE3\x83\x95\xE3\x82\xA1"
                                 "\xE3\x82\xA4\xE3\x83\xAB.txt";
    const std::string content = "Content with non-ASCII filename";

    createLocalFile(filename, content);

    auto result = executeInClient({"put", (localPath() / filename).string()});
    ASSERT_TRUE(result.ok()) << result.err();

    result = executeInClient({"ls"});
    ASSERT_TRUE(result.ok()) << result.err();
    EXPECT_THAT(result.out(), testing::HasSubstr(filename));
}

TEST_F(PutTests, UploadFileWithSpaces)
{
    const std::string filename = "test file with spaces.txt";
    const std::string content = "Content in file with spaces";

    createLocalFile(filename, content);

    auto result = executeInClient({"put", (localPath() / filename).string()});
    ASSERT_TRUE(result.ok()) << result.err();

    result = executeInClient({"ls"});
    ASSERT_TRUE(result.ok()) << result.err();
    EXPECT_THAT(result.out(), testing::HasSubstr(filename));
}

TEST_F(PutTests, ReplaceExistingFile)
{
    const std::string filename = "test_file.txt";
    const std::string originalContent = "Original content";
    const std::string newContent = "New content replacing original";

    createLocalFile(filename, originalContent);
    auto result = executeInClient({"put", (localPath() / filename).string()});
    ASSERT_TRUE(result.ok()) << result.err();

    result = executeInClient({"cat", filename});
    ASSERT_TRUE(result.ok()) << result.err();
    EXPECT_EQ(originalContent, stripTrailingNewlines(result.out()));

    createLocalFile(filename, newContent);
    result = executeInClient({"put", (localPath() / filename).string(), filename});
    ASSERT_TRUE(result.ok()) << result.err();

    result = executeInClient({"cat", filename});
    ASSERT_TRUE(result.ok()) << result.err();
    EXPECT_EQ(newContent, stripTrailingNewlines(result.out()));
}

TEST_F(PutTests, ReplaceFileInSubdirectory)
{
    const std::string dirname = "test_dir";
    const std::string filename = "test_file.txt";
    const std::string originalContent = "Original content";
    const std::string newContent = "New content in subdirectory";

    ASSERT_TRUE(executeInClient({"mkdir", dirname}).ok());

    createLocalFile(filename, originalContent);
    auto result = executeInClient({"put", (localPath() / filename).string(), dirname});
    ASSERT_TRUE(result.ok()) << result.err();

    createLocalFile(filename, newContent);
    result = executeInClient({"put", (localPath() / filename).string(), dirname + "/" + filename});
    ASSERT_TRUE(result.ok()) << result.err();

    result = executeInClient({"cat", dirname + "/" + filename});
    ASSERT_TRUE(result.ok()) << result.err();
    EXPECT_EQ(newContent, stripTrailingNewlines(result.out()));
}

TEST_F(PutTests, MultipleFilesToFileTargetShouldFail)
{
    const std::string file1 = "file1.txt";
    const std::string file2 = "file2.txt";
    const std::string targetFile = "target.txt";

    createLocalFile(file1, "Content 1");
    createLocalFile(file2, "Content 2");

    auto result = executeInClient({"put", (localPath() / file1).string(), targetFile});
    ASSERT_TRUE(result.ok()) << result.err();

    result = executeInClient({"put", (localPath() / file1).string(), (localPath() / file2).string(), targetFile});
    ASSERT_FALSE(result.ok());
    EXPECT_THAT(result.err(), testing::HasSubstr("Destination"));
}

TEST_F(PutTests, UploadToNonExistentPathWithoutCreateFlag)
{
    const std::string filename = "test_file.txt";
    const std::string nonExistentPath = "non_existent_dir/file.txt";

    createLocalFile(filename, "Test content");

    auto result = executeInClient({"put", (localPath() / filename).string(), nonExistentPath});
    ASSERT_FALSE(result.ok());
    EXPECT_THAT(result.err(),
                testing::AnyOf(testing::HasSubstr("Couldn't find destination"), testing::HasSubstr("destination")));
}

TEST_F(PutTests, UploadDirectoryToFileTargetShouldFail)
{
    const std::string dirname = "test_dir";
    const std::string targetFile = "target.txt";

    createLocalDir(dirname);
    createLocalFile("temp.txt", "temp");

    auto result = executeInClient({"put", (localPath() / "temp.txt").string(), targetFile});
    ASSERT_TRUE(result.ok()) << result.err();

    result = executeInClient({"put", (localPath() / dirname).string(), targetFile});
    ASSERT_FALSE(result.ok());
    EXPECT_THAT(result.err(), testing::HasSubstr("Destination"));
}

TEST_F(PutTests, PutSingleFileRoundTripViaGet)
{
    const std::string filename = "roundtrip.txt";
    const std::string content = "roundtrip-content\nline2\n";

    createLocalFile(filename, content);

    auto r = executeInClient({"put", (localPath() / filename).string()});
    ASSERT_TRUE(r.ok()) << r.err();

    // Download back and compare bytes (stronger than `cat`)
    SelfDeletingTmpFolder dlTmp;
    r = executeInClient({"get", filename, dlTmp.path().string()});
    ASSERT_TRUE(r.ok()) << r.err();

    const fs::path downloaded = dlTmp.path() / filename;
    ASSERT_TRUE(fs::exists(downloaded));

    EXPECT_EQ(readLocalFile(downloaded), content);
}

TEST_F(PutTests, PutCanRenameRemoteFile)
{
    const std::string localName = "src.txt";
    const std::string remoteName = "dst.txt";
    const std::string content = "rename me\n";

    createLocalFile(localName, content);

    auto r = executeInClient({"put", (localPath() / localName).string(), remoteName});
    ASSERT_TRUE(r.ok()) << r.err();

    r = executeInClient({"ls"});
    ASSERT_TRUE(r.ok()) << r.err();
    EXPECT_THAT(r.out(), testing::HasSubstr(remoteName));

    SelfDeletingTmpFolder dlTmp;
    r = executeInClient({"get", remoteName, dlTmp.path().string()});
    ASSERT_TRUE(r.ok()) << r.err();

    const fs::path downloaded = dlTmp.path() / remoteName;
    ASSERT_TRUE(fs::exists(downloaded));
    EXPECT_EQ(readLocalFile(downloaded), content);
}

TEST_F(PutTests, PutDirectoryTreeShowsUp)
{
    // Local tree:
    // tree/
    //   a.txt
    //   sub/b.txt
    const std::string treeDir = "tree";
    createLocalDir(treeDir + "/sub");
    createLocalFile(treeDir + "/a.txt", "A\n");
    createLocalFile(treeDir + "/sub/b.txt", "B\n");

    auto r = executeInClient({"put", (localPath() / treeDir).string()});
    ASSERT_TRUE(r.ok()) << r.err();

    r = executeInClient({"ls", treeDir});
    ASSERT_TRUE(r.ok()) << r.err();
    EXPECT_THAT(r.out(), testing::HasSubstr("a.txt"));
    EXPECT_THAT(r.out(), testing::HasSubstr("sub"));

    r = executeInClient({"ls", treeDir + "/sub"});
    ASSERT_TRUE(r.ok()) << r.err();
    EXPECT_THAT(r.out(), testing::HasSubstr("b.txt"));
}

TEST_F(PutTests, PutPrintTagAtStartPrintsDecimalTag)
{
    const std::string filename = "tag_test.txt";
    createLocalFile(filename, "tag\n");

    auto r = executeInClient({"put", "--print-tag-at-start", (localPath() / filename).string()});
    ASSERT_TRUE(r.ok()) << r.err();

    const std::string out = r.out();

    // Must contain the prefix
    const std::string prefix = "Upload started: Tag = ";
    auto pos = out.find(prefix);
    ASSERT_NE(pos, std::string::npos) << "Output did not contain expected prefix. out=\n" << out;

    pos += prefix.size();
    ASSERT_LT(pos, out.size()) << "No characters after tag prefix. out=\n" << out;

    // Parse consecutive decimal digits after the prefix
    std::size_t end = pos;
    while (end < out.size() && std::isdigit(static_cast<unsigned char>(out[end])))
    {
        ++end;
    }

    ASSERT_GT(end, pos) << "Tag was not a decimal number. out=\n" << out;

    // Optional: sanity check it is followed by "."
    // (keeps it aligned with the format, but still doesn't validate path)
    ASSERT_TRUE(end < out.size() && out[end] == '.') << "Expected '.' after decimal tag. out=\n" << out;
}
