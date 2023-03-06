/**
 * @file src/megacmdtransfermanager.cpp
 * @brief MEGAcmd: Executer of the commands
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
#ifdef HAVE_DOWNLOADS_COMMAND

#include "megacmdtransfermanager.h"
#include "configurationmanager.h"
#include "megacmd.h"
#include "megacmdutils.h"
#include "megacmdlogger.h"


#include <memory>
#include <utility>


using namespace mega;
using std::map;
using std::string;


namespace megacmd {


void printTransferColumnDisplayer(ColumnDisplayer *cd, MegaTransfer *transfer, const std::string &source, bool printstate)
{
    //Direction
    string type;
#ifdef _WIN32
    type += getutf8fromUtf16((transfer->getType() == MegaTransfer::TYPE_DOWNLOAD)?L"\u25bc":L"\u25b2");
#else
    type += (transfer->getType() == MegaTransfer::TYPE_DOWNLOAD)?"\u21d3":"\u21d1";
#endif
    //TODO: handle TYPE_LOCAL_TCP_DOWNLOAD

    //type (transfer/normal)
    if (transfer->isSyncTransfer())
    {
#ifdef _WIN32
        type += getutf8fromUtf16(L"\u21a8");
#else
        type += "\u21f5";
#endif
    }
#ifdef ENABLE_BACKUPS
    else if (transfer->isBackupTransfer())
    {
#ifdef _WIN32
        type += getutf8fromUtf16(L"\u2191");
#else
        type += "\u23eb";
#endif
    }
#endif

    cd->addValue("TYPE",type);
    cd->addValue("TAG", std::to_string(transfer->getTag()));

    if (transfer->getType() == MegaTransfer::TYPE_DOWNLOAD)
    {
        // source
        cd->addValue("SOURCEPATH", source);

        //destination
        string dest = transfer->getParentPath() ? transfer->getParentPath() : "";
        if (transfer->getFileName())
        {
            dest.append(transfer->getFileName());
        }
        else
        {
            dest.append("MISSINGFILENAME");
        }
        cd->addValue("DESTINYPATH",dest);
    }
    else
    {
        //source
        string uploadsource(transfer->getParentPath()?transfer->getParentPath():"");
        uploadsource.append(transfer->getFileName());

        cd->addValue("SOURCEPATH",uploadsource);

        //destination
        assert(false && "destination not available in global function");
//        MegaNode * parentNode = api->getNodeByHandle(transfer->getParentHandle());
//        if (parentNode)
//        {
//            char * parentnodepath = api->getNodePath(parentNode);
//            cd->addValue("DESTINYPATH",parentnodepath);
//            delete []parentnodepath;

//            delete parentNode;
//        }
//        else
//        {
//            cd->addValue("DESTINYPATH","---------");

//            LOG_warn << "Could not find destination (parent handle "<< ((transfer->getParentHandle()==INVALID_HANDLE)?" invalid":" valid")
//                     <<" ) for upload transfer. Source=" << transfer->getParentPath() << transfer->getFileName();
//        }
    }

    //progress
    float percent;
    if (transfer->getTotalBytes() == 0)
    {
        percent = 0;
    }
    else
    {
        percent = float(transfer->getTransferredBytes()*1.0/transfer->getTotalBytes());
    }

    stringstream osspercent;
    osspercent << percentageToText(percent) << " of " << getFixLengthString(sizeToText(transfer->getTotalBytes()),10,' ',true);
    cd->addValue("PROGRESS",osspercent.str());

    //state
    if (printstate)
    {
        cd->addValue("STATE",getTransferStateStr(transfer->getState()));
    }

    cd->addValue("TRANSFERRED", std::to_string(transfer->getTransferredBytes()));
    cd->addValue("TOTAL", std::to_string(transfer->getTotalBytes()));

}


DownloadsManager &DownloadsManager::Instance()
{
    static DownloadsManager myInstance;
    return myInstance;
}

void DownloadsManager::start()
{
    mMaxAllowedTransfer = ConfigurationManager::getConfigurationValue("downloads_tracking_max_finished_in_memory_high_threshold", 40000u);
    mLowthresholdMaxAllowedTransfers = ConfigurationManager::getConfigurationValue("downloads_tracking_max_finished_in_memory_low_threshold", 20000u);

    TransferInfoIOWriter::Instance().start();
}


void DownloadsManager::shutdown(bool loginout)
{
    TransferInfoIOWriter::Instance().shutdown(loginout);

    std::lock_guard<std::recursive_mutex> g(mTransfersMutex);

    mFinishedTransfers.clear();
    mActiveTransfers.clear();
    mTransfersInMemory.clear();
    mTransfers.clear();
}

void DownloadsManager::recoverUnHandleTransfer(MegaApi *api, MegaTransfer *transfer)
{
    auto tag = transfer->getTag();
    auto itUnhandled = mNewUnHandledTransfers.find(tag);
    assert (itUnhandled != mNewUnHandledTransfers.end());
    if (itUnhandled != mNewUnHandledTransfers.end())
    {
        // we don't know the path that originated the download
        // hence, we don't know the Object ID! Our best guess: use public handle
        // Note: this is not correct for subfiles/folders
        // TODO: if the tag were the same, we could look in our db for the object Id of the last transfer that had such tag
        //    but that's not guaranteed. Hence, there's not much we can do (unless we change the cached transfer db)
        string path;
        if (transfer->getPublicMegaNode())
        {
            path = handleToBase64(transfer->getPublicMegaNode()->getHandle());
        }
        else
        {
             path = transfer->getFileName();
        }

        DownloadsManager::Instance().addNewTopLevelTransfer(api, transfer, tag, path );

        mNewUnHandledTransfers.erase(itUnhandled);
    }
}


MapOfDlTransfers::iterator DownloadsManager::addNewTopLevelTransfer(MegaApi *api, MegaTransfer *transfer, int tag, const std::string &path)
{
    std::lock_guard<std::recursive_mutex> g(mTransfersMutex);

    DownloadId id(tag, path);
    auto transferInfo = std::make_shared<TransferInfo>(api, transfer, &id);
    transferInfo->updateTransfer(transfer, transferInfo); // we need to call this after creating a TransferInfo object, because IO writer  requires a shared_ptr, which is not available from ctor
    auto pair = mTransfers.emplace(id, transferInfo);

    assert(pair.second);

    mActiveTransfers.emplace(tag, pair.first);

    mTransfersInMemory[id.getObjectID()] = pair.first; //Insert / replace the existing value

    if (mTransfers.size() > mMaxAllowedTransfer)
    {
        while (!mFinishedTransfers.empty() && mTransfers.size() > mLowthresholdMaxAllowedTransfers)
        {
            auto finishedToDelete = mFinishedTransfersQueue.front();

            auto itInFinished = mFinishedTransfers.find(finishedToDelete);

            if (itInFinished != mFinishedTransfers.end())
            {
                auto itInGlobal = itInFinished->second;
                auto oId = itInFinished->second->first.getObjectID();
                auto itInMemory = mTransfersInMemory.find(oId);
                if (itInMemory != mTransfersInMemory.end() && itInGlobal == itInMemory->second)
                { //The one stored in memory map, is the one we are about to delete
                    mTransfersInMemory.erase(itInMemory);
                }

                //Finally remove the actual element from the real container
                mTransfers.erase(itInFinished->second);
            }

            mFinishedTransfers.erase(finishedToDelete);

            mFinishedTransfersQueue.pop();
        }
    }

    assert(pair.second);
    return pair.first;
}

void DownloadsManager::onTransferStart(MegaApi *api, MegaTransfer *transfer)
{
    std::lock_guard<std::recursive_mutex> g(mTransfersMutex);
    auto parentTag = transfer->getFolderTransferTag();
    if (parentTag > 0)
    {
        auto it = mActiveTransfers.find(parentTag);

        if (it != mActiveTransfers.end()) //found an active one
        {
            it->second->second->onSubTransferStarted(api, transfer);
        }
    }
    else //not a child! //already handled via addNewTopLevelTransfer...
    {
        auto tag = transfer->getTag();

        auto it = mActiveTransfers.find(tag);
        if (it == mActiveTransfers.end()) //i.e. not handled via addNewTopLevelTransfer (transfer resumption)
        {
            std::unique_ptr<MegaTransfer> t(transfer->copy());
            auto pair = std::make_pair<int, std::unique_ptr<MegaTransfer>>(std::move(tag), std::move(t));
            mNewUnHandledTransfers.insert(std::move(pair));
        }
    }
}

void DownloadsManager::onTransferUpdate(MegaApi *api, MegaTransfer *transfer)
{
    std::lock_guard<std::recursive_mutex> g(mTransfersMutex);
    auto parentTag = transfer->getFolderTransferTag();
    if (parentTag > 0)
    {
        auto it = mActiveTransfers.find(parentTag);

        if (it != mActiveTransfers.end()) //found an active one
        {
            it->second->second->onSubTransferUpdate(transfer);
        }
        else // not active one present (transfer resumption?)
        {
            LOG_warn << "Transfer update for folder tag that matches no active transfer. Probably coming from transfer resumption";
//            assert(false && "Receiving a transfer update on a child of a finished parent");

            //TODO: worst case scenario: we match a new folder tranfer that has nothing to do with
            // those resumed transfers. Needs investigation and a way to mitigate if there's a bug here
        }
    }
    else //not a child!
    {
        auto tag = transfer->getTag();
        auto it = mActiveTransfers.find(tag);
        if (it == mActiveTransfers.end())
        {
            LOG_warn << "Received onTransferUpdate on a not active parent transfer"; //can happen on transfer resumption
            recoverUnHandleTransfer(api, transfer);
            //search again
            it = mActiveTransfers.find(parentTag);
        }

        if (it != mActiveTransfers.end())
        {
            it->second->second->updateTransfer(transfer, it->second->second);
            mTransfersInMemory[it->second->first.getObjectID()] = it->second; //Insert / replace the existing value
        }
        else
        {
            LOG_warn << "Received onTransferUpdate on a not active parent transfer"; //can happen on transfer resumption
            //assert(false && "Received onTransferUpdate on a not active parent transfer");

            auto it = mFinishedTransfers.find(transfer->getTag());
            if (it != mFinishedTransfers.end())
            {
                it->second->second->updateTransfer(transfer, it->second->second);
                mTransfersInMemory[it->second->first.getObjectID()] = it->second; //Insert / replace the existing value
            }
        }
    }
}

void DownloadsManager::onTransferFinish(MegaApi *api, MegaTransfer *transfer, MegaError *error)
{
    std::lock_guard<std::recursive_mutex> g(mTransfersMutex);
    auto parentTag = transfer->getFolderTransferTag();
    if (parentTag > 0)
    {
        auto it = mActiveTransfers.find(parentTag);
        if (it != mActiveTransfers.end()) //found an active one
        {
            it->second->second->onSubTransferFinish(transfer, error);
            return; //the above will carry the update
        }
        else
        {
            LOG_warn << "Transfer finish for folder tag that matches no active transfer. Probably coming from transfer resumption";
            //assert(false && "Receiving a transfer finish update on a child of a finished parent");
//            if (it != mFinishedTransfers.end()) //found a finished one
//            {
//                it->second->second->onSubTransferFinish(transfer, error);
//            }
            return;
        }
    }
    else // not a child
    {
        auto it = mActiveTransfers.find(transfer->getTag());

        if (it == mActiveTransfers.end())
        {
            LOG_warn << "Received onTransferUpdate on a not active parent transfer"; //can happen on transfer resumption
            recoverUnHandleTransfer(api, transfer);
            //search again
            it = mActiveTransfers.find(parentTag);
        }

        if (it != mActiveTransfers.end())
        {
            auto &info = it->second->second;
            auto elementToMove = it->second;

            // update data
            it->second->second->updateTransfer(transfer, info, error);
            mTransfersInMemory[it->second->first.getObjectID()] = it->second; //Insert / replace the existing value

            // move to finished ones
            mFinishedTransfers.emplace(it->first, elementToMove);
            mFinishedTransfersQueue.push(it->first);

            mActiveTransfers.erase(it);
        }
        else
        {
            LOG_err << "Received onTransferFinish on a not active parent transfer";
        }
    }
}


void DownloadsManager::printAll(OUTSTREAMTYPE &os, map<string, int> *clflags, map<string, string> *cloptions)
{
    std::lock_guard<std::recursive_mutex> g(mTransfersMutex);

    ColumnDisplayer cd(clflags, cloptions);


    os << "  -----   ACTIVE -------- " << endl;

    for (auto & it : mActiveTransfers)
    {
        it.second->second->print(os, clflags, cloptions);
    }

    os << "  -----   FINISHED -------- " << endl;

    for (auto & it : mFinishedTransfers)
    {
        it.second->second->print(os, clflags, cloptions);
    }

}

void DownloadsManager::printOne(OUTSTREAMTYPE &os, std::string objectIDorTag, map<string, int> *clflags, map<string, string> *cloptions)
{
    std::lock_guard<std::recursive_mutex> g(mTransfersMutex);
    std::shared_ptr<TransferInfo> ti;
    DownloadId did;

    if (DownloadId::isObjectID(objectIDorTag))
    {
        auto it = mTransfersInMemory.find(objectIDorTag);
        if (it != mTransfersInMemory.end())
        {

            did = it->second->first;
            ti = it->second->second;
        }
        else
        {
            auto transferInfo = TransferInfoIOWriter::Instance().read(objectIDorTag);
            if (transferInfo)
            {
                ti = transferInfo;
                did = ti->getId();
            }
        }
    }
    else // a tag
    {
        try {
            auto tag = std::stoi(objectIDorTag);
            auto itActive = mActiveTransfers.find(tag);

            if (itActive != mActiveTransfers.end())
            {
                did = itActive->second->first;
                ti = itActive->second->second;
            }
            else
            {
                auto itFinished = mFinishedTransfers.find(tag);
                if (itFinished != mFinishedTransfers.end())
                {
                    did = itFinished->second->first;
                    ti = itFinished->second->second;
                }
            }
        }
        catch (...)
        {
            setCurrentOutCode(MCMD_INVALIDTYPE);
            LOG_err << " Invalid parameter: " << objectIDorTag;
            return;
        }
    }


   if (ti)
   {
        ti->print(os, clflags, cloptions);
   }
   else
   {
       setCurrentOutCode(MCMD_NOTFOUND);
       LOG_err << " Not found: " << objectIDorTag;
       return;
   }
}

void DownloadsManager::purge()
{
    std::lock_guard<std::recursive_mutex> g(mTransfersMutex);

    mActiveTransfers.clear();
    mFinishedTransfers.clear();
    mTransfersInMemory.clear();
    mTransfers.clear();

    mNewUnHandledTransfers.clear();

    TransferInfoIOWriter::Instance().purge();
}


OUTSTREAMTYPE &operator<<(OUTSTREAMTYPE &os, const DownloadId &p)
{
    return os << "[" << p.mTag << ":" << p.getObjectID() << "]";
}


void TransferInfo::addToColumnDisplayer(ColumnDisplayer &cd)
{
    if (!mTransfer)
    {
        LOG_err << "Cannot print transfer info, transfer not properly loaded";
        assert(false && "trying to print transfer info without mTransfer");
        return;
    }
    if (mTransfer->getFolderTransferTag() <= 0)
    {
        cd.addValue("OBJECT_ID", mId.getObjectID());
    }
    printTransferColumnDisplayer(&cd, mTransfer.get(), mSourcePath);

    if (mTransfer->getFolderTransferTag() <= 0)
    {
         cd.addValue("SUB_STARTED", std::to_string(mSubTransfersStarted));
         cd.addValue("SUB_OK", std::to_string(mSubTransfersFinishedOk));
         cd.addValue("SUB_FAIL", std::to_string(mSubTransfersFinishedWithFailure));
    }

    cd.addValue("ERROR_CODE", MegaError::getErrorString(mFinalError));
}


std::string TransferInfo::serializedTransferData()
{
    string toret;
    ParamWriter pr(toret);

    {
        std::lock_guard<std::mutex> g(mTransferMutex);
        if (mTransfer)
        {
            auto t = mTransfer.get();
            pr << t;
        }
    }

    pr << mSubTransfersStarted;
    pr << mSubTransfersFinishedOk;
    pr << mSubTransfersFinishedWithFailure;
    pr << mFinalError;
    pr << mSourcePath;

    return toret;
}

int TransferInfo::getType() const
{
    return mTransfer->getType();
}

int64_t TransferInfo::getStartTime() const
{
    return mTransfer->getStartTime();
}

long long TransferInfo::getTransferredBytes() const
{
    return mTransfer->getTransferredBytes();
}

long long TransferInfo::getTotalBytes() const
{
    return mTransfer->getTotalBytes();
}

const char *TransferInfo::getPath() const
{
    return mTransfer->getPath();
}

const char *TransferInfo::getParentPath() const
{
    return mTransfer->getParentPath();
}

MegaHandle TransferInfo::getNodeHandle() const
{
    return mTransfer->getNodeHandle();
}

MegaHandle TransferInfo::getParentHandle() const
{
    return mTransfer->getParentHandle();
}

long long TransferInfo::getStartPos() const
{
    return mTransfer->getStartPos();
}

long long TransferInfo::getEndPos() const
{
    return mTransfer->getEndPos();
}

const char *TransferInfo::getFileName() const
{
    return mTransfer->getFileName();
}

int TransferInfo::getNumRetry() const
{
    return mTransfer->getNumRetry();
}

int TransferInfo::getMaxRetries() const
{
    return mTransfer->getMaxRetries();
}

int TransferInfo::getTag() const
{
    return mTransfer->getTag();
}

long long TransferInfo::getSpeed() const
{
    return mTransfer->getSpeed();
}

long long TransferInfo::getMeanSpeed() const
{
    return mTransfer->getMeanSpeed();
}

long long TransferInfo::getDeltaSize() const
{
    return mTransfer->getDeltaSize();
}

int64_t TransferInfo::getUpdateTime() const
{
    return mTransfer->getUpdateTime();
}

MegaNode *TransferInfo::getPublicMegaNode() const
{
    return mTransfer->getPublicMegaNode();
}

bool TransferInfo::isSyncTransfer() const
{
    return mTransfer->isSyncTransfer();
}

bool TransferInfo::isStreamingTransfer() const
{
    return mTransfer->isStreamingTransfer();
}

bool TransferInfo::isFinished() const
{
    return mTransfer->isFinished();
}

bool TransferInfo::isBackupTransfer() const
{
    return mTransfer->isBackupTransfer();
}

bool TransferInfo::isForeignOverquota() const
{
    return mTransfer->isForeignOverquota();
}

bool TransferInfo::isForceNewUpload() const
{
    return mTransfer->isForceNewUpload();
}

char *TransferInfo::getLastBytes() const
{
    return mTransfer->getLastBytes();
}

MegaError TransferInfo::getLastError() const
{
    return mTransfer->getLastError();
}

const MegaError *TransferInfo::getLastErrorExtended() const
{
    return mTransfer->getLastErrorExtended();
}

bool TransferInfo::isFolderTransfer() const
{
    return mTransfer->isFolderTransfer();
}

int TransferInfo::getFolderTransferTag() const
{
    return mTransfer->getFolderTransferTag();
}

const char *TransferInfo::getAppData() const
{
    return mTransfer->getAppData();
}

int TransferInfo::getState() const
{
    return mTransfer->getState();
}

unsigned long long TransferInfo::getPriority() const
{
    return mTransfer->getPriority();
}

long long TransferInfo::getNotificationNumber() const
{
    return mTransfer->getNotificationNumber();
}

bool TransferInfo::getTargetOverride() const
{
    return mTransfer->getTargetOverride();
}

void TransferInfo::updateTransfer(MegaTransfer *transfer, const std::shared_ptr<TransferInfo> &transferInfo, MegaError *error)
{
    {
        std::lock_guard<std::mutex> g(mTransferMutex);

        if (!mTransfer || transfer != mTransfer.get())
        {
            mTransfer.reset(transfer->copy());
        }
    }

    if (error)
    {
        mFinalError = error->getErrorCode();
    }

    mLastUpdate = time(nullptr);


    TransferInfoIOWriter::Instance().write(transferInfo);
}

DownloadId TransferInfo::getId() const
{
    return mId;
}

int64_t TransferInfo::getParentDbId() const
{
    if (mParent)
    {
        return mParent->dbid();
    }
    return DataBaseEntry::INVALID_DB_ID;
}

int64_t TransferInfo::getLastUpdate() const
{
    return mLastUpdate;
}

void TransferInfo::onPersisted()
{

}

void TransferInfo::initialize(MegaApi *api, MegaTransfer *transfer, DownloadId *id)
{
    if (id)
    {
        mId = *id;
    }
    else
    {
        mId = DownloadId(transfer->getTag(), "");
    }

    if (api)
    {
        MegaNode * node = api->getNodeByHandle(transfer->getNodeHandle());
        if (node)
        {
            std::unique_ptr<char[]> nodepath (api->getNodePath(node));
            if (nodepath)
            {
                mSourcePath = nodepath.get();
            }

            delete node;
        }
        else
        {
            //assert(false && "should come from a persisted one: not expected");
            // this can happen also for links!
            mSourcePath = this->mId.mPath;
        }
    }

}




TransferInfo::TransferInfo(MegaApi *api, MegaTransfer *transfer, DownloadId *id)
{
    initialize(api, transfer, id);
}


TransferInfo::TransferInfo(MegaApi *api, MegaTransfer *transfer, TransferInfo * parent)
{
    mParent = parent;
    initialize(api, transfer, nullptr);
}

TransferInfo::TransferInfo(const std::string &serializedContents, int64_t lastUpdate, TransferInfo * parent, const string &objectId)
    : mParent(parent),
      mLastUpdate(lastUpdate)
{
    ParamReader pr(serializedContents, true);
    pr >> mTransfer;

    pr >> mSubTransfersStarted;
    pr >> mSubTransfersFinishedOk;
    pr >> mSubTransfersFinishedWithFailure;
    pr >> mFinalError;
    pr >> mSourcePath;

    mId = DownloadId(mTransfer->getTag(), objectId);
}

void TransferInfo::onSubTransferStarted(MegaApi *api, MegaTransfer *subtransfer)
{
    mSubTransfersStarted++;
    mSubTransfersInfo.emplace(std::make_pair(subtransfer->getTag(), std::make_shared<TransferInfo>(api, subtransfer, this)));

    onSubTransferUpdate(subtransfer);
}

void TransferInfo::addSubTransfer(const std::shared_ptr<TransferInfo> &transferInfo)
{
    mSubTransfersInfo.emplace(std::make_pair(transferInfo->getTag(), transferInfo));
}

void TransferInfo::onSubTransferFinish(MegaTransfer *subtransfer, MegaError *error)
{
    if (error->getErrorCode() == API_OK)
    {
        mSubTransfersFinishedOk++;
    }
    else
    {
        mSubTransfersFinishedWithFailure++;
    }

    auto it = mSubTransfersInfo.find(subtransfer->getTag());
    if (it != mSubTransfersInfo.end())
    {
        it->second->updateTransfer(subtransfer, it->second, error);
    }
}

void TransferInfo::onSubTransferUpdate(MegaTransfer *subtransfer)
{
    auto it = mSubTransfersInfo.find(subtransfer->getTag());
    if (it != mSubTransfersInfo.end())
    {
        it->second->updateTransfer(subtransfer, it->second);
    }
}

void TransferInfo::print(OUTSTREAMTYPE &os, std::map<std::string, int> *clflags, std::map<std::string, std::string> *cloptions, bool printHeader)
{
    std::lock_guard<std::mutex> g(mTransferMutex);

    ColumnDisplayer cd(clflags, cloptions);

    addToColumnDisplayer(cd);

    cd.print(os, printHeader);

    ColumnDisplayer cdSubTransfers(clflags, cloptions);

    //subtransfers
    bool showSubtransfers = getFlag(clflags, "show-subtransfers");
    if (showSubtransfers && mSubTransfersInfo.size())
    {
        auto colseparator = getOption(cloptions,"col-separator", "");

        for (auto &subitpair : mSubTransfersInfo)
        {
            std::string subTransfer(" SUBTRANSFER ");
            subTransfer.append(std::to_string(subitpair.first));

            cdSubTransfers.addValue("SUBTRANSFER", subTransfer);

            subitpair.second->addToColumnDisplayer(cdSubTransfers);
        }

        cdSubTransfers.print(os, true);
        os << "^^^^^^^^^^^^^^^^^^^^^^^\n\n";

    }
}


MegaTransferHeldInString::MegaTransferHeldInString(const string &s, size_t *size_read) : mSerialized(s)
{
    ParamReader pr(mSerialized);

    pr >> mType;
    pr >> mTransferString;
//    pr >> mtoString;
//    pr >> m__str__;
//    pr >> m__toString;
    pr >> mStartTime;
    pr >> mTransferredBytes;
    pr >> mTotalBytes;
    pr >> mPath;
    pr >> mParentPath;
    pr >> mNodeHandle;
    pr >> mParentHandle;
    pr >> mStartPos;
    pr >> mEndPos;
    pr >> mFileName;
//    pr >> mListener;
    pr >> mNumRetry;
    pr >> mMaxRetries;
    pr >> mTag;
    pr >> mSpeed;
    pr >> mMeanSpeed;
    pr >> mDeltaSize;
    pr >> mUpdateTime;
//    pr >> mPublicMegaNode;
    pr >> misSyncTransfer;
    pr >> misBackupTransfer;
    pr >> misForeignOverquota;
    pr >> misForceNewUpload;
    pr >> misStreamingTransfer;
    pr >> misFinished;
    pr >> mLastBytes;
//    pr >> mLastError;
//    pr >> mLastErrorExtended;
    pr >> misFolderTransfer;
    pr >> mFolderTransferTag;
    pr >> mAppData;
    pr >> mState;
    pr >> mPriority;
    pr >> mNotificationNumber;
    pr >> mTarOverride;

    if (size_read) *size_read = pr.sizeRead();
}





ParamWriter::ParamWriter(string &where)
    : mOut (where)
{
}

ParamReader::ParamReader(const string &params, bool duplicateConstCharPointer)
    : mDuplicateConstCharPointers(duplicateConstCharPointer)
{

    mP = (char *)params.c_str(); //this ensures a \0 at the end
    mStart = mP;
    mEnd = mP + params.size(); //TODO: review and see if there's a c++11 better way for all this
}

void ParamReader::advance(size_t size)
{
    mP += size;
    assert (mP <= mEnd);
}

void ParamReader::advanceString()
{
    while ( mP < mEnd && *mP != '\0')
    {
        mP++;
    }

    if (mP < mEnd) //'\0'
    {
        mP++;
    }
    assert (mP <= mEnd);
}



ParamReader& operator>>(ParamReader& x, MegaTransfer *&transfer)
{
    assert(x.mDuplicateConstCharPointers && "only output readers should create a non-smart pointer");

    bool null; x >> null; if (null) {transfer = nullptr; return x;}

    if (x.mP < x.mEnd /* && *x.mP != (char)0x1F*/)
    {
        size_t size_read = 0;
        transfer = new MegaTransferHeldInString(string(x.mP,x.mEnd), &size_read);
        x.advance(size_read);

        assert(x.mP <=x.mEnd);
    }
    else
    {
        LOG_err << "Issue reading transfer from db";
        transfer = nullptr;
    }

    return x;
}



