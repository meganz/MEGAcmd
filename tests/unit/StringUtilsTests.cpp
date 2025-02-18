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

#include "TestUtils.h"
#include "megacmdcommonutils.h"
#include "comunicationsmanager.h"

namespace StringUtilsTest
{
    static void trimming()
    {
        using megacmd::ltrim;
        using megacmd::rtrim;

        {
            std::string s("123456");
            G_SUBTEST << "Left trimming";
            ASSERT_STREQ(ltrim(s, '2').c_str(), "123456");
            ASSERT_STREQ(ltrim(s, '1').c_str(), "23456");
        }
        {
            std::string s("123456");
            G_SUBTEST << "Right trimming";
            ASSERT_STREQ(rtrim(s, '2').c_str(), "123456");
            ASSERT_STREQ(rtrim(s, '6').c_str(), "12345");
        }
    }
}

TEST(StringUtilsTest, trimming)
{
    StringUtilsTest::trimming();
}

TEST(StringUtilsTest, ValidateUtf8)
{
    // from: https://www.php.net/manual/en/reference.pcre.pattern.modifiers.php#54805
    EXPECT_TRUE(megacmd::isValidUtf8(std::string("a")));
    EXPECT_TRUE(megacmd::isValidUtf8(std::string("\xc3\xb1")));
    EXPECT_TRUE(megacmd::isValidUtf8(std::string("\xe2\x82\xa1")));
    EXPECT_TRUE(megacmd::isValidUtf8(std::string("\xf0\x90\x8c\xbc")));
    EXPECT_TRUE(megacmd::isValidUtf8(std::string("\xf4\x80\x80\x80")));
    EXPECT_TRUE(megacmd::isValidUtf8(std::string("\xf3\xa1\xa1\xa1")));
    EXPECT_TRUE(megacmd::isValidUtf8(std::string("\xc2\x80")));
    EXPECT_TRUE(megacmd::isValidUtf8(std::string("\xe0\xa0\x80")));
    EXPECT_TRUE(megacmd::isValidUtf8(std::string("\xf0\x90\x80\x80")));
    EXPECT_TRUE(megacmd::isValidUtf8(std::string("\xf4\x8f\xbf\xbf")));
    EXPECT_TRUE(megacmd::isValidUtf8(std::string("\xed\x9f\xbf")));

    EXPECT_FALSE(megacmd::isValidUtf8(std::string("\xf8\xa1\xa1\xa1\xa1")));      // 1st byte: 1111 1xxx
    EXPECT_FALSE(megacmd::isValidUtf8(std::string("\xfc\xa1\xa1\xa1\xa1\xa1")));  // 1st byte: 1111 1xxx
    EXPECT_FALSE(megacmd::isValidUtf8(std::string("\xE0\xA1\xD0")));              // 3rd byte: 11xx xxxx
    EXPECT_FALSE(megacmd::isValidUtf8(std::string("\xc3\x28")));                  // 2nd byte: 00xx xxxx
    EXPECT_FALSE(megacmd::isValidUtf8(std::string("\xa0\xa1")));                  // 1st byte: 10xx xxxx
    EXPECT_FALSE(megacmd::isValidUtf8(std::string("\xe2\x28\xa1")));              // 2nd byte: 00xx xxxx
    EXPECT_FALSE(megacmd::isValidUtf8(std::string("\xe2\x82\x28")));              // 3rd byte: 00xx xxxx
    EXPECT_FALSE(megacmd::isValidUtf8(std::string("\xf0\x28\x8c\xbc")));          // 2nd byte: 00xx xxxx
    EXPECT_FALSE(megacmd::isValidUtf8(std::string("\xf0\x90\x28\xbc")));          // 3rd byte: 00xx xxxx
    EXPECT_FALSE(megacmd::isValidUtf8(std::string("\xf0\x28\x8c\x28")));          // 2nd & 3rd bytes: 00xx xxxx
    EXPECT_FALSE(megacmd::isValidUtf8(std::string("\xc1\xbf")));                  // codepoint less than U+0080
    EXPECT_FALSE(megacmd::isValidUtf8(std::string("\xe0\x9f\xbf")));              // codepoint less than U+0800
    EXPECT_FALSE(megacmd::isValidUtf8(std::string("\xf0\x8f\xbf\xbf")));          // codepoint less than U+10000
    EXPECT_FALSE(megacmd::isValidUtf8(std::string("\xf4\x90\x80\x80")));          // codepoint greater than U+10FFFF
    EXPECT_FALSE(megacmd::isValidUtf8(std::string("\xf4\x8f\xbf")));              // Length: expected = 4, actual = 3
    EXPECT_FALSE(megacmd::isValidUtf8(std::string("\xe0\xa0")));                  // Length: expected = 3, actual = 2
    EXPECT_FALSE(megacmd::isValidUtf8(std::string("\xc2")));                      // Length: expected = 2, actual = 1
    EXPECT_FALSE(megacmd::isValidUtf8(std::string("\xed\xa0\x80")));              // surrogate codepoint U+D800
    EXPECT_FALSE(megacmd::isValidUtf8(std::string("\xed\xbf\xbf")));              // surrogate codepoint U+DFFF
}

