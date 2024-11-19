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

#include "sync_command.h"

#include "listeners.h"
#include "megacmdutils.h"
#include "megacmdlogger.h"

using std::string;

namespace {

string withoutTrailingSeparator(const fs::path& path)
{
    string str = path.string();
    if (!str.empty() && (str.back() == '/' || str.back() == '\\'))
    {
        str.pop_back();
    }
    return str;
}

string getSyncId(mega::MegaSync& sync)
{
    return syncBackupIdToBase64(sync.getBackupId());
}

void getInfoFromFolder(mega::MegaNode& n, mega::MegaApi& api, long long& nFiles, long long& nFolders, long long *nVersions = nullptr)
{
    auto megaCmdListener = std::make_unique<MegaCmdListener>(nullptr);
    api.getFolderInfo(&n, megaCmdListener.get());

    megaCmdListener->wait();
    if (megaCmdListener->getError()->getErrorCode() != mega::MegaError::API_OK)
    {
        LOG_err << "Failed to get folder info for " << n.getName();
        return;
    }

    mega::MegaFolderInfo* mfi = megaCmdListener->getRequest()->getMegaFolderInfo();
    if (!mfi)
    {
        return;
    }

    nFiles = mfi->getNumFiles();
    nFolders = mfi->getNumFolders();
    if (nVersions)
    {
        *nVersions = mfi->getNumVersions();
    }
}

void printSyncHeader(ColumnDisplayer &cd)
{
    // Add headers with unfixed width (those that can be ellided)
    cd.addHeader("LOCALPATH", false);
    cd.addHeader("REMOTEPATH", false);
}

void printSingleSync(mega::MegaApi& api, mega::MegaSync& sync, long long nFiles, long long nFolders, ColumnDisplayer &cd, bool showHandle, int syncIssuesCount)
{
    cd.addValue("ID", getSyncId(sync));
    cd.addValue("LOCALPATH", sync.getLocalFolder());

    if (showHandle)
    {
        cd.addValue("REMOTEHANDLE", string("<H:").append(handleToBase64(sync.getMegaHandle()).append(">")));
    }

    cd.addValue("REMOTEPATH", sync.getLastKnownMegaFolder());
    cd.addValue("RUN_STATE", syncRunStateStr(sync.getRunState()));

    string folder = withoutTrailingSeparator(sync.getLocalFolder());
    string localFolder;
    mega::LocalPath::path2local(&folder, &localFolder);

    int statepath = api.syncPathState(&localFolder);
    cd.addValue("STATUS", getSyncPathStateStr(statepath));

    if (sync.getError())
    {
        std::unique_ptr<const char[]> megaSyncErrorCode(sync.getMegaSyncErrorCode());
        cd.addValue("ERROR", megaSyncErrorCode.get());
    }
    else
    {
        string syncIssuesMsg = "NO";
        if (syncIssuesCount > 0)
        {
            syncIssuesMsg = "Sync Issues (" + std::to_string(syncIssuesCount) + ")";
        }
        cd.addValue("ERROR", syncIssuesMsg);
    }

    std::unique_ptr<mega::MegaNode> n(api.getNodeByHandle(sync.getMegaHandle()));
    cd.addValue("SIZE", sizeToText(api.getSize(n.get())));
    cd.addValue("FILES", std::to_string(nFiles));
    cd.addValue("DIRS", std::to_string(nFolders));
}

string getErrorString(MegaCmdListener& listener)
{
    std::string errorStr;

    bool hasMegaError = listener.getError()->getErrorCode() != mega::MegaError::API_OK;
    auto syncError = static_cast<mega::SyncError>(listener.getRequest()->getNumDetails());

    if (hasMegaError)
    {
        errorStr += "error: ";
        errorStr += listener.getError()->getErrorString();
    }
    else if (syncError)
    {
        errorStr += "sync error: ";
    }

    if (syncError)
    {
        if (hasMegaError)
        {
            errorStr += ". ";
        }

        std::unique_ptr<const char[]> syncErrorStr(mega::MegaSync::getMegaSyncErrorCode(syncError));
        errorStr += syncErrorStr.get();
    }

    return errorStr;

}

string getSyncErrorReason(MegaCmdListener& listener)
{
    auto syncError = static_cast<mega::SyncError>(listener.getRequest()->getNumDetails());
    if (syncError == mega::NO_SYNC_ERROR)
    {
        return "";
    }

    std::unique_ptr<const char[]> megaSyncErrorCode(mega::MegaSync::getMegaSyncErrorCode(syncError));
    return megaSyncErrorCode.get();
}
} // end namespace

