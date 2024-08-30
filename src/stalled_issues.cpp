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

    bool mSyncStalled;

    void onGlobalSyncStateChanged(mega::MegaApi* api) override
    {
        const bool scanning = api->isScanning();
        const bool waiting = api->isWaiting();
        if (scanning || waiting)
        {
            return;
        }

        const bool syncStalled = api->isSyncStalled();
        const bool syncStalledChanged = (syncStalled != mSyncStalled || (syncStalled && api->isSyncStalledChanged()));
        if (!syncStalledChanged)
        {
            return;
        }

        mCallback();
        mSyncStalled = syncStalled;
    }

public:
    template<typename Cb>
    StalledIssuesGlobalListener(Cb&& callback) :
        mCallback(std::move(callback)),
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
    template<typename Cb>
    StalledIssuesRequestListener(Cb&& callback) :
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