TEST(StringUtilsTest, nonAsciiToStringstream)
{
    const char* char_str = u8"\uc548\uc548\ub155\ud558\uc138\uc694\uc138\uacc4";
    const wchar_t* wchar_str = L"\uc548\uc548\ub155\ud558\uc138\uc694\uc138\uacc4";
    const std::string str = u8"\uc548\uc548\ub155\ud558\uc138\uc694\uc138\uacc4";
    const std::wstring wstr = L"\uc548\uc548\ub155\ud558\uc138\uc694\uc138\uacc4";

    {
        std::ostringstream ostream;
        ostream << char_str;
        EXPECT_STREQ(ostream.str().c_str(), char_str);

        std::wostringstream wostream;
        wostream << wchar_str;
        EXPECT_STREQ(wostream.str().c_str(), wchar_str);
    }
    {
        std::ostringstream ostream;
        ostream << str;
        EXPECT_EQ(ostream.str(), str);

        std::wostringstream wostream;
        wostream << wstr;
        EXPECT_EQ(wostream.str(), wstr);
    }

#ifdef _WIN32
    using namespace megacmd; // otherwise the ostream overload for wstrings won't be visible
                             // we need it to ensure the static_assert is not triggered
    {
        std::ostringstream ostream;
        ostream << utf16ToUtf8(wchar_str);
        EXPECT_STREQ(ostream.str().c_str(), char_str);

        std::wostringstream wostream;
        wostream << char_str;
        EXPECT_STREQ(wostream.str().c_str(), wchar_str);
    }
    {
        std::ostringstream ostream;
        ostream << utf16ToUtf8(wstr);
        EXPECT_EQ(ostream.str(), str);

        std::wostringstream wostream;
        wostream << str;
        EXPECT_EQ(wostream.str(), wstr);
    }
#endif
}

