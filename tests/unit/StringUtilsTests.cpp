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

TEST(StringUtilsTest, removeTrailingSeparators)
{
    using megacmd::removeTrailingSeparators;

    G_SUBTEST << "Empty string";
    {
        std::string path;
        std::string result = removeTrailingSeparators(path);
        EXPECT_EQ(result, "");
    }

    G_SUBTEST << "Root path";
    {
        std::string path = "/";
        std::string result = removeTrailingSeparators(path);
        EXPECT_EQ(result, "/");
    }

    G_SUBTEST << "Two slashes only";
    {
        std::string path = "//";
        std::string result = removeTrailingSeparators(path);
        EXPECT_EQ(result, "/");
    }

    G_SUBTEST << "Windows backslash only";
    {
        std::string path = R"(\)";
        std::string result = removeTrailingSeparators(path);
        EXPECT_EQ(result, "");
    }

    G_SUBTEST << "Path with only backslashes";
    {
        std::string path = R"(\\)";
        std::string result = removeTrailingSeparators(path);
        EXPECT_EQ(result, "");
    }

    G_SUBTEST << "Unix path without trailing separator";
    {
        std::string path = "/path/to/dir";
        std::string result = removeTrailingSeparators(path);
        EXPECT_EQ(result, "/path/to/dir");
    }

    G_SUBTEST << "Unix path with trailing separator";
    {
        std::string path = "/path/to/dir/";
        std::string result = removeTrailingSeparators(path);
        EXPECT_EQ(result, "/path/to/dir");
    }

    G_SUBTEST << "Unix path with multiple trailing separators";
    {
        std::string path = "/path/to/dir///";
        std::string result = removeTrailingSeparators(path);
        EXPECT_EQ(result, "/path/to/dir");
    }

    G_SUBTEST << "Unix path with file";
    {
        std::string path = "/path/to/dir/file.txt";
        std::string result = removeTrailingSeparators(path);
        EXPECT_EQ(result, "/path/to/dir/file.txt");
    }

    G_SUBTEST << "Windows path without trailing separator";
    {
        std::string path = R"(C:\path\to\dir)";
        std::string result = removeTrailingSeparators(path);
        EXPECT_EQ(result, R"(C:\path\to\dir)");
    }

    G_SUBTEST << "Windows path with trailing backslash";
    {
        std::string path = R"(C:\path\to\dir\)";
        std::string result = removeTrailingSeparators(path);
        EXPECT_EQ(result, R"(C:\path\to\dir)");
    }

    G_SUBTEST << "Windows path with multiple trailing backslashes";
    {
        std::string path = R"(C:\path\to\dir\\)";
        std::string result = removeTrailingSeparators(path);
        EXPECT_EQ(result, R"(C:\path\to\dir)");
    }

    G_SUBTEST << "Windows root path C:\\\\";
    {
        std::string path = R"(C:\\)";
        std::string result = removeTrailingSeparators(path);
        EXPECT_EQ(result, "C:");
    }

    G_SUBTEST << "Windows root path C:\\";
    {
        std::string path = R"(C:\)";
        std::string result = removeTrailingSeparators(path);
        EXPECT_EQ(result, "C:");
    }

    G_SUBTEST << "Windows path with file";
    {
        std::string path = R"(C:\path\to\dir\file.txt)";
        std::string result = removeTrailingSeparators(path);
        EXPECT_EQ(result, R"(C:\path\to\dir\file.txt)");
    }

    G_SUBTEST << "Windows path ending with file extension";
    {
        std::string path = R"(C:\path\file.txt\)";
        std::string result = removeTrailingSeparators(path);
        EXPECT_EQ(result, R"(C:\path\file.txt)");
    }

    G_SUBTEST << "Mixed trailing separators (not fully removed)";
    {
        {
            std::string path = R"(/path/to/dir/\)";
            std::string result = removeTrailingSeparators(path);
            EXPECT_EQ(result, "/path/to/dir/");
        }
        {
            std::string path = R"(C:\path\to\dir/\)";
            std::string result = removeTrailingSeparators(path);
            EXPECT_EQ(result, R"(C:\path\to\dir/)");
        }
    }

    G_SUBTEST << "Mixed separators in middle and trailing";
    {
        std::string path = R"(C:\path/to\dir/)";
        std::string result = removeTrailingSeparators(path);
        EXPECT_EQ(result, R"(C:\path/to\dir)");
    }

    G_SUBTEST << "Single character path without separator";
    {
        std::string path = "a";
        std::string result = removeTrailingSeparators(path);
        EXPECT_EQ(result, "a");
    }

    G_SUBTEST << "Single character path with forward slash";
    {
        std::string path = "a/";
        std::string result = removeTrailingSeparators(path);
        EXPECT_EQ(result, "a");
    }

    G_SUBTEST << "Single character path with backslash";
    {
        std::string path = R"(a\)";
        std::string result = removeTrailingSeparators(path);
        EXPECT_EQ(result, "a");
    }

    G_SUBTEST << "UNC path with trailing backslash";
    {
        std::string path = R"(\\server\share\)";
        std::string result = removeTrailingSeparators(path);
        EXPECT_EQ(result, R"(\\server\share)");
    }

    G_SUBTEST << "UNC path with multiple trailing backslashes";
    {
        std::string path = R"(\\server\share\\)";
        std::string result = removeTrailingSeparators(path);
        EXPECT_EQ(result, R"(\\server\share)");
    }

    G_SUBTEST << "Relative path with trailing separator";
    {
        std::string path = R"(relative\path\)";
        std::string result = removeTrailingSeparators(path);
        EXPECT_EQ(result, R"(relative\path)");
    }

    G_SUBTEST << "Relative path without trailing separator";
    {
        std::string path = R"(relative\path)";
        std::string result = removeTrailingSeparators(path);
        EXPECT_EQ(result, R"(relative\path)");
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

    G_SUBTEST << "Basic case";
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

    G_SUBTEST << "Empty input string";
    {
        std::string str;
        EXPECT_FALSE(replace(str, "test", "mega"));
        EXPECT_EQ(str, "");
    }

    G_SUBTEST << "Self-replacement";
    {
        std::string str("test");
        EXPECT_TRUE(replace(str, "test", "test"));
        EXPECT_EQ(str, "test");
    }

    G_SUBTEST << "Pattern longer than string";
    {
        std::string str("short");
        EXPECT_FALSE(replace(str, "very long pattern", "mega"));
        EXPECT_EQ(str, "short");
    }

    G_SUBTEST << "Unicode characters";
    {
        std::string str("\xD0\x9C\xD0\x95\xD0\x93\xD0\x90");
        EXPECT_TRUE(replace(str, "\xD0\x9C\xD0\x95\xD0\x93\xD0\x90", "MEGA"));
        EXPECT_EQ(str, "MEGA");
    }

    G_SUBTEST << "Special characters";
    {
        std::string str("test\ttest");
        EXPECT_TRUE(replace(str, "\t", " "));
        EXPECT_EQ(str, "test test");
    }

    G_SUBTEST << "Pattern is substring of replacement";
    {
        std::string str("a");
        EXPECT_TRUE(replace(str, "a", "ab"));
        EXPECT_EQ(str, "ab");
    }

    G_SUBTEST << "Very long string";
    {
        std::string str(10000, 'a');
        str += "test";
        EXPECT_TRUE(replace(str, "test", "mega"));
        EXPECT_EQ(str, std::string(10000, 'a') + "mega");
    }

    G_SUBTEST << "Replace entire string";
    {
        std::string str("full");
        EXPECT_TRUE(replace(str, "full", "empty"));
        EXPECT_EQ(str, "empty");
    }

    G_SUBTEST << "Single character string";
    {
        std::string str("a");
        EXPECT_TRUE(replace(str, "a", "b"));
        EXPECT_EQ(str, "b");
    }

    G_SUBTEST << "Pattern at multiple positions but only first replaced";
    {
        std::string str("abcabcabc");
        EXPECT_TRUE(replace(str, "abc", "x"));
        EXPECT_EQ(str, "xabcabc");
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
        EXPECT_EQ(str, "bb");
    }

    G_SUBTEST << "Empty input string";
    {
        std::string str;
        replaceAll(str, "test", "mega");
        EXPECT_EQ(str, "");
    }

    G_SUBTEST << "Empty pattern";
    {
        std::string str("test string");
        replaceAll(str, "", "mega");
        EXPECT_EQ(str, "test string");
    }

    G_SUBTEST << "Self-replacement";
    {
        std::string str("test test");
        replaceAll(str, "test", "test");
        EXPECT_EQ(str, "test test");
    }

    G_SUBTEST << "Pattern longer than string";
    {
        std::string str("short");
        replaceAll(str, "very long pattern", "mega");
        EXPECT_EQ(str, "short");
    }

    G_SUBTEST << "Unicode characters";
    {
        std::string str("\xD0\x9C\xD0\x95\xD0\x93\xD0\x90 \xD0\x9C\xD0\x95\xD0\x93\xD0\x90");
        replaceAll(str, "\xD0\x9C\xD0\x95\xD0\x93\xD0\x90", "MEGA");
        EXPECT_EQ(str, "MEGA MEGA");
    }

    G_SUBTEST << "Special characters";
    {
        std::string str("test\ttest\ntest");
        replaceAll(str, "\t", " ");
        EXPECT_EQ(str, "test test\ntest");
        replaceAll(str, "\n", " ");
        EXPECT_EQ(str, "test test test");
    }

    G_SUBTEST << "Adjacent occurrences";
    {
        std::string str("abcabc");
        replaceAll(str, "ab", "x");
        EXPECT_EQ(str, "xcxc");
    }

    G_SUBTEST << "Pattern at beginning and end";
    {
        std::string str("test content test");
        replaceAll(str, "test", "start");
        EXPECT_EQ(str, "start content start");
    }

    G_SUBTEST << "Pattern is substring of replacement";
    {
        std::string str("a a a");
        replaceAll(str, "a", "ab");
        EXPECT_EQ(str, "ab ab ab");
    }

    G_SUBTEST << "Very long string";
    {
        std::string str(10000, 'a');
        str += "test" + std::string(10000, 'a');
        replaceAll(str, "test", "mega");
        EXPECT_EQ(str, std::string(10000, 'a') + "mega" + std::string(10000, 'a'));
    }

    G_SUBTEST << "Replace entire string";
    {
        std::string str("full");
        replaceAll(str, "full", "empty");
        EXPECT_EQ(str, "empty");
    }

    G_SUBTEST << "Single character string";
    {
        std::string str("a");
        replaceAll(str, "a", "b");
        EXPECT_EQ(str, "b");
    }
}

TEST(StringUtilsTest, joinStrings)
{
    using megacmd::joinStrings;

    G_SUBTEST << "Basic join";
    {
        std::vector<std::string> vec = {"a", "b", "c"};
        std::string result = joinStrings(vec);
        EXPECT_EQ(result, R"("a" "b" "c")");
    }

    G_SUBTEST << "Empty vector";
    {
        std::vector<std::string> vec;
        std::string result = joinStrings(vec);
        EXPECT_EQ(result, "");
    }

    G_SUBTEST << "Single element";
    {
        {
            std::vector<std::string> vec = {"a"};
            std::string result = joinStrings(vec);
            EXPECT_EQ(result, R"("a")");
        }
        {
            std::vector<std::string> vec = {"a"};
            std::string result = joinStrings(vec, " ", false);
            EXPECT_EQ(result, "a");
        }
        {
            std::vector<std::string> vec = {"a"};
            std::string result = joinStrings(vec, ",", true);
            EXPECT_EQ(result, R"("a")");
        }
        {
            std::vector<std::string> vec = {"a"};
            std::string result = joinStrings(vec, "/", false);
            EXPECT_EQ(result, "a");
        }
    }

    G_SUBTEST << "With quotes";
    {
        {
            std::vector<std::string> vec = {"a", "b", "c"};
            std::string result = joinStrings(vec, "/", true);
            EXPECT_EQ(result, R"("a"/"b"/"c")");
        }
        {
            std::vector<std::string> vec = {"a", "", "c"};
            std::string result = joinStrings(vec, ",", true);
            EXPECT_EQ(result, R"("a","","c")");
        }
        {
            std::vector<std::string> vec = {"a", "b", ""};
            std::string result = joinStrings(vec, ",", true);
            EXPECT_EQ(result, R"("a","b","")");
        }
    }

    G_SUBTEST << "Without quotes";
    {
        {
            std::vector<std::string> vec = {"a", "b", "c"};
            std::string result = joinStrings(vec, " ", false);
            EXPECT_EQ(result, "a b c");
        }
        {
            std::vector<std::string> vec = {"a", "", "c"};
            std::string result = joinStrings(vec, " ", false);
            EXPECT_EQ(result, "a  c");
        }
        {
            std::vector<std::string> vec = {"", "a", "b"};
            std::string result = joinStrings(vec, " ", false);
            EXPECT_EQ(result, " a b");
        }
        {
            std::vector<std::string> vec = {"a", "b", ""};
            std::string result = joinStrings(vec, " ", false);
            EXPECT_EQ(result, "a b ");
        }
    }

    G_SUBTEST << "Strings containing quotes";
    {
        {
            std::vector<std::string> vec = {R"(a"b)", R"(c"d)", "e"};
            std::string result = joinStrings(vec, " ", true);
            EXPECT_EQ(result, R"("a"b" "c"d" "e")");
        }
        {
            std::vector<std::string> vec = {R"("quoted")", "normal", R"("another")"};
            std::string result = joinStrings(vec, ",", true);
            EXPECT_EQ(result, R"(""quoted"","normal",""another"")");
        }
    }

    G_SUBTEST << "Strings containing delimiter";
    {
        {
            std::vector<std::string> vec = {"a,b", "c,d", "e"};
            std::string result = joinStrings(vec, ",", false);
            EXPECT_EQ(result, "a,b,c,d,e");
        }
        {
            std::vector<std::string> vec = {"a/b", "c/d", "e"};
            std::string result = joinStrings(vec, "/", true);
            EXPECT_EQ(result, R"("a/b"/"c/d"/"e")");
        }
        {
            std::vector<std::string> vec = {"a::b", "c::d", "e"};
            std::string result = joinStrings(vec, "::", false);
            EXPECT_EQ(result, "a::b::c::d::e");
        }
    }

    G_SUBTEST << "Special characters";
    {
        {
            std::vector<std::string> vec = {"a\nb", "c\td", "e"};
            std::string result = joinStrings(vec, " ", true);
            EXPECT_EQ(result, "\"a\nb\" \"c\td\" \"e\"");
        }
        {
            std::vector<std::string> vec = {"a\r\nb", "c", "d"};
            std::string result = joinStrings(vec, ",", false);
            EXPECT_EQ(result, "a\r\nb,c,d");
        }
    }

    G_SUBTEST << "Unicode characters";
    {
        {
            std::vector<std::string> vec = {u8"\u043F\u0440\u0438\u0432\u0435\u0442", u8"\u043C\u0438\u0440", u8"\u0442\u0435\u0441\u0442"};
            std::string result = joinStrings(vec, " ", true);
            EXPECT_TRUE(megacmd::isValidUtf8(result));
            std::string expected = u8"\"\u043F\u0440\u0438\u0432\u0435\u0442\" \"\u043C\u0438\u0440\" \"\u0442\u0435\u0441\u0442\"";
            EXPECT_EQ(result, expected);
        }
        {
            std::vector<std::string> vec = {u8"\u4F60\u597D", u8"\u4E16\u754C", u8"\u6D4B\u8BD5"};
            std::string result = joinStrings(vec, ",", true);
            EXPECT_TRUE(megacmd::isValidUtf8(result));
            std::string expected = u8"\"\u4F60\u597D\",\"\u4E16\u754C\",\"\u6D4B\u8BD5\"";
            EXPECT_EQ(result, expected);
        }
    }

    G_SUBTEST << "Empty delimiter";
    {
        {
            std::vector<std::string> vec = {"a", "b", "c"};
            std::string result = joinStrings(vec, "", true);
            EXPECT_EQ(result, R"("a""b""c")");
        }
        {
            std::vector<std::string> vec = {"a", "b", "c"};
            std::string result = joinStrings(vec, "", false);
            EXPECT_EQ(result, "abc");
        }
    }

    G_SUBTEST << "Long delimiter";
    {
        {
            std::vector<std::string> vec = {"a", "b", "c"};
            std::string result = joinStrings(vec, "---", true);
            EXPECT_EQ(result, R"("a"---"b"---"c")");
        }
        {
            std::vector<std::string> vec = {"a", "b", "c"};
            std::string longDelim = std::string(100, '-');
            std::string result = joinStrings(vec, longDelim.c_str(), false);
            EXPECT_EQ(result, "a" + longDelim + "b" + longDelim + "c");
        }
    }

    G_SUBTEST << "Multiple consecutive empty strings";
    {
        {
            std::vector<std::string> vec = {""};
            std::string result = joinStrings(vec, ",", true);
            EXPECT_EQ(result, R"("")");
        }
        {
            std::vector<std::string> vec = {""};
            std::string result = joinStrings(vec, " ", false);
            EXPECT_EQ(result, "");
        }
        {
            std::vector<std::string> vec = {"", "", ""};
            std::string result = joinStrings(vec, ",", true);
            EXPECT_EQ(result, R"("","","")");
        }
        {
            std::vector<std::string> vec = {"", "", ""};
            std::string result = joinStrings(vec, " ", false);
            EXPECT_EQ(result, "  ");
        }
        {
            std::vector<std::string> vec = {"", "a", "b"};
            std::string result = joinStrings(vec, ",", true);
            EXPECT_EQ(result, R"("","a","b")");
        }
        {
            std::vector<std::string> vec = {"a", "", "", "b"};
            std::string result = joinStrings(vec, ",", true);
            EXPECT_EQ(result, R"("a","","","b")");
        }
    }

    G_SUBTEST << "Strings with spaces and space delimiter";
    {
        {
            std::vector<std::string> vec = {"  a  ", "  b  ", "  c  "};
            std::string result = joinStrings(vec, " ", true);
            EXPECT_EQ(result, R"("  a  " "  b  " "  c  ")");
        }
        {
            std::vector<std::string> vec = {"  a  ", "  b  ", "  c  "};
            std::string result = joinStrings(vec, " ", false);
            EXPECT_EQ(result, "  a     b     c  ");
        }
    }

    G_SUBTEST << "Long strings";
    {
        std::string longStr(10000, 'x');
        std::vector<std::string> vec = {longStr, "b", longStr};
        std::string result = joinStrings(vec, " ", true);
        EXPECT_EQ(result.length(), (longStr.length() + 2) * 2 + /* "b" */ 3 + /* 2 separators */ 2);
        EXPECT_EQ(result.substr(0, 1), R"(")");
        EXPECT_EQ(result.substr(1, longStr.length()), longStr);
        EXPECT_EQ(result.substr(longStr.length() + 1, 2), R"(" )");
        EXPECT_EQ(result.substr(longStr.length() + 3, 3), R"("b")");
    }
}

TEST(StringUtilsTest, startsWith)
{
    using megacmd::startsWith;

    G_SUBTEST << "Basic case";
    {
        EXPECT_TRUE(startsWith("test string", "test"));
        EXPECT_FALSE(startsWith("test string", "string"));
    }

    G_SUBTEST << "Empty prefix";
    {
        EXPECT_TRUE(startsWith("hello", ""));
        EXPECT_TRUE(startsWith("", ""));
    }

    G_SUBTEST << "Prefix longer than string";
    {
        EXPECT_FALSE(startsWith("hi", "hello"));
    }

    G_SUBTEST << "Exact match";
    {
        EXPECT_TRUE(startsWith("hello", "hello"));
    }

    G_SUBTEST << "Single character prefix";
    {
        EXPECT_TRUE(startsWith("hello", "h"));
        EXPECT_FALSE(startsWith("hello", "e"));
    }

    G_SUBTEST << "Empty string";
    {
        EXPECT_FALSE(startsWith("", "a"));
    }

    G_SUBTEST << "Prefix in middle of string";
    {
        EXPECT_FALSE(startsWith("atest", "test"));
        EXPECT_FALSE(startsWith("prefix suffix", "fix"));
    }

    G_SUBTEST << "Case sensitivity";
    {
        EXPECT_FALSE(startsWith("Hello", "hello"));
        EXPECT_FALSE(startsWith("hello", "Hello"));
    }

    G_SUBTEST << "Prefix differs by one character";
    {
        EXPECT_FALSE(startsWith("hello", "hellp"));
    }

    G_SUBTEST << "Newline and tab in string";
    {
        EXPECT_TRUE(startsWith("hello\nworld", "hello"));
        EXPECT_TRUE(startsWith("hello\tworld", "hello"));
        EXPECT_FALSE(startsWith("hello\nworld", "world"));
        EXPECT_FALSE(startsWith("hello\r\nworld", "world"));
    }

    G_SUBTEST << "Unicode characters";
    {
        EXPECT_TRUE(startsWith(u8"\u043F\u0440\u0438\u0432\u0435\u0442", u8"\u043F\u0440\u0438"));
        EXPECT_FALSE(startsWith(u8"hello\u043F\u0440\u0438\u0432\u0435\u0442", u8"\u043F\u0440\u0438\u0432\u0435\u0442"));
        EXPECT_FALSE(startsWith(u8"test\u4F60\u597D", u8"\u4F60\u597D"));
        EXPECT_FALSE(startsWith("hello", u8"\u043F\u0440\u0438\u0432\u0435\u0442"));
    }

    G_SUBTEST << "Strings with spaces";
    {
        EXPECT_FALSE(startsWith("  hello", "hello"));
        EXPECT_FALSE(startsWith("hello  ", "  "));
    }

    G_SUBTEST << "Embedded null (string_view is byte-oriented)";
    {
        std::string withNull("a\0b", 3);
        std::string prefixA("a", 1);
        std::string prefixFull("a\0b", 3);
        EXPECT_TRUE(startsWith(withNull, prefixA));
        EXPECT_TRUE(startsWith(withNull, prefixFull));
        EXPECT_FALSE(startsWith(withNull, std::string("b", 1)));
    }

    G_SUBTEST << "Long string (prefix longer, first char differs)";
    {
        std::string longStr(10000, 'a');
        std::string prefix = "b" + std::string(9999, 'a');
        EXPECT_FALSE(startsWith(longStr, prefix));
    }
}

TEST(StringUtilsTest, toLower)
{
    using megacmd::toLower;

    G_SUBTEST << "Basic case";
    {
        EXPECT_EQ(toLower("HELLO"), "hello");
        EXPECT_EQ(toLower("Hello"), "hello");
        EXPECT_EQ(toLower("hElLo"), "hello");
    }

    G_SUBTEST << "Already lowercase";
    {
        EXPECT_EQ(toLower("hello"), "hello");
        EXPECT_EQ(toLower("hello world"), "hello world");
    }

    G_SUBTEST << "Empty string";
    {
        EXPECT_EQ(toLower(""), "");
    }

    G_SUBTEST << "Special characters";
    {
        EXPECT_EQ(toLower("HELLO@WORLD"), "hello@world");
        EXPECT_EQ(toLower("TEST#123$"), "test#123$");
        EXPECT_EQ(toLower("A-B_C.D"), "a-b_c.d");
        EXPECT_EQ(toLower("@#$%"), "@#$%");
    }

    G_SUBTEST << "Strings with spaces";
    {
        EXPECT_EQ(toLower("HELLo WORLD"), "hello world");
        EXPECT_EQ(toLower("  TEST  "), "  test  ");
        EXPECT_EQ(toLower("A B C"), "a b c");
    }

    G_SUBTEST << "Numbers only";
    {
        EXPECT_EQ(toLower("123456"), "123456");
        EXPECT_EQ(toLower("0"), "0");
    }

    G_SUBTEST << "Newlines, tabs, and control characters";
    {
        EXPECT_EQ(toLower("HELLO\nWORLD"), "hello\nworld");
        EXPECT_EQ(toLower("HELLO\tWORLD"), "hello\tworld");
        EXPECT_EQ(toLower("HELLO\r\nWORLD"), "hello\r\nworld");
        EXPECT_EQ(toLower("\n\t\r"), "\n\t\r");
    }

    G_SUBTEST << "Whitespace-only";
    {
        EXPECT_EQ(toLower(" "), " ");
        EXPECT_EQ(toLower("   "), "   ");
        EXPECT_EQ(toLower("\t\t"), "\t\t");
        EXPECT_EQ(toLower(" \t \n "), " \t \n ");
    }

    G_SUBTEST << "Long strings";
    {
        std::string longStr(10000, 'A');
        std::string result = toLower(longStr);
        EXPECT_EQ(result.length(), 10000);
        EXPECT_EQ(result, std::string(10000, 'a'));
    }

    G_SUBTEST << "Single character";
    {
        EXPECT_EQ(toLower("A"), "a");
        EXPECT_EQ(toLower("Z"), "z");
        EXPECT_EQ(toLower("a"), "a");
        EXPECT_EQ(toLower("1"), "1");
    }

    G_SUBTEST << "Original string is not modified";
    {
        const std::string original = "HELLO";
        std::string result = toLower(original);
        EXPECT_EQ(result, "hello");
        EXPECT_EQ(original, "HELLO");
    }
}
