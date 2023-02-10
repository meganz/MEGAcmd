/**
 * @file src/listeners.h
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

#ifndef LISTENERS_H
#define LISTENERS_H

#include "megacmdlogger.h"
#include "megacmdsandbox.h"
#include "megacmdtransfermanager.h"


namespace megacmd {
class MegaCmdSandbox;

class MegaCmdListener : public mega::SynchronousRequestListener
{
private:
    float percentFetchnodes;
    bool alreadyFinished;

public:
    int clientID;

    MegaCmdListener(mega::MegaApi *megaApi, mega::MegaRequestListener *listener = NULL, int clientID=-1);
    virtual ~MegaCmdListener();

    //Request callbacks
    virtual void onRequestStart(mega::MegaApi* api, mega::MegaRequest *request);
    virtual void doOnRequestFinish(mega::MegaApi* api, mega::MegaRequest *request, mega::MegaError* error);
    virtual void onRequestUpdate(mega::MegaApi* api, mega::MegaRequest *request);
    virtual void onRequestTemporaryError(mega::MegaApi *api, mega::MegaRequest *request, mega::MegaError* e);

protected:
    mega::MegaRequestListener *listener;
};


/**
 * @brief The MegaCmdListenerFuncExecuter class
 *
 * it takes an std::function as parameter that will be called upon request finish.
 *
 */
class MegaCmdListenerFuncExecuter : public mega::MegaRequestListener
{
private:
    std::function<void(mega::MegaApi* api, mega::MegaRequest *request, mega::MegaError *e)> onRequestFinishCallback;
    bool mAutoremove = true;

public:

    /**
     * @brief MegaCmdListenerFuncExecuter
     * @param func to call upon onRequestFinish
     * @param autoremove whether this should be deleted after func is called
     */
    MegaCmdListenerFuncExecuter(std::function<void(mega::MegaApi* api, mega::MegaRequest *request, mega::MegaError *e)> func, bool autoremove = false)
    {
        onRequestFinishCallback = std::move(func);
        mAutoremove = autoremove;
    }

    void onRequestFinish(mega::MegaApi *api, mega::MegaRequest *request, mega::MegaError *e)
    {
        onRequestFinishCallback(api, request, e);

        if (mAutoremove)
        {
            delete this;
        }
    }
    virtual void onRequestStart(mega::MegaApi* api, mega::MegaRequest *request) {}
    virtual void onRequestUpdate(mega::MegaApi* api, mega::MegaRequest *request) {}
    virtual void onRequestTemporaryError(mega::MegaApi *api, mega::MegaRequest *request, mega::MegaError* e) {}
};

class MegaCmdTransferListener : public mega::SynchronousTransferListener
{
private:
    MegaCmdSandbox * sandboxCMD;
    float percentDownloaded;
    bool alreadyFinished;
    int clientID;

public:
    MegaCmdTransferListener(mega::MegaApi *megaApi, MegaCmdSandbox * sandboxCMD, mega::MegaTransferListener *listener = NULL, int clientID=-1);
    virtual ~MegaCmdTransferListener();

    //Transfer callbacks
    virtual void onTransferStart(mega::MegaApi* api, mega::MegaTransfer *transfer);
    virtual void doOnTransferFinish(mega::MegaApi* api, mega::MegaTransfer *transfer, mega::MegaError* e);
    virtual void onTransferUpdate(mega::MegaApi* api, mega::MegaTransfer *transfer);
    virtual void onTransferTemporaryError(mega::MegaApi *api, mega::MegaTransfer *transfer, mega::MegaError* e);
    virtual bool onTransferData(mega::MegaApi *api, mega::MegaTransfer *transfer, char *buffer, size_t size);

protected:
    mega::MegaTransferListener *listener;
};

class MegaCmdCatTransferListener : public MegaCmdTransferListener
{
private:
    LoggedStream *ls;
public:
    std::string contents;

    MegaCmdCatTransferListener(LoggedStream *_ls, mega::MegaApi *megaApi, MegaCmdSandbox * sandboxCMD, mega::MegaTransferListener *listener = NULL, int clientID=-1)
        :MegaCmdTransferListener(megaApi,sandboxCMD,listener,clientID),ls(_ls){};

