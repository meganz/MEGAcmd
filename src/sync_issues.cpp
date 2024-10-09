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
#include <filesystem>

#include "megacmdlogger.h"
#include "configurationmanager.h"
#include "listeners.h"

#ifdef MEGACMD_TESTING_CODE
    #include "../tests/common/Instruments.h"
    using TI = TestInstruments;
#endif

using namespace megacmd;
using namespace std::string_literals;
namespace fs = std::filesystem;

namespace
{
    bool startsWith(const char* str, const char* prefix)
    {
        return std::strncmp(str, prefix, std::strlen(prefix)) == 0;
    }

    std::string getPathProblemReasonStr(int _pathProblem)
    {
        switch(static_cast<mega::PathProblem>(_pathProblem))
        {
        #define SOME_GENERATOR_MACRO(pathProblem, str) case pathProblem: return str;
            GENERATE_FROM_PATH_PROBLEM(SOME_GENERATOR_MACRO)
        #undef SOME_GENERATOR_MACRO

            default:
                assert(false);
                return "<unsupported>";
        }
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

            syncIssues.mIssuesVec.emplace_back(*stall);
        }
        onSyncIssuesChanged(syncIssues);

        {
            std::lock_guard lock(mSyncIssuesMtx);
            mSyncIssues = std::move(syncIssues);
        }
    }

protected:
    virtual void onSyncIssuesChanged(const SyncIssueList& syncIssues) {}

public:
    virtual ~SyncIssuesRequestListener() = default;

    SyncIssueList popSyncIssues() { return std::move(mSyncIssues); }
};

// Whenever a callback happens we start a timer. Any subsequent callbacks after the first
// timer restart it. When the timer reaches a certain threshold, the broadcast is triggered.
// This effectivelly "combines" near callbacks into a single trigger.
class SyncIssuesBroadcastListener : public SyncIssuesRequestListener
{
    using SyncStalledChangedCb = std::function<void(unsigned int syncIssuesSize)>;
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;

    SyncStalledChangedCb mBroadcastSyncIssuesCb;
    unsigned int mSyncIssuesSize;
    bool mRunning;
    TimePoint mLastTriggerTime;

    std::mutex mDebouncerMtx;
    std::condition_variable mDebouncerCv;
    std::thread mDebouncerThread;

    void onSyncIssuesChanged(const SyncIssueList& syncIssues) override
    {
        {
            std::lock_guard lock(mDebouncerMtx);
            mSyncIssuesSize = syncIssues.size();
            mLastTriggerTime = Clock::now();
        }

        mDebouncerCv.notify_one();
    }

    void debouncerLoop()
    {
        while (true)
        {
            std::unique_lock lock(mDebouncerMtx);
            bool stopOrTrigger = mDebouncerCv.wait_for(lock, std::chrono::milliseconds(20), [this]
            {
                constexpr auto minTriggerTime = std::chrono::milliseconds(300);
                return !mRunning || Clock::now() - mLastTriggerTime > minTriggerTime;
            });

            if (!mRunning) // stop
            {
                return;
            }

            if (stopOrTrigger) // trigger
            {
                mBroadcastSyncIssuesCb(mSyncIssuesSize);
                mLastTriggerTime = TimePoint::max();
            }
        }
    }

public:
    template<typename BroadcastSyncIssuesCb>
    SyncIssuesBroadcastListener(BroadcastSyncIssuesCb&& broadcastSyncIssuesCb) :
        mBroadcastSyncIssuesCb(std::move(broadcastSyncIssuesCb)),
        mSyncIssuesSize(0),
        mRunning(true),
        mLastTriggerTime(TimePoint::max()),
        mDebouncerThread([this] { debouncerLoop(); }) {}

    virtual ~SyncIssuesBroadcastListener()
    {
        {
            std::lock_guard lock(mDebouncerMtx);
            mRunning = false;
        }

        mDebouncerCv.notify_one();
        mDebouncerThread.join();
    }
};

SyncIssue::SyncIssue(const mega::MegaSyncStall &stall) :
    mMegaStall(stall.copy())
{
}

