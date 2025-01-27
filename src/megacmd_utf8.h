/**
 * @file src/megacmd_utf8.h
 * @brief MEGAcmd: utf8 and console resources
 *
 * (c) 2013 by Mega Limited, Auckland, New Zealand
 *
 * This file is part of the MEGAcmd.
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

#pragma once

#include <iostream>
#include <cassert>
#include <mutex>

#ifdef _WIN32
#include <streambuf>
#include <conio.h>
#include <fstream>
#include <iomanip>

#include <io.h>
#include <fcntl.h>
#include <algorithm>
#include <string>
#include <cwctype>

#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#ifndef _O_U16TEXT
#define _O_U16TEXT 0x00020000
#endif
#ifndef _O_U8TEXT
#define _O_U8TEXT 0x00040000
#endif
#endif

#include <filesystem>
namespace fs = std::filesystem;

namespace megacmd {

std::string pathAsUtf8(const fs::path& path);
bool isValidUtf8(const char* data, size_t size);
bool isValidUtf8(const std::string &str);

struct StdoutMutexGuard
{
    inline static std::recursive_mutex sSetmodeMtx;
    std::lock_guard<std::recursive_mutex> mGuard;

    StdoutMutexGuard() : mGuard(sSetmodeMtx) {}
};

/* platform dependent */
#ifdef _WIN32

#define OUTSTREAMTYPE std::wostream
#define OUTFSTREAMTYPE std::wofstream
#define OUTSTRINGSTREAM std::wostringstream
#define OUTSTRING std::wstring
#define COUT std::wcout
#define CERR std::wcerr

//override << operators for wostream for string and const char *
std::wostream & operator<< ( std::wostream & ostr, std::string const & str );
std::wostream & operator<< ( std::wostream & ostr, const char * str );

// UTF-8 to wstrings (UTF-16) conversions:
void stringtolocalw(const char* path, std::wstring* local);
void stringtolocalw(const char* path, size_t lenutf8, std::wstring* local);
std::wstring utf8StringToUtf16WString(const char* str, size_t lenutf8);
std::wstring utf8StringToUtf16WString(const char* path);

// convert Utf-16 wide chars to UTF-8 std::strings
void localwtostring(const std::wstring* wide, std::string *multibyte);
void utf16ToUtf8(const wchar_t* utf16data, int utf16size, std::string* utf8string);
std::string utf16ToUtf8(const wchar_t *ws);
std::string utf16ToUtf8(const std::wstring &ws);

std::wstring nonMaxPathLimitedWstring(const fs::path &localpath);
fs::path nonMaxPathLimitedPath(const fs::path &localpath);

/***
 *    operator<< overloads to ensure proper handling of paths and widestrings
 *    This header would need to be included first in all project as a general rule, so that these are used
 *    As a way to ensure this, we could only define fs namespace here.
 *
 *    Note: beyond these, operator<< overloads for logging can be found at megacmdlogger.h
 **/
//// This is expected to be used when trying to << a wstring (supposedly in UTF-16, into a ostream (string/file/cout/...)
template <typename T>
inline std::enable_if_t<std::is_same_v<std::decay_t<T>, std::wstring>, std::ostream&>
operator<<(std::ostream& oss, const T& wstr)
{
    static_assert(false); // Let's forbid this to better control that we just write utf8 std::strings in non wide streams (e.g cout)
    // Notice that in the end, in stdout we want to write widestrings (utf16) to wcout instead, to have console rendering properly.

    // If we were to automatically support this we should convert them to utf8 string as follows:
    oss << megacmd::utf16ToUtf8(wstr);
    return oss;
}
} // end of namespace megacmd

namespace std::filesystem {

