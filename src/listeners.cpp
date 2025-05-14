/**
 * @file src/listeners.cpp
 * @brief MEGAcmd: Listeners
 *
 * (c) 2013 by Mega Limited, Auckland, New Zealand
 *
 * This file is part of the MEGAcmd.
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

#include "listeners.h"
#include "configurationmanager.h"
#include "megacmdutils.h"

#ifdef MEGACMD_TESTING_CODE
    #include "../tests/common/Instruments.h"
#endif

#include <utility>

using namespace mega;


namespace megacmd {
#ifdef ENABLE_CHAT
void MegaCmdGlobalListener::onChatsUpdate(MegaApi*, MegaTextChatList*)
{

}
#endif

void MegaCmdGlobalListener::onUsersUpdate(MegaApi *api, MegaUserList *users1)
{
    if (users1)
    {
        if (users1->size() == 1)
        {
            LOG_debug << " 1 user received or updated";
        }
        else
        {
            LOG_debug << users1->size() << " users received or updated";
        }
    }
    else //initial update or too many changes
    {
        std::unique_ptr<MegaUserList> users2(api->getContacts());

        if (users2 && users2->size())
        {
            if (users2->size() == 1)
            {
                LOG_debug << " 1 user received or updated";
            }
            else
            {
                LOG_debug << users2->size() << " users received or updated";
            }
        }
    }
}

MegaCmdGlobalListener::MegaCmdGlobalListener(MegaCmdLogger *logger, MegaCmdSandbox *sandboxCMD)
{
    this->loggerCMD = logger;
    this->sandboxCMD = sandboxCMD;

    ongoing = false;
}

void MegaCmdGlobalListener::onNodesUpdate(MegaApi *api, MegaNodeList *nodes)
{
    long long nfolders = 0;
    long long nfiles = 0;
    long long rfolders = 0;
    long long rfiles = 0;
    if (nodes)
    {
        for (int i = 0; i < nodes->size(); i++)
        {
            MegaNode *n = nodes->get(i);
            if (n->getType() == MegaNode::TYPE_FOLDER)
            {
                if (n->isRemoved())
                {
                    rfolders++;
                }
                else
                {
                    nfolders++;
                }
            }
            else if (n->getType() == MegaNode::TYPE_FILE)
            {
                if (n->isRemoved())
                {
                    rfiles++;
                }
                else
                {
                    nfiles++;
                }
            }
        }

        if (nfolders)
        {
            LOG_debug << nfolders << " folders "
                      << "added or updated ";
        }
        if (nfiles)
        {
            LOG_debug << nfiles << " files "
                      << "added or updated ";
        }
    }
    else if (loggerCMD->getMaxLogLevel() >= logInfo)  //initial update or too many changes
    {
        api->getAccountDetails(new MegaCmdListenerFuncExecuter(
            [nfiles, nfolders](mega::MegaApi *api, mega::MegaRequest *request, mega::MegaError *e)
            {
                auto details = std::unique_ptr<mega::MegaAccountDetails>(request->getMegaAccountDetails());
                if (details == nullptr)
                {
                    LOG_err << "could not get account details";
                    return;
                }
                auto nFiles = nfiles;
                auto nFolders = nfolders;
                auto nodeRoot = std::unique_ptr<MegaNode>(api->getRootNode());
                if (nodeRoot != nullptr)
                {
                    auto handle = nodeRoot->getHandle();
                    nFiles += details->getNumFiles(handle);
                    nFolders += details->getNumFolders(handle);
                }
                auto vaultNode = std::unique_ptr<MegaNode>(api->getVaultNode());
                if (vaultNode != nullptr)
                {
                    auto handle = vaultNode->getHandle();
                    nFiles += details->getNumFiles(handle);
                    nFolders += details->getNumFolders(handle);
                }
                auto rubbishNode = std::unique_ptr<MegaNode>(api->getRubbishNode());
                if (rubbishNode != nullptr)
                {
                    auto handle = rubbishNode->getHandle();
                    nFiles += details->getNumFiles(handle);
                    nFolders += details->getNumFolders(handle);
                }
                auto inshares = std::unique_ptr<MegaNodeList>(api->getInShares());
                if (inshares)
                {
                    nFolders += inshares->size(); // add the shares themselves.
                    for (int i = 0; i < inshares->size(); i++)
                    {
                        auto handle = inshares->get(i)->getHandle();
                        nFiles += details->getNumFiles(handle);
                        nFolders += details->getNumFolders(handle);
                    }
                }
                if (nFolders)
                {
                    LOG_debug << nFolders << " folders added or updated";
                }
                if (nFiles)
                {
                    LOG_debug << nFiles << " files added or updated";
                }
            },
            true));
    }

    if (rfolders)
    {
        LOG_debug << rfolders << " folders "
                  << "removed";
    }
    if (rfiles)
    {
        LOG_debug << rfiles << " files "
                  << "removed";
    }
}

void MegaCmdGlobalListener::onAccountUpdate(MegaApi *api)
{
    if (!api->getBandwidthOverquotaDelay())
    {
        sandboxCMD->setOverquota(false);
    }
    sandboxCMD->temporalbandwidth = 0; //This will cause account details to be queried again
}

void MegaCmdGlobalListener::onEvent(MegaApi *api, MegaEvent *event)
{
    if (event->getType() == MegaEvent::EVENT_ACCOUNT_BLOCKED)
    {
        if (getBlocked() == event->getNumber())
        {
            LOG_debug << " receivied EVENT_ACCOUNT_BLOCKED: number = " << event->getNumber();
            return;
        }
        setBlocked(int(event->getNumber())); //this should be true always

        switch (event->getNumber())
        {
        case MegaApi::ACCOUNT_BLOCKED_VERIFICATION_EMAIL:
        {
            sandboxCMD->setReasonblocked( "Your account has been temporarily suspended for your safety. "
                                        "Please verify your email and follow its steps to unlock your account.");
            break;
        }
        case MegaApi::ACCOUNT_BLOCKED_VERIFICATION_SMS:
        {
            if (!ongoing)
            {
                ongoing = true;

                sandboxCMD->setReasonPendingPromise();

                api->getSessionTransferURL("", new MegaCmdListenerFuncExecuter(
                                               [this](mega::MegaApi* api, mega::MegaRequest *request, mega::MegaError *e)
                {
                    string reason("Your account has been suspended temporarily due to potential abuse. "
                    "Please verify your phone number to unlock your account." );
                    if (e->getValue() == MegaError::API_OK)
                    {
                       reason.append(" Open the following link: ");
                       reason.append(request->getLink());
                    }

                    sandboxCMD->setPromisedReasonblocked(reason);
                    ongoing = false;
                },true));
            }
            break;
        }
        case MegaApi::ACCOUNT_BLOCKED_SUBUSER_DISABLED:
        {
            sandboxCMD->setReasonblocked("Your account has been disabled by your administrator. Please contact your business account administrator for further details.");
            break;
        }
        default:
        {
            sandboxCMD->setReasonblocked(event->getText());
            LOG_err << "Received event account blocked: " << event->getText();
        }
        }
    }
    else if (event->getType() == MegaEvent::EVENT_STORAGE)
    {
        if (event->getNumber() == MegaApi::STORAGE_STATE_CHANGE)
        {
            api->getAccountDetails();
        }
        else
        {
            int previousStatus = sandboxCMD->storageStatus;
            sandboxCMD->storageStatus = int(event->getNumber());
            if (sandboxCMD->storageStatus == MegaApi::STORAGE_STATE_PAYWALL || sandboxCMD->storageStatus == MegaApi::STORAGE_STATE_RED || sandboxCMD->storageStatus == MegaApi::STORAGE_STATE_ORANGE)
            {
                ConfigurationManager::savePropertyValue("ask4storage",true);

                if (previousStatus < sandboxCMD->storageStatus)
                {
                    if (sandboxCMD->storageStatus == MegaApi::STORAGE_STATE_PAYWALL)
                    {
                        api->getUserData(new MegaCmdListenerFuncExecuter(
                                        [this](mega::MegaApi* api, mega::MegaRequest *request, mega::MegaError *e)
                        {
                            if (e->getValue() != MegaError::API_OK)
                            {
                                // We don't have access to MegaCmdExecuter's error handling methods here
                                std::string errorString(e->getErrorString());
                                LOG_err << "Failed to get user data: " << errorString;
                                return;
                            }
                            api->getAccountDetails(new MegaCmdListenerFuncExecuter(
                                [this](mega::MegaApi *api, mega::MegaRequest *request, mega::MegaError *e)
                                {
                                    if (e->getValue() != MegaError::API_OK)
                                    {
                                        std::string errorString(e->getErrorString());
                                        LOG_err << "Failed to get account details: " << errorString;
                                        return;
                                    }
                                    std::unique_ptr<char[]> myEmail(api->getMyEmail());
                                    std::unique_ptr<MegaIntegerList> warningsList(api->getOverquotaWarningsTs());
                                    std::string s("We have contacted you by email");
                                    if (myEmail != nullptr)
                                    {
                                        s.append(" to ").append(myEmail.get());
                                    }
                                    if (warningsList != nullptr)
                                    {
                                        s.append(" on ").append(getReadableTime(warningsList->get(0), "%b %e %Y"));
                                        if (warningsList->size() > 1)
                                        {
                                            for (int i = 1; i < warningsList->size() - 1; i++)
                                            {
                                                s += ", " + getReadableTime(warningsList->get(i), "%b %e %Y");
                                            }
                                            s += " and " + getReadableTime(warningsList->get(warningsList->size() - 1), "%b %e %Y");
                                        }
                                    }

                                    std::unique_ptr<MegaNode> rootNode(api->getRootNode());
                                    std::unique_ptr<MegaAccountDetails> details(request->getMegaAccountDetails());
                                    if (details != nullptr && rootNode != nullptr)
                                    {
                                        auto rootNodeHandle = rootNode->getHandle();
                                        long long totalFiles = details->getNumFiles(rootNodeHandle);
                                        long long totalFolders = details->getNumFolders(rootNodeHandle);
                                        s += ", but you still have " + std::to_string(totalFiles) + " files and " + std::to_string(totalFolders) + " folders taking up " +
                                             sizeToText(sandboxCMD->receivedStorageSum);
                                        s += " in your MEGA account, which requires you to upgrade your account.\n\n";
                                    }
                                    else // If we weren't able to get the root node or details object, lets just display the total storage used.
                                    {
                                        s += ", but your MEGA account still uses " + sizeToText(sandboxCMD->receivedStorageSum) +
                                             ", which requires you to upgrade your account.\n\n";
                                    }
                                    long long daysLeft = (api->getOverquotaDeadlineTs() - m_time(nullptr)) / 86400;
                                    if (daysLeft > 0)
                                    {
                                        s += "You have " + std::to_string(daysLeft) + " days left to upgrade. ";
                                        s += "After that, your data is subject to deletion.\n";
                                    }
                                    else
                                    {
                                        s += "You must act immediately to save your data. From now on, your data is subject to deletion.\n";
                                    }
                                    s += "See \"help --upgrade\" for further details.";
                                    broadcastMessage(s);
                                }));
                        },true));
                    }
                    else
                    {
                        string s;
                        if (sandboxCMD->storageStatus == MegaApi::STORAGE_STATE_RED)
                        {
                            s+= "You have exeeded your available storage.\n";
                        }
                        else
                        {
                            s+= "You are running out of available storage.\n";
                        }
                        s+="You can change your account plan to increase your quota limit.\nSee \"help --upgrade\" for further details";
                        broadcastMessage(s);
                    }
                }
            }
            else
            {
                ConfigurationManager::savePropertyValue("ask4storage",false);
            }
        }
        LOG_info << "Received event storage changed: " << event->getNumber();
    }
    else if (event->getType() == MegaEvent::EVENT_STORAGE_SUM_CHANGED)
    {
        sandboxCMD->receivedStorageSum = event->getNumber();
    }
    else if (event->getType() == MegaEvent::EVENT_SYNCS_DISABLED)
    {
        std::unique_ptr<const char[]> megaSyncErrorCode {MegaSync::getMegaSyncErrorCode(int(event->getNumber()))};
        removeDelayedBroadcastMatching("Your sync has been disabled");
        broadcastMessage(std::string("Your syncs have been disabled. Reason: ")
                         .append(megaSyncErrorCode.get()), true);
    }
    else if (event->getType() == MegaEvent::EVENT_UPGRADE_SECURITY)
    {
        std::stringstream ss;
        ss << ""
              "Your account's security needs upgrading.\n"
              "Please execute: \"confirm --security\".\n"
              "This only needs to be done once. If you have seen this message for\n"
              "this account before, please exit MEGAcmd.";

        auto msg = ss.str();
        appendGreetingStatusAllListener(std::string("message:") + msg);
        broadcastMessage(std::move(msg)); // broadcast the message, so that it reaches currently open shell too!
    }
    else if (event->getType() == MegaEvent::EVENT_NODES_CURRENT)
    {
        sandboxCMD->mNodesCurrentPromise.fulfil();
    }
}


////////////////////////////////////////////
///      MegaCmdMegaListener methods     ///
////////////////////////////////////////////

void MegaCmdMegaListener::onRequestFinish(MegaApi *api, MegaRequest *request, MegaError *e)
{
    if (request->getType() == MegaRequest::TYPE_APP_VERSION)
    {
        LOG_verbose << "TYPE_APP_VERSION finished";
    }
    else if (request->getType() == MegaRequest::TYPE_LOGOUT)
    {
        LOG_debug << "Session closed";
        sandboxCMD->resetSandBox();
        reset();
    }
    else if (request->getType() == MegaRequest::TYPE_WHY_AM_I_BLOCKED)
    {
        if (e->getErrorCode() == MegaError::API_OK
                && request->getNumber() == MegaApi::ACCOUNT_NOT_BLOCKED)
        {
            if (getBlocked())
            {
                unblock();
            }
        }

    }
    else if (request->getType() == MegaRequest::TYPE_ACCOUNT_DETAILS)
    {
        if (e->getErrorCode() != MegaError::API_OK)
        {
            return;
        }

        bool storage = (request->getNumDetails() & 0x01) != 0;

        if (storage)
        {
            unique_ptr<MegaAccountDetails> details(request->getMegaAccountDetails());
            sandboxCMD->totalStorage = details->getStorageMax();
        }
    }
    else if (e && ( e->getErrorCode() == MegaError::API_ESID ))
    {
        LOG_err << "Session is no longer valid (it might have been invalidated from elsewhere) ";
        changeprompt(prompts[COMMAND]);
    }
    else if (e && request->getType() == MegaRequest::TYPE_WHY_AM_I_BLOCKED && e->getErrorCode() == API_EBLOCKED)
    {
        LOG_err << "Your account has been blocked. Reason: " << request->getText();
    }
}

std::atomic<int> DisableMountErrorsBroadcastingGuard::sDisableBroadcasting = 0;

MegaCmdMegaListener::MegaCmdMegaListener(MegaApi *megaApi, MegaListener *parent, MegaCmdSandbox *sandboxCMD)
{
    this->megaApi = megaApi;
    this->listener = parent;
    this->sandboxCMD = sandboxCMD;
}

MegaCmdMegaListener::~MegaCmdMegaListener()
{
    this->listener = NULL;
    if (megaApi)
    {
        megaApi->removeListener(this);
    }
}

#ifdef ENABLE_CHAT
void MegaCmdMegaListener::onChatsUpdate(MegaApi *api, MegaTextChatList *chats)
{}
#endif

//backup callbacks:
void MegaCmdMegaListener::onBackupStateChanged(MegaApi *api,  MegaScheduledCopy *backup)
{
    LOG_verbose << " At onBackupStateChanged: " << backupSatetStr(backup->getState());
}

void MegaCmdMegaListener::onBackupStart(MegaApi *api, MegaScheduledCopy *backup)
{
    LOG_verbose << " At onBackupStart";
}

void MegaCmdMegaListener::onBackupFinish(MegaApi* api, MegaScheduledCopy *backup, MegaError* error)
{
    LOG_verbose << " At onBackupFinish";
    if (error->getErrorCode() == MegaError::API_EEXPIRED)
    {
        LOG_warn << "Backup skipped (the time for the next one has been reached)";
    }
    else if (error->getErrorCode() != MegaError::API_OK)
    {
        LOG_err << "Backup failed: " << error->getErrorString();
    }
}

void MegaCmdMegaListener::onBackupUpdate(MegaApi *api, MegaScheduledCopy *backup)
{
    LOG_verbose << " At onBackupUpdate";
}

void MegaCmdMegaListener::onBackupTemporaryError(MegaApi *api, MegaScheduledCopy *backup, MegaError* error)
{
    LOG_verbose << " At onBackupTemporaryError";
    if (error->getErrorCode() != MegaError::API_OK)
    {
        LOG_err << "Backup temporary error: " << error->getErrorString();
    }
}

void MegaCmdMegaListener::onSyncAdded(MegaApi *api, MegaSync *sync)
{
    LOG_verbose << "Sync added: " << sync->getLocalFolder() << " to " << sync->getLastKnownMegaFolder();

    if (!ConfigurationManager::getConfigurationValue("firstSyncConfigured", false))
    {
        sendEvent(StatsManager::MegacmdEvent::FIRST_CONFIGURED_SYNC, api, false);
        ConfigurationManager::savePropertyValue("firstSyncConfigured", true);
    }
    else
    {
        sendEvent(StatsManager::MegacmdEvent::SUBSEQUENT_CONFIGURED_SYNC, api, false);
    }
}

void MegaCmdMegaListener::onSyncStateChanged(MegaApi *api, MegaSync *sync)
{
    std::stringstream ss;
    ss << "Your sync " << sync->getLocalFolder() << " to: " << sync->getLastKnownMegaFolder()
    << " has transitioned to state " << syncRunStateStr(sync->getRunState());
    if (sync->getError())
    {
        std::unique_ptr<const char[]> megaSyncErrorCode {sync->getMegaSyncErrorCode()};
        ss << ". ErrorCode: " << megaSyncErrorCode.get();
    }
    auto msg = ss.str();

    if (sync->getError() || sync->getRunState() >= MegaSync::RUNSTATE_SUSPENDED)
    {
        broadcastDelayedMessage(msg, true);
    }
    LOG_debug << msg;
}

void MegaCmdMegaListener::onSyncDeleted(MegaApi *api, MegaSync *sync)
{
    LOG_verbose << "Sync deleted: " << sync->getLocalFolder() << " to " << sync->getLastKnownMegaFolder();
}

void MegaCmdMegaListener::onMountAdded(mega::MegaApi* api, const char* path, int result)
{
    onMountEvent("Added", "add", path, result);
}

void MegaCmdMegaListener::onMountRemoved(mega::MegaApi* api, const char* path, int result)
{
    onMountEvent("Removed", "remove", path, result);
}

void MegaCmdMegaListener::onMountChanged(mega::MegaApi* api, const char* path, int result)
{
    onMountEvent("Changed", "change", path, result);
}

void MegaCmdMegaListener::onMountEnabled(mega::MegaApi* api, const char* path, int result)
{
    onMountEvent("Enabled", "enable", path, result);
}

void MegaCmdMegaListener::onMountDisabled(mega::MegaApi* api, const char* path, int result)
{
    onMountEvent("Disabled", "disable", path, result);
}

void MegaCmdMegaListener::onMountEvent(std::string_view pastTense, std::string_view presentTense, std::string_view path, int result)
{
    if (result != MegaMount::SUCCESS)
    {
        std::ostringstream oss;
        oss << "Failed to " << presentTense << " mount \"" << path << "\" "
            << "due to error:\n" << getMountResultStr(result);
        assert(getMountResultStr(result) != "Mount was successful");

        const std::string msg = oss.str();

        LOG_err << msg;
        if (DisableMountErrorsBroadcastingGuard::shouldBroadcast())
        {
            broadcastDelayedMessage(msg, true);
        }
        return;
    }

    LOG_debug << pastTense << " mount \"" << path << "\"";
}

////////////////////////////////////////
///      MegaCmdListener methods     ///
////////////////////////////////////////

void MegaCmdListener::onRequestStart(MegaApi* api, MegaRequest *request)
{
    if (!request)
    {
        LOG_err << " onRequestStart for undefined request ";
        return;
    }

    LOG_verbose << "onRequestStart request->getType(): " << request->getType();
}

void MegaCmdListener::doOnRequestFinish(MegaApi* api, MegaRequest *request, MegaError* e)
{
    if (!request)
    {
        LOG_err << " onRequestFinish for undefined request ";
        return;
    }

    LOG_verbose << "onRequestFinish request->getType(): " << request->getType();

    switch (request->getType())
    {
        case MegaRequest::TYPE_FETCH_NODES:
        {
            informProgressUpdate(PROGRESS_COMPLETE, request->getTotalBytes(), this->clientID, "Fetching nodes");
            break;
        }

        default:
            break;
    }
}

void MegaCmdListener::onRequestUpdate(MegaApi* api, MegaRequest *request)
{
    if (!request)
    {
        LOG_err << " onRequestUpdate for undefined request ";
        return;
    }

    LOG_verbose << "onRequestUpdate request->getType(): " << request->getType();

    switch (request->getType())
    {
        case MegaRequest::TYPE_FETCH_NODES:
        {
#ifdef MEGACMD_TESTING_CODE
        TestInstruments::Instance().fireEvent(TestInstruments::Event::FETCH_NODES_REQ_UPDATE);
#endif
            unsigned int cols = getNumberOfCols(80);
            string outputString;
            outputString.resize(cols+1);
            for (unsigned int i = 0; i < cols; i++)
            {
                outputString[i] = '.';
            }

            outputString[cols] = '\0';
            char *ptr = (char *)outputString.c_str();
            sprintf(ptr, "%s", "Fetching nodes ||");
            ptr += strlen("Fetching nodes ||");
            *ptr = '.'; //replace \0 char


            float oldpercent = percentFetchnodes;
            if (request->getTotalBytes() == 0)
            {
                percentFetchnodes = 0;
            }
            else
            {
                percentFetchnodes = float(request->getTransferredBytes() * 1.0 / request->getTotalBytes() * 100.0);
            }
            if (alreadyFinished || ( ( percentFetchnodes == oldpercent ) && ( oldpercent != 0 )) )
            {
                return;
            }
            if (percentFetchnodes < 0)
            {
                percentFetchnodes = 0;
            }

            char aux[40];
            if (request->getTotalBytes() < 0)
            {
                return;                         // after a 100% this happens
            }
            if (request->getTransferredBytes() < 0.001 * request->getTotalBytes())
            {
                return;                                                            // after a 100% this happens
            }

            if (request->getTotalBytes() < 1048576)
            {
                sprintf(aux,"||(%lld/%lld KB: %.2f %%) ", request->getTransferredBytes() / 1024, request->getTotalBytes() / 1024, percentFetchnodes);
            }
            else
            {
                sprintf(aux,"||(%lld/%lld MB: %.2f %%) ", request->getTransferredBytes() / 1024 / 1024, request->getTotalBytes() / 1024 / 1024, percentFetchnodes);
            }

            sprintf((char *)outputString.c_str() + cols - strlen(aux), "%s",                         aux);
            for (int i = 0; i <= ( cols - strlen("Fetching nodes ||") - strlen(aux)) * 1.0 * percentFetchnodes / 100.0; i++)
            {
                *ptr++ = '#';
            }

            if (percentFetchnodes == 100 && !alreadyFinished)
            {
                alreadyFinished = true;
                //cout << outputString << endl;
                LOG_debug << outputString;
            }
            else
            {
                LOG_debug << outputString;
                //cout << outputString << '\r' << flush;
            }

            informProgressUpdate(request->getTransferredBytes(), request->getTotalBytes(), this->clientID, "Fetching nodes");


            break;
        }

        default:
            LOG_debug << "onRequestUpdate of unregistered type of request: " << request->getType();
            break;
    }
}

void MegaCmdListener::onRequestTemporaryError(MegaApi *api, MegaRequest *request, MegaError* e)
{
}


MegaCmdListener::~MegaCmdListener()
{
}

MegaCmdListener::MegaCmdListener(MegaApi *megaApi, MegaRequestListener *listener, int clientID)
{
    this->megaApi = megaApi;
    this->listener = listener;
    percentFetchnodes = 0.0f;
    alreadyFinished = false;
    this->clientID = clientID;
}


////////////////////////////////////////////////
///      MegaCmdTransferListener methods     ///
////////////////////////////////////////////////

void MegaCmdTransferListener::onTransferStart(MegaApi* api, MegaTransfer *transfer)
{
    if (listener)
    {
        listener->onTransferStart(api,transfer);
    }
    if (!transfer)
    {
        LOG_err << " onTransferStart for undefined Transfer ";
        return;
    }

    LOG_verbose << "onTransferStart Transfer->getType(): " << transfer->getType();
}

void MegaCmdTransferListener::doOnTransferFinish(MegaApi* api, MegaTransfer *transfer, MegaError* e)
{
    if (listener)
    {
        listener->onTransferFinish(api,transfer,e);
    }
    if (!transfer)
    {
        LOG_err << " onTransferFinish for undefined transfer ";
        return;
    }

    LOG_verbose << "doOnTransferFinish Transfer->getType(): " << transfer->getType();
    informProgressUpdate(PROGRESS_COMPLETE, transfer->getTotalBytes(), clientID);

}


void MegaCmdTransferListener::onTransferUpdate(MegaApi* api, MegaTransfer *transfer)
{
    if (listener)
    {
        listener->onTransferUpdate(api,transfer);
    }

    if (!transfer)
    {
        LOG_err << " onTransferUpdate for undefined Transfer ";
        return;
    }

    unsigned int cols = getNumberOfCols(80);

    string outputString;
    outputString.resize(cols + 1);
    for (unsigned int i = 0; i < cols; i++)
    {
        outputString[i] = '.';
    }

    outputString[cols] = '\0';
    char *ptr = (char *)outputString.c_str();
    sprintf(ptr, "%s", "TRANSFERRING ||");
    ptr += strlen("TRANSFERRING ||");
    *ptr = '.'; //replace \0 char


    float oldpercent = percentDownloaded;
    if (transfer->getTotalBytes() == 0)
    {
        percentDownloaded = 0;
    }
    else
    {
        percentDownloaded = float(transfer->getTransferredBytes() * 1.0 / transfer->getTotalBytes() * 100.0);
    }
    if (alreadyFinished || ( (percentDownloaded == oldpercent ) && ( oldpercent != 0 ) ) )
    {
        return;
    }
    if (percentDownloaded < 0)
    {
        percentDownloaded = 0;
    }

    char aux[40];
    if (transfer->getTotalBytes() < 0)
    {
        return; // after a 100% this happens
    }
    if (transfer->getTransferredBytes() < 0.001 * transfer->getTotalBytes())
    {
        return; // after a 100% this happens
    }

    if (transfer->getTotalBytes() < 1048576)
    {
        sprintf(aux,"||(%lld/%lld KB: %.2f %%) ", (long long)(transfer->getTransferredBytes() / 1024), (long long)(transfer->getTotalBytes() / 1024), (float)percentDownloaded);

    }
    else
    {
        sprintf(aux,"||(%lld/%lld MB: %.2f %%) ", (long long)(transfer->getTransferredBytes() / 1024 / 1024), (long long)(transfer->getTotalBytes() / 1024 / 1024), (float)percentDownloaded);
    }
    sprintf((char *)outputString.c_str() + cols - strlen(aux), "%s",                         aux);
    for (int i = 0; i <= ( cols - strlen("TRANSFERRING ||") - strlen(aux)) * 1.0 * percentDownloaded / 100.0; i++)
    {
        *ptr++ = '#';
    }

    if (percentDownloaded == 100 && !alreadyFinished)
    {
        alreadyFinished = true;
        //cout << outputString << endl;
        LOG_debug << outputString;
    }
    else
    {
        LOG_debug << outputString;
        //cout << outputString << '\r' << flush;
    }

    LOG_verbose << "onTransferUpdate transfer->getType(): " << transfer->getType() << " clientID=" << this->clientID;

    informTransferUpdate(transfer, this->clientID);
}


void MegaCmdTransferListener::onTransferTemporaryError(MegaApi *api, MegaTransfer *transfer, MegaError* e)
{
    if (listener)
    {
        listener->onTransferTemporaryError(api,transfer,e);
    }
}


MegaCmdTransferListener::~MegaCmdTransferListener()
{

}

MegaCmdTransferListener::MegaCmdTransferListener(MegaApi *megaApi, MegaCmdSandbox *sandboxCMD, MegaTransferListener *listener, int clientID)
{
    this->megaApi = megaApi;
    this->sandboxCMD = sandboxCMD;
    this->listener = listener;
    percentDownloaded = 0.0f;
    alreadyFinished = false;
    this->clientID = clientID;
}

bool MegaCmdTransferListener::onTransferData(MegaApi *api, MegaTransfer *transfer, char *buffer, size_t size)
{
    return true;
}


/////////////////////////////////////////////////////
///      MegaCmdMultiTransferListener methods     ///
/////////////////////////////////////////////////////

void MegaCmdMultiTransferListener::onTransferStart(MegaApi* api, MegaTransfer *transfer)
{
    if (!transfer)
    {
        LOG_err << " onTransferStart for undefined Transfer ";
        return;
    }
    alreadyFinished = false;
    if (totalbytes == 0)
    {
        percentDownloaded = 0;
    }
    else
    {
        percentDownloaded = float((transferredbytes + getOngoingTransferredBytes()) * 1.0 / totalbytes * 1.0);
    }

    onTransferUpdate(api,transfer);

    LOG_verbose << "onTransferStart Transfer->getType(): " << transfer->getType();
}

void MegaCmdMultiTransferListener::doOnTransferFinish(MegaApi* api, MegaTransfer *transfer, MegaError* e)
{
    finished++;
    finalerror = (finalerror!=API_OK)?finalerror:e->getErrorCode();

    if (!transfer)
    {
        LOG_err << " onTransferFinish for undefined transfer ";
        return;
    }

    LOG_verbose << "doOnTransferFinish MegaCmdMultiTransferListener Transfer->getType(): " << transfer->getType() << " transferring " << transfer->getFileName();

    if (e->getErrorCode() == API_OK)
    {
        // communicate status info
        string s= "endtransfer:";
        s+=((transfer->getType() == MegaTransfer::TYPE_DOWNLOAD)?"D":"U");
        s+=":";
        if (transfer->getType() == MegaTransfer::TYPE_UPLOAD)
        {
            std::unique_ptr<MegaNode> n(api->getNodeByHandle(transfer->getNodeHandle()));
            if (n)
            {
                std::unique_ptr<char[]> path(api->getNodePath(n.get()));
                if (path)
                {
                    s += path.get();
                }
            }
        }
        else
        {
            s+=transfer->getPath();
        }
        informStateListenerByClientId(this->clientID, s);
    }

    map<int, long long>::iterator itr = ongoingtransferredbytes.find(transfer->getTag());
    if ( itr!= ongoingtransferredbytes.end())
    {
        ongoingtransferredbytes.erase(itr);
    }

    itr = ongoingtotalbytes.find(transfer->getTag());
    if ( itr!= ongoingtotalbytes.end())
    {
        ongoingtotalbytes.erase(itr);
    }

    transferredbytes+=transfer->getTransferredBytes();
    totalbytes+=transfer->getTotalBytes();
}

void MegaCmdMultiTransferListener::waitMultiEnd()
{
    for (unsigned i = 0; i < created; i++)
    {
        wait();
    }
}

void MegaCmdMultiTransferListener::waitMultiStart()
{
    std::unique_lock<std::mutex> lock(mStartedTransfersMutex);
    mStartedConditionVariable.wait(lock, [this]() {
            return (mStartedTransfersCount >= created);
    });
}


void MegaCmdMultiTransferListener::onTransferUpdate(MegaApi* api, MegaTransfer *transfer)
{
    if (!transfer)
    {
        LOG_err << " onTransferUpdate for undefined Transfer ";
        return;
    }
    ongoingtransferredbytes[transfer->getTag()] = transfer->getTransferredBytes();
    ongoingtotalbytes[transfer->getTag()] = transfer->getTotalBytes();

    unsigned int cols = getNumberOfCols(80);

    string outputString;
    outputString.resize(cols + 1);
    for (unsigned int i = 0; i < cols; i++)
    {
        outputString[i] = '.';
    }

    outputString[cols] = '\0';
    char *ptr = (char *)outputString.c_str();
    sprintf(ptr, "%s", "TRANSFERRING ||");
    ptr += strlen("TRANSFERRING ||");
    *ptr = '.'; //replace \0 char


    float oldpercent = percentDownloaded;
    if ((totalbytes + getOngoingTotalBytes() ) == 0)
    {
        percentDownloaded = 0;
    }
    else
    {
        percentDownloaded = float((transferredbytes + getOngoingTransferredBytes()) * 1.0 / (totalbytes + getOngoingTotalBytes() ) * 100.0);
    }
    if (alreadyFinished || ( (percentDownloaded == oldpercent ) && ( oldpercent != 0 ) ) )
    {
        return;
    }
    if (percentDownloaded < 0)
    {
        percentDownloaded = 0;
    }
    assert(percentDownloaded <=100);

    char aux[40];
    if (transfer->getTotalBytes() < 0)
    {
        return; // after a 100% this happens
    }
    if (transfer->getTransferredBytes() < 0.001 * transfer->getTotalBytes())
    {
        return; // after a 100% this happens
    }
    if (totalbytes + getOngoingTotalBytes() < 1048576)
    {
        sprintf(aux,"||(%lld/%lld KB: %.2f %%) ", (transferredbytes + getOngoingTransferredBytes()) / 1024, (totalbytes + getOngoingTotalBytes() ) / 1024, percentDownloaded);
    }
    else
    {
        sprintf(aux,"||(%lld/%lld MB: %.2f %%) ", (transferredbytes + getOngoingTransferredBytes()) / 1024 / 1024, (totalbytes + getOngoingTotalBytes() ) / 1024 / 1024, percentDownloaded);
    }
    sprintf((char *)outputString.c_str() + cols - strlen(aux), "%s",                         aux);
    for (int i = 0; i <= ( cols - strlen("TRANSFERRING ||") - strlen(aux)) * 1.0 * min (100.0f, percentDownloaded) / 100.0; i++)
    {
        *ptr++ = '#';
    }

    if (percentDownloaded == 100 && !alreadyFinished)
    {
        alreadyFinished = true;
        //cout << outputString << endl;
        LOG_debug << outputString;
    }
    else
    {
        LOG_debug << outputString;
        //cout << outputString << '\r' << flush;
    }

    LOG_verbose << "onTransferUpdate transfer->getType(): " << transfer->getType() << " clientID=" << this->clientID;

    informProgressUpdate((transferredbytes + getOngoingTransferredBytes()),(totalbytes + getOngoingTotalBytes() ), clientID);
    progressinformed = true;

}


void MegaCmdMultiTransferListener::onTransferTemporaryError(MegaApi *api, MegaTransfer *transfer, MegaError* e)
{
}


MegaCmdMultiTransferListener::~MegaCmdMultiTransferListener()
{
}

int MegaCmdMultiTransferListener::getFinalerror() const
{
    return finalerror;
}

long long MegaCmdMultiTransferListener::getTotalbytes() const
{
    return totalbytes;
}

long long MegaCmdMultiTransferListener::getOngoingTransferredBytes()
{
    long long total = 0;
    for (map<int, long long>::iterator itr = ongoingtransferredbytes.begin(); itr!= ongoingtransferredbytes.end(); itr++)
    {
        total += itr->second;
    }
    return total;
}

long long MegaCmdMultiTransferListener::getOngoingTotalBytes()
{
    long long total = 0;
    for (map<int, long long>::iterator itr = ongoingtotalbytes.begin(); itr!= ongoingtotalbytes.end(); itr++)
    {
        total += itr->second;
    }
    return total;
}

bool MegaCmdMultiTransferListener::getProgressinformed() const
{
    return progressinformed;
}

MegaCmdMultiTransferListener::MegaCmdMultiTransferListener(MegaApi *megaApi, MegaCmdSandbox *sandboxCMD, MegaTransferListener *listener, int clientID)
{
    this->megaApi = megaApi;
    this->sandboxCMD = sandboxCMD;
    this->listener = listener;
    percentDownloaded = 0.0f;
    alreadyFinished = false;
    this->clientID = clientID;

    created = 0;
    finished = 0;
    totalbytes = 0;
    transferredbytes = 0;

    progressinformed = false;

    finalerror = MegaError::API_OK;

}


bool MegaCmdMultiTransferListener::onTransferData(MegaApi *api, MegaTransfer *transfer, char *buffer, size_t size)
{
    return true;
}

void MegaCmdMultiTransferListener::onNewTransfer()
{
    std::lock_guard<std::mutex> g(mStartedTransfersMutex);
    created ++;
}

void MegaCmdMultiTransferListener::onTransferStarted(const std::string &path, int tag)
{
    {
        std::lock_guard<std::mutex> g(mStartedTransfersMutex);
        mStartedTransfersCount++;
        mStartedConditionVariable.notify_one();
    }
}

////////////////////////////////////////
///  MegaCmdGlobalTransferListener   ///
////////////////////////////////////////
const int MegaCmdGlobalTransferListener::MAXCOMPLETEDTRANSFERSBUFFER = 10000;

MegaCmdGlobalTransferListener::MegaCmdGlobalTransferListener(MegaApi *megaApi, MegaCmdSandbox *sandboxCMD, MegaTransferListener *parent)
{
    this->megaApi = megaApi;
    this->sandboxCMD = sandboxCMD;
    this->listener = parent;
}

void MegaCmdGlobalTransferListener::onTransferFinish(MegaApi* api, MegaTransfer *transfer, MegaError* error)
{
    completedTransfersMutex.lock();
    completedTransfers.push_front(transfer->copy());

    // source
    MegaNode * node = api->getNodeByHandle(transfer->getNodeHandle());
    if (node)
    {
        char * nodepath = api->getNodePath(node);
        completedPathsByHandle[transfer->getNodeHandle()]=nodepath;
        delete []nodepath;
        delete node;
    }

    if (completedTransfers.size()>MAXCOMPLETEDTRANSFERSBUFFER)
    {
        MegaTransfer * todelete = completedTransfers.back();
        completedPathsByHandle.erase(todelete->getNodeHandle()); //TODO: this can be potentially eliminate a handle that has been added twice
        delete todelete;
        completedTransfers.pop_back();
    }
    completedTransfersMutex.unlock();
}

void MegaCmdGlobalTransferListener::onTransferTemporaryError(MegaApi *api, MegaTransfer *transfer, MegaError* e)
{
    if (e && e->getErrorCode() == MegaError::API_EOVERQUOTA && e->getValue())
    {
        if (!sandboxCMD->isOverquota())
        {
            LOG_warn  << "Reached bandwidth quota. Your download could not proceed "
                         "because it would take you over the current free transfer allowance for your IP address. "
                         "This limit is dynamic and depends on the amount of unused bandwidth we have available. "
                         "You can change your account plan to increase such bandwidth. "
                         "See \"help --upgrade\" for further details";
        }
        sandboxCMD->setOverquota(true);
        sandboxCMD->timeOfOverquota = m_time(NULL);
        sandboxCMD->secondsOverQuota=e->getValue();
    }
}

bool MegaCmdGlobalTransferListener::onTransferData(MegaApi *api, MegaTransfer *transfer, char *buffer, size_t size) {return false;};

MegaCmdGlobalTransferListener::~MegaCmdGlobalTransferListener()
{
    completedTransfersMutex.lock();
    while (completedTransfers.size())
    {
        delete completedTransfers.front();
        completedTransfers.pop_front();
    }
    completedTransfersMutex.unlock();
}

bool MegaCmdCatTransferListener::onTransferData(MegaApi *api, MegaTransfer *transfer, char *buffer, size_t size)
{
    if (!ls->isClientConnected())
    {
        LOG_verbose << " CatTransfer listener, cancelled transfer due to client disconnected";
        api->cancelTransfer(transfer);
    }
    else
    {

        LOG_verbose << " CatTransfer listener, streaming " << size << " bytes";
        *ls << BinaryStringView(buffer, size);
    }

    return true;
}

ATransferListener::ATransferListener(const std::shared_ptr<MegaCmdMultiTransferListener> &mMultiTransferListener, const std::string &path)
    : mMultiTransferListener(mMultiTransferListener), mPath(path)
{
    assert(mMultiTransferListener);
}

ATransferListener::~ATransferListener()
{

}

void ATransferListener::onTransferStart(MegaApi *api, MegaTransfer *transfer)
{
    auto tag = transfer->getTag();
    mMultiTransferListener->onTransferStarted(mPath, tag);
    mMultiTransferListener->onTransferStart(api, transfer);
}

void ATransferListener::onTransferFinish(MegaApi *api, MegaTransfer *transfer, MegaError *e)
{
    static_cast<MegaTransferListener *>(mMultiTransferListener.get())->onTransferFinish(api, transfer, e);
    delete this;
}

void ATransferListener::onTransferUpdate(MegaApi *api, MegaTransfer *transfer)
{
    mMultiTransferListener->onTransferUpdate(api, transfer);
}

void ATransferListener::onTransferTemporaryError(MegaApi *api, MegaTransfer *transfer, MegaError *e)
{
    mMultiTransferListener->onTransferTemporaryError(api, transfer, e);
}

bool ATransferListener::onTransferData(MegaApi *api, MegaTransfer *transfer, char *buffer, size_t size)
{
    return mMultiTransferListener->onTransferData(api, transfer, buffer, size);
}

std::string_view MegaCmdFatalErrorListener::getFatalErrorStr(int64_t fatalErrorType)
{
    switch (fatalErrorType)
    {
        case MegaEvent::REASON_ERROR_UNKNOWN:                  return "REASON_ERROR_UNKNOWN";
        case MegaEvent::REASON_ERROR_NO_ERROR:                 return "REASON_ERROR_NO_ERROR";
        case MegaEvent::REASON_ERROR_FAILURE_UNSERIALIZE_NODE: return "REASON_ERROR_FAILURE_UNSERIALIZE_NODE";
        case MegaEvent::REASON_ERROR_DB_IO_FAILURE:            return "REASON_ERROR_DB_IO_FAILURE";
        case MegaEvent::REASON_ERROR_DB_FULL:                  return "REASON_ERROR_DB_FULL";
        case MegaEvent::REASON_ERROR_DB_INDEX_OVERFLOW:        return "REASON_ERROR_DB_INDEX_OVERFLOW";
        case MegaEvent::REASON_ERROR_NO_JSCD:                  return "REASON_ERROR_NO_JSCD";
        case MegaEvent::REASON_ERROR_REGENERATE_JSCD:          return "REASON_ERROR_REGENERATE_JSCD";
        default:                                               assert(false);
                                                               return "<unhandled fatal error>";
    }
}

template<bool localLogout>
MegaRequestListener* MegaCmdFatalErrorListener::createLogoutListener(std::string_view msg)
{
    return new MegaCmdListenerFuncExecuter(
        [this, msg] (mega::MegaApi *api, mega::MegaRequest *request, mega::MegaError *e)
        {
            broadcastMessage(std::string(msg));
            mCmdSandbox.cmdexecuter->actUponLogout(*api, e, localLogout);
        }, true /* autoremove */
    );
}

