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

using namespace mega;


namespace megacmd {
#ifdef ENABLE_CHAT
void MegaCmdGlobalListener::onChatsUpdate(MegaApi*, MegaTextChatList*)
{

}
#endif

void MegaCmdGlobalListener::onUsersUpdate(MegaApi *api, MegaUserList *users)
{
    if (users)
    {
        if (users->size() == 1)
        {
            LOG_debug << " 1 user received or updated";
        }
        else
        {
            LOG_debug << users->size() << " users received or updated";
        }
    }
    else //initial update or too many changes
    {
        MegaUserList *users = api->getContacts();

        if (users && users->size())
        {
            if (users->size() == 1)
            {
                LOG_debug << " 1 user received or updated";
            }
            else
            {
                LOG_debug << users->size() << " users received or updated";
            }

            delete users;
        }
    }
}

MegaCmdGlobalListener::MegaCmdGlobalListener(MegaCMDLogger *logger, MegaCmdSandbox *sandboxCMD)
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
    }
    else //initial update or too many changes
    {
        if (loggerCMD->getMaxLogLevel() >= logInfo)
        {
            MegaNode * nodeRoot = api->getRootNode();
            getNumFolderFiles(nodeRoot, api, &nfiles, &nfolders);
            delete nodeRoot;

            MegaNode * inboxNode = api->getInboxNode();
            getNumFolderFiles(inboxNode, api, &nfiles, &nfolders);
            delete inboxNode;

            MegaNode * rubbishNode = api->getRubbishNode();
            getNumFolderFiles(rubbishNode, api, &nfiles, &nfolders);
            delete rubbishNode;

            MegaNodeList *inshares = api->getInShares();
            if (inshares)
            {
                for (int i = 0; i < inshares->size(); i++)
                {
                    nfolders++; //add the share itself
                    getNumFolderFiles(inshares->get(i), api, &nfiles, &nfolders);
                }
            }
            delete inshares;
        }

        if (nfolders)
        {
            LOG_debug << nfolders << " folders " << "added or updated ";
        }
        if (nfiles)
        {
            LOG_debug << nfiles << " files " << "added or updated ";
        }
        if (rfolders)
        {
            LOG_debug << rfolders << " folders " << "removed";
        }
        if (rfiles)
        {
            LOG_debug << rfiles << " files " << "removed";
        }
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
        setBlocked(event->getNumber()); //this should be true always

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
            sandboxCMD->storageStatus = event->getNumber();
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
                            if (e->getValue() == MegaError::API_OK)
                            {
                                std::unique_ptr<char[]> myEmail(api->getMyEmail());
                                std::unique_ptr<MegaIntegerList> warningsList(api->getOverquotaWarningsTs());
                                std::string s;
                                s += "We have contacted you by email to " + string(myEmail.get()) + " on ";
                                s += getReadableTime(warningsList->get(0),"%b %e %Y");
                                if (warningsList->size() > 1)
                                {
                                    for (int i = 1; i < warningsList->size() - 1; i++)
                                    {
                                        s += ", " + getReadableTime(warningsList->get(i),"%b %e %Y");
                                    }
                                    s += " and " + getReadableTime(warningsList->get(warningsList->size() - 1),"%b %e %Y");
                                }
                                std::unique_ptr<MegaNode> rootNode(api->getRootNode());
                                long long totalFiles = 0;
                                long long totalFolders = 0;
                                getNumFolderFiles(rootNode.get(),api,&totalFiles,&totalFolders);
                                s += ", but you still have " + std::to_string(totalFiles) + " files taking up " + sizeToText(sandboxCMD->receivedStorageSum);
                                s += " in your MEGA account, which requires you to upgrade your account.\n\n";
                                long long daysLeft = (api->getOverquotaDeadlineTs() - m_time(NULL)) / 86400;
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
                            }
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

#ifdef ENABLE_BACKUPS
//backup callbacks:
void MegaCmdMegaListener::onBackupStateChanged(MegaApi *api,  MegaBackup *backup)
{
    LOG_verbose << " At onBackupStateChanged: " << backupSatetStr(backup->getState());
}

void MegaCmdMegaListener::onBackupStart(MegaApi *api, MegaBackup *backup)
{
    LOG_verbose << " At onBackupStart";
}

void MegaCmdMegaListener::onBackupFinish(MegaApi* api, MegaBackup *backup, MegaError* error)
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

void MegaCmdMegaListener::onBackupUpdate(MegaApi *api, MegaBackup *backup)
{
    LOG_verbose << " At onBackupUpdate";
}

void MegaCmdMegaListener::onBackupTemporaryError(MegaApi *api, MegaBackup *backup, MegaError* error)
{
    LOG_verbose << " At onBackupTemporaryError";
    if (error->getErrorCode() != MegaError::API_OK)
    {
        LOG_err << "Backup temporary error: " << error->getErrorString();
    }
}
#endif
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
            map<string, sync_struct *>::iterator itr;
            int i = 0;
#ifdef ENABLE_SYNC

            std::shared_ptr<std::lock_guard<std::recursive_mutex>> g = std::make_shared<std::lock_guard<std::recursive_mutex>>(ConfigurationManager::settingsMutex);
            // shared pointed lock_guard. will be freed when all resuming are complete

            for (itr = ConfigurationManager::configuredSyncs.begin(); itr != ConfigurationManager::configuredSyncs.end(); ++itr, i++)
            {
                sync_struct *oldsync = ((sync_struct*)( *itr ).second );

                MegaNode * node = api->getNodeByHandle(oldsync->handle);
                api->resumeSync(oldsync->localpath.c_str(), node, oldsync->fingerprint, new MegaCmdListenerFuncExecuter([g, oldsync, node](mega::MegaApi* api, mega::MegaRequest *request, mega::MegaError *e)
                {
                    std::unique_ptr<char []>nodepath (api->getNodePath(node));

                    if ( e->getErrorCode() == MegaError::API_OK )
                    {
                        if (request->getNumber())
                        {
                            oldsync->fingerprint = request->getNumber();
                        }
                        oldsync->active = true;
                        oldsync->loadedok = true;

                        LOG_info << "Loaded sync: " << oldsync->localpath << " to " << nodepath.get();
                    }
                    else
                    {
                        oldsync->loadedok = false;
                        oldsync->active = false;

                        LOG_err << "Failed to resume sync: " << oldsync->localpath << " to " << nodepath.get();
                    }
                    delete node;
                }, true));
            }
#endif
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
            MegaNode *n = api->getNodeByHandle(transfer->getNodeHandle());
            if (n)
            {
                const char *path = api->getNodePath(n);
                if (path)
                {
                    s+=path;
                }
                delete [] path;
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
    for (int i=0; i < started; i++)
    {
        wait();
    }
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

    started = 0;
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
    started ++;
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
};

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

void MegaCmdGlobalTransferListener::onTransferStart(MegaApi* api, MegaTransfer *transfer) {};
void MegaCmdGlobalTransferListener::onTransferUpdate(MegaApi* api, MegaTransfer *transfer) {};
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
};

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
        LOG_debug << " CatTransfer listener, cancelled transfer due to client disconnected";
        api->cancelTransfer(transfer);
    }
    else
    {

        LOG_debug << " CatTransfer listener, streaming " << size << " bytes";  //TODO: verbose
        *ls << string(buffer,size);
    }

    return true;
}
} //end namespace
