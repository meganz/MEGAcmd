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

#include "megacmdtransfermanager.h"
#include "megacmd.h"
#include "megacmdutils.h"


#include <memory>


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
    cd->addValue("TAG", std::to_string(transfer->getTag())); //TODO: do SSTR within ColumnDisplayer

    if (transfer->getType() == MegaTransfer::TYPE_DOWNLOAD)
    {
        // source
        cd->addValue("SOURCEPATH", source);

        //destination
        string dest = transfer->getParentPath() ? transfer->getParentPath() : "";
        dest.append(transfer->getFileName());
        cd->addValue("DESTINYPATH",dest);
    }
    else
    {
        //source
        string source(transfer->getParentPath()?transfer->getParentPath():"");
        source.append(transfer->getFileName());

        cd->addValue("SOURCEPATH",source);

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

std::map<DownloadId, std::unique_ptr<TransferInfo>>::iterator DownloadsManager::addNewTopLevelTransfer(MegaApi *api, MegaTransfer *transfer, int tag, const std::string &path)
{
    DownloadId id(tag, path);
    auto pair = mTransfers.emplace(id, make_unique<TransferInfo>(api, transfer, &id));
    assert(pair.second);

    mActiveTransfers.emplace(tag, pair.first);

    mTransfersInMemory[id.getObjectID()] = pair.first; //Insert / replace the existing value

    assert(pair.second);
    return pair.first;
}

void DownloadsManager::onTransferStart(MegaApi *api, MegaTransfer *transfer)
{
    auto parentTag = transfer->getFolderTransferTag();
    if (parentTag != -1)
    {
        auto it = mActiveTransfers.find(parentTag);

        if (it != mActiveTransfers.end()) //found an active one
        {
            it->second->second->onSubTransferStarted(api, transfer);
        }
    }
    else //not a child! //already handled via addNewTopLevelTransfer...
    {
        //            auto it = mActiveTransfers.find(transfer->getTag());
        //            if (it != mActiveTransfers.end())
        //            {
        //                it->second->second->onTransferUpdate(transfer);
        //            }
    }
}

void DownloadsManager::onTransferUpdate(MegaTransfer *transfer)
{
    auto parentTag = transfer->getFolderTransferTag();
    if (parentTag != -1)
    {
        auto it = mActiveTransfers.find(parentTag);

        if (it != mActiveTransfers.end()) //found an active one
        {
            it->second->second->onSubTransferUpdate(transfer);
        }
        else // not active one present (transfer resumption?)
        {
            //TODO: need to load from db, but mark as _resumed_will_not_complete_
        }
    }
    else //not a child!
    {
        auto it = mActiveTransfers.find(transfer->getTag());
        if (it != mActiveTransfers.end())
        {
            it->second->second->onTransferUpdate(transfer);
            mTransfersInMemory[it->second->first.getObjectID()] = it->second; //Insert / replace the existing value
        }
        else
        {
            auto it = mFinishedTransfers.find(transfer->getTag());
            if (it != mFinishedTransfers.end())
            {
                it->second->second->onTransferUpdate(transfer);
                mTransfersInMemory[it->second->first.getObjectID()] = it->second; //Insert / replace the existing value
            }
        }

    }
}

void DownloadsManager::onTransferFinish(MegaTransfer *transfer, MegaError *error)
{
    auto parentTag = transfer->getFolderTransferTag();
    if (parentTag)
    {
        auto it = mActiveTransfers.find(parentTag);

        if (it != mActiveTransfers.end()) //found an active one
        {
            it->second->second->onSubTransferFinish(transfer, error);
        }
    }
    else // not a child
    {
        auto it = mActiveTransfers.find(transfer->getTag());
        if (it != mActiveTransfers.end())
        {
            auto &info = it->second->second;

            auto elementToMove = it->second;

            info->onTransferFinish(transfer, error);

            mFinishedTransfers.emplace(it->first, elementToMove);
            mActiveTransfers.erase(it);
        }

    }

    onTransferUpdate(transfer);
}


void DownloadsManager::printAll(OUTSTREAMTYPE &os, map<string, int> *clflags, map<string, string> *cloptions)
{
    ColumnDisplayer cd(clflags, cloptions);


    os << "  -----   ACTIVE -------- " << endl;

    for (auto & it : mActiveTransfers)
    {
        os << it.second->first << "  ----> " << "\n";
        it.second->second->print(os, clflags, cloptions);
        //os << *it.second->second.get();
    }

    os << "  -----   FINISHED -------- " << endl;

    for (auto & it : mFinishedTransfers)
    {
        os << it.second->first << "  ----> " << "\n";
        it.second->second->print(os, clflags, cloptions);
        //os << *it.second->second.get();
    }

}

void DownloadsManager::printOne(OUTSTREAMTYPE &os, std::string objectIDorTag, map<string, int> *clflags, map<string, string> *cloptions)
{
//    std::map<int, std::map<DownloadId, std::unique_ptr<TransferInfo>>::iterator> mActiveTransfers; //All active transfers added
//    std::map<int, std::map<DownloadId, std::unique_ptr<TransferInfo>>::iterator> mFinishedTransfers; //All finished transfers in current run

//    //Map betwen OID -> TransferInfo
//    // since 2 transfer can have the same OID, it will be keep the last recently updated
//    std::map<std::string, std::map<DownloadId, std::unique_ptr<TransferInfo>>::iterator> mTransfersInMemory;


    TransferInfo *ti = nullptr;
    DownloadId did;

    if (DownloadId::isObjectID(objectIDorTag))
    {
        auto it = mTransfersInMemory.find(objectIDorTag);
        if (it != mTransfersInMemory.end())
        {

            did = it->second->first;
            ti = it->second->second.get();
        }
        else
        {
            //TODO: look in db

            //TODO: set error and print it;
            os << objectIDorTag << " NOT FOUND";
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
                ti = itActive->second->second.get();
            }
            else
            {
                auto itFinished = mFinishedTransfers.find(tag);
                if (itFinished != mFinishedTransfers.end())
                {
                    did = itFinished->second->first;
                    ti = itFinished->second->second.get();
                }
            }
        }
        catch (...)
        {
            //TODO: set error and print out

        }
    }


   if (ti)
   {
 //       ColumnDisplayer cd(clflags, cloptions);
        os << did << "  ----> " << "\n";
        ti->print(os, clflags, cloptions);
   }
   else
   {
       //TODO: add error code and print not found error
   }


}