ParamReader& operator>>(ParamReader& x, const char *&o)
{
    bool null; x >> null; if (null) { o = nullptr; return x;}

    if (x.mDuplicateConstCharPointers)
    {
        o = MegaApi::strdup(x.mP);
    }
    else
    {
        o = x.mP;
    }

    x.advanceString();

    return x;
}

ParamReader& operator>>(ParamReader& x, char *&o)
{
    bool null; x >> null; if (null) { o = nullptr; return x;}

    if (x.mDuplicateConstCharPointers)
    {
        o = MegaApi::strdup(x.mP);
    }
    else
    {
        o = x.mP;
    }

    x.advanceString();

    return x;
}

ParamReader& operator>>(ParamReader& x, string &o)
{
    size_t size;
    x >> size;
    if (x.mP + size <= x.mEnd)
    {
        o.resize(size);
        memcpy((void *)o.data(),x.mP, size);
        x.advance(size);
    }
    return x;
}


ParamWriter& operator<<(ParamWriter& x, const string&s)
{
    x << s.size();//write the size
    x.write(s.data(), s.size());
    return x;
}


ParamWriter& operator<<(ParamWriter& x, MegaTransfer *&t)
{
    x << (t ? false: true); //to be able to handle nullptrs!
    if (t)
    {
        //string d;
        //static_cast<MegaTransferPrivate *>(transfer)->serialize(&d);
//        x << d;
        x << t->getType();
        x << t->getTransferString();
//        x << t->toString();
//        x << t->__str__();
//        x << t->__toString();
        x << t->getStartTime();
        x << t->getTransferredBytes();
        x << t->getTotalBytes();
        x << t->getPath();
        x << t->getParentPath();
        x << t->getNodeHandle();
        x << t->getParentHandle();
        x << t->getStartPos();
        x << t->getEndPos();
        x << t->getFileName();
//        x << t->getListener();
        x << t->getNumRetry();
        x << t->getMaxRetries();
        x << t->getTag();
        x << t->getSpeed();
        x << t->getMeanSpeed();
        x << t->getDeltaSize();
        x << t->getUpdateTime();
//        x << t->getPublicMegaNode();
        x << t->isSyncTransfer();
        x << t->isBackupTransfer();
        x << t->isForeignOverquota();
        x << t->isForceNewUpload();
        x << t->isStreamingTransfer();
        x << t->isFinished();
        x << t->getLastBytes();
//        x << t->getLastError();
//        x << t->getLastErrorExtended();
        x << t->isFolderTransfer();
        x << t->getFolderTransferTag();
        x << t->getAppData();
        x << t->getState();
        x << t->getPriority();
        x << t->getNotificationNumber();
        x << t->getTargetOverride();

    }
    return x;
}


