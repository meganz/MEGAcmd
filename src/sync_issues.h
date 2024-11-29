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

#include <map>
#include <memory>
#include <mutex>
#include <optional>

#include "megaapi.h"
#include "mega/types.h"
#include "megacmdcommonutils.h"

#define GENERATE_FROM_PATH_PROBLEM(GENERATOR_MACRO) \
        GENERATOR_MACRO(mega::PathProblem::NoProblem,                             "-") \
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
        GENERATOR_MACRO(mega::PathProblem::UnknownDownloadIssue,                  "There was an issue downloading to destination directory") \
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

struct SyncInfo
{
    mega::SyncWaitReason mReasonType = mega::SyncWaitReason::NoReason;
    std::string mReason;
    std::string mDescription;
};

class SyncIssue
{
    std::unique_ptr<const mega::MegaSyncStall> mMegaStall;

    // Returns the cloud/local file path with index i
    template<bool isCloud>
    std::string getFilePath(int i = 0) const;

    // Returns the file name of the cloud/local path with index i
    template<bool isCloud>
    std::string getFileName(int i) const;

    // Returns the file name of path 0, with preference for cloud or local
    template<bool preferCloud = false>
    std::string getDefaultFileName() const;

    // Returns the index of the first path problem of a particular type (if any)
    template<bool isCloud>
    std::optional<int> getPathProblem(mega::PathProblem pathProblem) const;

public:
    // We add this prefix at the start to distinguish between cloud and local absolute paths
    inline static const std::string CloudPrefix = "<CLOUD>";

    struct PathProblem
    {
        bool mIsCloud = false;
        std::string mPath;
        std::string mPathType = "<unknown>";
        mega::PathProblem mProblem = mega::PathProblem::NoProblem;
        int64_t mModifiedTime = 0; // in seconds since epoch
        int64_t mUploadedTime = 0; // in seconds since epoch
        int64_t mFileSize = 0;

        std::string_view getProblemStr() const;
    };

    SyncIssue(const mega::MegaSyncStall& stall);

    std::string getId() const;
    SyncInfo getSyncInfo(mega::MegaSync const* parentSync) const;

    std::vector<PathProblem> getPathProblems(mega::MegaApi& api) const;

    template<bool isCloud>
    std::vector<PathProblem> getPathProblems(mega::MegaApi& api) const;

    std::unique_ptr<mega::MegaSync> getParentSync(mega::MegaApi& api) const;
    bool belongsToSync(const mega::MegaSync& sync) const;
};

class SyncIssueList
{
    using SyncIssuesMapT = std::map<std::string, SyncIssue>;
    SyncIssuesMapT mIssuesMap;

    friend class SyncIssuesRequestListener; // only one that can actually populate this

public:
    template<typename Cb>
    void forEach(Cb&& callback, size_t sizeLimit) const
    {
        for (auto it = begin(); it != end() && std::distance(begin(), it) < sizeLimit; ++it)
        {
            callback(it->second);
        }
    }

    SyncIssue const* getSyncIssue(const std::string& id) const;
    unsigned int getSyncIssuesCount(const mega::MegaSync& sync) const;

    bool empty() const { return mIssuesMap.empty(); }
    unsigned int size() const { return mIssuesMap.size(); }

    SyncIssuesMapT::const_iterator begin() const { return mIssuesMap.begin(); }
    SyncIssuesMapT::const_iterator end() const { return mIssuesMap.end(); }
};

class SyncIssuesManager final
{
    mega::MegaApi& mApi;
    bool mWarningEnabled;
    std::mutex mWarningMtx;

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

namespace SyncIssuesCommand
{
    void printAllIssues(mega::MegaApi& api, megacmd::ColumnDisplayer& cd, const SyncIssueList& syncIssues, bool disablePathCollapse, int rowCountLimit);

    void printSingleIssueDetail(mega::MegaApi& api, megacmd::ColumnDisplayer& cd, const SyncIssue& syncIssue, bool disablePathCollapse, int rowCountLimit);
    void printAllIssuesDetail(mega::MegaApi& api, megacmd::ColumnDisplayer& cd, const SyncIssueList& syncIssues, bool disablePathCollapse, int rowCountLimit);
}