    bool onTransferData(mega::MegaApi *api, mega::MegaTransfer *transfer, char *buffer, size_t size);
};

class MegaCmdMultiTransferListener : public mega::SynchronousTransferListener
{
private:

    MegaCmdSandbox * sandboxCMD;
    float percentDownloaded;
    bool alreadyFinished;
    int clientID;
    unsigned created;
    int finished;
    long long transferredbytes;
    std::map<int, long long> ongoingtransferredbytes;
    std::map<int, long long> ongoingtotalbytes;
    long long totalbytes;
    int finalerror;

    long long getOngoingTransferredBytes();
    long long getOngoingTotalBytes();

    bool progressinformed;

    std::mutex mStartedTransfersMutex; //to protect mStartedTransfers
    std::condition_variable mStartedConditionVariable;
    unsigned mStartedTransfersCount = 0;

#ifdef HAVE_DOWNLOADS_COMMAND
    // string to tag map
    std::vector<DownloadId> mStartedTransfers;
#endif
public:
    MegaCmdMultiTransferListener(mega::MegaApi *megaApi, MegaCmdSandbox * sandboxCMD, mega::MegaTransferListener *listener = NULL, int clientID=-1);
    virtual ~MegaCmdMultiTransferListener();

    //Transfer callbacks
    virtual void onTransferStart(mega::MegaApi* api, mega::MegaTransfer *transfer);
    virtual void doOnTransferFinish(mega::MegaApi* api, mega::MegaTransfer *transfer, mega::MegaError* e);
    virtual void onTransferUpdate(mega::MegaApi* api, mega::MegaTransfer *transfer);
    virtual void onTransferTemporaryError(mega::MegaApi *api, mega::MegaTransfer *transfer, mega::MegaError* e);
    virtual bool onTransferData(mega::MegaApi *api, mega::MegaTransfer *transfer, char *buffer, size_t size);

    void onNewTransfer();

    void onTransferStarted(const std::string &path, int tag);

    void waitMultiEnd();

    // waits untill all transfers have started
    void waitMultiStart();

    int getFinalerror() const;

    long long getTotalbytes() const;

    bool getProgressinformed() const;

#ifdef HAVE_DOWNLOADS_COMMAND
    std::vector<DownloadId> getStartedTransfers() const;
#endif

protected:
    mega::MegaTransferListener *listener;
};

/**
 * @brief TODO: document and rename
 * Note: self destructive
 */
class ATransferListener : public mega::MegaTransferListener
{
private:
    std::shared_ptr<MegaCmdMultiTransferListener> mMultiTransferListener;  //the listener this belongs too
    const std::string mPath; //The path that originated the transfer

public:
    ATransferListener(const std::shared_ptr<MegaCmdMultiTransferListener> &mMultiTransferListener, const std::string &path);
    virtual ~ATransferListener();

    //Transfer callbacks
    virtual void onTransferStart(mega::MegaApi* api, mega::MegaTransfer *transfer);
    virtual void onTransferFinish(mega::MegaApi* api, mega::MegaTransfer *transfer, mega::MegaError* e);
    virtual void onTransferUpdate(mega::MegaApi* api, mega::MegaTransfer *transfer);
    virtual void onTransferTemporaryError(mega::MegaApi *api, mega::MegaTransfer *transfer, mega::MegaError* e);
    virtual bool onTransferData(mega::MegaApi *api, mega::MegaTransfer *transfer, char *buffer, size_t size);
};


class MegaCmdGlobalListener : public mega::MegaGlobalListener
{
private:
    MegaCMDLogger *loggerCMD;
    MegaCmdSandbox *sandboxCMD;

