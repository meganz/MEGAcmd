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
#include "listeners.h"

#ifdef MEGACMD_TESTING_CODE
    #include "../tests/common/Instruments.h"
    using TI = TestInstruments;
#endif

using namespace megacmd;
using namespace std::string_literals;

namespace
{
    bool startsWith(const char* str, const char* prefix)
    {
        return std::strncmp(str, prefix, std::strlen(prefix)) == 0;
    }
}

class SyncIssuesGlobalListener : public mega::MegaGlobalListener
{
    using SyncStalledChangedCb = std::function<void()>;
    SyncStalledChangedCb mSyncStalledChangedCb;

    bool mSyncStalled;

    void onGlobalSyncStateChanged(mega::MegaApi* api) override
    {
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

class SyncIssuesRequestListener : public mega::SynchronousRequestListener
{
    std::mutex mSyncIssuesMtx;
    SyncIssueList mSyncIssues;

    void doOnRequestFinish(mega::MegaApi *api, mega::MegaRequest *request, mega::MegaError *e) override
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

        SyncIssueList syncIssues;
        for (size_t i = 0; i < stalls->size(); ++i)
        {
            auto stall = stalls->get(i);
            assert(stall);

            syncIssues.mIssuesVec.emplace_back(i + 1, *stall);
        }
        onSyncIssuesChanged(syncIssues);

        {
            std::lock_guard lock(mSyncIssuesMtx);
            mSyncIssues = std::move(syncIssues);
        }
    }

protected:
    virtual void onSyncIssuesChanged(const SyncIssueList& syncIssues) { }

public:
    virtual ~SyncIssuesRequestListener() = default;

    SyncIssueList popSyncIssues() { return std::move(mSyncIssues); }
};

class SyncIssuesBroadcastListener : public SyncIssuesRequestListener
{
    using SyncStalledChangedCb = std::function<void()>;
    SyncStalledChangedCb mBroadcastSyncIssuesCb;

    void onSyncIssuesChanged(const SyncIssueList& syncIssues) override
    {
        if (!syncIssues.empty())
        {
            mBroadcastSyncIssuesCb();
        }

#ifdef MEGACMD_TESTING_CODE
        TI::Instance().setTestValue(TI::TestValue::SYNC_ISSUES_LIST_SIZE, static_cast<uint64_t>(syncIssues.size()));
        TI::Instance().fireEvent(TI::Event::SYNC_ISSUES_LIST_UPDATED);
#endif
    }

public:
    template<typename BroadcastSyncIssuesCb>
    SyncIssuesBroadcastListener(BroadcastSyncIssuesCb&& broadcastSyncIssuesCb) :
        mBroadcastSyncIssuesCb(std::move(broadcastSyncIssuesCb)) {}

    virtual ~SyncIssuesBroadcastListener() = default;
};

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

bool SyncIssue::belongsToSync(const mega::MegaSync& sync) const
{
    const char* issueCloudPath = mMegaStall->path(true, 0);
    const char* syncCloudPath = sync.getLastKnownMegaFolder();
    assert(syncCloudPath);
    if (startsWith(issueCloudPath, syncCloudPath))
    {
        return true;
    }

    const char* issueLocalPath = mMegaStall->path(false, 0);
    const char* syncLocalPath = sync.getLocalFolder();
    assert(syncLocalPath);
    if (startsWith(issueLocalPath, syncLocalPath))
    {
        return true;
    }

    return false;
}

unsigned int SyncIssueList::getSyncIssueCount(const mega::MegaSync& sync) const
{
    unsigned int count = 0;
    for (const auto& syncIssue : *this)
    {
        if (syncIssue.belongsToSync(sync))
        {
            ++count;
        }
    }
    return count;
}

void SyncIssuesManager::broadcastWarning()
{
    if (!mWarningEnabled)
    {
        return;
    }

    std::string message = "Sync issues detected: your syncs have encountered conflicts that may require your intervention.\n" +
                          "Use the \""s + commandPrefixBasedOnMode() + "sync-issues\" command to display them.\n" +
                          "This message can be disabled with \"" + commandPrefixBasedOnMode() + "sync-issues --disable-warning\".";
    broadcastMessage(message, true);
}

SyncIssuesManager::SyncIssuesManager(mega::MegaApi *api) :
    mApi(*api)
{
    // The global listener will be triggered whenever there's a change in the sync state
    // It'll request the sync issue list from the API (if the stalled state changed)
    // This will be used to notify the user if they have sync issues
    mGlobalListener = std::make_unique<SyncIssuesGlobalListener>(
         [this, api] { api->getMegaSyncStallList(mRequestListener.get()); });

    // The broadcast listener will be triggered whenever the api call above finishes
    // getting the list of stalls; it'll be used to notify the user (and the integration tests)
    mRequestListener = std::make_unique<SyncIssuesBroadcastListener>(
        [this] { broadcastWarning(); });

    mWarningEnabled = ConfigurationManager::getConfigurationValue("stalled_issues_warning", true);
}

SyncIssueList SyncIssuesManager::getSyncIssues() const
{
    auto listener = std::make_unique<SyncIssuesRequestListener>();
    mApi.getMegaSyncStallList(listener.get());
    listener->wait();

    return listener->popSyncIssues();
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
