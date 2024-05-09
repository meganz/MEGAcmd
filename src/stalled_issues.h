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

class StalledIssue
{
    const size_t mId;
    std::unique_ptr<const mega::MegaSyncStall> mMegaStall;

public:
    StalledIssue(size_t id, const mega::MegaSyncStall& stall);

    inline size_t getId() const { return mId; }

    std::string getSyncWaitReasonStr() const;
    std::string getMainPath() const;
};

using StalledIssueList = std::vector<StalledIssue>;

class StalledIssueCache
{
    const StalledIssueList& mStalledIssues;
    std::unique_lock<std::mutex> mStalledIssuesLock;

public:
    StalledIssueCache(const StalledIssueList& stalledIssues, std::mutex& stalledIssuesMutex);

    bool empty() const { return mStalledIssues.empty(); }

    StalledIssueList::const_iterator begin() const { return mStalledIssues.begin(); }
    StalledIssueList::const_iterator end() const { return mStalledIssues.end(); }
};

class StalledIssuesManager : public mega::MegaGlobalListener, public mega::MegaRequestListener
{
    mega::MegaApi* mMegaApi;

    bool mSyncStalled;

    StalledIssueList mStalledIssues;
    std::mutex mStalledIssuesMutex;

private:
    void onGlobalSyncStateChanged(mega::MegaApi *api) override;
    void onRequestFinish(mega::MegaApi*, mega::MegaRequest *request, mega::MegaError*) override;

    void populateStalledIssues(const mega::MegaSyncStallList &stalls);

public:
    StalledIssuesManager(mega::MegaApi *api);

    StalledIssueCache getLockedCache();
};
