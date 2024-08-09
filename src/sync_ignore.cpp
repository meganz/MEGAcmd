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

#include "sync_ignore.h"

#include <fstream>
#include <cstring>
#include <regex>

#include "megacmdcommonutils.h"

namespace {
    const char* BOMStr = "\xEF\xBB\xBF";

#ifdef _WIN32
    const char* NL = "\r\n";
#else
    const char* NL = "\n";
#endif

    void removeCarriageReturn(std::string& str)
    {
#ifdef _WIN32
        if (!str.empty() && str.back() == '\r')
        {
            str.pop_back();
        }
#endif
    }
}

bool MegaIgnoreFile::checkBOMAndSkip(std::ifstream& file)
{
    char bom[4];
    file.read(bom, 3);
    bom[3] = '\0';
    if (strcmp(bom, BOMStr))
    {
        // If there's no utf-8 BOM, seek to the begining of the file
        file.seekg(0);
        return false;
    }
    return true;
}

void MegaIgnoreFile::loadFilters(std::ifstream& file)
{
    mFilters.clear();
    for (std::string line; getline(file, line);)
    {
        removeCarriageReturn(line);
        if (line.empty() || line[0] == '#')
        {
            continue;
        }
        mFilters.insert(line);
    }
}

std::string MegaIgnoreFile::getDefaultPath()
{
    auto platformDirs = megacmd::PlatformDirectories::getPlatformSpecificDirectories();
    auto configDirPath = platformDirs->configDirPath();
    return configDirPath + "/.megaignore.default";
}

bool MegaIgnoreFile::isValidFilter(const std::string& filter)
{
    thread_local std::regex filterRegex(R"([-+][adfs]?[Nnp]?[GgRr]?:\S+)");
    return std::regex_match(filter, filterRegex);
}

MegaIgnoreFile::MegaIgnoreFile(const std::string& path) :
    mPath(path),
    mValid(false)
{
    load();
}

void MegaIgnoreFile::load()
{
    std::ifstream file(mPath);
    if (!file.is_open() || file.fail())
    {
        return;
    }
    checkBOMAndSkip(file);
    loadFilters(file);
    mValid = true;
}

void MegaIgnoreFile::addFilters(const std::set<std::string>& filters)
{
    assert(mValid);

    std::ofstream file(mPath, std::ios_base::app);
    for (const std::string& filter : filters)
    {
        file << filter << NL;
    }
    mValid = false;
}

void MegaIgnoreFile::removeFilters(const std::set<std::string>& filters)
{
    assert(mValid);

    std::string newContents;
    bool hasBOM = false;
    {
        std::ifstream file(mPath);
        hasBOM = checkBOMAndSkip(file);
        for (std::string line; getline(file, line);)
        {
            removeCarriageReturn(line);
            if (filters.count(line))
            {
                continue;
            }
            newContents += line + NL;
        }
    }

    std::ofstream outFile(mPath, std::ios_base::trunc);
    outFile << (hasBOM ? BOMStr : "") << newContents;
    mValid = false;
}

bool MegaIgnoreFile::containsFilter(const std::string& filter) const
{
    assert(mValid);
    return mFilters.count(filter) > 0;
}

std::string MegaIgnoreFile::getFilterContents() const
{
    assert(mValid);

    std::string contents;
    for (const std::string& filter : mFilters)
    {
        contents += filter + '\n';
    }
    return contents;
}
