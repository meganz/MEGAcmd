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

#include "stalled_issues.h"

#include <cassert>
#include <functional>

#include "megacmdlogger.h"

#ifdef MEGACMD_TESTING_CODE
    #include "../tests/common/Instruments.h"
    using TI = TestInstruments;
#endif

namespace {

class StalledIssuesGlobalListener : public mega::MegaGlobalListener
{
    using Callback = std::function<void()>;
    Callback mCallback;

    bool mScanning;
    bool mWaiting;
    bool mSyncStalled;

    void onGlobalSyncStateChanged(mega::MegaApi* api) override
    {
        // Trigger for "scanning status changed"; ignore
        bool scanning = api->isScanning();
        if (mScanning != scanning)
        {
            mScanning = scanning;
            return;
        }

        // Trigger for "waiting status changed"; ignore
        bool waiting = (api->isWaiting() != mega::MegaApi::RETRY_NONE);
        if (mWaiting != waiting)
        {
            mWaiting = waiting;
            return;
        }

        bool syncStalled = api->isSyncStalled();
        bool syncStalledChanged = (mSyncStalled != syncStalled || (syncStalled && api->isSyncStalledChanged()));

        // Trigger for "stalled status changed"
        if (syncStalledChanged)
        {
            bool syncBusy = mScanning || mWaiting;
            bool notStalledToStalledTransition = syncStalled && !mSyncStalled;

            if (!syncBusy || notStalledToStalledTransition)
            {
                // Generally, we want to ignore triggers when we're in a "busy"
                // state (scanning or waiting), since they might contain outdated info.
                // Unless we're going from "not stalled" to "stalled"; in that case
                // the info will always be up-to-date.
                mCallback();
            }

            // Update mSyncStalled even if we're not triggering the callback,
            // so it still matches the sync engine's internal value
            mSyncStalled = syncStalled;
        }
    }

public:
    StalledIssuesGlobalListener(Callback&& callback) :
        mCallback(std::move(callback)),
        mScanning(false),
        mWaiting(false),
        mSyncStalled(false) {}
};

class StalledIssuesRequestListener : public mega::MegaRequestListener
{
   using Callback = std::function<void(const mega::MegaSyncStallList& stalls)>;
   Callback mCallback;

   void onRequestFinish(mega::MegaApi *api, mega::MegaRequest *request, mega::MegaError *e) override
   {
       assert(request && request->getType() == mega::MegaRequest::TYPE_GET_SYNC_STALL_LIST);

        auto stalls = request->getMegaSyncStallList();
        if (!stalls)
        {
            LOG_err << "Sync stall list pointer is null";
            return;
        }

        mCallback(*stalls);
   }

public:
   StalledIssuesRequestListener(Callback&& callback) :
       mCallback(std::move(callback)) {}
};
}

StalledIssue::StalledIssue(size_t id, const mega::MegaSyncStall &stall) :
    mId(id),
    mMegaStall(stall.copy())
{
}

std::string StalledIssue::getSyncWaitReasonStr() const
{
    assert(mMegaStall);

    switch(mMegaStall->reason())
    {
    #define SOME_GENERATOR_MACRO(reason, str) case reason: return str;
        GENERATE_FROM_STALL_REASON(SOME_GENERATOR_MACRO)
    #undef SOME_GENERATOR_MACRO

        default:                                                     assert(false);
                                                                     return "<unsupported>";
    }
}

std::string StalledIssue::getMainPath() const
{
    assert(mMegaStall);

    const char* cloudPath = mMegaStall->path(true, 0);
    const char* localPath = mMegaStall->path(false, 0);

    if (cloudPath && strcmp(cloudPath, ""))
    {
        // We add a '/' at the start to distinguish between cloud and local absolute paths
        return std::string("/") + cloudPath;
    }

    return localPath ? localPath : "";
}

StalledIssueCache::StalledIssueCache(const StalledIssueList &stalledIssues, std::mutex &stalledIssuesMutex) :
    mStalledIssues(stalledIssues),
    mStalledIssuesLock(stalledIssuesMutex)
{
}

void StalledIssuesManager::populateStalledIssues(const mega::MegaSyncStallList& stalls)
{
    StalledIssueList stalledIssues;

    for (size_t i = 0; i < stalls.size(); ++i)
    {
        auto stall = stalls.get(i);
        assert(stall);

        stalledIssues.emplace_back(i + 1, *stall);
    }

    {
        std::lock_guard<std::mutex> lock(mStalledIssuesMutex);
        mStalledIssues = std::move(stalledIssues);
    }

#ifdef MEGACMD_TESTING_CODE
    TI::Instance().setTestValue(TI::TestValue::STALLED_ISSUES_LIST_SIZE, static_cast<uint64_t>(stalls.size()));
    TI::Instance().fireEvent(TI::Event::STALLED_ISSUES_LIST_UPDATED);
#endif
}

StalledIssuesManager::StalledIssuesManager(mega::MegaApi *api)
{
    assert(api);

    mGlobalListener = std::make_unique<StalledIssuesGlobalListener>(
         [this, api] { api->getMegaSyncStallList(mRequestListener.get()); });

    mRequestListener = std::make_unique<StalledIssuesRequestListener>(
        [this] (const mega::MegaSyncStallList& stalls) { populateStalledIssues(stalls); });
}

StalledIssueCache StalledIssuesManager::getLockedCache()
{
    return StalledIssueCache(mStalledIssues, mStalledIssuesMutex);
}