const std::string& SyncIssue::getId() const
{
    if (mId.empty())
    {
        std::hash<std::string> hasher;

        std::string combinedDataStr;
        combinedDataStr += std::to_string(mMegaStall->reason());

        for (bool isCloud : {false, true})
        {
            for (int i = 0; i < mMegaStall->pathCount(isCloud); ++i)
            {
                combinedDataStr += mMegaStall->path(isCloud, i);
                combinedDataStr += std::to_string(mMegaStall->pathProblem(isCloud, i));
            }
        }

        // Ensure the id has a size of 11, same as the sync ID
        std::stringstream ss;
        ss << std::setw(11) << std::setfill('0') << hasher(combinedDataStr);
        mId = ss.str();

        assert(mId.size() >= 11);
        mId = mId.substr(0, 11);
    }
    return mId;
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
        return CloudPrefix + cloudPath;
    }

    const char* localPath = mMegaStall->path(false, 0);
    return localPath ? localPath : "";
}

std::vector<SyncIssue::PathProblem> SyncIssue::getPathProblems(mega::MegaApi& api) const
{
    return getPathProblems<false>(api) + getPathProblems<true>(api);
}

template<bool isCloud>
std::vector<SyncIssue::PathProblem> SyncIssue::getPathProblems(mega::MegaApi& api) const
{
    std::vector<SyncIssue::PathProblem> pathProblems;
    for (int i = 0; i < mMegaStall->pathCount(isCloud); ++i)
    {
        PathProblem pathProblem;
        pathProblem.mIsCloud = isCloud;
        pathProblem.mPath = (isCloud ? CloudPrefix : "") + mMegaStall->path(isCloud, i);
        pathProblem.mProblem = getPathProblemReasonStr(mMegaStall->pathProblem(isCloud, i));

        if constexpr (isCloud)
        {
            auto nodeHandle = mMegaStall->cloudNodeHandle(i);

            std::unique_ptr<mega::MegaNode> n(api.getNodeByHandle(nodeHandle));
            if (n)
            {
                pathProblem.mUploadedTime = n->getCreationTime();
                pathProblem.mModifiedTime = n->getModificationTime();
                pathProblem.mFileSize = std::max<int64_t>(n->getSize(), 0);
            }
            else
            {
                LOG_warn << "Path problem " << pathProblem.mPath << " does not have a valid node associated with it";
            }
        }
        else if (fs::exists(pathProblem.mPath))
        {
            auto lastWriteTime = fs::last_write_time(pathProblem.mPath);
            auto systemNow = std::chrono::system_clock::now();
            auto fileTimeNow = fs::file_time_type::clock::now();
            auto lastWriteTimePoint = std::chrono::time_point_cast<std::chrono::seconds>(lastWriteTime - fileTimeNow + systemNow);
            pathProblem.mModifiedTime = lastWriteTimePoint.time_since_epoch().count();

            try
            {
                // Size for non-files (dirs, symlinks, etc.) is implementation-defined
                // In some cases, an exception might be thrown
                pathProblem.mFileSize = fs::file_size(pathProblem.mPath);
            }
            catch (...) {}
        }

        pathProblems.emplace_back(std::move(pathProblem));
    }
    return pathProblems;
}

std::unique_ptr<mega::MegaSync> SyncIssue::getParentSync(mega::MegaApi& api) const
{
    auto syncList = std::unique_ptr<mega::MegaSyncList>(api.getSyncs());
    assert(syncList);

    for (int i = 0; i < syncList->size(); ++i)
    {
        mega::MegaSync* sync = syncList->get(i);
        assert(sync);

        if (belongsToSync(*sync))
        {
            return std::unique_ptr<mega::MegaSync>(sync->copy());
        }
    }

    return nullptr;
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

SyncIssue const* SyncIssueList::getSyncIssue(const std::string& id) const
{
    for (const auto& syncIssue : *this)
    {
        if (syncIssue.getId() == id)
        {
            return &syncIssue;
        }
    }
    return nullptr;
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

void SyncIssuesManager::onSyncIssuesChanged(unsigned int newSyncIssuesSize)
{
    if (mWarningEnabled && newSyncIssuesSize > 0)
    {
        std::string message = "Sync issues detected: your syncs have encountered conflicts that may require your intervention.\n"s +
                            "Use the \"%%mega-%%sync-issues\" command to display them.\n" +
                            "This message can be disabled with \"%%mega-%%sync-issues --disable-warning\".";
        broadcastMessage(message, true);
    }

#ifdef MEGACMD_TESTING_CODE
        TI::Instance().setTestValue(TI::TestValue::SYNC_ISSUES_LIST_SIZE, static_cast<uint64_t>(newSyncIssuesSize));
        TI::Instance().fireEvent(TI::Event::SYNC_ISSUES_LIST_UPDATED);
#endif
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
        [this] (unsigned int newSyncIssuesSize) { onSyncIssuesChanged(newSyncIssuesSize); });

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