ParamWriter& operator<<(ParamWriter& x, MegaError *&error)
{
    x << (error ? false: true); //to be able to handle nullptrs!
    if (error)
    {
        x << error->getErrorCode();
        x << error->getValue();
        x << error->getUserStatus();
        x << error->getLinkStatus();
    }
    return x;
}

ParamWriter& operator<<(ParamWriter& x, const char *s)
{
    x << (s ? false: true); //to be able to handle nullptrs!
    if (s)
    {
        x.mOut.append(s);

        auto end = x.mOut.end();
        x.mOut.resize(x.mOut.size()+1);
        *end =  (char)0x0; //we need to preserve 0x0

    }
    return x;
}


ParamWriter& operator<<(ParamWriter& x, char *s)
{
    x << (s ? false: true); //to be able to handle nullptrs!
    if (s)
    {
        x.mOut.append(s);

        auto end = x.mOut.end();
        x.mOut.resize(x.mOut.size()+1);
        *end =  (char)0x0; //we need to preserve 0x0

    }
    return x;
}


void ParamWriter::write(const char *buffer, size_t size)
{
    mOut.append(buffer, size);
}

std::string DownloadId::getObjectID() const
{
    if (mPath.find("O:") == 0)
    {
        return mPath;
    }

    if (isPublicLink(mPath))
    {
        auto linkHandle = getPublicLinkObjectId(mPath);
        if (!linkHandle.empty())
        {
            return string("O:").append(linkHandle);
        }
    }

    return mPath.empty() ? string() : string("O:").append(mPath);
}

