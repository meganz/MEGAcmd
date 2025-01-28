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

class CatTests : public NOINTERACTIVELoggedInTest
{
    SelfDeletingTmpFolder mTmpDir;

    void SetUp() override
    {
        NOINTERACTIVELoggedInTest::SetUp();
        TearDown();
    }

    void TearDown() override
    {
        auto result = executeInClient({"rm", "-f", fileName});
        NOINTERACTIVELoggedInTest::TearDown();
    }

protected:
    const std::string fileName = "file.txt";

    fs::path localPath() const
    {
        return mTmpDir.path();
    }
};

TEST_F(CatTests, NoFile)
{
    auto result = executeInClient({"cat", fileName});
    ASSERT_FALSE(result.ok());
}

TEST_F(CatTests, AsciiContents)
{
    const fs::path filePath = localPath() / "file_ascii.txt";
    const std::string contents = "Hello world!";

    {
        std::ofstream file(filePath);
        file << contents;
    }

    auto result = executeInClient({"put", filePath.string(), fileName});
    ASSERT_TRUE(result.ok());

    result = executeInClient({"cat", fileName});
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(contents, result.out());
}

TEST_F(CatTests, NonAsciiContents)
{
    const fs::path filePath = localPath() / "file_non_ascii.txt";
    const std::string contents = u8"\u3053\u3093\u306b\u3061\u306f\u3001\u4e16\u754c";


    {
        std::ofstream file(filePath, std::ios::binary);
        file << contents;
    }

    auto result = executeInClient({"put", filePath.string(), fileName});
    ASSERT_TRUE(result.ok());

    result = executeInClient({"cat", fileName});
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(contents, result.out());
}

