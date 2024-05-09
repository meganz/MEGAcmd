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

#include "megacmdlogger.h"

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
        case mega::MegaSyncStall::NoReason:                          return "No reason";
        case mega::MegaSyncStall::FileIssue:                         return "File issue";
        case mega::MegaSyncStall::MoveOrRenameCannotOccur:           return "Move/Rename cannot occur";

        case mega::MegaSyncStall::DeleteOrMoveWaitingOnScanning:     return "Delete waiting on scanning";
        case mega::MegaSyncStall::DeleteWaitingOnMoves:              return "Delete waiting on move";

        case mega::MegaSyncStall::UploadIssue:                       return "Upload issue";
        case mega::MegaSyncStall::DownloadIssue:                     return "Download issue";
        case mega::MegaSyncStall::CannotCreateFolder:                return "Cannot create folder";
        case mega::MegaSyncStall::CannotPerformDeletion:             return "Cannot delete";
        case mega::MegaSyncStall::SyncItemExceedsSupportedTreeDepth: return "Supported tree depth exceeded";
        case mega::MegaSyncStall::FolderMatchedAgainstFile:          return "Folder matched against file";

        case mega::MegaSyncStall::LocalAndRemoteChangedSinceLastSyncedState_userMustChoose:
        case mega::MegaSyncStall::LocalAndRemotePreviouslyUnsyncedDiffer_userMustChoose:
                                                                     return "Local and remote differ";

        case mega::MegaSyncStall::NamesWouldClashWhenSynced:         return "Name clash";

        default:                                                     return "<out-of-range>";
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

void StalledIssuesManager::onGlobalSyncStateChanged(mega::MegaApi *api)
{
    bool syncStalled = api->isSyncStalled();
    bool syncStalledChanged = (mSyncStalled != syncStalled || api->isSyncStalledChanged());

    if (syncStalledChanged)
    {
        mMegaApi->getMegaSyncStallList(this);
    }

    mSyncStalled = syncStalled;
}

void StalledIssuesManager::onRequestFinish(mega::MegaApi*, mega::MegaRequest *request, mega::MegaError*)
{
    assert(request && request->getType() == mega::MegaRequest::TYPE_GET_SYNC_STALL_LIST);

    auto stalls = request->getMegaSyncStallList();
    if (!stalls)
    {
        LOG_err << "Sync stall list pointer is null";
        return;
    }

    populateStalledIssues(*stalls);
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

    std::lock_guard<std::mutex> lock(mStalledIssuesMutex);
    mStalledIssues = std::move(stalledIssues);
}

StalledIssuesManager::StalledIssuesManager(mega::MegaApi *api) :
    mMegaApi(api),
    mSyncStalled(false)
{
    assert(mMegaApi);
}

StalledIssueCache StalledIssuesManager::getLockedCache()
{
    return StalledIssueCache(mStalledIssues, mStalledIssuesMutex);
}
