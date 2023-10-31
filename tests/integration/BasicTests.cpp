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

class NOINTERACTIVEBasicTest : public BasicGenericTest{};
class NOINTERACTIVELoggedInTest : public LoggedInTest{};
class NOINTERACTIVEReadTest : public ReadTest{};

TEST_F(NOINTERACTIVEBasicTest, Version)
{
    executeInClient({"version"});
}

TEST_F(NOINTERACTIVEBasicTest, Help)
{
    executeInClient({"help"});
}

TEST_F(NOINTERACTIVELoggedInTest, Whoami)
{
    auto r = executeInClient({"whoami"});
    ASSERT_STREQ(r.mOut.c_str(), OUTSTRING("Account e-mail: ").append(getenv("MEGACMD_TEST_USER")).append("\n").c_str());
}

TEST_F(NOINTERACTIVEReadTest, Find)
{
    auto r = executeInClient({"find"});
    //TODO: CMD-308: provide find coverage.
    ASSERT_NE(r.mOut.find("/testReadingFolder01/folder02/subfolder02/file02.txt"), OUTSTRING::npos);
}
