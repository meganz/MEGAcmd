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
        if (!str || !prefix)
        {
            return false;
        }
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

template<bool preferCloud>
std::string SyncIssue::getFilePath() const
{
    assert(mMegaStall);
    const char* cloudPath = mMegaStall->path(true, 0);
    const char* localPath = mMegaStall->path(false, 0);

    if constexpr (preferCloud)
    {
        if (cloudPath && strcmp(cloudPath, ""))
        {
            return CloudPrefix + cloudPath;
        }
        else if (localPath && strcmp(localPath, ""))
        {
            return localPath;
        }
    }
    else
    {
        if (localPath && strcmp(localPath, ""))
        {
            return localPath;
        }
        else if (cloudPath && strcmp(cloudPath, ""))
        {
            return CloudPrefix + cloudPath;
        }
    }
    return "";
}

template<bool preferCloud>
std::string SyncIssue::getFileName() const
{
    std::string filePath = getFilePath<preferCloud>();
    auto pos = std::find_if(filePath.rbegin(), filePath.rend(), [] (char c)
    {
        return c == '/' || c == '\\';
    });

    if (pos == filePath.rend())
    {
        return "";
    }
    return std::string(pos.base(), filePath.end());
}

template<bool isCloud>
bool SyncIssue::hasPathProblem(mega::MegaSyncStall::SyncPathProblem pathProblem) const
{
    for (int i = 0; i < mMegaStall->pathCount(isCloud); ++i)
    {
        if (mMegaStall->pathProblem(isCloud, i) == pathProblem)
        {
            return true;
        }
    }
    return false;
}

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

        for (bool isLocal : {true, false})
        {
            for (int i = 0; i < mMegaStall->pathCount(isLocal); ++i)
            {
                combinedDataStr += mMegaStall->path(isLocal, i);
                combinedDataStr += std::to_string(mMegaStall->pathProblem(isLocal, i));
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

SyncInfo SyncIssue::getSyncInfo(mega::MegaSync const* parentSync) const
{
    assert(mMegaStall);

    SyncInfo info;
    switch(mMegaStall->reason())
    {
        case mega::MegaSyncStall::NoReason:
        {
            info.mReason = "Error detected with '" + getFileName() + "'";
            info.mDescription = "Reason not found";
            break;
        }
        case mega::MegaSyncStall::FileIssue:
        {
            if (hasPathProblem<false>(mega::MegaSyncStall::DetectedSymlink))
            {
                info.mReason = "Detected symlink: '" + getFileName() + "'";
            }
            else if (hasPathProblem<false>(mega::MegaSyncStall::DetectedHardLink))
            {
                info.mReason = "Detected hard link: '" + getFileName() + "'";
            }
            else if (hasPathProblem<false>(mega::MegaSyncStall::DetectedSpecialFile))
            {
                info.mReason = "Detected special link: '" + getFileName() + "'";
            }
            else
            {
                info.mReason = "Can't sync '" + getFileName() + "'";
            }
            break;
        }
        case mega::MegaSyncStall::MoveOrRenameCannotOccur:
        {
            std::string syncName = (parentSync ? "'"s + parentSync->getName() + "'" : "the sync");
            info.mReason = "Can't move or rename some items in " + syncName;
            info.mDescription = "The local and remote locations have changed at the same time";
            break;
        }
        case mega::MegaSyncStall::DeleteOrMoveWaitingOnScanning:
        {
            info.mReason = "Can't find '" + getFileName() + "'";
            info.mDescription = "Waiting to finish scan to see if the file was moved or deleted";
            break;
        }
        case mega::MegaSyncStall::DeleteWaitingOnMoves:
        {
            info.mReason = "Waiting to move '" + getFileName() + "'";
            info.mDescription = "Waiting for other processes to complete";
            break;
        }
        case mega::MegaSyncStall::UploadIssue:
        {
            info.mReason = "Can't upload '" + getFileName() + "' to the selected location";
            info.mDescription = "Cannot reach the destination folder";
            break;
        }
        case mega::MegaSyncStall::DownloadIssue:
        {
            info.mReason = "Can't download '" + getFileName<true>() + "' to the selected location";

            if (hasPathProblem<true>(mega::MegaSyncStall::CloudNodeInvalidFingerprint))
            {
                info.mDescription = "File fingerprint missing";
            }
            else if (hasPathProblem<true>(mega::MegaSyncStall::CloudNodeIsBlocked))
            {
                info.mDescription = "The file '" + getFileName() + "' is unavailable because it was reported to contain content in breach of MEGA's Terms of Service (https://mega.io/terms)";
            }
            else
            {
                info.mDescription = "A failure occurred either downloading the file, or moving the downloaded temporary file to its final name and location";
            }
            break;
        }
        case mega::MegaSyncStall::CannotCreateFolder:
        {
            info.mReason = "Cannot create '" + getFileName() + "'";
            info.mDescription = "Filesystem error preventing folder access";
            break;
        }
        case mega::MegaSyncStall::CannotPerformDeletion:
        {
            info.mReason = "Cannot perform deletion '" + getFileName() + "'";
            info.mDescription = "Filesystem error preventing folder access";
            break;
        }
        case mega::MegaSyncStall::SyncItemExceedsSupportedTreeDepth:
        {
            info.mReason = "Unable to sync '" + getFileName() + "'";
            info.mDescription = "Target is too deep on your folder structure; please move it to a location that is less than 64 folders deep";
            break;
        }
        case mega::MegaSyncStall::FolderMatchedAgainstFile:
        {
            info.mReason = "Unable to sync '" + getFileName() + "'";
            info.mDescription = "Cannot sync folders against files";
            break;
        }
        case mega::MegaSyncStall::LocalAndRemoteChangedSinceLastSyncedState_userMustChoose:
        case mega::MegaSyncStall::LocalAndRemotePreviouslyUnsyncedDiffer_userMustChoose:
        {
            info.mReason = "Unable to sync '" + getFileName() + "'";
            info.mDescription = "This file has conflicting copies";
            break;
        }
        case mega::MegaSyncStall::NamesWouldClashWhenSynced:
        {
            info.mReason = "Name conflicts: '" + getFileName<true>() + "'";
            info.mDescription = "These items contain multiple names on one side that would all become the same single name on the other side (this may be due to syncing to case insensitive local filesystems, or the effects of escaped characters)";
            break;
        }
        default:
        {
            assert(false);
            info.mReason = "<unsupported>";
            break;
        }
    }
    return info;
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

    // This is a valid result that must be taken into account
    // E.g., the sync was removed by another client while this issue is being handled
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
