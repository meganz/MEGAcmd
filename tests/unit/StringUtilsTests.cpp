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

#include "Instruments.h"
#include "TestUtils.h"
#include "comunicationsmanager.h"
#include "megacmdcommonutils.h"

TEST(StringUtilsTest, rtrim)
{
    using megacmd::rtrim;

    G_SUBTEST << "Basic case";
    {
        std::string s("123456");
        EXPECT_EQ(rtrim(s, '2'), "123456");
        EXPECT_EQ(rtrim(s, '6'), "12345");
    }

    G_SUBTEST << "Empty string";
    {
        std::string s;
        EXPECT_EQ(rtrim(s, ' ').length(), 0);
    }

    G_SUBTEST << "Only trim character";
    {
        std::string s("   ");
        EXPECT_EQ(rtrim(s, ' ').length(), 0);
    }

    G_SUBTEST << "Multiple consecutive";
    {
        std::string s("11223344");
        EXPECT_EQ(rtrim(s, '4'), "112233");
    }

    G_SUBTEST << "Special case with next character check";
    {
        std::string s("aab");
        rtrim(s, 'a');
        EXPECT_EQ(s, "aab");
    }

    G_SUBTEST << "Single character at end";
    {
        std::string s("abc123");
        EXPECT_EQ(rtrim(s, '3'), "abc12");
    }

    G_SUBTEST << "Unicode characters";
    {
        std::string s("\xD0\x9C\xD0\x95\xD0\x93\xD0\x90   ");
        EXPECT_TRUE(megacmd::isValidUtf8(s));
        std::string result = rtrim(s, ' ');
        EXPECT_TRUE(megacmd::isValidUtf8(result));
        EXPECT_EQ(result, "\xD0\x9C\xD0\x95\xD0\x93\xD0\x90");
    }

    G_SUBTEST << "Single character string (trim character)";
    {
        std::string s("X");
        EXPECT_EQ(rtrim(s, 'X').length(), 0);
    }

    G_SUBTEST << "Single character string (not trim character)";
    {
        std::string s("X");
        EXPECT_EQ(rtrim(s, 'Y'), "X");
    }

    G_SUBTEST << "Special characters";
    {
        std::string s("test\t\t\t");
        EXPECT_EQ(rtrim(s, '\t'), "test");
    }
}