void MegaCmdFatalErrorListener::onEvent(mega::MegaApi *api, mega::MegaEvent *event)
{
    assert(api); assert(event);
    if (event->getType() != MegaEvent::EVENT_FATAL_ERROR)
    {
        return;
    }

    const int64_t fatalErrorType = event->getNumber();
    LOG_err << "Received fatal error " << getFatalErrorStr(fatalErrorType) << " (type: " << fatalErrorType << ")";

    switch (fatalErrorType)
    {
        case MegaEvent::REASON_ERROR_UNKNOWN:
        {
            broadcastMessage("An error is causing the communication with MEGA to fail. Your syncs and backups "
                             "are unable to update, and there may be further issues if you continue using MEGAcmd "
                             "without restarting. We strongly recommend immediately restarting the MEGAcmd server to "
                             "resolve this problem. If the issue persists, please contact support.");
            break;
        }
        case MegaEvent::REASON_ERROR_NO_ERROR:
        {
            break;
        }
        case MegaEvent::REASON_ERROR_FAILURE_UNSERIALIZE_NODE:
        {
            broadcastMessage("A serious issue has been detected in MEGAcmd, or in the connection between "
                             "this device and MEGA. Delete your local \".megaCmd\" folder and reinstall the app "
                             "from https://mega.io/cmd, or contact support for further assistance.");
            break;
        }
        case MegaEvent::REASON_ERROR_DB_IO_FAILURE:
        {
            std::string_view msg = "Critical system files which are required by MEGACmd are unable to be reached. "
                                   "Please check permissions in the \".megaCmd\" folder, or try restarting the "
                                   "MEGAcmd server. If the issue still persists, please contact support.";

            api->localLogout(createLogoutListener<true>(msg));
            break;
        }
        case MegaEvent::REASON_ERROR_DB_FULL:
        {
            std::string_view msg = "There's not enough space in your local storage to run MEGAcmd. Please make "
                                   "more space available before running MEGAcmd.";

            api->localLogout(createLogoutListener<true>(msg));
            break;
        }
        case MegaEvent::REASON_ERROR_DB_INDEX_OVERFLOW:
        {
            std::string_view msg = "MEGAcmd has detected a critical internal error and needs to reload. "
                                   "You've been logged out. If you experience this issue more than once, please contact support.";

            // According to the Confluence documentation on fatal errors, this should be an account reload
            // We'll do a logout instead to avoid problems with api folders
            api->logout(false, createLogoutListener<false>(msg));
            break;
        }
        case MegaEvent::REASON_ERROR_NO_JSCD:
        {
            broadcastMessage("MEGAcmd has detected an error in your sync configuration data. You need to manually "
                             "logout of MEGAcmd, and log back in, to resolve this issue. If the problem persists "
                             "afterwards, please contact support.");
            break;
        }
        case MegaEvent::REASON_ERROR_REGENERATE_JSCD:
        {
            broadcastMessage("MEGAcmd has detected an error in your sync data. Please, reconfigure your syncs now. "
                             "If the issue persists afterwards, please contact support.");
            break;
        }
        default:
        {
            LOG_err << "Unhandled fatal error type " << fatalErrorType;
            assert(false);
            break;
        }
    }
}

MegaCmdFatalErrorListener::MegaCmdFatalErrorListener(MegaCmdSandbox& cmdSandbox) :
    mCmdSandbox(cmdSandbox)
{
}

} //end namespace