bool DownloadId::isObjectID(const std::string &what)
{
    return what.find("O:") == 0;
}


int64_t DataBaseEntry::dbid() const
{
    return mDbid;
}

void DataBaseEntry::setDbid(const int64_t &dbid)
{
    mDbid = dbid;
}





std::string TransferInfoIOWriter::transferInfoDbPath() const
{
    return mTransferInfoDbPath;
}

TransferInfoIOWriter::~TransferInfoIOWriter()
{
    if (mThreadIoProcessor)
    {
        end();
        mThreadIoProcessor->join();
    }
}

bool TransferInfoIOWriter::write(std::shared_ptr<TransferInfo> transferInfo)
{
    bool delayNotification = false;
    {
        std::lock_guard<std::mutex> g(mActionsMutex);
        mActions.push_back(TransferInfoIOAction(TransferInfoIOAction::Action::Write, transferInfo));
        //Note: write stores a weak pointer and whenever processing, if locking shared_ptr fails,
        // we can dismiss the write, without needing to control further the existence of the transferInfo

        delayNotification = mActions.size() < mIOMaxActionBeforeNotifying;
    }

    if (!delayNotification)
    {
        mIOcv.notify_all();
    }

    return true;
}

bool TransferInfoIOWriter::remove(std::shared_ptr<TransferInfo> transferInfo, bool delayNotification)
{
    {
        std::lock_guard<std::mutex> g(mActionsMutex);
        //Ideally if there's an ongoing action for transferInfo, either write or remove, we should discard that and keep only this one
        // anyway, using the commiter guard would make this fast enough to care much
        mActions.push_back(TransferInfoIOAction(TransferInfoIOAction::Action::Remove, transferInfo, true)); // we give ownership
    }
    if (!delayNotification)
    {
        mIOcv.notify_all();
    }
    return true;
}