    // overload that may be used when building some stringstream.
    // Note: LOG_xxx << path should are handled by SimpleLogger overloads, not this one
    inline std::ostream &operator<<(std::ostream& oss, const fs::path& path)
    {
        // caveat: outputting its contents (utf-8) to stdout would need to be done converting to utf-16 and using wcout
        //   and a valid mode to stdout (See WindowsUtf8StdoutGuard)
        assert(&oss != &std::cout);
        assert(&oss != &std::cerr);

        oss << megacmd::pathAsUtf8(path);
        return oss;
    }

} // end of namespace std::filesystem
namespace megacmd {

/**
 * @brief This class is used to:
 * - guard no meddling while writting/setting output mode on stdout/stderr
 * - ensure setting the output modes to mOutputMode
 */
class OutputsModeGuard : public StdoutMutexGuard
{
        unsigned int mOutputMode;
        int mOldModeStdout;
        int mOldModeStderr;
    public:
        OutputsModeGuard(unsigned int outputMode)
            : mOutputMode(outputMode)
        {
            fflush(stdout);
            fflush(stderr);
            mOldModeStdout = _setmode(_fileno(stdout), mOutputMode);
            mOldModeStderr = _setmode(_fileno(stderr), mOutputMode);
            assert(mOldModeStdout != -1);
            assert(mOldModeStderr != -1);
        }

        virtual ~OutputsModeGuard()
        {
            fflush(stdout);
            fflush(stderr);
            _setmode(_fileno(stdout), mOldModeStdout);
            _setmode(_fileno(stderr), mOldModeStderr);
        }
};

template <unsigned int OUTPUT_MODE>
class WindowsOutputsModeGuardGeneric : public OutputsModeGuard
{
public:
    WindowsOutputsModeGuardGeneric()
     : OutputsModeGuard(OUTPUT_MODE)
    {}
};

using WindowsUtf8StdoutGuard = WindowsOutputsModeGuardGeneric<_O_U8TEXT>;
using WindowsNarrowStdoutGuard = WindowsOutputsModeGuardGeneric<_O_TEXT>;
using WindowsBinaryStdoutGuard = WindowsOutputsModeGuardGeneric<O_BINARY>;

class InterceptStreamBuffer : public std::streambuf
{
    private:
    std::streambuf* mOriginalStreamBuffer; // Store the original buffer
    std::ostream& mNarrowStream;            // Reference to the original stream (e.g., std::cout)
    std::wostream& mWideOstream;            // Reference to the original stream (e.g., std::cout)

    bool hasNonAscii(const char* str, size_t length)
    {
        for (size_t i = 0; i < length; ++i)
        {
            if (static_cast<unsigned char>(str[i]) > 127)
            {
                return true;
            }
        }
        return false;
    }

public:
    InterceptStreamBuffer(std::ostream& outStream, std::wostream &wideStream)
        : mNarrowStream(outStream), mWideOstream(wideStream)
    {
        mOriginalStreamBuffer = mNarrowStream.rdbuf(); // Save the original buffer
        mNarrowStream.rdbuf(this);               // Replace with this buffer
    }

    ~InterceptStreamBuffer()
    {
        mNarrowStream.rdbuf(mOriginalStreamBuffer); // Restore the original buffer on destruction
    }

protected:
    virtual int overflow(int c) override
    {
        char cc = char(c);
        xsputn(&cc, 1);
        return c;
    }

    virtual std::streamsize xsputn(const char* s, std::streamsize count) override
    {
        if (hasNonAscii(s, count)) // why cannot:
        {
            WindowsUtf8StdoutGuard utf8Guard;
            //  assert(false && "This should ideally be controlled in calling code"); //TODO: enable this assert to fix cases in origin (assumed better performance)
            mWideOstream << utf8StringToUtf16WString(s, count);
            return count;
        }
        else
        {
           WindowsNarrowStdoutGuard narrowGuard;
            return mOriginalStreamBuffer->sputn(s, count); // This is protesting when having _setmode and doing cout << "odd" (odd number of chars)
        }
    }
};

class WindowsConsoleController
{
    std::unique_ptr<megacmd::InterceptStreamBuffer> mInterceptCout;
    std::unique_ptr<megacmd::InterceptStreamBuffer> mInterceptCerr;

public:
    WindowsConsoleController();
    void enableInterceptors(bool enable);
};

#else
#define OUTSTREAMTYPE std::ostream
#define OUTFSTREAMTYPE std::ofstream
#define OUTSTRINGSTREAM std::ostringstream
#define OUTSTRING std::string
#define COUT std::cout
#define CERR std::cerr

#endif

}//end namespace
