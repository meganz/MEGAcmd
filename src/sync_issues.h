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

#include <map>
#include <memory>
#include <mutex>
#include <optional>

#include "megaapi.h"

struct SyncInfo
{
    std::string mReason;
    std::string mDescription;
};

class SyncIssue
{
    std::unique_ptr<const mega::MegaSyncStall> mMegaStall;

    // Returns the cloud/local file path with index i
    template<bool isCloud>
    std::string getFilePath(int i = 0) const;

    // Returns the file name of the cloud/local path with index i
    template<bool isCloud>
    std::string getFileName(int i) const;

    // Returns the file name of path 0, with preference for cloud or local
    template<bool preferCloud = false>
    std::string getDefaultFileName() const;

    // Returns the index of the first path problem of a particular type (if any)
    template<bool isCloud>
    std::optional<int> getPathProblem(mega::MegaSyncStall::SyncPathProblem pathProblem) const;

public:
    // We add this prefix at the start to distinguish between cloud and local absolute paths
    inline static const std::string CloudPrefix = "<CLOUD>";

    SyncIssue(const mega::MegaSyncStall& stall);

    unsigned int getId() const;
    SyncInfo getSyncInfo(mega::MegaSync const* parentSync) const;

    std::unique_ptr<mega::MegaSync> getParentSync(mega::MegaApi& api) const;
    bool belongsToSync(const mega::MegaSync& sync) const;
};

class SyncIssueList
{
    std::map<unsigned int, SyncIssue> mIssuesMap;
    friend class SyncIssuesRequestListener; // only one that can actually populate this

public:
    template<typename Cb>
    void forEach(Cb&& callback, size_t sizeLimit) const
    {
        for (auto it = begin(); it != end() && std::distance(begin(), it) < sizeLimit; ++it)
        {
            callback(it->second);
        }
    }

    unsigned int getSyncIssuesCount(const mega::MegaSync& sync) const;

    bool empty() const { return mIssuesMap.empty(); }
    unsigned int size() const { return mIssuesMap.size(); }

    std::map<unsigned int, SyncIssue>::const_iterator begin() const { return mIssuesMap.begin(); }
    std::map<unsigned int, SyncIssue>::const_iterator end() const { return mIssuesMap.end(); }
};

class SyncIssuesManager final
{
    mega::MegaApi& mApi;
    bool mWarningEnabled;
    std::mutex mWarningMtx;

    std::unique_ptr<mega::MegaGlobalListener> mGlobalListener;
    std::unique_ptr<mega::MegaRequestListener> mRequestListener;

private:
    void onSyncIssuesChanged(unsigned int newSyncIssuesSize);

public:
    SyncIssuesManager(mega::MegaApi *api);

    SyncIssueList getSyncIssues() const;

    void disableWarning();
    void enableWarning();

    mega::MegaGlobalListener* getGlobalListener() const { return mGlobalListener.get(); }
};