TEST(StringUtilsTest, redactedCmdPetition)
{
    using megacmd::CmdPetition;

    {
        G_SUBTEST << "Redact password";

        CmdPetition petition;
        petition.setLine("some-command --password=MySecretPassword --some-arg=Something -fv");
        const std::string redacted = petition.getRedactedLine();

        EXPECT_THAT(redacted, testing::Not(testing::HasSubstr("MySecretPassword")));
        EXPECT_THAT(redacted, testing::HasSubstr("--password=********"));
        EXPECT_THAT(redacted, testing::HasSubstr("--some-arg=Something"));
    }

    {
        G_SUBTEST << "Redact password with single quotes";

        CmdPetition petition;
        petition.setLine("some-command --password='My Secret Password' --some-arg=Something -fv");
        const std::string redacted = petition.getRedactedLine();

        EXPECT_THAT(redacted, testing::Not(testing::HasSubstr("My Secret Password")));
        EXPECT_THAT(redacted, testing::HasSubstr("--password=********"));
        EXPECT_THAT(redacted, testing::HasSubstr("--some-arg=Something"));

        // Ensure the full password gets redacted, not only the first word
        EXPECT_THAT(redacted, testing::Not(testing::HasSubstr("Secret Password")));
        EXPECT_THAT(redacted, testing::Not(testing::HasSubstr("Password")));
    }

    {
        G_SUBTEST << "Redact password with double quotes";

        CmdPetition petition;
        petition.setLine("some-command --password=\"My Secret Password\" --some-arg=Something -fv");
        const std::string redacted = petition.getRedactedLine();

        EXPECT_THAT(redacted, testing::Not(testing::HasSubstr("My Secret Password")));
        EXPECT_THAT(redacted, testing::HasSubstr("--password=********"));
        EXPECT_THAT(redacted, testing::HasSubstr("--some-arg=Something"));

        // Ensure the full password gets redacted, not only the first word
        EXPECT_THAT(redacted, testing::Not(testing::HasSubstr("Secret Password")));
        EXPECT_THAT(redacted, testing::Not(testing::HasSubstr("Password")));
    }

    {
        G_SUBTEST << "Redact auth-key and auth-code";

        CmdPetition petition;
        petition.setLine("some-command --auth-code=abc123 --some-arg=Something --auth-key=def456");
        const std::string redacted = petition.getRedactedLine();

        EXPECT_THAT(redacted, testing::Not(testing::HasSubstr("abc123")));
        EXPECT_THAT(redacted, testing::Not(testing::HasSubstr("def456")));
        EXPECT_THAT(redacted, testing::HasSubstr("--auth-code=********"));
        EXPECT_THAT(redacted, testing::HasSubstr("--auth-key=********"));
        EXPECT_THAT(redacted, testing::HasSubstr("--some-arg=Something"));
    }

    {
        G_SUBTEST << "Redact login";

        CmdPetition petition;
        petition.setLine("login some-email@real-website.com SuperSecret1234!'");
        const std::string redacted = petition.getRedactedLine();

        EXPECT_EQ(redacted, "login <REDACTED>");
    }

    {
        G_SUBTEST << "Redact login no email";

        CmdPetition petition;
        petition.setLine("login https://mega.nz/folder/bxomFKwL#3V1dUJFzL98t1GqXX29IXg");
        const std::string redacted = petition.getRedactedLine();

        EXPECT_EQ(redacted, "login <REDACTED>");
    }

    {
        G_SUBTEST << "Redact login even with auth-code";

        CmdPetition petition;
        petition.setLine("login --auth-code=1SomeAuthCode1 SomeSuperSecret");
        const std::string redacted = petition.getRedactedLine();

        EXPECT_EQ(redacted, "login <REDACTED>");
    }

    {
        G_SUBTEST << "Redact passwd";

        CmdPetition petition;
        petition.setLine("passwd -f NewPassword");
        const std::string redacted = petition.getRedactedLine();

        EXPECT_EQ(redacted, "passwd <REDACTED>");
    }

    {
        G_SUBTEST << "Redact passwd from shell";

        CmdPetition petition;
        petition.setLine("Xpasswd -f NewPassword");
        const std::string redacted = petition.getRedactedLine();

        EXPECT_EQ(redacted, "Xpasswd <REDACTED>");
    }

    {
        G_SUBTEST << "Redact confirm";

        CmdPetition petition;
        petition.setLine("confirm somelink1234 Password");
        const std::string redacted = petition.getRedactedLine();

        EXPECT_EQ(redacted, "confirm <REDACTED>");
    }

    {
        G_SUBTEST << "Redact confirmcancel";

        CmdPetition petition;
        petition.setLine("confirmcancel somelink123 Password");
        const std::string redacted = petition.getRedactedLine();

        EXPECT_EQ(redacted, "confirmcancel <REDACTED>");
    }

    {
        G_SUBTEST << "Redact folder link";

        CmdPetition petition;
        petition.setLine("import https://mega.nz/folder/bxomFKwL#3V1dUJFzL98t1GqXX29IXg");
        const std::string redacted = petition.getRedactedLine();

        EXPECT_THAT(redacted, testing::Not(testing::HasSubstr("3V1dUJFzL98t1GqXX29IXg")));
        EXPECT_THAT(redacted, testing::HasSubstr("https://mega.nz/folder/bxomFKwL#********"));
    }

    {
        G_SUBTEST << "Redact file link";

        CmdPetition petition;
        petition.setLine("get https://mega.nz/file/d0w0nyiy#egvjqp5r-anbbdsjg8qrvg");
        const std::string redacted = petition.getRedactedLine();

        EXPECT_THAT(redacted, testing::Not(testing::HasSubstr("egvjqp5r-anbbdsjg8qrvg")));
        EXPECT_THAT(redacted, testing::HasSubstr("https://mega.nz/file/d0w0nyiy#********"));
    }

    {
        G_SUBTEST << "Redact old folder link";

        CmdPetition petition;
        petition.setLine("get https://mega.nz/#F!bxomFKwL#3V1dUJFzL98t1GqXX29IXg");
        const std::string redacted = petition.getRedactedLine();

        EXPECT_THAT(redacted, testing::Not(testing::HasSubstr("3V1dUJFzL98t1GqXX29IXg")));
        EXPECT_THAT(redacted, testing::HasSubstr("https://mega.nz/#F!bxomFKwL#********"));
    }

    {
        G_SUBTEST << "Redact old file link";

        CmdPetition petition;
        petition.setLine("import https://mega.nz/#!d0w0nyiy#egvjqp5r-anbbdsjg8qrvg");
        const std::string redacted = petition.getRedactedLine();

        EXPECT_THAT(redacted, testing::Not(testing::HasSubstr("egvjqp5r-anbbdsjg8qrvg")));
        EXPECT_THAT(redacted, testing::HasSubstr("https://mega.nz/#!d0w0nyiy#********"));
    }

    {
        G_SUBTEST << "Redact encrypted link";

        CmdPetition petition;
        petition.setLine("some-other-command https://mega.nz/#P!egvjqp5r-anbbdsjg8qrvg");
        const std::string redacted = petition.getRedactedLine();

        EXPECT_THAT(redacted, testing::Not(testing::HasSubstr("egvjqp5r-anbbdsjg8qrvg")));
        EXPECT_THAT(redacted, testing::HasSubstr("https://mega.nz/#P!********"));
    }
}
