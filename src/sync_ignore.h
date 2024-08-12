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
        std::string mMegaIgnoreDirPath;
        std::set<std::string> mFilters;
    };

    void executeCommand(const Args& args);
}

class MegaIgnoreFile
{
    std::set<std::string> mFilters;
    std::string mPath;
    bool mValid;

    bool checkBOMAndSkip(std::ifstream& file);
    void loadFilters(std::ifstream& file);

public:
    static std::string getDefaultPath();
    static bool isValidFilter(const std::string& filter);

    MegaIgnoreFile(const std::string& path);

    void load();
    void addFilters(const std::set<std::string>& filters);
    void removeFilters(const std::set<std::string>& filters);

    bool isValid() const { return mValid; }
    bool containsFilter(const std::string& filter) const;
    std::string getFilterContents() const; // without comments, bom, etc.
};