void TransferInfoIOWriter::forceExecution()
{
    mIOcv.notify_all();
}


TransferInfoIOAction TransferInfoIOWriter::getNextAction()
{
    std::unique_lock<std::mutex> lk(mActionsMutex);
    auto action = mActions.front();
    mActions.pop_front();

    return action;
}

bool TransferInfoIOWriter::hasActions()
{
    std::unique_lock<std::mutex> lk(mActionsMutex);
    return !mActions.empty();
}

void TransferInfoIOWriter::loopIOActionProcessor()
{
    while(true)
    {
        {
            std::unique_lock<std::mutex> lk(mActionsMutex);
            mIOcv.wait_for(lk, std::chrono::milliseconds(mIOScheduleMs), [this]{ return (mFinished || !mActions.empty());});
        }

        if (hasActions())
        {
            std::lock_guard<std::mutex> g(mIOMutex);

            CommitGuard cg(db, true);

            while (hasActions())
            {
                auto action = getNextAction();
                auto weakTransferInfo = action.transferInfo();
                auto transferInfo = weakTransferInfo.lock();

                if (transferInfo)// otherwise we can dismiss the write: no TransferInfo
                {
                    switch(action.action())
                    {
                    case TransferInfoIOAction::Action::Write:
                        doWrite(transferInfo);
                        transferInfo->onPersisted();
                        break;
                    case TransferInfoIOAction::Action::Remove:
                        doRemove(transferInfo);
                        break;
                    }
                }
                else
                {
                    LOG_err << "IO writer found a deleted TansferInfo, its data may be lost";
                }

            }
        }


        if (mFinished) break;
    }
}