OUTSTREAMTYPE &operator<<(OUTSTREAMTYPE &os, const DownloadId &p)
{
    return os << "[" << p.mTag << ":" << p.mPath << "]";
}


void TransferInfo::addToColumnDisplayer(ColumnDisplayer &cd)
{
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

void TransferInfo::onTransferUpdate(MegaTransfer *transfer)
{
    assert(!firstUpdate);
    mTransfer.reset(transfer->copy());



    // TODO: persist here!
    // TODO: use commiters
}

void TransferInfo::onTransferUpdate(MegaApi *api, MegaTransfer *transfer)
{
    if (firstUpdate && api)
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
        firstUpdate = false;
    }

    onTransferUpdate(transfer);
}

TransferInfo::TransferInfo(MegaApi *api, MegaTransfer *transfer, DownloadId *id)
{
    if (id)
    {
        mId = *id;
    }
    onTransferUpdate(api, transfer);
}

void TransferInfo::onTransferFinish(MegaTransfer *subtransfer, MegaError *error) //TODO: have this called
{
    if (error->getErrorCode() == API_OK)
    {
        mSubTransfersFinishedOk++;
    }
    else
    {
        mSubTransfersFinishedWithFailure++;
    }

    mFinalError = error->getErrorCode();
    onTransferUpdate(subtransfer);
}

void TransferInfo::onSubTransferStarted(MegaApi *api, MegaTransfer *subtransfer)
{
    mSubTransfersStarted++;
    mSubTransfersInfo.emplace(std::make_pair(subtransfer->getTag(), make_unique<TransferInfo>(api, subtransfer)));

    onSubTransferUpdate(subtransfer);//TODO: probably we want transfer info to have a ctor taking MegaTransfer that calls this

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
        it->second->onTransferFinish(subtransfer, error);
    }
}

void TransferInfo::onSubTransferUpdate(MegaTransfer *subtransfer)
{
    auto it = mSubTransfersInfo.find(subtransfer->getTag());
    if (it != mSubTransfersInfo.end())
    {
        it->second->onTransferUpdate(subtransfer);
    }
}

void TransferInfo::print(OUTSTREAMTYPE &os, std::map<std::string, int> *clflags, std::map<std::string, std::string> *cloptions, bool printHeader)
{
    ColumnDisplayer cd(clflags, cloptions);

    addToColumnDisplayer(cd);

    cd.print(os, printHeader);

    ColumnDisplayer cdSubTransfers(clflags, cloptions);

    //subtransfers
    if (mSubTransfersInfo.size())
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
        //TODO: log error!
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
    if (isPublicLink(mPath))
    {
        auto linkHandle = getPublicLinkHandlev2(mPath);
        if (!linkHandle.empty())
        {
            return string("O:").append(linkHandle);
        }
    }

    return string("O:").append(mPath);
}

bool DownloadId::isObjectID(const std::string &what)
{
    return what.find("O:") == 0;
}

}
