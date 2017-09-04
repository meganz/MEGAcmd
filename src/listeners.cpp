/**
 * @file examples/megacmd/listeners.cpp
 * @brief MEGAcmd: Listeners
 *
 * (c) 2013-2016 by Mega Limited, Auckland, New Zealand
 *
 * This file is part of the MEGA SDK - Client Access Engine.
 *
 * Applications using the MEGA API must present a valid application key
 * and comply with the the rules set forth in the Terms of Service.
 *
 * The MEGA SDK is distributed in the hope that it will be useful,
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

#define PROGRESS_COMPLETE -2

using namespace mega;

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
}

void MegaCmdGlobalListener::onNodesUpdate(MegaApi *api, MegaNodeList *nodes)
{
    int nfolders = 0;
    int nfiles = 0;
    int rfolders = 0;
    int rfiles = 0;
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
            int * nFolderFiles = getNumFolderFiles(nodeRoot, api);
            nfolders += nFolderFiles[0];
            nfiles += nFolderFiles[1];
            delete []nFolderFiles;
            delete nodeRoot;

            MegaNode * inboxNode = api->getInboxNode();
            nFolderFiles = getNumFolderFiles(inboxNode, api);
            nfolders += nFolderFiles[0];
            nfiles += nFolderFiles[1];
            delete []nFolderFiles;
            delete inboxNode;

            MegaNode * rubbishNode = api->getRubbishNode();
            nFolderFiles = getNumFolderFiles(rubbishNode, api);
            nfolders += nFolderFiles[0];
            nfiles += nFolderFiles[1];
            delete []nFolderFiles;
            delete rubbishNode;

            MegaNodeList *inshares = api->getInShares();
            if (inshares)
            {
                for (int i = 0; i < inshares->size(); i++)
                {
                    nfolders++; //add the share itself
                    nFolderFiles = getNumFolderFiles(inshares->get(i), api);
                    nfolders += nFolderFiles[0];
                    nfiles += nFolderFiles[1];
                    delete []nFolderFiles;
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


////////////////////////////////////////////
///      MegaCmdMegaListener methods     ///
////////////////////////////////////////////

void MegaCmdMegaListener::onRequestFinish(MegaApi *api, MegaRequest *request, MegaError *e)
{
    if (e && ( e->getErrorCode() == MegaError::API_ESID ))
    {
        LOG_err << "Session is no longer valid (it might have been invalidated from elsewhere) ";
        changeprompt(prompts[COMMAND]);
    }
}

MegaCmdMegaListener::MegaCmdMegaListener(MegaApi *megaApi, MegaListener *parent)
{
    this->megaApi = megaApi;
    this->listener = parent;
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

            for (itr = ConfigurationManager::configuredSyncs.begin(); itr != ConfigurationManager::configuredSyncs.end(); ++itr, i++)
            {
                sync_struct *oldsync = ((sync_struct*)( *itr ).second );
                sync_struct *thesync = new sync_struct;
                *thesync = *oldsync;

                MegaCmdListener *megaCmdListener = new MegaCmdListener(api, NULL);
                MegaNode * node = api->getNodeByHandle(thesync->handle);
                api->resumeSync(thesync->localpath.c_str(), node, thesync->fingerprint, megaCmdListener);
                megaCmdListener->wait();
                if (megaCmdListener->getError() && ( megaCmdListener->getError()->getErrorCode() == MegaError::API_OK ))
                {
                    thesync->fingerprint = megaCmdListener->getRequest()->getNumber();
                    thesync->active = true;

                    if (ConfigurationManager::loadedSyncs.find(thesync->localpath) != ConfigurationManager::loadedSyncs.end())
                    {
                        delete ConfigurationManager::loadedSyncs[thesync->localpath];
                    }
                    ConfigurationManager::loadedSyncs[thesync->localpath] = thesync;
                    char *nodepath = api->getNodePath(node);
                    LOG_info << "Loaded sync: " << thesync->localpath << " to " << nodepath;
                    delete []nodepath;
                }
                else
                {
                    delete thesync;
                }

                delete megaCmdListener;
                delete node;
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
            percentFetchnodes = request->getTransferredBytes() * 1.0 / request->getTotalBytes() * 100.0;
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
            sprintf(aux,"||(%lld/%lld MB: %.2f %%) ", request->getTransferredBytes() / 1024 / 1024, request->getTotalBytes() / 1024 / 1024, percentFetchnodes);
            sprintf((char *)outputString.c_str() + cols - strlen(aux), "%s",                         aux);
            for (int i = 0; i <= ( cols - strlen("Fetching nodes ||") - strlen(aux)) * 1.0 * percentFetchnodes / 100.0; i++)
            {
                *ptr++ = '#';
            }

            if (percentFetchnodes == 100 && !alreadyFinished)
            {
                alreadyFinished = true;
                cout << outputString << endl;
            }
            else
            {
                cout << outputString << '\r' << flush;
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


////////////////////////////////////////
///      MegaCmdListener methods     ///
////////////////////////////////////////

void MegaCmdTransferListener::onTransferStart(MegaApi* api, MegaTransfer *Transfer)
{
    if (!Transfer)
    {
        LOG_err << " onTransferStart for undefined Transfer ";
        return;
    }

    LOG_verbose << "onTransferStart Transfer->getType(): " << Transfer->getType();
}

void MegaCmdTransferListener::doOnTransferFinish(MegaApi* api, MegaTransfer *transfer, MegaError* e)
{
    if (!transfer)
    {
        LOG_err << " onTransferFinish for undefined transfer ";
        return;
    }

    LOG_verbose << "onTransferFinish Transfer->getType(): " << transfer->getType();
    informProgressUpdate(PROGRESS_COMPLETE, transfer->getTotalBytes(), clientID);

}


void MegaCmdTransferListener::onTransferUpdate(MegaApi* api, MegaTransfer *transfer)
{
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
    sprintf(ptr, "%s", "TRANSFERING ||");
    ptr += strlen("TRANSFERING ||");
    *ptr = '.'; //replace \0 char


    float oldpercent = percentDowloaded;
    percentDowloaded = transfer->getTransferredBytes() * 1.0 / transfer->getTotalBytes() * 100.0;
    if (alreadyFinished || ( ( percentDowloaded == oldpercent ) && ( oldpercent != 0 ) ) )
    {
        return;
    }
    if (percentDowloaded < 0)
    {
        percentDowloaded = 0;
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
    sprintf(aux,"||(%lld/%lld MB: %.2f %%) ", transfer->getTransferredBytes() / 1024 / 1024, transfer->getTotalBytes() / 1024 / 1024, percentDowloaded);
    sprintf((char *)outputString.c_str() + cols - strlen(aux), "%s",                         aux);
    for (int i = 0; i <= ( cols - strlen("TRANSFERING ||") - strlen(aux)) * 1.0 * percentDowloaded / 100.0; i++)
    {
        *ptr++ = '#';
    }

    if (percentDowloaded == 100 && !alreadyFinished)
    {
        alreadyFinished = true;
        cout << outputString << endl;
    }
    else
    {
        cout << outputString << '\r' << flush;
    }

    LOG_verbose << "onTransferUpdate transfer->getType(): " << transfer->getType() << " clientID=" << this->clientID;

    informTransferUpdate(transfer, this->clientID);
}


void MegaCmdTransferListener::onTransferTemporaryError(MegaApi *api, MegaTransfer *transfer, MegaError* e)
{
}


MegaCmdTransferListener::~MegaCmdTransferListener()
{

}

MegaCmdTransferListener::MegaCmdTransferListener(MegaApi *megaApi, MegaCmdSandbox *sandboxCMD, MegaTransferListener *listener, int clientID)
{
    this->megaApi = megaApi;
    this->sandboxCMD = sandboxCMD;
    this->listener = listener;
    percentDowloaded = 0.0f;
    alreadyFinished = false;
    this->clientID = clientID;
}

bool MegaCmdTransferListener::onTransferData(MegaApi *api, MegaTransfer *transfer, char *buffer, size_t size)
{
    return true;
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
    completedTransfersMutex.init(false);
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

    if (e && e->getErrorCode() == MegaError::API_EOVERQUOTA)
    {
        if (!sandboxCMD->isOverquota())
        {
            LOG_warn  << "Reached bandwidth quota. Your download could not proceed "
                         "because it would take you over the current free transfer allowance for your IP address. "
                         "This limit is dynamic and depends on the amount of unused bandwidth we have available. "
                         "You can change your account plan to increse such bandwidth. "
                         "See \"help --upgrade\" for further details";
        }
        sandboxCMD->setOverquota(true);
        sandboxCMD->timeOfOverquota=time(NULL);
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

