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
#include "megacmdutils.h"

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

            syncIssues.mIssuesMap.emplace(static_cast<uint32_t>(stall->getHash()), *stall);
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
bool SyncIssue::hasPathProblem(mega::PathProblem pathProblem) const
{
    for (int i = 0; i < mMegaStall->pathCount(isCloud); ++i)
    {
        if (mMegaStall->pathProblem(isCloud, i) == static_cast<int>(pathProblem))
        {
            return true;
        }
    }
    return false;
}

std::string SyncIssue::PathProblem::getProblemStr() const
{
    switch(mProblem)
    {
    #define SOME_GENERATOR_MACRO(pathProblem, str) case pathProblem: return str;
        GENERATE_FROM_PATH_PROBLEM(SOME_GENERATOR_MACRO)
    #undef SOME_GENERATOR_MACRO

        default:
            assert(false);
            return "<unsupported>";
    }
}

SyncIssue::SyncIssue(const mega::MegaSyncStall &stall) :
    mMegaStall(stall.copy())
{
}

unsigned int SyncIssue::getId() const
{
    assert(mMegaStall);
    return static_cast<uint32_t>(mMegaStall->getHash());
}

SyncInfo SyncIssue::getSyncInfo(mega::MegaSync const* parentSync) const
{
    assert(mMegaStall);

    SyncInfo info;
    info.mReasonType = static_cast<mega::SyncWaitReason>(mMegaStall->reason());

    switch(info.mReasonType)
    {
        case mega::SyncWaitReason::NoReason:
        {
            info.mReason = "Error detected with '" + getFileName() + "'";
            info.mDescription = "Reason not found";
            break;
        }
        case mega::SyncWaitReason::FileIssue:
        {
            if (hasPathProblem<false>(mega::PathProblem::DetectedSymlink))
            {
                info.mReason = "Detected symlink: '" + getFileName() + "'";
            }
            else if (hasPathProblem<false>(mega::PathProblem::DetectedHardLink))
            {
                info.mReason = "Detected hard link: '" + getFileName() + "'";
            }
            else if (hasPathProblem<false>(mega::PathProblem::DetectedSpecialFile))
            {
                info.mReason = "Detected special link: '" + getFileName() + "'";
            }
            else
            {
                info.mReason = "Can't sync '" + getFileName() + "'";
            }
            break;
        }
        case mega::SyncWaitReason::MoveOrRenameCannotOccur:
        {
            std::string syncName = (parentSync ? "'"s + parentSync->getName() + "'" : "the sync");
            info.mReason = "Can't move or rename some items in " + syncName;
            info.mDescription = "The local and remote locations have changed at the same time";
            break;
        }
        case mega::SyncWaitReason::DeleteOrMoveWaitingOnScanning:
        {
            info.mReason = "Can't find '" + getFileName() + "'";
            info.mDescription = "Waiting to finish scan to see if the file was moved or deleted";
            break;
        }
        case mega::SyncWaitReason::DeleteWaitingOnMoves:
        {
            info.mReason = "Waiting to move '" + getFileName() + "'";
            info.mDescription = "Waiting for other processes to complete";
            break;
        }
        case mega::SyncWaitReason::UploadIssue:
        {
            info.mReason = "Can't upload '" + getFileName() + "' to the selected location";
            info.mDescription = "Cannot reach the destination folder";
            break;
        }
        case mega::SyncWaitReason::DownloadIssue:
        {
            info.mReason = "Can't download '" + getFileName<true>() + "' to the selected location";

            if (hasPathProblem<true>(mega::PathProblem::CloudNodeInvalidFingerprint))
            {
                info.mDescription = "File fingerprint missing";
            }
            else if (hasPathProblem<true>(mega::PathProblem::CloudNodeIsBlocked))
            {
                info.mDescription = "The file '" + getFileName() + "' is unavailable because it was reported to contain content in breach of MEGA's Terms of Service (https://mega.io/terms)";
            }
            else
            {
                info.mDescription = "A failure occurred either downloading the file, or moving the downloaded temporary file to its final name and location";
            }
            break;
        }
        case mega::SyncWaitReason::CannotCreateFolder:
        {
            info.mReason = "Cannot create '" + getFileName() + "'";
            info.mDescription = "Filesystem error preventing folder access";
            break;
        }
        case mega::SyncWaitReason::CannotPerformDeletion:
        {
            info.mReason = "Cannot perform deletion '" + getFileName() + "'";
            info.mDescription = "Filesystem error preventing folder access";
            break;
        }
        case mega::SyncWaitReason::SyncItemExceedsSupportedTreeDepth:
        {
            info.mReason = "Unable to sync '" + getFileName() + "'";
            info.mDescription = "Target is too deep on your folder structure; please move it to a location that is less than 64 folders deep";
            break;
        }
        case mega::SyncWaitReason::FolderMatchedAgainstFile:
        {
            info.mReason = "Unable to sync '" + getFileName() + "'";
            info.mDescription = "Cannot sync folders against files";
            break;
        }
        case mega::SyncWaitReason::LocalAndRemoteChangedSinceLastSyncedState_userMustChoose:
        case mega::SyncWaitReason::LocalAndRemotePreviouslyUnsyncedDiffer_userMustChoose:
        {
            info.mReason = "Unable to sync '" + getFileName() + "'";
            info.mDescription = "This file has conflicting copies";
            break;
        }
        case mega::SyncWaitReason::NamesWouldClashWhenSynced:
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
        pathProblem.mProblem = static_cast<mega::PathProblem>(mMegaStall->pathProblem(isCloud, i));

        if constexpr (isCloud)
        {
            auto nodeHandle = mMegaStall->cloudNodeHandle(i);

            std::unique_ptr<mega::MegaNode> n(api.getNodeByHandle(nodeHandle));
            if (n)
            {
                if (n->isFile())
                {
                    pathProblem.mPathType = "File";
                }
                else if (n->isFolder())
                {
                    pathProblem.mPathType = "Folder";
                }

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
            if (fs::is_directory(pathProblem.mPath))
            {
                pathProblem.mPathType = "Folder";
            }
            else if (fs::is_regular_file(pathProblem.mPath))
            {
                pathProblem.mPathType = "File";
            }
            else if (fs::is_symlink(pathProblem.mPath))
            {
                pathProblem.mPathType = "Symlink";
            }

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

SyncIssue const* SyncIssueList::getSyncIssue(unsigned int id) const
{
    auto it = mIssuesMap.find(id);
    if (it == mIssuesMap.end())
    {
        return nullptr;
    }
    return &it->second;
}

unsigned int SyncIssueList::getSyncIssuesCount(const mega::MegaSync& sync) const
{
    unsigned int count = 0;
    for (const auto& syncIssue : *this)
    {
        if (syncIssue.second.belongsToSync(sync))
        {
            ++count;
        }
    }
    return count;
}

void SyncIssuesManager::onSyncIssuesChanged(unsigned int newSyncIssuesSize)
{
    std::lock_guard lock(mWarningMtx);
    if (mWarningEnabled && newSyncIssuesSize > 0)
    {
        std::string message = "Sync issues detected: your syncs have encountered conflicts that may require your intervention.\n"s +
                              "Use the \"%mega-%sync-issues\" command to display them.\n" +
                              "This message can be disabled with \"%mega-%sync-issues --disable-warning\".";
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
    std::lock_guard lock(mWarningMtx);
    if (!mWarningEnabled)
    {
        OUTSTREAM << "Note: warning is already disabled" << endl;
    }

    mWarningEnabled = false;
    ConfigurationManager::savePropertyValue("stalled_issues_warning", mWarningEnabled);
}

void SyncIssuesManager::enableWarning()
{
    std::lock_guard lock(mWarningMtx);
    if (mWarningEnabled)
    {
        OUTSTREAM << "Note: warning is already enabled" << endl;
    }

    mWarningEnabled = true;
    ConfigurationManager::savePropertyValue("stalled_issues_warning", mWarningEnabled);
}

namespace SyncIssuesCommand
{
    void printAllIssues(mega::MegaApi& api, ColumnDisplayer& cd, const SyncIssueList& syncIssues, bool disablePathCollapse, int rowCountLimit)
    {
        cd.addHeader("PARENT SYNC", disablePathCollapse);

        syncIssues.forEach([&api, &cd] (const SyncIssue& syncIssue)
        {
            auto parentSync = syncIssue.getParentSync(api);

            cd.addValue("ISSUE_ID", std::to_string(syncIssue.getId()));
            cd.addValue("PARENT_SYNC", parentSync ? parentSync->getName() : "<not found>");
            cd.addValue("REASON", syncIssue.getSyncInfo(parentSync.get()).mReason);
            cd.addValue("SOLVABLE", "NO" /* Until CMD-311 */);
        }, rowCountLimit);

        OUTSTREAM << cd.str();
        OUTSTREAM << endl;
        if (rowCountLimit < syncIssues.size())
        {
            OUTSTREAM << "Note: showing " << rowCountLimit << " out of " << syncIssues.size() << " issues. "
                      << "Use \"" << commandPrefixBasedOnMode() << "sync-issues --limit=0\" to see all of them." << endl;
        }
        OUTSTREAM << "Use \"" << commandPrefixBasedOnMode() << "sync-issues --detail <ISSUE ID>\" to get further details on a specific issue." << endl;
    }

    void printSingleIssueDetail(mega::MegaApi& api, megacmd::ColumnDisplayer& cd, const SyncIssue& syncIssue, bool disablePathCollapse, int rowCountLimit)
    {
        auto parentSync = syncIssue.getParentSync(api);
        auto syncInfo = syncIssue.getSyncInfo(parentSync.get());

        OUTSTREAM << syncInfo.mReason;
        if (!syncInfo.mDescription.empty())
        {
            OUTSTREAM << ". " << syncInfo.mDescription << ".";
        }
        OUTSTREAM << endl;

        OUTSTREAM << "Parent sync: ";
        if (parentSync)
        {
            OUTSTREAM << syncBackupIdToBase64(parentSync->getBackupId())
                      << " (" << parentSync->getLocalFolder() << " to " << SyncIssue::CloudPrefix << parentSync->getLastKnownMegaFolder() << ")";
        }
        else
        {
            OUTSTREAM << "<not found>";
        }
        OUTSTREAM << endl << endl;

        cd.addHeader("PATH", disablePathCollapse);

        mega::SyncWaitReason syncIssueReason = syncIssue.getSyncInfo(parentSync.get()).mReasonType;

        auto pathProblems = syncIssue.getPathProblems(api);
        for (int i = 0; i < pathProblems.size() && i < rowCountLimit; ++i)
        {
            const auto& pathProblem = pathProblems[i];
            const char* timeFmt = "%Y-%m-%d %H:%M:%S";

            cd.addValue("PATH", pathProblem.mPath);

            if (syncIssueReason == mega::SyncWaitReason::FolderMatchedAgainstFile)
            {
                cd.addValue("TYPE", pathProblem.mPathType);
            }
            else
            {
                cd.addValue("PATH ISSUE", pathProblem.getProblemStr());
            }

            cd.addValue("LAST MODIFIED", pathProblem.mModifiedTime ? getReadableTime(pathProblem.mModifiedTime, timeFmt) : "-");
            cd.addValue("UPLOADED", pathProblem.mUploadedTime ? getReadableTime(pathProblem.mUploadedTime, timeFmt) : "-");
            cd.addValue("SIZE", sizeToText(pathProblem.mFileSize));
        }

        OUTSTREAM << cd.str();
        if (rowCountLimit < pathProblems.size())
        {
            OUTSTREAM << endl;
            OUTSTREAM << "Note: showing " << rowCountLimit << " out of " << static_cast<unsigned int>(pathProblems.size()) << " path problems. "
                      << "Use \"" << commandPrefixBasedOnMode() << "sync-issues --detail " << syncIssue.getId() << " --limit=0\" to see all of them." << endl;
        }
    }

    void printAllIssuesDetail(mega::MegaApi& api, megacmd::ColumnDisplayer& cd, const SyncIssueList& syncIssues, bool disablePathCollapse, int rowCountLimit)
    {
        // We'll use the row count limit only for the path problems; since the user has added "--all",
        // we assume they want to see all issues, and thus the limit doesn't apply to the issue list in this case
        for (const auto& syncIssue : syncIssues)
        {
            assert(syncIssue.first == syncIssue.second.getId());
            OUTSTREAM << "[Details on issue " << syncIssue.first << "]" << endl;

            printSingleIssueDetail(api, cd, syncIssue.second, disablePathCollapse, rowCountLimit);
            OUTSTREAM << endl;

            cd.clear();
        }
    }
}
