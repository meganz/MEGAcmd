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
#include <filesystem>

#include "megacmdcommonutils.h"
#include "megacmdlogger.h"

using namespace megacmd;
namespace fs = std::filesystem;

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

std::unique_lock<std::mutex> getFileLock(const std::string& str)
{
    static std::mutex mapMutex;
    static std::map<std::string, std::mutex> fileMutexMap;

    std::lock_guard mapLock(mapMutex);
    return std::unique_lock(fileMutexMap[str]);
}

void executeAdd(MegaIgnoreFile& megaIgnoreFile, const std::set<std::string>& filters)
{
    // Only check the format of the filters when adding, to allow users to remove invalid filters from the file
    for (const std::string& filter : filters)
    {
        if (MegaIgnoreFile::isValidFilter(filter))
        {
            continue;
        }

        // At least one incorrect filter format must cause the entire command to fail
        // Otherwise, if we allowed partial filters to come through, it could result in unwanted sync states
        setCurrentOutCode(MCMD_EARGS);
        LOG_err << "The filter \"" << filter << "\" does not have valid format";
        return;
    }

    std::set<std::string> toAdd;
    for (const std::string& filter : filters)
    {
        if (megaIgnoreFile.containsFilter(filter))
        {
            OUTSTREAM << "Cannot add filter \"" << filter << "\" because it's already in the .megaignore file (skipped)" << endl;
            continue;
        }
        toAdd.insert(filter);
        OUTSTREAM << "Added filter \"" << filter << "\"" << endl;
    }
    megaIgnoreFile.addFilters(toAdd);
}

void executeRemove(MegaIgnoreFile& megaIgnoreFile, const std::set<std::string>& filters)
{
    std::set<std::string> toRemove;
    for (const std::string& filter : filters)
    {
        if (!megaIgnoreFile.containsFilter(filter))
        {
            OUTSTREAM << "Cannot remove filter \"" << filter << "\" because it's not in the .megaignore file (skipped)" << endl;
            continue;
        }
        toRemove.insert(filter);
        OUTSTREAM << "Removed filter \"" << filter << "\"" << endl;
    }
    megaIgnoreFile.removeFilters(toRemove);
}

void executeSyncIgnoreCommand(const SyncIgnore::Args& args, MegaIgnoreFile& megaIgnoreFile)
{
    switch (args.mAction)
    {
        case SyncIgnore::Action::Show:
        {
            OUTSTREAM << megaIgnoreFile.getFilterContents();
            break;
        }
        case SyncIgnore::Action::Add:
        {
            executeAdd(megaIgnoreFile, args.mFilters);
            break;
        }
        case SyncIgnore::Action::Remove:
        {
            executeRemove(megaIgnoreFile, args.mFilters);
            break;
        }
    }
}

} // end namespace

namespace SyncIgnore {

void executeCommand(const Args& args)
{
    if (args.mFilters.empty() && (args.mAction == Action::Add || args.mAction == Action::Remove))
    {
        setCurrentOutCode(MCMD_EARGS);
        LOG_err << "At least one name filter is required for add or remove";
        LOG_err << "      " << getUsageStr("sync-ignore");
        return;
    }

    std::string megaIgnoreFilePath;
    bool isDefault = true;
    if (args.mMegaIgnoreDirPath.empty())
    {
        megaIgnoreFilePath = MegaIgnoreFile::getDefaultPath();
    }
    else
    {
        megaIgnoreFilePath = args.mMegaIgnoreDirPath + "/.megaignore";
        isDefault = false;
    }

    if (!fs::exists(megaIgnoreFilePath))
    {
        setCurrentOutCode(MCMD_NOTFOUND);
        if (isDefault)
        {
            // For the default file to be created, a sync that DOES NOT already contain a .megaignore file has to be added
            LOG_err << "Default .megaignore file does not exist (will be created when a new sync is added)";
        }
        else
        {
            LOG_err << "Mega ignore file \"" << megaIgnoreFilePath << "\" does not exist";
        }
        return;
    }

    MegaIgnoreFile megaIgnoreFile(megaIgnoreFilePath);
    if (!megaIgnoreFile.isValid())
    {
        setCurrentOutCode(MCMD_INVALIDSTATE);
        LOG_err << "There was an error opening mega ignore file \"" << megaIgnoreFilePath;
        return;
    }

    executeSyncIgnoreCommand(args, megaIgnoreFile);
}

std::string getFilterFromLegacyPattern(const std::string& pattern)
{
    return "-:" + pattern;
}

} // end namespace

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
    if (fs::exists(mPath))
    {
        load();
    }
    else
    {
        createWithBOM();
    }
}

void MegaIgnoreFile::load()
{
    auto fileLock = getFileLock(mPath);

    std::ifstream file(mPath);
    if (!file.is_open() || file.fail())
    {
        return;
    }
    checkBOMAndSkip(file);
    loadFilters(file);
    mValid = true;
}

void MegaIgnoreFile::createWithBOM()
{
    auto fileLock = getFileLock(mPath);

    std::ofstream file(mPath);
    if (!file.is_open() || file.fail())
    {
        return;
    }
    file << BOMStr;
    mValid = true;
}

void MegaIgnoreFile::addFilters(const std::set<std::string>& filters)
{
    assert(mValid);
    auto fileLock = getFileLock(mPath);

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
    auto fileLock = getFileLock(mPath);

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
