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

#define GENERATE_FROM_STALL_REASON(GENERATOR_MACRO) \
        GENERATOR_MACRO(mega::MegaSyncStall::NoReason,                                                  "No reason") \
        GENERATOR_MACRO(mega::MegaSyncStall::FileIssue,                                                 "File issue") \
        GENERATOR_MACRO(mega::MegaSyncStall::MoveOrRenameCannotOccur,                                   "Move/Rename cannot occur") \
        GENERATOR_MACRO(mega::MegaSyncStall::DeleteOrMoveWaitingOnScanning,                             "Delete waiting on scanning") \
        GENERATOR_MACRO(mega::MegaSyncStall::DeleteWaitingOnMoves,                                      "Delete waiting on move") \
        GENERATOR_MACRO(mega::MegaSyncStall::UploadIssue,                                               "Upload issue") \
        GENERATOR_MACRO(mega::MegaSyncStall::DownloadIssue,                                             "Download issue") \
        GENERATOR_MACRO(mega::MegaSyncStall::CannotCreateFolder,                                        "Cannot create folder") \
        GENERATOR_MACRO(mega::MegaSyncStall::CannotPerformDeletion,                                     "Cannot delete") \
        GENERATOR_MACRO(mega::MegaSyncStall::SyncItemExceedsSupportedTreeDepth,                         "Supported tree depth exceeded") \
        GENERATOR_MACRO(mega::MegaSyncStall::FolderMatchedAgainstFile,                                  "Folder matched against file") \
        GENERATOR_MACRO(mega::MegaSyncStall::LocalAndRemoteChangedSinceLastSyncedState_userMustChoose,  "Local and remote differ") \
        GENERATOR_MACRO(mega::MegaSyncStall::LocalAndRemotePreviouslyUnsyncedDiffer_userMustChoose,     "Local and remote differ") \
        GENERATOR_MACRO(mega::MegaSyncStall::NamesWouldClashWhenSynced,                                 "Name clash")

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