namespace SyncCommand {

std::unique_ptr<mega::MegaSync> getSync(mega::MegaApi& api, const string& pathOrId)
{
    std::unique_ptr<mega::MegaSync> sync;
    sync.reset(api.getSyncByBackupId(base64ToSyncBackupId(pathOrId)));

    if (!sync)
    {
        sync.reset(api.getSyncByPath(pathOrId.c_str()));
    }
    return sync;
}

std::unique_ptr<mega::MegaSync> reloadSync(mega::MegaApi& api, std::unique_ptr<mega::MegaSync> sync)
{
    assert(sync);
    return std::unique_ptr<mega::MegaSync>(api.getSyncByBackupId(sync->getBackupId()));
}

void printSync(mega::MegaApi& api, ColumnDisplayer& cd, bool showHandle, mega::MegaSync& sync,  const SyncIssueList& syncIssues)
{
    std::unique_ptr<mega::MegaNode> node(api.getNodeByHandle(sync.getMegaHandle()));
    if (!node)
    {
        LOG_warn << "Remote node not found for sync " << getSyncId(sync);
        return;
    }

    long long nFiles = 0;
    long long nFolders = 0;
    getInfoFromFolder(*node, api, nFiles, nFolders);

    printSyncHeader(cd);

    unsigned int syncIssuesCount = syncIssues.getSyncIssuesCount(sync);
    printSingleSync(api, sync, nFiles, nFolders, cd, showHandle, syncIssuesCount);
}

void printSyncList(mega::MegaApi& api, ColumnDisplayer& cd, bool showHandles, const mega::MegaSyncList& syncList, const SyncIssueList& syncIssues)
{
    if (syncList.size() > 0)
    {
        printSyncHeader(cd);
    }

    for (int i = 0; i < syncList.size(); ++i)
    {
        mega::MegaSync& sync = *syncList.get(i);

        std::unique_ptr<mega::MegaNode> node(api.getNodeByHandle(sync.getMegaHandle()));

        long long nFiles = 0;
        long long nFolders = 0;
        if (node)
        {
            getInfoFromFolder(*node, api, nFiles, nFolders);
        }
        else
        {
            LOG_warn << "Remote node not found for sync " << getSyncId(sync);
        }

        unsigned int syncIssuesCount = syncIssues.getSyncIssuesCount(sync);
        printSingleSync(api, sync, nFiles, nFolders, cd, showHandles, syncIssuesCount);
    }
}

void addSync(mega::MegaApi& api, const fs::path& localPath, mega::MegaNode& node)
{
    std::unique_ptr<const char[]> nodePath(api.getNodePath(&node));

    if (node.getType() == mega::MegaNode::TYPE_FILE)
    {
        setCurrentOutCode(MCMD_NOTPERMITTED);
        LOG_err << "Remote sync root " << nodePath << " must be a directory";
        return;
    }

    if (api.getAccess(&node) < mega::MegaShare::ACCESS_FULL)
    {
        setCurrentOutCode(MCMD_NOTPERMITTED);
        LOG_err << "Syncing requires full access to path (current access: " << api.getAccess(&node) << ")";
        return;
    }

    auto megaCmdListener = std::make_unique<MegaCmdListener>(nullptr);
    api.syncFolder(mega::MegaSync::TYPE_TWOWAY, localPath.string().c_str(), nullptr, node.getHandle(), nullptr, megaCmdListener.get());

    megaCmdListener->wait();

    string errorStr = getErrorString(*megaCmdListener);
    if (!errorStr.empty())
    {
        LOG_err << "Failed to sync " << localPath.string() << " to " << nodePath << " (" << errorStr << ")";
        return;
    }

    string syncLocalPath = megaCmdListener->getRequest()->getFile();
    OUTSTREAM << "Added sync: " << syncLocalPath << " to " << nodePath.get() << endl;

    string syncErrorReason = getSyncErrorReason(*megaCmdListener);
    if (!syncErrorReason.empty())
    {
        LOG_err << "Sync added as temporarily disabled. Reason: " << syncErrorReason;
    }
}

void modifySync(mega::MegaApi& api, mega::MegaSync& sync, ModifyOpts opts)
{
    auto megaCmdListener = std::make_unique<MegaCmdListener>(nullptr);

    if (opts == ModifyOpts::Delete)
    {
        api.removeSync(sync.getBackupId(), megaCmdListener.get());
        megaCmdListener->wait();

        string errorStr = getErrorString(*megaCmdListener);
        if (errorStr.empty())
        {
            OUTSTREAM << "Sync removed: " << sync.getLocalFolder() << " to " << sync.getLastKnownMegaFolder() << endl;
        }
        else
        {
            LOG_err << "Failed to remove sync " << getSyncId(sync) << " (" << errorStr << ")";
        }
    }
    else
    {
        auto newState = (opts == ModifyOpts::Pause ? mega::MegaSync::RUNSTATE_SUSPENDED : mega::MegaSync::RUNSTATE_RUNNING);

        api.setSyncRunState(sync.getBackupId(), newState, megaCmdListener.get());
        megaCmdListener->wait();

        string errorStr = getErrorString(*megaCmdListener);
        if (errorStr.empty())
        {
            const char* action = (opts == ModifyOpts::Pause ? "paused" : "enabled");
            LOG_info << "Sync " << action << ": " << sync.getLocalFolder() << " to " << sync.getLastKnownMegaFolder();

            string syncErrorReason = getSyncErrorReason(*megaCmdListener);
            if (!syncErrorReason.empty())
            {
                LOG_err << "Sync might be temporarily disabled. Reason: " << syncErrorReason;
            }
        }
        else
        {
            const char* action = (opts == ModifyOpts::Pause ? "pause" : "enable");
            LOG_err << "Failed to " << action << " sync " << getSyncId(sync) << " (" << errorStr << ")";
        }
    }
}
} // end namespace
