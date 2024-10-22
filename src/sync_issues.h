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

#include <vector>
#include <memory>
#include <mutex>

#include "megaapi.h"

struct SyncInfo
{
    std::string mReason;
    std::string mDescription;
};

class SyncIssue
{
    mutable std::string mId;
    std::unique_ptr<const mega::MegaSyncStall> mMegaStall;

    template<bool preferCloud = false>
    std::string getFilePath() const;

    template<bool preferCloud = false>
    std::string getFileName() const;

    template<bool isCloud>
    bool hasPathProblem(mega::MegaSyncStall::SyncPathProblem pathProblem) const;

public:
    // We add this prefix at the start to distinguish between cloud and local absolute paths
    inline static const std::string CloudPrefix = "<CLOUD>";

    SyncIssue(const mega::MegaSyncStall& stall);

    const std::string& getId() const;
    SyncInfo getSyncInfo(mega::MegaSync const* parentSync) const;

    std::unique_ptr<mega::MegaSync> getParentSync(mega::MegaApi& api) const;
    bool belongsToSync(const mega::MegaSync& sync) const;
};

class SyncIssueList
{
    std::vector<SyncIssue> mIssuesVec;
    friend class SyncIssuesRequestListener; // only one that can actually populate this

public:
    template<typename Cb>
    void forEach(Cb&& callback, size_t sizeLimit)
    {
        for (size_t i = 0; i < size() && i < sizeLimit; ++i)
        {
            callback(mIssuesVec[i]);
        }
    }

    unsigned int getSyncIssueCount(const mega::MegaSync& sync) const;

    bool empty() const { return mIssuesVec.empty(); }
    unsigned int size() const { return mIssuesVec.size(); }

    std::vector<SyncIssue>::const_iterator begin() const { return mIssuesVec.begin(); }
    std::vector<SyncIssue>::const_iterator end() const { return mIssuesVec.end(); }
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
