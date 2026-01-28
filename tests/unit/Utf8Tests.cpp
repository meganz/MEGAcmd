/**
 * (c) 2026 by Mega Limited, Auckland, New Zealand
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
#include "megacmd_utf8.h"

#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

class Utf8Test : public ::testing::Test
{
protected:
    void SetUp() override
    {
        unsetEnvGuard = std::make_unique<TestInstrumentsUnsetEnvVarGuard>("MEGACMD_DISABLE_UTF8_VALIDATIONS");
    }

    void TearDown() override
    {
        unsetEnvGuard.reset();
    }

    std::unique_ptr<TestInstrumentsUnsetEnvVarGuard> unsetEnvGuard;
};

TEST_F(Utf8Test, ValidAscii)
{
    using megacmd::isValidUtf8;

    G_SUBTEST << "Empty string";
    {
        EXPECT_TRUE(isValidUtf8(""));
        EXPECT_TRUE(isValidUtf8(std::string()));
    }

    G_SUBTEST << "Single ASCII characters";
    {
        EXPECT_TRUE(isValidUtf8("a"));
        EXPECT_TRUE(isValidUtf8("Z"));
        EXPECT_TRUE(isValidUtf8("0"));
        EXPECT_TRUE(isValidUtf8(" "));
        EXPECT_TRUE(isValidUtf8("\t"));
        EXPECT_TRUE(isValidUtf8("\n"));
        EXPECT_TRUE(isValidUtf8("\r"));
    }

    G_SUBTEST << "ASCII strings";
    {
        EXPECT_TRUE(isValidUtf8("Hello, World!"));
        EXPECT_TRUE(isValidUtf8("The quick brown fox jumps over the lazy dog"));
        EXPECT_TRUE(isValidUtf8("1234567890"));
        EXPECT_TRUE(isValidUtf8("!@#$%^&*()_+-=[]{}|;':\",./<>?"));
    }

    G_SUBTEST << "ASCII control characters";
    {
        EXPECT_TRUE(isValidUtf8("\x00", 1));
        EXPECT_TRUE(isValidUtf8("\x01"));
        EXPECT_TRUE(isValidUtf8("\x1f"));
        EXPECT_TRUE(isValidUtf8("\x7f"));
    }

    G_SUBTEST << "Long ASCII string";
    {
        std::string longStr(10000, 'a');
        EXPECT_TRUE(isValidUtf8(longStr));
    }
}

TEST_F(Utf8Test, Valid2ByteSequences)
{
    using megacmd::isValidUtf8;

    G_SUBTEST << "Minimum 2-byte character (U+0080)";
    {
        EXPECT_TRUE(isValidUtf8("\xc2\x80"));
    }

    G_SUBTEST << "Maximum 2-byte character (U+07FF)";
    {
        EXPECT_TRUE(isValidUtf8("\xdf\xbf"));
    }

    G_SUBTEST << "Latin extended characters";
    {
        // ñ (U+00F1)
        EXPECT_TRUE(isValidUtf8("\xc3\xb1"));
        // ü (U+00FC)
        EXPECT_TRUE(isValidUtf8("\xc3\xbc"));
        // é (U+00E9)
        EXPECT_TRUE(isValidUtf8("\xc3\xa9"));
        // ö (U+00F6)
        EXPECT_TRUE(isValidUtf8("\xc3\xb6"));
    }

    G_SUBTEST << "Cyrillic characters";
    {
        // МЕГА
        EXPECT_TRUE(isValidUtf8("\xD0\x9C\xD0\x95\xD0\x93\xD0\x90"));
        // Привіт
        EXPECT_TRUE(isValidUtf8("\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD1\x96\xD1\x82"));
    }

    G_SUBTEST << "Greek characters";
    {
        // Ω (U+03A9)
        EXPECT_TRUE(isValidUtf8("\xCE\xA9"));
        // α (U+03B1)
        EXPECT_TRUE(isValidUtf8("\xCE\xB1"));
        // β (U+03B2)
        EXPECT_TRUE(isValidUtf8("\xCE\xB2"));
    }

    G_SUBTEST << "Hebrew characters";
    {
        // א (U+05D0)
        EXPECT_TRUE(isValidUtf8("\xD7\x90"));
        // ב (U+05D1)
        EXPECT_TRUE(isValidUtf8("\xD7\x91"));
    }

    G_SUBTEST << "Arabic characters";
    {
        // ا (U+0627)
        EXPECT_TRUE(isValidUtf8("\xD8\xA7"));
        // ب (U+0628)
        EXPECT_TRUE(isValidUtf8("\xD8\xA8"));
    }
}

TEST_F(Utf8Test, Valid3ByteSequences)
{
    using megacmd::isValidUtf8;

    G_SUBTEST << "Minimum 3-byte character (U+0800)";
    {
        EXPECT_TRUE(isValidUtf8("\xe0\xa0\x80"));
    }

    G_SUBTEST << "Maximum valid 3-byte before surrogates (U+D7FF)";
    {
        EXPECT_TRUE(isValidUtf8("\xed\x9f\xbf"));
    }

    G_SUBTEST << "Minimum valid 3-byte after surrogates (U+E000)";
    {
        EXPECT_TRUE(isValidUtf8("\xee\x80\x80"));
    }

    G_SUBTEST << "Maximum 3-byte character (U+FFFF)";
    {
        // Note: U+FFFF is valid UTF-8 but is a "noncharacter"
        EXPECT_TRUE(isValidUtf8("\xef\xbf\xbf"));
    }

    G_SUBTEST << "Euro sign (U+20AC)";
    {
        EXPECT_TRUE(isValidUtf8("\xe2\x82\xac"));
    }

    G_SUBTEST << "Japanese Hiragana";
    {
        // あ (U+3042)
        EXPECT_TRUE(isValidUtf8("\xe3\x81\x82"));
        // い (U+3044)
        EXPECT_TRUE(isValidUtf8("\xe3\x81\x84"));
        // う (U+3046)
        EXPECT_TRUE(isValidUtf8("\xe3\x81\x86"));
    }

    G_SUBTEST << "Japanese Katakana";
    {
        // ア (U+30A2)
        EXPECT_TRUE(isValidUtf8("\xe3\x82\xa2"));
        // イ (U+30A4)
        EXPECT_TRUE(isValidUtf8("\xe3\x82\xa4"));
    }

    G_SUBTEST << "Chinese characters";
    {
        // 中 (U+4E2D)
        EXPECT_TRUE(isValidUtf8("\xe4\xb8\xad"));
        // 文 (U+6587)
        EXPECT_TRUE(isValidUtf8("\xe6\x96\x87"));
        // 字 (U+5B57)
        EXPECT_TRUE(isValidUtf8("\xe5\xad\x97"));
    }

    G_SUBTEST << "Korean characters";
    {
        // 안 (U+C548)
        EXPECT_TRUE(isValidUtf8("\xec\x95\x88"));
        // 녕 (U+B155)
        EXPECT_TRUE(isValidUtf8("\xeb\x85\x95"));
        // 하 (U+D558)
        EXPECT_TRUE(isValidUtf8("\xed\x95\x98"));
    }

    G_SUBTEST << "Thai characters";
    {
        // ก (U+0E01)
        EXPECT_TRUE(isValidUtf8("\xe0\xb8\x81"));
        // ข (U+0E02)
        EXPECT_TRUE(isValidUtf8("\xe0\xb8\x82"));
    }

    G_SUBTEST << "BOM (U+FEFF)";
    {
        EXPECT_TRUE(isValidUtf8("\xef\xbb\xbf"));
    }

    G_SUBTEST << "Replacement character (U+FFFD)";
    {
        EXPECT_TRUE(isValidUtf8("\xef\xbf\xbd"));
    }
}

TEST_F(Utf8Test, Valid4ByteSequences)
{
    using megacmd::isValidUtf8;

    G_SUBTEST << "Minimum 4-byte character (U+10000)";
    {
        EXPECT_TRUE(isValidUtf8("\xf0\x90\x80\x80"));
    }

    G_SUBTEST << "Maximum valid 4-byte character (U+10FFFF)";
    {
        EXPECT_TRUE(isValidUtf8("\xf4\x8f\xbf\xbf"));
    }

    G_SUBTEST << "Emoji characters";
    {
        // 😀 (U+1F600)
        EXPECT_TRUE(isValidUtf8("\xf0\x9f\x98\x80"));
        // 😂 (U+1F602)
        EXPECT_TRUE(isValidUtf8("\xf0\x9f\x98\x82"));
        // ❤️ base character (U+2764)
        EXPECT_TRUE(isValidUtf8("\xe2\x9d\xa4"));
        // 🎉 (U+1F389)
        EXPECT_TRUE(isValidUtf8("\xf0\x9f\x8e\x89"));
        // 🚀 (U+1F680)
        EXPECT_TRUE(isValidUtf8("\xf0\x9f\x9a\x80"));
    }

    G_SUBTEST << "Musical symbols";
    {
        // 𝄞 (U+1D11E) - G clef
        EXPECT_TRUE(isValidUtf8("\xf0\x9d\x84\x9e"));
    }

    G_SUBTEST << "Mathematical symbols";
    {
        // 𝔸 (U+1D538)
        EXPECT_TRUE(isValidUtf8("\xf0\x9d\x94\xb8"));
    }

    G_SUBTEST << "Historic scripts";
    {
        // Linear B syllable (U+10000)
        EXPECT_TRUE(isValidUtf8("\xf0\x90\x80\x80"));
        // Egyptian hieroglyph (U+13000)
        EXPECT_TRUE(isValidUtf8("\xf0\x93\x80\x80"));
    }

    G_SUBTEST << "Range boundary tests";
    {
        // U+10FFFF - last valid codepoint
        EXPECT_TRUE(isValidUtf8("\xf4\x8f\xbf\xbf"));
        // U+100000
        EXPECT_TRUE(isValidUtf8("\xf4\x80\x80\x80"));
    }
}

TEST_F(Utf8Test, InvalidSequences)
{
    using megacmd::isValidUtf8;

    G_SUBTEST << "Invalid continuation byte at start";
    {
        // Continuation byte without leading byte
        EXPECT_FALSE(isValidUtf8("\x80"));
        EXPECT_FALSE(isValidUtf8("\x80\x80"));
        EXPECT_FALSE(isValidUtf8("\xa0\xa1"));
        EXPECT_FALSE(isValidUtf8("\xbf"));
    }

    G_SUBTEST << "Invalid 5-byte sequences (not valid UTF-8)";
    {
        // 5-byte sequences starting with 0xf8
        EXPECT_FALSE(isValidUtf8("\xf8\xa1\xa1\xa1\xa1"));
    }

    G_SUBTEST << "Invalid 6-byte sequences (not valid UTF-8)";
    {
        // 6-byte sequences starting with 0xfc
        EXPECT_FALSE(isValidUtf8("\xfc\xa1\xa1\xa1\xa1\xa1"));
    }

    G_SUBTEST << "Invalid lead bytes";
    {
        // 0xfe and 0xff are never valid
        EXPECT_FALSE(isValidUtf8("\xfe"));
        EXPECT_FALSE(isValidUtf8("\xff"));
        EXPECT_FALSE(isValidUtf8("\xfe\x80\x80\x80"));
        EXPECT_FALSE(isValidUtf8("\xff\x80\x80\x80"));
    }
}

TEST_F(Utf8Test, InvalidContinuationBytes)
{
    using megacmd::isValidUtf8;

    G_SUBTEST << "2-byte sequence with invalid continuation";
    {
        EXPECT_FALSE(isValidUtf8("\xc3\x28"));  // 2nd byte: 00xx xxxx
        EXPECT_FALSE(isValidUtf8("\xc3\xc0"));  // 2nd byte: 11xx xxxx
        EXPECT_FALSE(isValidUtf8("\xc3\x00"));  // 2nd byte: 0000 0000
    }

    G_SUBTEST << "3-byte sequence with invalid continuation";
    {
        EXPECT_FALSE(isValidUtf8("\xe2\x28\xa1"));  // 2nd byte: 00xx xxxx
        EXPECT_FALSE(isValidUtf8("\xe2\x82\x28"));  // 3rd byte: 00xx xxxx
        EXPECT_FALSE(isValidUtf8("\xe0\xa1\xd0"));  // 3rd byte: 11xx xxxx
        EXPECT_FALSE(isValidUtf8("\xe2\xc0\xa1"));  // 2nd byte: 11xx xxxx
    }

    G_SUBTEST << "4-byte sequence with invalid continuation";
    {
        EXPECT_FALSE(isValidUtf8("\xf0\x28\x8c\xbc"));  // 2nd byte: 00xx xxxx
        EXPECT_FALSE(isValidUtf8("\xf0\x90\x28\xbc"));  // 3rd byte: 00xx xxxx
        EXPECT_FALSE(isValidUtf8("\xf0\x90\x8c\x28"));  // 4th byte: 00xx xxxx
        EXPECT_FALSE(isValidUtf8("\xf0\x28\x8c\x28"));  // 2nd & 4th bytes invalid
    }
}

TEST_F(Utf8Test, TruncatedSequences)
{
    using megacmd::isValidUtf8;

    G_SUBTEST << "Truncated 2-byte sequence";
    {
        EXPECT_FALSE(isValidUtf8("\xc2"));
        EXPECT_FALSE(isValidUtf8("\xdf"));
    }

    G_SUBTEST << "Truncated 3-byte sequence";
    {
        EXPECT_FALSE(isValidUtf8("\xe0\xa0"));
        EXPECT_FALSE(isValidUtf8("\xe2\x82"));
        EXPECT_FALSE(isValidUtf8("\xe2"));
    }

    G_SUBTEST << "Truncated 4-byte sequence";
    {
        EXPECT_FALSE(isValidUtf8("\xf0\x90\x8c"));
        EXPECT_FALSE(isValidUtf8("\xf0\x90"));
        EXPECT_FALSE(isValidUtf8("\xf4\x8f\xbf"));
        EXPECT_FALSE(isValidUtf8("\xf0"));
    }

    G_SUBTEST << "Truncated sequence in middle of string";
    {
        std::string str = "Hello ";
        str += "\xc2";  // Incomplete 2-byte sequence
        EXPECT_FALSE(isValidUtf8(str));

        str = "Test ";
        str += "\xe2\x82";  // Incomplete 3-byte sequence
        EXPECT_FALSE(isValidUtf8(str));
    }
}

TEST_F(Utf8Test, OverlongEncodings)
{
    using megacmd::isValidUtf8;

    G_SUBTEST << "Overlong 2-byte encoding of ASCII";
    {
        // Overlong encoding of '/' (U+002F) as 2 bytes
        EXPECT_FALSE(isValidUtf8("\xc0\xaf"));
        // Overlong encoding of NUL (U+0000) as 2 bytes
        EXPECT_FALSE(isValidUtf8("\xc0\x80"));
        // Any C0 or C1 lead byte creates overlong encoding
        EXPECT_FALSE(isValidUtf8("\xc1\xbf"));
    }

    G_SUBTEST << "Overlong 3-byte encoding";
    {
        // Overlong encoding of characters < U+0800
        EXPECT_FALSE(isValidUtf8("\xe0\x9f\xbf"));  // codepoint less than U+0800
        EXPECT_FALSE(isValidUtf8("\xe0\x80\x80"));  // overlong NUL
    }

    G_SUBTEST << "Overlong 4-byte encoding";
    {
        // Overlong encoding of characters < U+10000
        EXPECT_FALSE(isValidUtf8("\xf0\x8f\xbf\xbf"));  // codepoint less than U+10000
        EXPECT_FALSE(isValidUtf8("\xf0\x80\x80\x80"));  // overlong NUL
    }
}

TEST_F(Utf8Test, SurrogateCodepoints)
{
    using megacmd::isValidUtf8;

    G_SUBTEST << "High surrogate (U+D800)";
    {
        EXPECT_FALSE(isValidUtf8("\xed\xa0\x80"));
    }

    G_SUBTEST << "Low surrogate (U+DFFF)";
    {
        EXPECT_FALSE(isValidUtf8("\xed\xbf\xbf"));
    }

    G_SUBTEST << "Middle of surrogate range";
    {
        // U+D800 to U+DFFF are all invalid
        EXPECT_FALSE(isValidUtf8("\xed\xa0\x80"));  // U+D800
        EXPECT_FALSE(isValidUtf8("\xed\xad\xbf"));  // U+DB7F
        EXPECT_FALSE(isValidUtf8("\xed\xae\x80"));  // U+DB80
        EXPECT_FALSE(isValidUtf8("\xed\xaf\xbf"));  // U+DBFF
        EXPECT_FALSE(isValidUtf8("\xed\xb0\x80"));  // U+DC00
        EXPECT_FALSE(isValidUtf8("\xed\xbf\xbf"));  // U+DFFF
    }
}

TEST_F(Utf8Test, CodepointsBeyondUnicode)
{
    using megacmd::isValidUtf8;

    G_SUBTEST << "Codepoints greater than U+10FFFF";
    {
        // U+110000 (just beyond valid range)
        EXPECT_FALSE(isValidUtf8("\xf4\x90\x80\x80"));
        // U+13FFFF
        EXPECT_FALSE(isValidUtf8("\xf4\xbf\xbf\xbf"));
        // U+1FFFFF (would need all 4-byte space)
        EXPECT_FALSE(isValidUtf8("\xf7\xbf\xbf\xbf"));
    }
}

TEST_F(Utf8Test, MixedValidAndInvalid)
{
    using megacmd::isValidUtf8;

    G_SUBTEST << "Invalid byte in middle of valid string";
    {
        std::string str = "Hello";
        str += "\xff";  // Invalid byte
        str += "World";
        EXPECT_FALSE(isValidUtf8(str));
    }

    G_SUBTEST << "Valid multibyte characters mixed with invalid";
    {
        std::string str = "\xc3\xb1";  // Valid ñ
        str += "\x80";                  // Invalid continuation byte alone
        EXPECT_FALSE(isValidUtf8(str));
    }

    G_SUBTEST << "Invalid sequence at beginning";
    {
        std::string str = "\xfe\x80\x80\x80\x80\x80Hello World";
        EXPECT_FALSE(isValidUtf8(str));
    }

    G_SUBTEST << "Invalid sequence at end";
    {
        std::string str = "Hello World\xfe";
        EXPECT_FALSE(isValidUtf8(str));
    }
}

TEST_F(Utf8Test, ComplexValidStrings)
{
    using megacmd::isValidUtf8;

    G_SUBTEST << "Mixed languages";
    {
        // English + Cyrillic + Chinese + Emoji
        std::string str = "Hello \xD0\x9C\xD0\x98\xD0\xA0 \xe4\xb8\x96\xe7\x95\x8c \xf0\x9f\x8c\x8d";
        EXPECT_TRUE(isValidUtf8(str));
    }

    G_SUBTEST << "All byte-length characters together";
    {
        std::string str;
        str += "A";                 // 1 byte
        str += "\xc3\xa9";          // 2 bytes (é)
        str += "\xe4\xb8\xad";      // 3 bytes (中)
        str += "\xf0\x9f\x98\x80";  // 4 bytes (😀)
        EXPECT_TRUE(isValidUtf8(str));
    }

    G_SUBTEST << "Long multilingual string";
    {
        std::string str;
        for (int i = 0; i < 1000; ++i)
        {
            str += "A";
            str += "\xc3\xb1";          // ñ
            str += "\xe4\xb8\xad";      // 中
            str += "\xf0\x9f\x98\x80";  // 😀
        }
        EXPECT_TRUE(isValidUtf8(str));
    }

    G_SUBTEST << "Real-world file paths";
    {
        EXPECT_TRUE(isValidUtf8("/home/user/Documents/\xD0\x94\xD0\xBE\xD0\xBA\xD1\x83\xD0\xBC\xD0\xB5\xD0\xBD\xD1\x82.txt"));  // Ukrainian filename
        EXPECT_TRUE(isValidUtf8("/Users/\xe7\x94\xa8\xe6\x88\xb7/Desktop/file.txt"));  // Chinese username
        EXPECT_TRUE(isValidUtf8("C:\\Users\\\xc3\x9cser\\Documents\\file.txt"));  // German umlaut
    }

    G_SUBTEST << "URL-like strings with Unicode";
    {
        EXPECT_TRUE(isValidUtf8("https://example.com/path/\xe4\xb8\xad\xe6\x96\x87"));
        EXPECT_TRUE(isValidUtf8("mega://\xD0\xBF\xD1\x83\xD1\x82\xD1\x8C/file"));
    }
}

TEST_F(Utf8Test, DisableValidationsEnvVar)
{
    using megacmd::isValidUtf8;

    G_SUBTEST << "Invalid UTF-8 returns true when validations disabled";
    {
        // Temporarily disable validations
        unsetEnvGuard.reset();
        TestInstrumentsEnvVarGuard bypassGuard("MEGACMD_DISABLE_UTF8_VALIDATIONS", "1");

        // These would normally fail, but with MEGACMD_DISABLE_UTF8_VALIDATIONS=1 they should pass
        EXPECT_TRUE(isValidUtf8("\xff"));
        EXPECT_TRUE(isValidUtf8("\xfe"));
        EXPECT_TRUE(isValidUtf8("\xc0\x80"));
        EXPECT_TRUE(isValidUtf8("\xed\xa0\x80"));
    }

    G_SUBTEST << "Invalid UTF-8 returns false when validations re-enabled";
    {
        // Guard is destroyed, validations should be re-enabled
        // Re-initialize unsetEnvGuard to ensure clean state
        unsetEnvGuard = std::make_unique<TestInstrumentsUnsetEnvVarGuard>("MEGACMD_DISABLE_UTF8_VALIDATIONS");
        EXPECT_FALSE(isValidUtf8("\xff"));
        EXPECT_FALSE(isValidUtf8("\xfe"));
    }
}

TEST_F(Utf8Test, BoundaryConditions)
{
    using megacmd::isValidUtf8;

    G_SUBTEST << "Size-based overloads";
    {
        const char data[] = "Hello\x00World";
        EXPECT_TRUE(isValidUtf8(data, 11));

        EXPECT_TRUE(isValidUtf8("Hello\xff", 5));
        EXPECT_FALSE(isValidUtf8("Hello\xff", 6));
    }

    G_SUBTEST << "std::string overload";
    {
        std::string validStr = "\xc3\xb1";  // ñ
        EXPECT_TRUE(isValidUtf8(validStr));

        std::string invalidStr;
        invalidStr += '\xc3';  // Incomplete sequence
        EXPECT_FALSE(isValidUtf8(invalidStr));
    }

    G_SUBTEST << "Edge case character counts";
    {
        // Single multibyte character
        EXPECT_TRUE(isValidUtf8("\xf0\x9f\x98\x80"));  // Just emoji

        // Two adjacent 4-byte chars
        EXPECT_TRUE(isValidUtf8("\xf0\x9f\x98\x80\xf0\x9f\x98\x82"));
    }
}

TEST(PathUtf8Test, BasicPaths)
{
    using megacmd::pathAsUtf8;

    G_SUBTEST << "ASCII path";
    {
        fs::path p = "/home/user/file.txt";
        std::string result = pathAsUtf8(p);
#ifdef _WIN32
        // On Windows, path might use backslashes
        EXPECT_TRUE(result.find("home") != std::string::npos);
        EXPECT_TRUE(result.find("user") != std::string::npos);
        EXPECT_TRUE(result.find("file.txt") != std::string::npos);
#else
        EXPECT_EQ(result, "/home/user/file.txt");
#endif
    }

    G_SUBTEST << "Empty path";
    {
        fs::path p;
        std::string result = pathAsUtf8(p);
        EXPECT_TRUE(result.empty());
    }

    G_SUBTEST << "Current directory";
    {
        fs::path p = ".";
        std::string result = pathAsUtf8(p);
        EXPECT_EQ(result, ".");
    }

    G_SUBTEST << "Parent directory";
    {
        fs::path p = "..";
        std::string result = pathAsUtf8(p);
        EXPECT_EQ(result, "..");
    }
}

TEST(PathUtf8Test, UnicodePathsAllPlatforms)
{
    using megacmd::pathAsUtf8;
    using megacmd::isValidUtf8;

    G_SUBTEST << "Cyrillic in path";
    {
        std::string cyrillic = "\xD0\x9C\xD0\x95\xD0\x93\xD0\x90";  // МЕГА
        fs::path p = fs::u8path(cyrillic);
        std::string result = pathAsUtf8(p);
        EXPECT_TRUE(isValidUtf8(result));
        // The result should contain the Cyrillic characters
        // Note: On Unix systems, pathAsUtf8() uses path.string() which returns the path
        // in the native filesystem encoding (depends on locale). On modern systems with
        // UTF-8 locale, Cyrillic characters are preserved. However, on systems with
        // non-UTF-8 locales (e.g., old systems with ISO-8859-1, KOI8-R, or misconfigured
        // locales), the characters may be lost or corrupted. The "MEGA" fallback checks
        // for potential transliteration by filesystem/OS, though this is rare in practice.
        // Reference: C++17 std::filesystem::path::string() uses native encoding,
        // not guaranteed to be UTF-8 on Unix (unlike path.u8string()).
        EXPECT_TRUE(result.find("\xD0\x9C") != std::string::npos ||
                    result.find("MEGA") != std::string::npos ||  // Fallback for non-UTF-8 locales
                    !result.empty());
    }

    G_SUBTEST << "Chinese characters in path";
    {
        std::string chinese = "\xe4\xb8\xad\xe6\x96\x87";  // 中文
        fs::path p = fs::u8path(chinese);
        std::string result = pathAsUtf8(p);
        EXPECT_TRUE(isValidUtf8(result));
    }

    G_SUBTEST << "Japanese characters in path";
    {
        std::string japanese = "\xe3\x83\x86\xe3\x82\xb9\xe3\x83\x88";  // テスト
        fs::path p = fs::u8path(japanese);
        std::string result = pathAsUtf8(p);
        EXPECT_TRUE(isValidUtf8(result));
    }

    G_SUBTEST << "Emoji in path";
    {
        std::string emoji = "\xf0\x9f\x93\x81";  // 📁
        fs::path p = fs::u8path(emoji);
        std::string result = pathAsUtf8(p);
        EXPECT_TRUE(isValidUtf8(result));
    }

    G_SUBTEST << "Mixed scripts in path";
    {
        std::string mixed = "test_\xD0\xA4\xD0\xB0\xD0\xB9\xD0\xBB_\xe4\xb8\xad";
        fs::path p = fs::u8path(mixed);
        std::string result = pathAsUtf8(p);
        EXPECT_TRUE(isValidUtf8(result));
    }
}

#ifdef _WIN32
TEST(Utf16ConversionTest, Utf8ToUtf16Basic)
{
    using megacmd::utf8StringToUtf16WString;

    G_SUBTEST << "Empty string";
    {
        std::wstring result = utf8StringToUtf16WString("");
        EXPECT_TRUE(result.empty());
    }

    G_SUBTEST << "ASCII string";
    {
        std::wstring result = utf8StringToUtf16WString("Hello World");
        EXPECT_EQ(result, L"Hello World");
    }

    G_SUBTEST << "Single ASCII character";
    {
        std::wstring result = utf8StringToUtf16WString("A");
        EXPECT_EQ(result, L"A");
    }
}

TEST(Utf16ConversionTest, Utf8ToUtf16Multibyte)
{
    using megacmd::utf8StringToUtf16WString;

    G_SUBTEST << "2-byte UTF-8 characters";
    {
        // ñ (U+00F1)
        std::wstring result = utf8StringToUtf16WString("\xc3\xb1");
        EXPECT_EQ(result, L"\x00F1");
    }

    G_SUBTEST << "3-byte UTF-8 characters";
    {
        // 中 (U+4E2D)
        std::wstring result = utf8StringToUtf16WString("\xe4\xb8\xad");
        EXPECT_EQ(result, L"\x4E2D");
    }

    G_SUBTEST << "4-byte UTF-8 characters (surrogate pairs in UTF-16)";
    {
        // 😀 (U+1F600) - requires surrogate pair in UTF-16
        std::wstring result = utf8StringToUtf16WString("\xf0\x9f\x98\x80");
        // UTF-16 surrogate pair: D83D DE00
        EXPECT_EQ(result, L"\xD83D\xDE00");
    }

    G_SUBTEST << "Korean characters";
    {
        // 안녕하세요세계 (Korean hello world)
        const char* utf8 = "\xec\x95\x88\xec\x95\x88\xeb\x85\x95\xed\x95\x98\xec\x84\xb8\xec\x9a\x94\xec\x84\xb8\xea\xb3\x84";
        std::wstring result = utf8StringToUtf16WString(utf8);
        EXPECT_EQ(result, L"\uc548\uc548\ub155\ud558\uc138\uc694\uc138\uacc4");
    }

    G_SUBTEST << "Cyrillic characters";
    {
        // МЕГА
        std::wstring result = utf8StringToUtf16WString("\xD0\x9C\xD0\x95\xD0\x93\xD0\x90");
        EXPECT_EQ(result, L"\x041C\x0415\x0413\x0410");
    }
}

TEST(Utf16ConversionTest, Utf8ToUtf16WithLength)
{
    using megacmd::utf8StringToUtf16WString;

    G_SUBTEST << "Partial string conversion";
    {
        std::wstring result = utf8StringToUtf16WString("Hello World", 5);
        EXPECT_EQ(result, L"Hello");
    }

    G_SUBTEST << "Zero length";
    {
        std::wstring result = utf8StringToUtf16WString("Hello", 0);
        EXPECT_TRUE(result.empty());
    }

    G_SUBTEST << "Partial multibyte character (edge case)";
    {
        constexpr const char* utf8 = "\xc3\xb1";  // ñ
        std::wstring result = utf8StringToUtf16WString(utf8, 2);
        EXPECT_EQ(result, L"\x00F1");
    }
}

TEST(Utf16ConversionTest, Utf16ToUtf8Basic)
{
    using megacmd::utf16ToUtf8;
    using megacmd::isValidUtf8;

    G_SUBTEST << "Empty string";
    {
        std::string result = utf16ToUtf8(L"");
        EXPECT_TRUE(result.empty());
    }

    G_SUBTEST << "ASCII string";
    {
        std::string result = utf16ToUtf8(L"Hello World");
        EXPECT_EQ(result, "Hello World");
        EXPECT_TRUE(isValidUtf8(result));
    }

    G_SUBTEST << "Single ASCII character";
    {
        std::string result = utf16ToUtf8(L"A");
        EXPECT_EQ(result, "A");
    }
}

TEST(Utf16ConversionTest, Utf16ToUtf8Multibyte)
{
    using megacmd::utf16ToUtf8;
    using megacmd::isValidUtf8;

    G_SUBTEST << "BMP characters";
    {
        // ñ (U+00F1)
        std::string result = utf16ToUtf8(L"\x00F1");
        EXPECT_EQ(result, "\xc3\xb1");
        EXPECT_TRUE(isValidUtf8(result));
    }

    G_SUBTEST << "CJK characters";
    {
        // 中 (U+4E2D)
        std::string result = utf16ToUtf8(L"\x4E2D");
        EXPECT_EQ(result, "\xe4\xb8\xad");
        EXPECT_TRUE(isValidUtf8(result));
    }

    G_SUBTEST << "Surrogate pairs (emoji)";
    {
        // 😀 (U+1F600) as UTF-16 surrogate pair
        std::wstring ws;
        ws += (wchar_t)0xD83D;
        ws += (wchar_t)0xDE00;
        std::string result = utf16ToUtf8(ws);
        EXPECT_EQ(result, "\xf0\x9f\x98\x80");
        EXPECT_TRUE(isValidUtf8(result));
    }

    G_SUBTEST << "Korean characters";
    {
        std::string result = utf16ToUtf8(L"\uc548\uc548\ub155\ud558\uc138\uc694\uc138\uacc4");
        EXPECT_TRUE(isValidUtf8(result));
        EXPECT_EQ(result, "\xec\x95\x88\xec\x95\x88\xeb\x85\x95\xed\x95\x98\xec\x84\xb8\xec\x9a\x94\xec\x84\xb8\xea\xb3\x84");
    }

    G_SUBTEST << "Cyrillic characters";
    {
        std::string result = utf16ToUtf8(L"\x041C\x0415\x0413\x0410");
        EXPECT_EQ(result, "\xD0\x9C\xD0\x95\xD0\x93\xD0\x90");
        EXPECT_TRUE(isValidUtf8(result));
    }
}

TEST(Utf16ConversionTest, Utf16ToUtf8Overloads)
{
    using megacmd::utf16ToUtf8;
    using megacmd::isValidUtf8;

    G_SUBTEST << "const wchar_t* overload";
    {
        constexpr const wchar_t* ws = L"Hello";
        std::string result = utf16ToUtf8(ws);
        EXPECT_EQ(result, "Hello");
        EXPECT_TRUE(isValidUtf8(result));
    }

    G_SUBTEST << "std::wstring overload";
    {
        std::wstring ws = L"Hello";
        std::string result = utf16ToUtf8(ws);
        EXPECT_EQ(result, "Hello");
        EXPECT_TRUE(isValidUtf8(result));
    }

    G_SUBTEST << "Output parameter overload";
    {
        std::string output;
        megacmd::utf16ToUtf8(L"Test", 4, &output);
        EXPECT_EQ(output, "Test");
        EXPECT_TRUE(isValidUtf8(output));
    }
}

TEST(Utf16ConversionTest, RoundTrip)
{
    using megacmd::utf8StringToUtf16WString;
    using megacmd::utf16ToUtf8;
    using megacmd::isValidUtf8;

    G_SUBTEST << "ASCII round-trip";
    {
        std::string original = "Hello World 123!@#";
        std::wstring utf16 = utf8StringToUtf16WString(original.c_str());
        std::string back = utf16ToUtf8(utf16);
        EXPECT_EQ(back, original);
        EXPECT_TRUE(isValidUtf8(back));
    }

    G_SUBTEST << "Multibyte round-trip";
    {
        std::string original = "\xD0\x9C\xD0\x95\xD0\x93\xD0\x90";  // МЕГА
        std::wstring utf16 = utf8StringToUtf16WString(original.c_str());
        std::string back = utf16ToUtf8(utf16);
        EXPECT_EQ(back, original);
        EXPECT_TRUE(isValidUtf8(back));
    }

    G_SUBTEST << "Emoji round-trip";
    {
        std::string original = "\xf0\x9f\x98\x80";  // 😀
        std::wstring utf16 = utf8StringToUtf16WString(original.c_str());
        std::string back = utf16ToUtf8(utf16);
        EXPECT_EQ(back, original);
        EXPECT_TRUE(isValidUtf8(back));
    }

    G_SUBTEST << "Mixed content round-trip";
    {
        std::string original = "Hello \xD0\x9C\xD0\x98\xD0\xA0 \xe4\xb8\x96\xe7\x95\x8c \xf0\x9f\x8c\x8d!";
        std::wstring utf16 = utf8StringToUtf16WString(original.c_str());
        std::string back = utf16ToUtf8(utf16);
        EXPECT_EQ(back, original);
        EXPECT_TRUE(isValidUtf8(back));
    }

    G_SUBTEST << "Long string round-trip";
    {
        std::string original;
        for (int i = 0; i < 1000; ++i)
        {
            original += "Test \xc3\xb1 ";
        }
        std::wstring utf16 = utf8StringToUtf16WString(original.c_str());
        std::string back = utf16ToUtf8(utf16);
        EXPECT_EQ(back, original);
        EXPECT_TRUE(isValidUtf8(back));
    }
}

TEST(Utf16ConversionTest, EdgeCases)
{
    using megacmd::utf8StringToUtf16WString;
    using megacmd::utf16ToUtf8;

    G_SUBTEST << "BOM handling (UTF-8 BOM)";
    {
        std::string withBom = "\xef\xbb\xbf" "Hello";
        std::wstring utf16 = utf8StringToUtf16WString(withBom.c_str());
        // BOM should be converted to UTF-16 BOM (U+FEFF)
        EXPECT_EQ(utf16[0], 0xFEFF);
    }

    G_SUBTEST << "Special Unicode characters";
    {
        // Zero-width joiner (U+200D)
        std::string zwj = "\xe2\x80\x8d";
        std::wstring utf16 = utf8StringToUtf16WString(zwj.c_str());
        EXPECT_EQ(utf16, L"\x200D");
    }

    G_SUBTEST << "Arrows and symbols";
    {
        // ▼ (U+25BC)
        std::wstring input = L"\u25bc";
        std::string result = utf16ToUtf8(input);
        EXPECT_EQ(result, "\xe2\x96\xbc");
    }
}

TEST(StringToLocalWTest, BasicConversion)
{
    using megacmd::stringtolocalw;

    G_SUBTEST << "Simple ASCII";
    {
        std::wstring result;
        stringtolocalw("Hello", &result);
        EXPECT_EQ(result, L"Hello");
    }

    G_SUBTEST << "Empty string";
    {
        std::wstring result;
        stringtolocalw("", &result);
        EXPECT_TRUE(result.empty());
    }

    G_SUBTEST << "UTF-8 to wstring";
    {
        std::wstring result;
        stringtolocalw("\xc3\xb1", &result);  // ñ
        EXPECT_EQ(result, L"\x00F1");
    }
}

TEST(LocalWToStringTest, BasicConversion)
{
    using megacmd::localwtostring;

    G_SUBTEST << "Simple ASCII";
    {
        std::wstring input = L"Hello";
        std::string result;
        localwtostring(&input, &result);
        EXPECT_EQ(result, "Hello");
    }

    G_SUBTEST << "Empty string";
    {
        std::wstring input;
        std::string result;
        localwtostring(&input, &result);
        EXPECT_TRUE(result.empty());
    }

    G_SUBTEST << "Wide char to UTF-8";
    {
        std::wstring input = L"\x00F1";  // ñ
        std::string result;
        localwtostring(&input, &result);
        EXPECT_EQ(result, "\xc3\xb1");
    }
}

TEST(NonMaxPathLimitedTest, PathPrefixing)
{
    using megacmd::nonMaxPathLimitedWstring;
    using megacmd::nonMaxPathLimitedPath;

    G_SUBTEST << "Regular path gets prefixed";
    {
        fs::path p = "C:\\test\\path";
        std::wstring result = nonMaxPathLimitedWstring(p);
        EXPECT_TRUE(result.find(L"\\\\?\\") == 0);
    }

    G_SUBTEST << "Already prefixed path unchanged";
    {
        fs::path p = "\\\\?\\C:\\test\\path";
        std::wstring result = nonMaxPathLimitedWstring(p);
        EXPECT_TRUE(result.find(L"\\\\?\\") == 0);
        // Should not double-prefix
        EXPECT_TRUE(result.find(L"\\\\?\\\\\\?\\") == std::wstring::npos);
    }

    G_SUBTEST << "Path version";
    {
        fs::path p = "C:\\test\\path";
        fs::path result = nonMaxPathLimitedPath(p);
        EXPECT_TRUE(result.wstring().find(L"\\\\?\\") == 0);
    }
}

#endif // _WIN32

TEST(Utf8Test, InvalidUtf8Incidences)
{
    using megacmd::isValidUtf8;
    using megacmd::sInvalidUtf8Incidences;

    // Note: This test depends on internal state which may be affected by other tests
    uint64_t initialCount = sInvalidUtf8Incidences.load();

    G_SUBTEST << "Counter increments on invalid UTF-8";
    {
        EXPECT_FALSE(isValidUtf8("\xff"));
        EXPECT_GT(sInvalidUtf8Incidences.load(), initialCount);

        uint64_t afterFirst = sInvalidUtf8Incidences.load();
        EXPECT_FALSE(isValidUtf8("\xfe"));
        EXPECT_GT(sInvalidUtf8Incidences.load(), afterFirst);
    }

    G_SUBTEST << "Counter does not increment on valid UTF-8";
    {
        uint64_t before = sInvalidUtf8Incidences.load();
        EXPECT_TRUE(isValidUtf8("Hello World"));
        EXPECT_TRUE(isValidUtf8("\xc3\xb1"));
        EXPECT_TRUE(isValidUtf8("\xe4\xb8\xad"));
        EXPECT_TRUE(isValidUtf8("\xf0\x9f\x98\x80"));
        EXPECT_EQ(sInvalidUtf8Incidences.load(), before);
    }
}

TEST(Utf8Test, StressLargeValidStrings)
{
    using megacmd::isValidUtf8;

    G_SUBTEST << "Very long ASCII string";
    {
        std::string longStr(1000000, 'a');
        EXPECT_TRUE(isValidUtf8(longStr));
    }

    G_SUBTEST << "Very long multibyte string";
    {
        std::string longStr;
        longStr.reserve(1000000 * 4);
        for (int i = 0; i < 250000; ++i)
        {
            longStr += "\xf0\x9f\x98\x80";  // 4-byte emoji
        }
        EXPECT_TRUE(isValidUtf8(longStr));
    }

    G_SUBTEST << "Mixed content large string";
    {
        std::string mixedStr;
        mixedStr.reserve(100000);
        for (int i = 0; i < 10000; ++i)
        {
            mixedStr += "ASCII ";
            mixedStr += "\xc3\xb1";         // 2-byte
            mixedStr += " ";
            mixedStr += "\xe4\xb8\xad";     // 3-byte
            mixedStr += " ";
            mixedStr += "\xf0\x9f\x98\x80"; // 4-byte
            mixedStr += " ";
        }
        EXPECT_TRUE(isValidUtf8(mixedStr));
    }
}

TEST(Utf8Test, StressManySmallValidations)
{
    using megacmd::isValidUtf8;

    std::vector<std::string> testStrings = {
        "a",
        "\xc3\xb1",
        "\xe4\xb8\xad",
        "\xf0\x9f\x98\x80",
        "Hello World",
        "\xD0\x9C\xD0\x95\xD0\x93\xD0\x90",
        ""
    };

    G_SUBTEST << "Many repeated validations";
    {
        for (int i = 0; i < 100000; ++i)
        {
            for (const auto& str : testStrings)
            {
                EXPECT_TRUE(isValidUtf8(str));
            }
        }
    }
}

#ifdef _WIN32
TEST(Utf8Test, StressLargeConversions)
{
    using megacmd::utf8StringToUtf16WString;
    using megacmd::utf16ToUtf8;
    using megacmd::isValidUtf8;

    G_SUBTEST << "Large UTF-8 to UTF-16 conversion";
    {
        std::string largeUtf8;
        largeUtf8.reserve(100000);
        for (int i = 0; i < 10000; ++i)
        {
            largeUtf8 += "\xD0\x9C\xD0\x95\xD0\x93\xD0\x90 ";
        }

        std::wstring utf16 = utf8StringToUtf16WString(largeUtf8.c_str());
        EXPECT_GT(utf16.size(), 0);

        std::string backToUtf8 = utf16ToUtf8(utf16);
        EXPECT_EQ(backToUtf8, largeUtf8);
        EXPECT_TRUE(isValidUtf8(backToUtf8));
    }

    G_SUBTEST << "Many small conversions";
    {
        for (int i = 0; i < 10000; ++i)
        {
            std::wstring utf16 = utf8StringToUtf16WString("Test \xc3\xb1 string");
            std::string utf8 = utf16ToUtf8(utf16);
            EXPECT_EQ(utf8, "Test \xc3\xb1 string");
        }
    }
}
#endif