TEST(StringUtilsTest, ltrim)
{
    using megacmd::ltrim;

    G_SUBTEST << "Basic cases";
    {
        std::string s("123456");
        EXPECT_EQ(ltrim(s, '2'), "123456");
        EXPECT_EQ(ltrim(s, '1'), "23456");
    }

    G_SUBTEST << "Empty string";
    {
        std::string s("");
        EXPECT_EQ(ltrim(s, ' '), "");
    }

    G_SUBTEST << "Only trim character";
    {
        std::string s("   ");
        EXPECT_EQ(ltrim(s, ' '), "");
    }

    G_SUBTEST << "Multiple consecutive";
    {
        std::string s("1111222333");
        EXPECT_EQ(ltrim(s, '1'), "222333");
    }

    G_SUBTEST << "Character not at the beginning";
    {
        std::string s("abc123");
        EXPECT_EQ(ltrim(s, 'a'), "bc123");
        std::string s2("abc123");
        EXPECT_EQ(ltrim(s2, 'b'), "abc123");
    }

    G_SUBTEST << "Unicode characters";
    {
        std::string s("   \xD0\x9C\xD0\x95\xD0\x93\xD0\x90");
        EXPECT_TRUE(megacmd::isValidUtf8(s));
        std::string result = ltrim(s, ' ');
        EXPECT_TRUE(megacmd::isValidUtf8(result));
        EXPECT_EQ(result, "\xD0\x9C\xD0\x95\xD0\x93\xD0\x90");
    }

    G_SUBTEST << "Very long string";
    {
        std::string s(10000, '1');
        s += "test";
        std::string result = ltrim(s, '1');
        EXPECT_EQ(result, "test");
    }

    G_SUBTEST << "Single character string with trim character";
    {
        std::string s("X");
        EXPECT_EQ(ltrim(s, 'X'), "");
    }

    G_SUBTEST << "Single character string with no trim characters";
    {
        std::string s("X");
        EXPECT_EQ(ltrim(s, 'Y'), "X");
    }

    G_SUBTEST << "Mixed characters at start";
    {
        std::string s("XXYXXtest");
        EXPECT_EQ(ltrim(s, 'X'), "YXXtest");
    }

    G_SUBTEST << "Special characters";
    {
        std::string s("\t\t\ttest");
        EXPECT_EQ(ltrim(s, '\t'), "test");
    }

    G_SUBTEST << "string_view overload";
    {
        std::string_view sv("   test");
        std::string_view result = ltrim(sv, ' ');
        EXPECT_EQ(result, "test");
        EXPECT_EQ(sv, "   test");
    }

    G_SUBTEST << "Empty string_view";
    {
        std::string_view sv;
        std::string_view result = ltrim(sv, ' ');
        EXPECT_EQ(result.length(), 0);
    }

    G_SUBTEST << "string_view with multiple characters";
    {
        std::string_view sv("XXXtest");
        std::string_view result = ltrim(sv, 'X');
        EXPECT_EQ(result, "test");
    }

    G_SUBTEST << "string_view with only trim characters";
    {
        std::string_view sv("   ");
        std::string_view result = ltrim(sv, ' ');
        EXPECT_EQ(result.length(), 0);
    }

    G_SUBTEST << "string_view with no trim characters";
    {
        std::string_view sv("test");
        std::string_view result = ltrim(sv, ' ');
        EXPECT_EQ(result, "test");
    }

    G_SUBTEST << "string_view with a single character";
    {
        std::string_view sv("X");
        std::string_view result = ltrim(sv, 'X');
        EXPECT_EQ(result.length(), 0);
    }

    G_SUBTEST << "string_view from string";
    {
        std::string str("   test");
        std::string_view sv(str);
        std::string_view result = ltrim(sv, ' ');
        EXPECT_EQ(result, "test");
        EXPECT_EQ(str, "   test");
    }
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

TEST(StringUtilsTest, split)
{
    using megacmd::split;

    G_SUBTEST << "Basic split";
    {
        std::vector<std::string> result = split("a,b,c", ",");
        EXPECT_THAT(result, testing::ElementsAre("a", "b", "c"));
    }

    G_SUBTEST << "Empty string";
    {
        std::vector<std::string> result = split("", ",");
        EXPECT_TRUE(result.empty());
    }

    G_SUBTEST << "No delimiter found";
    {
        std::vector<std::string> result = split("abc", ",");
        EXPECT_THAT(result, testing::ElementsAre("abc"));
    }

    G_SUBTEST << "Multiple consecutive delimiters";
    {
        std::vector<std::string> result = split("a,,b,,c", ",");
        EXPECT_THAT(result, testing::ElementsAre("a", "b", "c"));
    }

    G_SUBTEST << "Delimiter at start";
    {
        std::vector<std::string> result = split(",a,b", ",");
        EXPECT_THAT(result, testing::ElementsAre("a", "b"));
    }

    G_SUBTEST << "Delimiter at end";
    {
        std::vector<std::string> result = split("a,b,", ",");
        EXPECT_THAT(result, testing::ElementsAre("a", "b"));
    }

    G_SUBTEST << "Multi-character delimiter";
    {
        std::vector<std::string> result = split("a::b::c", "::");
        EXPECT_THAT(result, testing::ElementsAre("a", "b", "c"));
    }

    G_SUBTEST << "Single character";
    {
        std::vector<std::string> result = split("a", ",");
        EXPECT_THAT(result, testing::ElementsAre("a"));
    }
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

    {
        G_SUBTEST << "DO_NOT_REDACT env variable override";
        auto doNotRedactGuard = TestInstrumentsEnvVarGuard("MEGACMD_DO_NOT_REDACT_LINES", "1");

        {
            CmdPetition petition;
            petition.setLine("some-command --password=MySecretPassword --some-arg=Something -fv");
            const std::string redacted = petition.getRedactedLine();

            EXPECT_THAT(redacted, testing::HasSubstr("--password=MySecretPassword"));
        }

        {
            CmdPetition petition;
            petition.setLine("login some-email@real-website.com SuperSecret1234!'");
            const std::string redacted = petition.getRedactedLine();

            EXPECT_THAT(redacted, testing::HasSubstr("some-email@real-website.com"));
            EXPECT_THAT(redacted, testing::HasSubstr("SuperSecret1234!'"));
            EXPECT_THAT(redacted, testing::Not(testing::HasSubstr("<REDACTED>")));
        }
    }
}

TEST(StringUtilsTest, hasWildCards)
{
    using megacmd::hasWildCards;

    G_SUBTEST << "Both wildcards";
    {
        std::string str = "file*?.txt";
        EXPECT_TRUE(hasWildCards(str));
        str = "*?";
        EXPECT_TRUE(hasWildCards(str));
    }

    G_SUBTEST << "Asterisk at different positions";
    {
        std::string str1 = "*file.txt";
        EXPECT_TRUE(hasWildCards(str1));
        std::string str2 = "file*.txt";
        EXPECT_TRUE(hasWildCards(str2));
        std::string str3 = "file.txt*";
        EXPECT_TRUE(hasWildCards(str3));
        std::string str4 = "*";
        EXPECT_TRUE(hasWildCards(str4));
    }

    G_SUBTEST << "Question mark at different positions";
    {
        std::string str1 = "?file.txt";
        EXPECT_TRUE(hasWildCards(str1));
        std::string str2 = "file?.txt";
        EXPECT_TRUE(hasWildCards(str2));
        std::string str3 = "file.txt?";
        EXPECT_TRUE(hasWildCards(str3));
        std::string str4 = "?";
        EXPECT_TRUE(hasWildCards(str4));
    }

    G_SUBTEST << "Multiple wildcards";
    {
        std::string str1 = "file**.*";
        EXPECT_TRUE(hasWildCards(str1));
        std::string str2 = "file??.txt";
        EXPECT_TRUE(hasWildCards(str2));
        std::string str3 = "*file*.txt";
        EXPECT_TRUE(hasWildCards(str3));
        std::string str4 = "?file?.txt";
        EXPECT_TRUE(hasWildCards(str4));
    }

    G_SUBTEST << "Special characters without wildcards";
    {
        std::string str1 = "file@.txt";
        EXPECT_FALSE(hasWildCards(str1));
        std::string str2 = "file#.txt";
        EXPECT_FALSE(hasWildCards(str2));
        std::string str3 = "file$.txt";
        EXPECT_FALSE(hasWildCards(str3));
        std::string str4 = "file[].txt";
        EXPECT_FALSE(hasWildCards(str4));
    }

    G_SUBTEST << "Unicode characters";
    {
        std::string str = u8"\u043F\u0440\u0438\u0432\u0435\u0442.txt";
        EXPECT_FALSE(hasWildCards(str));
        str = u8"\u043F\u0440\u0438*.txt";
        EXPECT_TRUE(hasWildCards(str));
    }

    G_SUBTEST << "Long strings";
    {
        std::string longStr(10000, 'a');
        EXPECT_FALSE(hasWildCards(longStr));
        longStr[5000] = '*';
        EXPECT_TRUE(hasWildCards(longStr));
        longStr[5000] = 'a';
        longStr[5000] = '?';
        EXPECT_TRUE(hasWildCards(longStr));
    }

    G_SUBTEST << "Wildcards with spaces";
    {
        std::string str1 = "file *.txt";
        EXPECT_TRUE(hasWildCards(str1));
        std::string str2 = "file ?.txt";
        EXPECT_TRUE(hasWildCards(str2));
        std::string str3 = "file .txt";
        EXPECT_FALSE(hasWildCards(str3));
    }

    G_SUBTEST << "Numbers and wildcards";
    {
        std::string str1 = "file123*.txt";
        EXPECT_TRUE(hasWildCards(str1));
        std::string str2 = "file?123.txt";
        EXPECT_TRUE(hasWildCards(str2));
        std::string str3 = "file123.txt";
        EXPECT_FALSE(hasWildCards(str3));
    }

    G_SUBTEST << "No wildcards";
    {
        std::string str = "file.txt";
        EXPECT_FALSE(hasWildCards(str));
    }

    G_SUBTEST << "Empty string";
    {
        std::string str;
        EXPECT_FALSE(hasWildCards(str));
    }
}

TEST(StringUtilsTest, isValidEmail)
{
    using megacmd::isValidEmail;

    G_SUBTEST << "Valid emails";
    {
        EXPECT_TRUE(isValidEmail("t@x.be"));
        EXPECT_TRUE(isValidEmail("test@example.com"));
        EXPECT_TRUE(isValidEmail("user.name@domain.co.uk"));
        EXPECT_TRUE(isValidEmail("user+tag@example.com"));
        EXPECT_TRUE(isValidEmail("user_x@example.dot.com"));
    }

    G_SUBTEST << "Invalid emails";
    {
        EXPECT_FALSE(isValidEmail("notanemail"));
        EXPECT_FALSE(isValidEmail("@example.com"));
        EXPECT_FALSE(isValidEmail("user@"));
        EXPECT_FALSE(isValidEmail("user@."));
        EXPECT_FALSE(isValidEmail("user@.com"));
        EXPECT_FALSE(isValidEmail("user@com."));
        EXPECT_FALSE(isValidEmail(" "));
    }

    G_SUBTEST << "Empty email";
    {
        EXPECT_FALSE(isValidEmail(""));
    }
}

TEST(StringUtilsTest, toInteger)
{
    using megacmd::toInteger;

    G_SUBTEST << "Valid positive integer";
    {
        EXPECT_EQ(toInteger("0"), 0);
        EXPECT_EQ(toInteger("12"), 12);
        EXPECT_EQ(toInteger("999"), 999);
    }

    G_SUBTEST << "Valid negative integer";
    {
        EXPECT_EQ(toInteger("-1", 0), -1);
        EXPECT_EQ(toInteger("-123"), -123);
    }

    G_SUBTEST << "Invalid string";
    {
        EXPECT_EQ(toInteger("abc"), -1);
        EXPECT_EQ(toInteger("12abc"), -1);
        EXPECT_EQ(toInteger("abc12"), -1);
    }

    G_SUBTEST << "Custom fail value";
    {
        EXPECT_EQ(toInteger("xyz", 0), 0);
        EXPECT_EQ(toInteger("abc", 999), 999);
    }

    G_SUBTEST << "Empty string";
    {
        EXPECT_EQ(toInteger(""), -1);
        EXPECT_EQ(toInteger("", 42), 42);
    }

    G_SUBTEST << "String with spaces";
    {
        EXPECT_EQ(toInteger(" 123 "), -1);
        EXPECT_EQ(toInteger("123 "), -1);
    }

    G_SUBTEST << "Out of range values";
    {
        std::string maxPlusOne = std::to_string(static_cast<long long>(INT_MAX) + 1);
        EXPECT_EQ(toInteger(maxPlusOne), -1);
        EXPECT_EQ(toInteger(maxPlusOne, 42), 42);

        std::string minMinusOne = std::to_string(static_cast<long long>(INT_MIN) - 1);
        EXPECT_EQ(toInteger(minMinusOne), -1);
        EXPECT_EQ(toInteger(minMinusOne, 99), 99);

        EXPECT_EQ(toInteger("999999999999999999999"), -1);
        EXPECT_EQ(toInteger("999999999999999999999", 0), 0);

        EXPECT_EQ(toInteger("-999999999999999999999"), -1);
        EXPECT_EQ(toInteger("-999999999999999999999", 0), 0);
    }
}

TEST(StringUtilsTest, getListOfWords)
{
    using megacmd::getlistOfWords;

    G_SUBTEST << "Basic case";
    {
        std::vector<std::string> words = getlistOfWords("mega-share -a --with=some-email@mega.co.nz some_dir/some_file.txt");
        EXPECT_THAT(words, testing::ElementsAre("mega-share", "-a", "--with=some-email@mega.co.nz", "some_dir/some_file.txt"));
    }

    G_SUBTEST << "Quotes";
    {
        {
            std::vector<std::string> words = getlistOfWords(R"(mkdir "some dir" "another dir")");
            EXPECT_THAT(words, testing::ElementsAre("mkdir", "some dir", "another dir"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(mkdir 'some dir' 'another dir')");
            EXPECT_THAT(words, testing::ElementsAre("mkdir", "some dir", "another dir"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(some "case" here)");
            EXPECT_THAT(words, testing::ElementsAre("some", "case", "here"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(some "other case with spaces" here)");
            EXPECT_THAT(words, testing::ElementsAre("some", "other case with spaces", "here"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(--another="here")");
            EXPECT_THAT(words, testing::ElementsAre(R"(--another="here")"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(--another="here" with extra bits)");
            EXPECT_THAT(words, testing::ElementsAre(R"(--another="here")", "with", "extra", "bits"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(--arg1="a b" word --arg2="c d")");
            EXPECT_THAT(words, testing::ElementsAre(R"(--arg1="a b")", "word", R"(--arg2="c d")"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(command "text\"quoted")");
            EXPECT_THAT(words, testing::ElementsAre("command", R"(text\)", R"(quoted")"));
        }
    }

    G_SUBTEST << "Unmatched quotes";
    {
        {
            std::vector<std::string> words = getlistOfWords(R"(find "something with quotes" odd/file"01.txt)");
            EXPECT_THAT(words, testing::ElementsAre("find", "something with quotes", R"(odd/file"01.txt)"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(--something="quote missing)");
            EXPECT_THAT(words, testing::ElementsAre(R"(--something="quote missing)"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(find 'something with quotes' odd/file'01.txt)");
            EXPECT_THAT(words, testing::ElementsAre("find", "something with quotes", R"(odd/file'01.txt)"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(--something='quote missing)");
            EXPECT_THAT(words, testing::ElementsAre(R"(--something='quote)", "missing"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(command 'unclosed quote)");
            EXPECT_THAT(words, testing::ElementsAre("command", "unclosed quote"));
        }
    }

    G_SUBTEST << "Completion mode and flags";
    {
        {
            constexpr const char* str = R"(get root_dir\some_dir\some_file.jpg another_file.txt)";
            constexpr const char* completion_str = R"(completion get root_dir\some_dir\some_file.jpg another_file.txt)";

            std::vector<std::string> words = getlistOfWords(str, true);
            EXPECT_THAT(words, testing::ElementsAre("get", R"(root_dir\some_dir\some_file.jpg)", "another_file.txt"));

            words = getlistOfWords(completion_str, true);
            EXPECT_THAT(words, testing::ElementsAre("completion", "get", R"(root_dir\\some_dir\\some_file.jpg)", "another_file.txt"));

            words = getlistOfWords(str, false);
            EXPECT_THAT(words, testing::ElementsAre("get", R"(root_dir\some_dir\some_file.jpg)", "another_file.txt"));
        }
        {
            std::vector<std::string> words = getlistOfWords("completion", true);
            EXPECT_THAT(words, testing::ElementsAre("completion"));
        }
        {
            std::vector<std::string> words = getlistOfWords("completion   ", true, true);
            EXPECT_THAT(words, testing::ElementsAre("completion"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(notcompletion path\file.txt)", true);
            EXPECT_THAT(words, testing::ElementsAre("notcompletion", R"(path\file.txt)"));
        }
        {
            constexpr const char* str = R"(completion get path\file.txt    )";
            std::vector<std::string> words = getlistOfWords(str, true, false);
            EXPECT_THAT(words, testing::ElementsAre("completion", "get", R"(path\\file.txt)", ""));
        }
        {
            constexpr const char* str = R"(completion get path\file.txt    )";
            std::vector<std::string> words = getlistOfWords(str, true, true);
            EXPECT_THAT(words, testing::ElementsAre("completion", "get", R"(path\\file.txt)"));
        }
        {
            constexpr const char* str = "completion arg    ";
            std::vector<std::string> words = getlistOfWords(str, true, true);
            EXPECT_THAT(words, testing::ElementsAre("completion", "arg"));
        }
        {
            constexpr const char* str = "  completion arg  ";
            std::vector<std::string> words = getlistOfWords(str, true, true);
            EXPECT_THAT(words, testing::ElementsAre("completion", "arg"));
        }
    }

    G_SUBTEST << "Very long strings";
    {
        {
            std::string longString = "command ";
            longString += std::string(10000, 'a');
            longString += " argument";
            std::vector<std::string> words = getlistOfWords(longString.c_str());
            EXPECT_THAT(words, testing::ElementsAre("command", testing::SizeIs(10000), "argument"));
        }
        {
            std::string longQuotedString = R"(command ")";
            longQuotedString += std::string(5000, 'b');
            longQuotedString += R"(" argument)";
            std::vector<std::string> words = getlistOfWords(longQuotedString.c_str());
            EXPECT_THAT(words, testing::ElementsAre("command", testing::SizeIs(5000), "argument"));
        }
    }

    G_SUBTEST << "Nested quotes";
    {
        {
            std::vector<std::string> words = getlistOfWords(R"(command 'text "nested" text')");
            EXPECT_THAT(words, testing::ElementsAre("command", R"(text "nested" text)"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(command "text 'nested' text")");
            EXPECT_THAT(words, testing::ElementsAre("command", R"(text 'nested' text)"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(cmd1 'outer "inner" outer' cmd2 "outer 'inner' outer")");
            EXPECT_THAT(words, testing::ElementsAre("cmd1", R"(outer "inner" outer)", "cmd2", R"(outer 'inner' outer)"));
        }
    }

    G_SUBTEST << "Backslash handling";
    {
        {
            std::vector<std::string> words = getlistOfWords("command \"text\\nnewline\" \"text\\ttab\"");
            EXPECT_THAT(words, testing::ElementsAre("command", "text\\nnewline", "text\\ttab"));
        }
        {
            std::vector<std::string> words = getlistOfWords("command 'text\\nnewline' 'text\\ttab'");
            EXPECT_THAT(words, testing::ElementsAre("command", "text\\nnewline", "text\\ttab"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(command "path\\to\\file")");
            EXPECT_THAT(words, testing::ElementsAre("command", R"(path\\to\\file)"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(command "path\\\\to\\\\file")");
            EXPECT_THAT(words, testing::ElementsAre("command", R"(path\\\\to\\\\file)"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(completion get path\\\\file.txt)", true);
            EXPECT_THAT(words, testing::ElementsAre("completion", "get", R"(path\\\\\\\\file.txt)"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(cmd path\\\\\file)");
            EXPECT_THAT(words, testing::ElementsAre("cmd", R"(path\\\\\file)"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(command "text\" arg)");
            EXPECT_THAT(words, testing::ElementsAre("command", R"(text\)", "arg"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(command 'text\' arg)");
            EXPECT_THAT(words, testing::ElementsAre("command", R"(text\)", "arg"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(command path\)");
            EXPECT_THAT(words, testing::ElementsAre("command", R"(path\)"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(command path\ with\ spaces)");
            EXPECT_THAT(words, testing::ElementsAre("command", R"(path\ with\ spaces)"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(cmd file\ name)");
            EXPECT_THAT(words, testing::ElementsAre("cmd", R"(file\ name)"));
        }
    }

    G_SUBTEST << "Special characters";
    {
        {
            std::vector<std::string> words = getlistOfWords("command file@name#123$test%value");
            EXPECT_THAT(words, testing::ElementsAre("command", "file@name#123$test%value"));
        }
        {
            constexpr const char* unicodeStr = "command \"\xD0\x9C\xD0\x95\xD0\x93\xD0\x90\"";
            std::vector<std::string> words = getlistOfWords(unicodeStr);
            EXPECT_THAT(words, testing::ElementsAre("command", "\xD0\x9C\xD0\x95\xD0\x93\xD0\x90"));
        }
        {
            std::vector<std::string> words = getlistOfWords("command \"text\1bell\"");
            EXPECT_THAT(words, testing::ElementsAre("command", "text\1bell"));
        }
        {
            std::vector<std::string> words = getlistOfWords("cmd\1arg1\2arg2");
            EXPECT_THAT(words, testing::ElementsAre("cmd\1arg1\2arg2"));
        }
    }

    G_SUBTEST << "Whitespace and empty strings";
    {
        {
            std::vector<std::string> words = getlistOfWords("");
            EXPECT_TRUE(words.empty());
        }
        {
            std::vector<std::string> words = getlistOfWords("   ", false, true);
            EXPECT_TRUE(words.empty());
        }
        {
            std::vector<std::string> words = getlistOfWords("   ", false, false);
            EXPECT_THAT(words, testing::ElementsAre(""));
        }
        {
            constexpr const char* str = "export -f --writable some_dir/some_file.txt    ";
            std::vector<std::string> words = getlistOfWords(str, false, false);
            EXPECT_THAT(words, testing::ElementsAre("export", "-f", "--writable", "some_dir/some_file.txt", ""));
            words = getlistOfWords(str, false, true);
            EXPECT_THAT(words, testing::ElementsAre("export", "-f", "--writable", "some_dir/some_file.txt"));
        }
        {
            std::vector<std::string> words = getlistOfWords("   command arg", false, false);
            EXPECT_THAT(words, testing::ElementsAre("command", "arg"));
        }
        {
            std::vector<std::string> words = getlistOfWords("   command arg", false, true);
            EXPECT_THAT(words, testing::ElementsAre("command", "arg"));
        }
        {
            std::vector<std::string> words = getlistOfWords("\t\tcommand\targ", false, true);
            EXPECT_THAT(words, testing::ElementsAre("command\targ"));
        }
        {
            std::vector<std::string> words = getlistOfWords("command\targ1\targ2");
            EXPECT_THAT(words, testing::ElementsAre("command\targ1\targ2"));
        }
        {
            std::vector<std::string> words = getlistOfWords("  \t  command  \t  arg  \t  ");
            EXPECT_THAT(words, testing::ElementsAre("command", "arg", ""));
        }
        {
            std::vector<std::string> words = getlistOfWords("command \"text\twith\ttabs\"");
            EXPECT_THAT(words, testing::ElementsAre("command", "text\twith\ttabs"));
        }
        {
            std::vector<std::string> words = getlistOfWords("command\narg1\narg2");
            EXPECT_THAT(words, testing::ElementsAre("command\narg1\narg2"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(command "" "")");
            EXPECT_THAT(words, testing::ElementsAre("command", "", ""));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(command " " ' ')");
            EXPECT_THAT(words, testing::ElementsAre("command", " ", " "));
        }
        {
            std::vector<std::string> words = getlistOfWords("command    ", false, false);
            EXPECT_THAT(words, testing::ElementsAre("command", ""));
        }
    }

    G_SUBTEST << "Unquoted text with embedded quotes";
    {
        {
            std::vector<std::string> words = getlistOfWords(R"(command path"with"quote)");
            EXPECT_THAT(words, testing::ElementsAre("command", R"(path"with"quote)"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(cmd file"name.txt)");
            EXPECT_THAT(words, testing::ElementsAre("cmd", R"(file"name.txt)"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(cmd file'name.txt)");
            EXPECT_THAT(words, testing::ElementsAre("cmd", R"(file'name.txt)"));
        }
        {
            std::vector<std::string> words = getlistOfWords(R"(cmd "quoted"and"more")");
            EXPECT_THAT(words, testing::ElementsAre("cmd", "quoted", R"(and"more")"));
        }
    }

    G_SUBTEST << "Invalid UTF-8 sequences";
    {
        {
            // Invalid UTF-8: continuation byte without start byte
            constexpr const char* invalidUtf8 = "command \x80\x81\x82";
            std::vector<std::string> words = getlistOfWords(invalidUtf8);
            EXPECT_THAT(words, testing::ElementsAre("command", "\x80\x81\x82"));
        }
        {
            // Invalid UTF-8: incomplete sequence
            constexpr const char* invalidUtf8 = "cmd \"\xC2\"";
            std::vector<std::string> words = getlistOfWords(invalidUtf8);
            EXPECT_THAT(words, testing::ElementsAre("cmd", "\xC2"));
        }
        {
            // Invalid UTF-8: overlong encoding
            constexpr const char* invalidUtf8 = "command \xC0\x80";
            std::vector<std::string> words = getlistOfWords(invalidUtf8);
            EXPECT_THAT(words, testing::ElementsAre("command", "\xC0\x80"));
        }
    }
}

TEST(StringUtilsTest, replace)
{
    using megacmd::replace;

    G_SUBTEST << "Basic replace";
    {
        std::string str("hello world");
        EXPECT_TRUE(replace(str, "world", "mega"));
        EXPECT_EQ(str, "hello mega");
    }

    G_SUBTEST << "Replace not found";
    {
        std::string str("test string");
        EXPECT_FALSE(replace(str, "xyz", "mega"));
        EXPECT_EQ(str, "test string");
    }

    G_SUBTEST << "Replace empty string";
    {
        std::string str("hello");
        EXPECT_TRUE(replace(str, "", "test"));
        EXPECT_EQ(str, "testhello");
    }

    G_SUBTEST << "Replace with empty string";
    {
        std::string str("remove this");
        EXPECT_TRUE(replace(str, "this", ""));
        EXPECT_EQ(str, "remove ");
    }

    G_SUBTEST << "Multiple occurrences with only first replaced";
    {
        std::string str("hello hello");
        EXPECT_TRUE(replace(str, "hello", "hi"));
        EXPECT_EQ(str, "hi hello");
    }

    G_SUBTEST << "Replace at beginning";
    {
        std::string str("start end");
        EXPECT_TRUE(replace(str, "start", "begin"));
        EXPECT_EQ(str, "begin end");
    }

    G_SUBTEST << "Replace at end";
    {
        std::string str("begin end");
        EXPECT_TRUE(replace(str, "end", "finish"));
        EXPECT_EQ(str, "begin finish");
    }
}

TEST(StringUtilsTest, replaceAll)
{
    using megacmd::replaceAll;

    G_SUBTEST << "Basic replaceAll";
    {
        std::string str("hello world world");
        replaceAll(str, "world", "mega");
        EXPECT_EQ(str, "hello mega mega");
    }

    G_SUBTEST << "No occurrences";
    {
        std::string str("test string");
        replaceAll(str, "xyz", "mega");
        EXPECT_EQ(str, "test string");
    }

    G_SUBTEST << "Multiple occurrences";
    {
        std::string str("a b a b a");
        replaceAll(str, "a", "x");
        EXPECT_EQ(str, "x b x b x");
    }

    G_SUBTEST << "Replace with empty string";
    {
        std::string str("remove this");
        replaceAll(str, "this", "");
        EXPECT_EQ(str, "remove ");
    }

    G_SUBTEST << "Overlapping patterns";
    {
        std::string str("aaa");
        replaceAll(str, "aa", "b");
        EXPECT_EQ(str, "ba");
        replaceAll(str, "a", "b");
        EXPECT_EQ(str, "");
    }
}