void TransferInfoIOWriter::end()
{
    {
        std::lock_guard<std::mutex> g(mActionsMutex);
        mFinished = true;
    }
    mIOcv.notify_all();
}

std::weak_ptr<TransferInfo> TransferInfoIOAction::transferInfo() const
{
    return mTransferInfo;
}

TransferInfoIOAction::Action TransferInfoIOAction::action() const
{
    return mAction;
}

bool TransferInfoIOWriter::initializeSqlite()
{
    if (mInitialized)
    {
        return true;
    }

    MegaFileSystemAccess fsAccess;
    LocalPath defaultDlPath = LocalPath::fromAbsolutePath(ConfigurationManager::getConfigFolder());
    auto extra = LocalPath::fromRelativePath("downloads.db");
    defaultDlPath.appendWithSeparator(extra, false);
    mTransferInfoDbPath = ConfigurationManager::getConfigurationValue("downloads_db_path", defaultDlPath.toPath());

    auto rc = sqlite3_open(mTransferInfoDbPath.c_str(), &db);

    std::string sql = "CREATE TABLE IF NOT EXISTS transferInfo (id INTEGER PRIMARY KEY ASC NOT NULL,"
            " objectId TEXT, parentDbId INTEGER, lastUpdate DATETIME, content BLOB NOT NULL)";

    rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);

    if (rc)
    {
        LOG_err << "Error initializing transferInfosdb: " << sqlite3_errmsg(db) ;
        return false;
    }


    // indexes
    sql = "CREATE INDEX IF NOT EXISTS objectId_index ON transferInfo (objectId)";
    rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
    if (rc)
    {
        LOG_err << "Error creating index objectId_index for transferInfosdb: " << sqlite3_errmsg(db) ;
        return false;
    }

    // indexes
    sql = "CREATE INDEX IF NOT EXISTS parentDbId_index ON transferInfo (parentDbId)";
    rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
    if (rc)
    {
        LOG_err << "Error creating index parentDbId_index for transferInfosdb: " << sqlite3_errmsg(db) ;
        return false;
    }

    mInitialized = true;

    return true;
}

