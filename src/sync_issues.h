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

#define GENERATE_FROM_PATH_PROBLEM(GENERATOR_MACRO) \
        GENERATOR_MACRO(mega::PathProblem::NoProblem,                             "No Problem") \
        GENERATOR_MACRO(mega::PathProblem::FileChangingFrequently,                "File is changing frequently") \
        GENERATOR_MACRO(mega::PathProblem::IgnoreRulesUnknown,                    "Ignore rules are unknown") \
        GENERATOR_MACRO(mega::PathProblem::DetectedHardLink,                      "Hard link detected") \
        GENERATOR_MACRO(mega::PathProblem::DetectedSymlink,                       "Symlink detected") \
        GENERATOR_MACRO(mega::PathProblem::DetectedSpecialFile,                   "Special file detected") \
        GENERATOR_MACRO(mega::PathProblem::DifferentFileOrFolderIsAlreadyPresent, "A different file/folder is already present") \
        GENERATOR_MACRO(mega::PathProblem::ParentFolderDoesNotExist,              "Parent folder does not exist") \
        GENERATOR_MACRO(mega::PathProblem::FilesystemErrorDuringOperation,        "There was a filesystem error during operation") \
        GENERATOR_MACRO(mega::PathProblem::NameTooLongForFilesystem,              "Name is too long for filesystem") \
        GENERATOR_MACRO(mega::PathProblem::CannotFingerprintFile,                 "File fingerprint cannot be obtained") \
        GENERATOR_MACRO(mega::PathProblem::DestinationPathInUnresolvedArea,       "Destination path (or one of its parents) is unresolved") \
        GENERATOR_MACRO(mega::PathProblem::MACVerificationFailure,                "Failure verifying MAC") \
        GENERATOR_MACRO(mega::PathProblem::DeletedOrMovedByUser,                  "Deleted or moved by user") \
        GENERATOR_MACRO(mega::PathProblem::FileFolderDeletedByUser,               "File or folder deleted by user") \
        GENERATOR_MACRO(mega::PathProblem::MoveToDebrisFolderFailed,              "Move to debris folder failed") \
        GENERATOR_MACRO(mega::PathProblem::IgnoreFileMalformed,                   "Ignore file is malformed") \
        GENERATOR_MACRO(mega::PathProblem::FilesystemErrorListingFolder,          "There was a filesystem error listing the folder") \
        GENERATOR_MACRO(mega::PathProblem::WaitingForScanningToComplete,          "Waiting for scanning to complete") \
        GENERATOR_MACRO(mega::PathProblem::WaitingForAnotherMoveToComplete,       "Waiting for another move to complete") \
        GENERATOR_MACRO(mega::PathProblem::SourceWasMovedElsewhere,               "The source was moved somewhere else") \
        GENERATOR_MACRO(mega::PathProblem::FilesystemCannotStoreThisName,         "The filesystem cannot store this name") \
        GENERATOR_MACRO(mega::PathProblem::CloudNodeInvalidFingerprint,           "Cloud node has invalid fingerprint") \
        GENERATOR_MACRO(mega::PathProblem::CloudNodeIsBlocked,                    "Cloud node is blocked") \
        GENERATOR_MACRO(mega::PathProblem::PutnodeDeferredByController,           "Putnode deferred by controller") \
        GENERATOR_MACRO(mega::PathProblem::PutnodeCompletionDeferredByController, "Putnode completion deferred by controller") \
        GENERATOR_MACRO(mega::PathProblem::PutnodeCompletionPending,              "Putnode completion pending") \
        GENERATOR_MACRO(mega::PathProblem::UploadDeferredByController,            "Upload deferred by controller") \
        GENERATOR_MACRO(mega::PathProblem::DetectedNestedMount,                   "Nested mount detected")

class SyncIssue
{
    mutable std::string mId;
    std::unique_ptr<const mega::MegaSyncStall> mMegaStall;

public:
    struct PathProblem
    {
        std::string mPath;
        std::string mProblem;
    };

    SyncIssue(const mega::MegaSyncStall& stall);

    const std::string& getId() const;

    std::string getSyncWaitReasonStr() const;
    std::string getMainPath() const;

    std::vector<PathProblem> getPathProblems() const;
    std::vector<PathProblem> getPathProblems(bool local) const;

    std::unique_ptr<mega::MegaSync> getParentSync(mega::MegaApi& api) const;
    bool belongsToSync(const mega::MegaSync& sync) const;
};

class SyncIssueList
{
    std::vector<SyncIssue> mIssuesVec;
    friend class SyncIssuesRequestListener; // only one that can actually populate this

public:
    template<typename Cb>
    void forEach(Cb&& callback, size_t sizeLimit)
    {
        for (size_t i = 0; i < size() && i < sizeLimit; ++i)
        {
            callback(mIssuesVec[i]);
        }
    }

    SyncIssue const* getSyncIssue(const std::string& id) const;
    unsigned int getSyncIssueCount(const mega::MegaSync& sync) const;

    bool empty() const { return mIssuesVec.empty(); }
    unsigned int size() const { return mIssuesVec.size(); }

    std::vector<SyncIssue>::const_iterator begin() const { return mIssuesVec.begin(); }
    std::vector<SyncIssue>::const_iterator end() const { return mIssuesVec.end(); }
};

class SyncIssuesManager final
{
    mega::MegaApi& mApi;
    bool mWarningEnabled;

    std::unique_ptr<mega::MegaGlobalListener> mGlobalListener;
    std::unique_ptr<mega::MegaRequestListener> mRequestListener;

private:
    void onSyncIssuesChanged(unsigned int newSyncIssuesSize);

public:
    SyncIssuesManager(mega::MegaApi *api);

    SyncIssueList getSyncIssues() const;

    void disableWarning();
    void enableWarning();

    mega::MegaGlobalListener* getGlobalListener() const { return mGlobalListener.get(); }
};