    std::atomic_bool ongoing;

public:
    MegaCmdGlobalListener(MegaCMDLogger *logger, MegaCmdSandbox *sandboxCMD);
    void onNodesUpdate(mega::MegaApi* api, mega::MegaNodeList *nodes);
    void onUsersUpdate(mega::MegaApi* api, mega::MegaUserList *users);
    void onAccountUpdate(mega::MegaApi *api);
    void onEvent(mega::MegaApi *api, mega::MegaEvent *event);
#ifdef ENABLE_CHAT
    void onChatsUpdate(mega::MegaApi*, mega::MegaTextChatList*);
#endif
};


class MegaCmdMegaListener : public mega::MegaListener
{

public:
    MegaCmdMegaListener(mega::MegaApi *megaApi, mega::MegaListener *parent=NULL, MegaCmdSandbox *sandboxCMD = NULL);
    virtual ~MegaCmdMegaListener();

    virtual void onRequestFinish(mega::MegaApi* api, mega::MegaRequest *request, mega::MegaError* e);

#ifdef ENABLE_CHAT
    void onChatsUpdate(mega::MegaApi *api, mega::MegaTextChatList *chats);
#endif

    virtual void onBackupStateChanged(mega::MegaApi *api,  mega::MegaScheduledCopy *backup);
    virtual void onBackupStart(mega::MegaApi *api, mega::MegaScheduledCopy *backup);
    virtual void onBackupFinish(mega::MegaApi* api, mega::MegaScheduledCopy *backup, mega::MegaError* error);
    virtual void onBackupUpdate(mega::MegaApi *api, mega::MegaScheduledCopy *backup);
    virtual void onBackupTemporaryError(mega::MegaApi *api, mega::MegaScheduledCopy *backup, mega::MegaError* error);

    void onMountAdded(mega::MegaApi* api,
                      const mega::MegaMount* mount,
                      int result) override;

    void onMountChanged(mega::MegaApi* api,
                        const mega::MegaMount* mount,
                        int result) override;

    void onMountDisabled(mega::MegaApi* api,
                         const mega::MegaMount* mount,
                         int result) override;

    void onMountEnabled(mega::MegaApi* api,
                        const mega::MegaMount* mount,
                        int result) override;

    void onMountRemoved(mega::MegaApi* api,
                        const mega::MegaMount* mount,
                        int result) override;

    void onSyncAdded(mega::MegaApi *api, mega::MegaSync *sync) override;
    void onSyncStateChanged(mega::MegaApi *api, mega::MegaSync *sync) override;
    void onSyncDeleted(mega::MegaApi *api, mega::MegaSync *sync) override;

protected:
    void onMountEvent(mega::MegaApi* api,
                      const mega::MegaMount* mount,
                      const char* pastTense,
                      const char* presentTense,
                      int result);

    mega::MegaApi *megaApi;
    mega::MegaListener *listener;
    MegaCmdSandbox *sandboxCMD;
};

class MegaCmdGlobalTransferListener : public mega::MegaTransferListener
{
private:
    MegaCmdSandbox *sandboxCMD;
    static const int MAXCOMPLETEDTRANSFERSBUFFER;

public:
    std::mutex completedTransfersMutex;
    std::deque<mega::MegaTransfer *> completedTransfers;
    std::map<mega::MegaHandle,std::string> completedPathsByHandle;
public:
    MegaCmdGlobalTransferListener(mega::MegaApi *megaApi, MegaCmdSandbox *sandboxCMD, mega::MegaTransferListener *parent = NULL);
    virtual ~MegaCmdGlobalTransferListener();

    //Transfer callbacks
    void onTransferStart(mega::MegaApi* api, mega::MegaTransfer *transfer);
    void onTransferFinish(mega::MegaApi* api, mega::MegaTransfer *transfer, mega::MegaError* error);

    void onTransferUpdate(mega::MegaApi* api, mega::MegaTransfer *transfer);
    void onTransferTemporaryError(mega::MegaApi *api, mega::MegaTransfer *transfer, mega::MegaError* e);
    bool onTransferData(mega::MegaApi *api, mega::MegaTransfer *transfer, char *buffer, size_t size);


protected:
    mega::MegaApi *megaApi;
    mega::MegaTransferListener *listener;
};

} //end namespace
#endif // LISTENERS_H
