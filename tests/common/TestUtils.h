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

#pragma once

#include <stdio.h>
#include <stdarg.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include <memory>
#include <future>

#include <sstream>

#define ULOG_COLOR(color, fmt, ...) \
    do { if (isatty(1)) printf(color fmt "\033[0m\n", ##__VA_ARGS__); \
         else printf(fmt "\n", ##__VA_ARGS__); \
    } while(0)

#define ULOG(fmt, ...) ULOG_COLOR("\033[38;5;250m", fmt, ##__VA_ARGS__)
#define ULOG_ERR(fmt, ...) ULOG_COLOR("\033[31m", fmt, ##__VA_ARGS__)
#define ULOG_WARN(fmt, ...) ULOG_COLOR("\033[33m", fmt, ##__VA_ARGS__)
#define ULOG_NOTE(fmt, ...) ULOG_COLOR("\033[38;5;81m", fmt, ##__VA_ARGS__)

namespace gti
{
    enum GTestColor
    {
        COLOR_DEFAULT,
        COLOR_RED,
        COLOR_GREEN,
        COLOR_YELLOW
    };
    static void ColoredPrintf(GTestColor color, const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);

#ifdef WIN32
        static const bool in_color_mode = isatty(_fileno(stdout)) != 0;
#else
        static const bool in_color_mode = isatty(STDOUT_FILENO) != 0;
#endif

        const bool use_color = in_color_mode && (color != GTestColor::COLOR_DEFAULT);

        if (use_color)
        {
            printf("\033[0;3%sm", std::to_string(static_cast<int>(color)).c_str());
        }
        vprintf(fmt, args);
        if (use_color)
        {
            printf("\033[m");
        }
        va_end(args);
    }
}

class GTestMessager
{
    using GTestColor = gti::GTestColor;
    std::ostringstream mOstr;
    GTestColor mColor;
    std::string mTitle;

public:
    GTestMessager(GTestColor color, std::string title = {}) : mColor(color), mTitle(title)
    {
    }
    ~GTestMessager()
    {
        gti::ColoredPrintf(GTestColor::COLOR_GREEN, "[ %8s ] ", mTitle.c_str());
        gti::ColoredPrintf(GTestColor::COLOR_DEFAULT, "%s\n", mOstr.str().c_str());
    }

    template <typename T>
    GTestMessager& operator<<(T* obj)
    {
        if (obj)
        {
            mOstr << obj;
        }
        else
        {
            mOstr << "(NULL)";
        }
        return *this;
    }

    template <typename T, typename = typename std::enable_if<std::is_scalar<T>::value>::type>
    GTestMessager& operator<<(const T obj)
    {
        static_assert(!std::is_same<T, std::nullptr_t>::value, "T cannot be nullptr_t");
        mOstr << obj;
        return *this;
    }

    template <typename T, typename = typename std::enable_if<!std::is_scalar<T>::value>::type>
    GTestMessager& operator<<(const T& obj)
    {
        mOstr << obj;
        return *this;
    }
};
// Fancy message defines
#define G_TEST_TITLED_MSG(title) GTestMessager(gti::COLOR_GREEN, title)
#define G_TEST_MSG   GTestMessager(gti::COLOR_GREEN)
#define G_TEST_INFO  GTestMessager(gti::COLOR_GREEN, "INFO")
#define G_SUBTEST    GTestMessager(gti::COLOR_GREEN, "SUBTEST")
#define G_SUBSUBTEST GTestMessager(gti::COLOR_GREEN, "SSUBTEST")


