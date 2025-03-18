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
#include "configurationmanager.h"

using std::string;

namespace {

bool isSyncTransfer(mega::MegaApi& api, int transferTag)
{
    mega::MegaTransfer* transfer = api.getTransferByTag(transferTag);
    return transfer && transfer->isSyncTransfer();
}

unsigned int getPendingSyncTransferCount(mega::MegaApi& api)
{
    unsigned int count = 0;

    mega::MegaTransferData* transferData = api.getTransferData();
    assert(transferData);

    for (int i = 0; i < transferData->getNumDownloads(); ++i)
    {
        const int tag = transferData->getDownloadTag(i);
        if (isSyncTransfer(api, tag))
        {
            ++count;
        }
    }

    for (int i = 0; i < transferData->getNumUploads(); ++i)
    {
        const int tag = transferData->getUploadTag(i);
        if (isSyncTransfer(api, tag))
        {
            ++count;
        }
    }

    return count;
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

void printSingleSync(mega::MegaApi& api, mega::MegaSync& sync, mega::MegaNode* node, long long nFiles, long long nFolders, ColumnDisplayer &cd, bool showHandle, int syncIssuesCount)
{
    cd.addValue("ID", getSyncId(sync));
    cd.addValue("LOCALPATH", sync.getLocalFolder());

    if (showHandle)
    {
        cd.addValue("REMOTEHANDLE", string("<H:").append(handleToBase64(sync.getMegaHandle()).append(">")));
    }

    cd.addValue("REMOTEPATH", sync.getLastKnownMegaFolder());
    cd.addValue("RUN_STATE", syncRunStateStr(sync.getRunState()));

    std::string localFolder = sync.getLocalFolder();

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

    cd.addValue("SIZE", sizeToText(node ? api.getSize(node) : 0));
    cd.addValue("FILES", std::to_string(nFiles));
    cd.addValue("DIRS", std::to_string(nFolders));
}

std::pair<std::optional<string>, std::optional<string>> getErrorsAndSetOutCode(MegaCmdListener& listener)
{
    std::optional<string> errorOpt, syncErrorOpt;

    int errorCode = listener.getError()->getErrorCode();
    bool hasMegaError = errorCode != mega::MegaError::API_OK;
    auto syncError = static_cast<mega::SyncError>(listener.getRequest()->getNumDetails());

    if (hasMegaError)
    {
        setCurrentThreadOutCode(errorCode);
        errorOpt = listener.getError()->getErrorString();
    }

    if (syncError != mega::SyncError::NO_SYNC_ERROR)
    {
        if (!hasMegaError)
        {
            setCurrentThreadOutCode(MCMD_INVALIDSTATE);
        }

        std::unique_ptr<const char[]> syncErrorStr(mega::MegaSync::getMegaSyncErrorCode(syncError));
        syncErrorOpt = syncErrorStr.get();
    }

    return {errorOpt, syncErrorOpt};
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

std::unique_ptr<mega::MegaSync> reloadSync(mega::MegaApi& api, std::unique_ptr<mega::MegaSync>&& sync)
{
    assert(sync);
    return std::unique_ptr<mega::MegaSync>(api.getSyncByBackupId(sync->getBackupId()));
}

bool isAnySyncUploadDelayed(mega::MegaApi& api)
{
    const bool isSyncing = api.isSyncing();
    if (!isSyncing)
    {
        return false;
    }

    const bool syncStalled = api.isSyncStalled();
    if (syncStalled)
    {
        return false;
    }

    const uint pendingSyncTransfers = getPendingSyncTransferCount(api);
    if (pendingSyncTransfers != 0)
    {
        return false;
    }

    auto listener = std::make_unique<MegaCmdListener>(nullptr);
    api.checkSyncUploadsThrottled(listener.get());
    listener->wait();

    if (listener->getError()->getErrorCode() != mega::MegaError::API_OK)
    {
        LOG_err << "Failed to get the list of delayed sync uploads";
        return false;
    }

    mega::MegaRequest* request = listener->getRequest();
    assert(request != nullptr);

    return request->getFlag();
}

void printSync(mega::MegaApi& api, ColumnDisplayer& cd, bool showHandle, mega::MegaSync& sync,  const SyncIssueList& syncIssues)
{
    std::unique_ptr<mega::MegaNode> node(api.getNodeByHandle(sync.getMegaHandle()));
    if (!node)
    {
        LOG_err << "Remote node not found for sync " << getSyncId(sync);
        return;
    }

    long long nFiles = 0;
    long long nFolders = 0;
    getInfoFromFolder(*node, api, nFiles, nFolders);

    printSyncHeader(cd);

    unsigned int syncIssuesCount = syncIssues.getSyncIssuesCount(sync);
    printSingleSync(api, sync, node.get(), nFiles, nFolders, cd, showHandle, syncIssuesCount);
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
        printSingleSync(api, sync, node.get(), nFiles, nFolders, cd, showHandles, syncIssuesCount);
    }
}

void addSync(mega::MegaApi& api, const fs::path& localPath, mega::MegaNode& node)
{
    std::unique_ptr<const char[]> nodePathPtr(api.getNodePath(&node));
    const char* nodePath = (nodePathPtr ? nodePathPtr.get() : "<path not found>");

    if (node.getType() == mega::MegaNode::TYPE_FILE)
    {
        setCurrentThreadOutCode(MCMD_NOTPERMITTED);
        LOG_err << "Remote sync root " << nodePath << " must be a directory";
        return;
    }

    if (api.getAccess(&node) < mega::MegaShare::ACCESS_FULL)
    {
        setCurrentThreadOutCode(MCMD_NOTPERMITTED);
        LOG_err << "Syncing requires full access to path (current access: " << api.getAccess(&node) << ")";
        return;
    }

    auto megaCmdListener = std::make_unique<MegaCmdListener>(nullptr);
    api.syncFolder(mega::MegaSync::TYPE_TWOWAY, localPath.string().c_str(), nullptr, node.getHandle(), nullptr, megaCmdListener.get());

    megaCmdListener->wait();

    auto [errorOpt, syncErrorOpt] = getErrorsAndSetOutCode(*megaCmdListener);

    if (errorOpt)
    {
        LOG_err << "Failed to sync " << localPath.string() << " to " << nodePath
                << " (Error: " << *errorOpt << (syncErrorOpt ? ". Reason: " + *syncErrorOpt : "") << ")";
        return;
    }

    string syncLocalPath = megaCmdListener->getRequest()->getFile();
    OUTSTREAM << "Added sync: " << syncLocalPath << " to " << nodePath << endl;

    if (syncErrorOpt)
    {
        LOG_err << "Sync added as temporarily disabled. Reason: " << *syncErrorOpt;
    }
}

void modifySync(mega::MegaApi& api, mega::MegaSync& sync, ModifyOpts opts)
{
    auto megaCmdListener = std::make_unique<MegaCmdListener>(nullptr);

    if (opts == ModifyOpts::Delete)
    {
        api.removeSync(sync.getBackupId(), megaCmdListener.get());
        megaCmdListener->wait();

        auto [errorOpt, syncErrorOpt] = getErrorsAndSetOutCode(*megaCmdListener);

        if (!errorOpt)
        {
            OUTSTREAM << "Sync removed: " << sync.getLocalFolder() << " to " << sync.getLastKnownMegaFolder() << endl;
        }
        else
        {
            LOG_err << "Failed to remove sync " << getSyncId(sync)
                    << " (Error: " << *errorOpt << (syncErrorOpt ? ". Reason: " + *syncErrorOpt : "") << ")";
        }
    }
    else
    {
        auto newState = (opts == ModifyOpts::Pause ? mega::MegaSync::RUNSTATE_SUSPENDED : mega::MegaSync::RUNSTATE_RUNNING);

        api.setSyncRunState(sync.getBackupId(), newState, megaCmdListener.get());
        megaCmdListener->wait();

        auto [errorOpt, syncErrorOpt] = getErrorsAndSetOutCode(*megaCmdListener);

        if (!errorOpt)
        {
            const char* action = (opts == ModifyOpts::Pause ? "paused" : "enabled");
            OUTSTREAM << "Sync " << action << ": " << sync.getLocalFolder() << " to " << sync.getLastKnownMegaFolder() << std::endl;

            if (syncErrorOpt)
            {
                LOG_err << "Sync might be temporarily disabled. Reason: " << *syncErrorOpt;
            }
        }
        else
        {
            const char* action = (opts == ModifyOpts::Pause ? "pause" : "enable");
            LOG_err << "Failed to " << action << " sync " << getSyncId(sync)
                    << " (Error: " << *errorOpt << (syncErrorOpt ? ". Reason: " + *syncErrorOpt : "") << ")";
        }
    }
}
} // end namespace

namespace GlobalSyncConfig {
void loadFromConfigurationManager(mega::MegaApi& api)
{
    const int duWaitSecs = ConfigurationManager::getConfigurationValue("GlobalSyncConfig:delayedUploadsWaitSecs", -1);
    if (duWaitSecs != -1)
    {
        DelayedUploads::updateWaitSecs(api, duWaitSecs);
    }

    const int duMaxAttempts = ConfigurationManager::getConfigurationValue("GlobalSyncConfig:delayedUploadsMaxAttempts", -1);
    if (duMaxAttempts != -1)
    {
        DelayedUploads::updateMaxAttempts(api, duMaxAttempts);
    }
}

namespace DelayedUploads {
namespace {

template<typename RequestFunc>
std::optional<Config> makeApiRequest(RequestFunc&& requestFunc)
{
    auto listener = std::make_unique<MegaCmdListener>(nullptr);
    requestFunc(listener.get());
    listener->wait();

    mega::MegaError* error = listener->getError();
    assert(error);

    if (error->getErrorCode() != mega::MegaError::API_OK)
    {
        return std::nullopt;
    }

    mega::MegaRequest* request = listener->getRequest();
    assert(request);

    Config config;
    config.mWaitSecs = request->getNumber();
    config.mMaxAttempts = request->getTotalBytes();
    return config;
}

std::optional<Config> getCurrentLowerLimits(mega::MegaApi& api)
{
    return makeApiRequest([&api] (mega::MegaRequestListener* listener)
    {
        api.getSyncUploadThrottleLowerLimits(listener);
    });
}

std::optional<Config> getCurrentUpperLimits(mega::MegaApi& api)
{
    return makeApiRequest([&api] (mega::MegaRequestListener* listener)
    {
        api.getSyncUploadThrottleUpperLimits(listener);
    });
}
} // end namespace

std::optional<Config> getCurrentConfig(mega::MegaApi& api)
{
    return makeApiRequest([&api] (mega::MegaRequestListener* listener)
    {
        api.getSyncUploadThrottleValues(listener);
    });
}

bool Limits::isWaitSecsValid(int waitSecs) const
{
    return waitSecs >= mLower.mWaitSecs && waitSecs <= mUpper.mWaitSecs;
}

bool Limits::isMaxAttemptsValid(int maxAttempts) const
{
    return maxAttempts >= mLower.mMaxAttempts && maxAttempts <= mUpper.mMaxAttempts;
}

std::optional<Limits> getCurrentLimits(mega::MegaApi& api)
{
    auto lowerLimitsOpt = getCurrentLowerLimits(api);
    if (!lowerLimitsOpt)
    {
        LOG_warn << "Failed to get the delayed uploads lower limits";
        return std::nullopt;
    }

    auto upperLimitsOpt = getCurrentUpperLimits(api);
    if (!upperLimitsOpt)
    {
        LOG_warn << "Failed to get the delayed uploads upper limits";
        return std::nullopt;
    }

    return Limits{*lowerLimitsOpt, *upperLimitsOpt};
}

void updateWaitSecs(mega::MegaApi& api, int waitSecs)
{
    auto limits = getCurrentLimits(api);
    if (limits && !limits->isWaitSecsValid(waitSecs))
    {
        setCurrentThreadOutCode(MCMD_EARGS);
        LOG_err << "Delayed sync uploads wait time must be between "
                << limits->mLower.mWaitSecs << " and " << limits->mUpper.mWaitSecs << " seconds";
        return;
    }

    auto result = makeApiRequest([&api, waitSecs] (mega::MegaRequestListener* listener)
    {
        api.setSyncUploadThrottleUpdateRate(waitSecs, listener);
    });

    if (!result)
    {
        LOG_err << "Failed to set the wait time for delayed sync uploads";
        return;
    }

    OUTSTREAM << "Successfully set the wait time for delayed sync uploads to " << result->mWaitSecs << " seconds" << endl;
    ConfigurationManager::savePropertyValue("GlobalSyncConfig:delayedUploadsWaitSecs", result->mWaitSecs);
}

void updateMaxAttempts(mega::MegaApi& api, int maxAttempts)
{
    auto limits = getCurrentLimits(api);
    if (limits && !limits->isMaxAttemptsValid(maxAttempts))
    {
        setCurrentThreadOutCode(MCMD_EARGS);
        LOG_err << "Max delayed uploads attempts must be between "
                << limits->mLower.mMaxAttempts << " and " << limits->mUpper.mMaxAttempts;
        return;
    }

    auto result = makeApiRequest([&api, maxAttempts] (mega::MegaRequestListener* listener)
    {
        api.setSyncMaxUploadsBeforeThrottle(maxAttempts, listener);
    });

    if (!result)
    {
        LOG_err << "Failed to set the max attempts for delayed sync uploads";
        return;
    }

    OUTSTREAM << "Successfully set the max attempts for delayed sync uploads to " << result->mMaxAttempts << endl;
    ConfigurationManager::savePropertyValue("GlobalSyncConfig:delayedUploadsMaxAttempts", result->mMaxAttempts);
}
} // end namespace
} // end namespace
