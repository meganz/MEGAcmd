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

#include "megacmdcommonutils.h"

#include <string>
#include <set>

namespace SyncIgnore
{
    enum class Action
    {
        Show,
        Add,
        Remove
    };

    struct Args
    {
        Action mAction;
        fs::path mMegaIgnoreDirPath;
        std::set<std::string> mFilters;
    };

    void executeCommand(const Args& args);

    std::string getFilterFromLegacyPattern(const std::string& pattern);
}

class MegaIgnoreFile
{
    std::set<std::string> mFilters;
    fs::path mPath;
    bool mValid;

    bool checkBOMAndSkip(std::ifstream& file);
    void loadFilters(std::ifstream& file);

    void loadFromPath();
    void createWithBOM();

public:
    static fs::path getDefaultPath();
    static bool isValidFilter(const std::string& filter);

    MegaIgnoreFile(const fs::path& path);

    bool isValid() const { return mValid; }

    void addFilters(const std::set<std::string>& filters);
    void removeFilters(const std::set<std::string>& filters);

    bool containsFilter(const std::string& filter) const;
    std::string getFilterContents() const; // without comments, bom, etc.
};
