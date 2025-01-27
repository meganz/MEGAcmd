/**
 * @file src/megacmd_utf8.cpp
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

#include "megacmd_utf8.h"

#ifdef WIN32
#include <Shlwapi.h>
#include <Shellapi.h>
#include <windows.h>
#endif

namespace megacmd {

#ifdef _WIN32
std::wostream & operator<< ( std::wostream & ostr, std::string const & str )
{
    std::wstring toout;
    stringtolocalw(str.c_str(),&toout);
    ostr << toout;
    return ( ostr );
}

std::wostream & operator<< ( std::wostream & ostr, const char * str )
{
    std::wstring toout;
    stringtolocalw(str,&toout);
    ostr << toout;
    return ( ostr );
}

// convert UTF-8 to Windows Unicode wstring (UTF-16)
void stringtolocalw(const char* path, size_t lenutf8, std::wstring* local)
{
    assert(isValidUtf8(std::string(path, lenutf8)));

    // make space for the worst case
    local->resize((strlen(path) + 1) * sizeof(wchar_t));

    int wchars_num = MultiByteToWideChar(CP_UTF8, 0, path, lenutf8 + 1, NULL,0);
    local->resize(wchars_num);

    int len = MultiByteToWideChar(CP_UTF8, 0, path, lenutf8 + 1, (wchar_t*)local->data(), wchars_num);

    if (len)
    {
        local->resize(len-1);
    }
    else
    {
        local->clear();
    }
}

// convert UTF-8 to Windows Unicode wstring (UTF-16)
void stringtolocalw(const char* path, std::wstring* local)
{
    stringtolocalw(path, strlen(path), local);
}

std::wstring utf8StringToUtf16WString(const char* str)
{
    std::wstring toret;
    stringtolocalw(str, &toret);
    return toret;
}

std::wstring utf8StringToUtf16WString(const char* str, size_t lenutf8)
{
    std::wstring toret;
    stringtolocalw(str, lenutf8, &toret);
    return toret;
}

void localwtostring(const std::wstring* wide, std::string *multibyte)
{
    utf16ToUtf8(wide->data(), (int)wide->size(), multibyte);
}

void utf16ToUtf8(const wchar_t* utf16data, int utf16size, std::string* utf8string)
{
    if(!utf16size)
    {
        utf8string->clear();
        return;
    }

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, utf16data, utf16size, NULL, 0, NULL, NULL);
    utf8string->resize(size_needed);
    WideCharToMultiByte(CP_UTF8, 0, utf16data, utf16size, (char*)utf8string->data(),  size_needed, NULL, NULL);
    assert(isValidUtf8(*utf8string));
}

std::string utf16ToUtf8(const std::wstring &ws)
{
    std::string utf8s;
    utf16ToUtf8(ws.c_str(), ws.size(), &utf8s);
    return utf8s;
}

std::string utf16ToUtf8(const wchar_t *ws)
{
    std::string utf8s;
    utf16ToUtf8(ws, int(wcslen(ws)), &utf8s);
    return utf8s;
}

std::wstring nonMaxPathLimitedWstring(const fs::path &localpath)
{
    static const std::wstring wprefix(LR"(\\?\)");
    if (localpath.wstring().rfind(wprefix, 0) == 0)
    {
        return localpath.wstring();
    }
    auto prefixedPathWstring = std::wstring(wprefix).append(localpath.wstring());
    assert(prefixedPathWstring.rfind(wprefix, 0) == 0);
    return prefixedPathWstring;
}

fs::path nonMaxPathLimitedPath(const fs::path &localpath)
{
    auto prefixedPath = fs::path(nonMaxPathLimitedWstring(localpath));
    assert(prefixedPath.wstring().rfind(LR"(\\?\)", 0) == 0);
    return prefixedPath;
}

WindowsConsoleController::WindowsConsoleController()
{
    if (!getenv("MEGACMD_DISABLE_UTF8_OUTPUT_MODE_BY_DEFAULT"))
    {
        // set default mode to U8TEXT.
        // PROs: we could actually get rid of _setmode to U8TEXT in WindowsUtf8StdoutGuard
        // CONS: any low level printing to stdout may crash if the mode is not _O_TEXT.
        //  Even if we do not do that, for instance, gtests would do.
        auto OUTPUT_MODE = _O_U8TEXT;
        auto oldModeStdout = _setmode(_fileno(stdout), OUTPUT_MODE);
        auto oldModeStderr = _setmode(_fileno(stderr), OUTPUT_MODE);
    }

    if (!getenv("MEGACMD_DISABLE_NARROW_OUTSTREAMS_INTERCEPTING"))
    {
        enableInterceptors(true);
    }
}

void WindowsConsoleController::enableInterceptors(bool enable)
{
    std::cout.flush();
    std::cerr.flush();

    if (enable)
    {
        mInterceptCout.reset(new InterceptStreamBuffer(std::cout, std::wcout));
        mInterceptCerr.reset(new InterceptStreamBuffer(std::cerr, std::wcerr));
    }
    else
    {
        mInterceptCout.reset();
        mInterceptCerr.reset();
    }
}

#endif

std::string pathAsUtf8(const fs::path& path)
{
#ifdef _WIN32
    return utf16ToUtf8(path.wstring().c_str());
#else
    return path.string();
#endif
}

bool isValidUtf8(const char* data, size_t size)
{
    static bool disableUTF8Valiations = getenv("MEGACMD_DISABLE_UTF8_VALIDATIONS");
    if (disableUTF8Valiations)
    {
        return true;
    }
    // checks that the byte starts with bits 10 (i.e. continuation bytes)
    auto check10 = [&data](size_t n) -> bool {
        return (data[n] & 0xc0) == 0x80;
    };

    while (size)
    {
        const uint8_t lead = static_cast<uint8_t>(*data);

        // 0xxxxxxx -> U+0000..U+007F (1-byte character)
        if (lead < 0x80)
        {
            ++data;
            --size;
            continue;
        }
        // 110xxxxx -> U+0080..U+07FF (2-byte character)
        else if ((lead & 0xe0) == 0xc0)
        {
            // check codepoint is at least 0x80 and check continuation byte
            if (lead > 0xc1 && size >= 2 && check10(1))
            {
                data += 2;
                size -= 2;
                continue;
            }
        }
        // 1110xxxx -> U+0800..U+FFFF (3-byte character)
        else if ((lead & 0xf0) == 0xe0)
        {
            // check continuation bytes
            if (size >= 3 && check10(1) && check10(2))
            {
                const auto secondByte = static_cast<uint8_t>(data[1]);

                // check codepoint is at least 0x800 and not a surrogate codepoint in the range 0xd800-0xdfff
                if (((lead << 8) | secondByte) > 0xe09f &&
                    (lead != 0xed || secondByte < 0xa0))
                {
                    data += 3;
                    size -= 3;
                    continue;
                }
            }
        }
        // 11110xxx -> U+10000..U+10FFFF (4-byte character)
        else if ((lead & 0xf8) == 0xf0)
        {
            // check continuation bytes5
            if (size >= 4 && check10(1) && check10(2) && check10(3))
            {
                const auto firstHalf = (lead << 8) | static_cast<uint8_t>(data[1]);

                // check codepoint is at least 0x10000 and not greater than 0x10FFFF (not encodable by UTF-16)
                if (firstHalf > 0xf08f && firstHalf < 0xf490)
                {
                    data += 4;
                    size -= 4;
                    continue;
                }
            }
        }

        return false;
    }
    return true;
}
bool isValidUtf8(const std::string &str)
{
    return isValidUtf8(str.data(), str.size());
}


} //end namespace
