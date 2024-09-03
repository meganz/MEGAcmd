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

#include "sync_issues.h"

#include <cassert>
#include <functional>

#include "megacmdlogger.h"
#include "configurationmanager.h"

#ifdef MEGACMD_TESTING_CODE
    #include "../tests/common/Instruments.h"
    using TI = TestInstruments;
#endif

using namespace megacmd;
using namespace std::string_literals;

namespace {

class SyncIssuesGlobalListener : public mega::MegaGlobalListener
{
    using SyncStalledChangedCb = std::function<void()>;
    SyncStalledChangedCb mSyncStalledChangedCb;

    bool mSyncStalled;

    void onGlobalSyncStateChanged(mega::MegaApi* api) override
    {
        const bool scanning = api->isScanning();
        const bool waiting = api->isWaiting();
        if (scanning || waiting)
        {
            // Busy state (scanning or waiting); ignore
            return;
        }

        const bool syncStalled = api->isSyncStalled();
        const bool syncStalledChanged = (syncStalled != mSyncStalled || (syncStalled && api->isSyncStalledChanged()));
        if (!syncStalledChanged)
        {
            return;
        }

        mSyncStalledChangedCb();
        mSyncStalled = syncStalled;
    }

public:
    template<typename SyncStalledChangedCb>
    SyncIssuesGlobalListener(SyncStalledChangedCb&& syncStalledChangedCb) :
        mSyncStalledChangedCb(std::move(syncStalledChangedCb)),
        mSyncStalled(false) {}
};

class SyncIssuesRequestListener : public mega::MegaRequestListener
{
    using PopulateSyncIssueListCb = std::function<void(const mega::MegaSyncStallList& stalls)>;
    PopulateSyncIssueListCb mPopulateSyncIssueListCb;

    void onRequestFinish(mega::MegaApi *api, mega::MegaRequest *request, mega::MegaError *e) override
    {
        assert(e);
        if (e->getValue() != mega::MegaError::API_OK)
        {
            LOG_err << "Failed to get list of sync issues: " << e->getErrorString();
            return;
        }

        assert(request && request->getType() == mega::MegaRequest::TYPE_GET_SYNC_STALL_LIST);

        auto stalls = request->getMegaSyncStallList();
        if (!stalls)
        {
            LOG_err << "Sync issue list pointer is null";
            return;
        }

        mPopulateSyncIssueListCb(*stalls);
    }

public:
    template<typename PopulateSyncIssueListCb>
    SyncIssuesRequestListener(PopulateSyncIssueListCb&& populateSyncIssueListCb) :
        mPopulateSyncIssueListCb(std::move(populateSyncIssueListCb)) {}
};
}

SyncIssue::SyncIssue(size_t id, const mega::MegaSyncStall &stall) :
    mId(id),
    mMegaStall(stall.copy())
{
}

std::string SyncIssue::getSyncWaitReasonStr() const
{
    assert(mMegaStall);

    switch(mMegaStall->reason())
    {
    #define SOME_GENERATOR_MACRO(reason, str) case reason: return str;
        GENERATE_FROM_STALL_REASON(SOME_GENERATOR_MACRO)
    #undef SOME_GENERATOR_MACRO

        default:
            assert(false);
            return "<unsupported>";
    }
}

std::string SyncIssue::getMainPath() const
{
    assert(mMegaStall);

    const char* cloudPath = mMegaStall->path(true, 0);
    if (cloudPath && strcmp(cloudPath, ""))
    {
        // We add a '/' at the start to distinguish between cloud and local absolute paths
        return std::string("/") + cloudPath;
    }

    const char* localPath = mMegaStall->path(false, 0);
    return localPath ? localPath : "";
}

SyncIssueCache::SyncIssueCache(const SyncIssueList &syncIssues, std::mutex &syncIssuesMutex) :
    mSyncIssues(syncIssues),
    mSyncIssuesLock(syncIssuesMutex)
{
}

void SyncIssuesManager::populateSyncIssues(const mega::MegaSyncStallList& stalls)
{
    SyncIssueList syncIssues;

    for (size_t i = 0; i < stalls.size(); ++i)
    {
        auto stall = stalls.get(i);
        assert(stall);

        syncIssues.emplace_back(i + 1, *stall);
    }

    {
        std::lock_guard lock(mSyncIssuesMutex);

        if (mWarningEnabled && mSyncIssues.empty())
        {
            assert(!syncIssues.empty());
            broadcastWarning();
        }
        mSyncIssues = std::move(syncIssues);
    }

#ifdef MEGACMD_TESTING_CODE
    TI::Instance().setTestValue(TI::TestValue::SYNC_ISSUES_LIST_SIZE, static_cast<uint64_t>(stalls.size()));
    TI::Instance().fireEvent(TI::Event::SYNC_ISSUES_LIST_UPDATED);
#endif
}

void SyncIssuesManager::broadcastWarning()
{
    std::string message = "Sync issues detected: your syncs have encountered conflicts that may require your intervention.\n" +
                          "Use the \""s + commandPrefixBasedOnMode() + "sync-issues\" command to display them.\n" +
                          "This message can be disabled with \"" + commandPrefixBasedOnMode() + "sync-issues --disable-warning\".";
    broadcastMessage(message, true);
}

SyncIssuesManager::SyncIssuesManager(mega::MegaApi *api)
{
    assert(api);

    // The global listener will be triggered whenever there's a change in the sync state
    // It'll check whether or not this change requires an update in the sync issue list
    // If that's the case, it'll request the latest list of stalls from the API
    mGlobalListener = std::make_unique<SyncIssuesGlobalListener>(
         [this, api] { api->getMegaSyncStallList(mRequestListener.get()); });

    // The request listener will be triggered whenever the api call above (to get the
    // list of stalls) finishes; it will populate the internal list with its data
    mRequestListener = std::make_unique<SyncIssuesRequestListener>(
        [this] (const mega::MegaSyncStallList& stalls) { populateSyncIssues(stalls); });

    mWarningEnabled = ConfigurationManager::getConfigurationValue("stalled_issues_warning", true);
}

SyncIssueCache SyncIssuesManager::getLockedCache()
{
    return SyncIssueCache(mSyncIssues, mSyncIssuesMutex);
}

bool SyncIssuesManager::hasSyncIssues() const
{
    std::lock_guard lock(mSyncIssuesMutex);
    return !mSyncIssues.empty();
}

void SyncIssuesManager::disableWarning()
{
    if (!mWarningEnabled)
    {
        OUTSTREAM << "Note: warning is already disabled" << endl;
    }

    mWarningEnabled = false;
    ConfigurationManager::savePropertyValue("stalled_issues_warning", mWarningEnabled);
}

void SyncIssuesManager::enableWarning()
{
    if (mWarningEnabled)
    {
        OUTSTREAM << "Note: warning is already enabled" << endl;
    }

    mWarningEnabled = true;
    ConfigurationManager::savePropertyValue("stalled_issues_warning", mWarningEnabled);
}