std::vector<std::shared_ptr<TransferInfo>> TransferInfoIOWriter::loadSubTransfers(TransferInfo *parent)
{
    initializeSqlite();
    std::vector<std::shared_ptr<TransferInfo>>toret;
    int rc = false;
    std::string sql("SELECT id, objectId, lastUpdate, content FROM transferInfo where parentDbId = ?;");
    sqlite3_stmt *stmt;


    if (sqlite3_prepare(db, sql.data(), static_cast<int>(sql.size()), &stmt, nullptr) != SQLITE_OK)
    {
        LOG_err << "Error open DB " << sqlite3_errmsg(db) ;
        return toret;
    }
    if (sqlite3_bind_int64(stmt, 1, parent->dbid()) != SQLITE_OK)
    {
        LOG_err << "Error binding parent dbid for subtransfers: " << sqlite3_errmsg(db) ;
        return toret;
    }

    while (true)
    {
        rc = sqlite3_step(stmt);

        if (rc == SQLITE_ROW)
        {
            int64_t id = sqlite3_column_int64(stmt, 0);
            mLastId = std::max(id, mLastId);
            std::string objectId((char*)sqlite3_column_text(stmt, 1), sqlite3_column_bytes(stmt, 1));
            auto lastUpdate(sqlite3_column_int64(stmt, 2));
            std::string contents((char*)sqlite3_column_blob(stmt, 3), sqlite3_column_bytes(stmt, 3));

            auto transferInfo = std::make_shared<TransferInfo>(contents, lastUpdate, parent, objectId);
            transferInfo->setDbid(id);

            toret.push_back(transferInfo);
        }
        else //finished
        {
            if (rc != SQLITE_DONE)
            {
                LOG_err << "Error stepping for subtransfers: " << sqlite3_errmsg(db) ;
            }

            break;
        }
    }

    return toret;
}

std::shared_ptr<TransferInfo> TransferInfoIOWriter::read(const std::string &objectId)
{
    std::lock_guard<std::mutex> g(mIOMutex);
    return loadTransferInfo(objectId);
}

bool TransferInfoIOWriter::purge()
{
    std::lock_guard<std::mutex> g(mIOMutex);

    mActions.clear();
    initializeSqlite();
    std::shared_ptr<TransferInfo> toret;
    std::string sql("DELETE FROM transferInfo;");

    auto rc = sqlite3_exec(db, "DELETE FROM transferInfo", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK)
    {
        LOG_err << "Error deleting transferInfos db: " << sqlite3_errmsg(db) ;
        return false;
    }

    return true;
}

