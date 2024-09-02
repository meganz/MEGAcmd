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

class SyncIssue
{
    const size_t mId;
    std::unique_ptr<const mega::MegaSyncStall> mMegaStall;

public:
    SyncIssue(size_t id, const mega::MegaSyncStall& stall);

    inline size_t getId() const { return mId; }

    std::string getSyncWaitReasonStr() const;
    std::string getMainPath() const;
};

using SyncIssueList = std::vector<SyncIssue>;

class SyncIssueCache
{
    const SyncIssueList& mSyncIssues;
    std::unique_lock<std::mutex> mSyncIssuesLock;

public:
    SyncIssueCache(const SyncIssueList& syncIssues, std::mutex& syncIssuesMutex);

    template<typename Cb>
    void forEach(Cb&& callback, size_t sizeLimit)
    {
        for (size_t i = 0; i < size() && i < sizeLimit; ++i)
        {
            callback(mSyncIssues[i]);
        }
    }

    bool empty() const { return mSyncIssues.empty(); }
    unsigned int size() const { return mSyncIssues.size(); }

    SyncIssueList::const_iterator begin() const { return mSyncIssues.begin(); }
    SyncIssueList::const_iterator end() const { return mSyncIssues.end(); }
};

class SyncIssuesManager final
{
    SyncIssueList mSyncIssues;
    std::mutex mSyncIssuesMutex;

    std::unique_ptr<mega::MegaGlobalListener> mGlobalListener;
    std::unique_ptr<mega::MegaRequestListener> mRequestListener;

private:
    void populateSyncIssues(const mega::MegaSyncStallList &stalls);

public:
    SyncIssuesManager(mega::MegaApi *api);

    SyncIssueCache getLockedCache();

    mega::MegaGlobalListener* getGlobalListener() const { return mGlobalListener.get(); }
    mega::MegaRequestListener* getRequestListener() const { return mRequestListener.get(); }
};