std::shared_ptr<TransferInfo> TransferInfoIOWriter::loadTransferInfo(const std::string &objectId)
{
    initializeSqlite();
    std::shared_ptr<TransferInfo> toret;
    int rc = false;
    std::string sql("SELECT id, objectId, parentDbId, lastUpdate, content FROM transferInfo where objectId = ? order by lastUpdate DESC LIMIT 1;");
    sqlite3_stmt *stmt;

    if (sqlite3_prepare(db, sql.data(), static_cast<int>(sql.size()), &stmt, nullptr) != SQLITE_OK)
    {
        LOG_err << "Error open DB " << sqlite3_errmsg(db) ;
        return toret;
    }

    if (sqlite3_bind_text(stmt, 1, objectId.data(), static_cast<int>(objectId.size()), nullptr) != SQLITE_OK)
    {
        LOG_err << "Error binding objectId transferInfos: " << sqlite3_errmsg(db) ;
        return toret;
    }

    while (true)
    {
        rc = sqlite3_step(stmt);

        if (rc == SQLITE_ROW)
        {
            int64_t id = sqlite3_column_int64(stmt, 0);
            mLastId = std::max(id, mLastId);
            std::string objectIdRead((char*)sqlite3_column_text(stmt, 1), sqlite3_column_bytes(stmt, 1));
            int64_t parentDbId = sqlite3_column_int64(stmt, 2);
            auto lastUpdate(sqlite3_column_int64(stmt, 3));
            std::string contents((char*)sqlite3_column_blob(stmt, 4), sqlite3_column_bytes(stmt, 4));

            assert(parentDbId == DataBaseEntry::INVALID_DB_ID && "Reading some parent transfer with assigned parent db id");

            auto transferInfo = std::make_shared<TransferInfo>(contents, lastUpdate, nullptr, objectIdRead);
            transferInfo->setDbid(id);

            auto subtransfers = loadSubTransfers(transferInfo.get());
            for (auto &sub : subtransfers)
            {
                transferInfo->addSubTransfer(sub);
            }

            return transferInfo; //we don't want to loop further!
        }
        else //finished
        {
            if (rc != SQLITE_DONE)
            {
                LOG_err << "Error stepping for transferInfos " << sqlite3_errmsg(db) ;
            }
            break;
        }
    }

    return toret;
}

bool TransferInfoIOWriter::doWrite(std::shared_ptr<TransferInfo> transferInfo)
{
    sqlite3_stmt *stmt;
    bool result = false;

    auto id = transferInfo->dbid();
    if (id == DataBaseEntry::INVALID_DB_ID)
    {
        id = ++mLastId;
        transferInfo->setDbid(id);
    }

    auto transferInfoData = transferInfo->serializedTransferData();

    if (sqlite3_prepare(db, "INSERT OR REPLACE INTO transferInfo (id, objectId, parentDbId, lastUpdate, content) VALUES (?, ?, ?, ?, ?)", -1, &stmt, nullptr) == SQLITE_OK)
    {
        if (sqlite3_bind_int64(stmt, 1, id) == SQLITE_OK)
        {
            auto objectid = transferInfo->getId().getObjectID();


            if (sqlite3_bind_text(stmt, 2, objectid.data(), static_cast<int>(objectid.size()), nullptr) == SQLITE_OK)
            {
                auto parentDbId = transferInfo->getParentDbId();
                if (sqlite3_bind_int64(stmt, 3, parentDbId) == SQLITE_OK)
                {
                    if (sqlite3_bind_int64(stmt, 4, transferInfo->getLastUpdate()) == SQLITE_OK)
                    {
                        if (sqlite3_bind_blob(stmt, 5, transferInfoData.data(), static_cast<int>(transferInfoData.size()), SQLITE_STATIC) == SQLITE_OK)
                        {
                            if (sqlite3_step(stmt) == SQLITE_DONE)
                            {
                                result = true;
                            }
                            else
                            {
                                LOG_err << "Error steping transferInfos: " << sqlite3_errmsg(db) ;
                            }
                        }
                        else
                        {
                            LOG_err << "Error binding blob for transferInfos  " << sqlite3_errmsg(db) ;
                        }
                    }
                    else
                    {
                        LOG_err << "Error binding lastupdate for transferInfos: " << sqlite3_errmsg(db) ;
                    }
                }
                else
                {
                    LOG_err << "Error binding parentDbId for transferInfos: " << sqlite3_errmsg(db) ;
                }
            }
            else
            {
                LOG_err << "Error binding objecit transferInfos: " << sqlite3_errmsg(db) ;
            }
        }
        else
        {
            LOG_err << "Error binding dbid transferInfos: " << sqlite3_errmsg(db) ;
        }
    }
    else
    {
        LOG_err << "Error preparing transferInfos: " << sqlite3_errmsg(db) ;
    }

    sqlite3_finalize(stmt);

    return result;
}

bool TransferInfoIOWriter::doRemove(std::shared_ptr<TransferInfo> transferInfo)
{
    char buf[128];
    sprintf(buf, "DELETE FROM transferInfo WHERE objectId = \"%s\"" , transferInfo->getId().getObjectID().c_str());

    auto rc = sqlite3_exec(db, buf, nullptr, nullptr, nullptr);
    if (rc)
    {
        LOG_err << "Error removing transferInfo from DB " << sqlite3_errmsg(db) ;
    }

    return !rc;
}

TransferInfoIOWriter& TransferInfoIOWriter::Instance() {
    static TransferInfoIOWriter myInstance;
    return myInstance;
}

bool TransferInfoIOWriter::start()
{

    if (mThreadIoProcessor)
    {
        return true;
    }

    auto ret = initializeSqlite();

    mIOScheduleMs = ConfigurationManager::getConfigurationValue("downloads_db_io_frequency_ms", 10000);
    mIOMaxActionBeforeNotifying = ConfigurationManager::getConfigurationValue("downloads_db_max_queued_changes", 1000u);

    if (!ret)
    {
        return false;
    }

    mThreadIoProcessor.reset(new std::thread([this]()
    {
        this->loopIOActionProcessor();
    }
    ));

    return true;
}

void TransferInfoIOWriter::shutdown(bool loginout)
{
    if (mThreadIoProcessor)
    {
        end();
        mThreadIoProcessor->join();
        mThreadIoProcessor.reset();
    }

    if (mInitialized)
    {
        if (loginout)
        {
            sqlite3_close(db);
            MegaFileSystemAccess fsAccess;
            auto toremove = LocalPath::fromAbsolutePath(mTransferInfoDbPath);
            fsAccess.unlinklocal(toremove);
        }
    }

    //Reset all variables:
    mActions.clear();
    mFinished = false;
    mIOScheduleMs = 0;
    mIOMaxActionBeforeNotifying = 0;
    mTransferInfoDbPath = string();
    db = nullptr;
    mLastId = 0;
    mInitialized = false;
}

}
#endif
