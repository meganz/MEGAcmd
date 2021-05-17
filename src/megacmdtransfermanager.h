/**
 * @file src/megacmdtransfermanager.h
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

#pragma once

//#include "megacmdlogger.h"
//#include "megacmdsandbox.h"
//#include "listeners.h"
#include "megacmdcommonutils.h"

#include "megaapi.h"
#include "mega/types.h" //make_unique


#include <memory>

#include <sqlite3.h> //TODO: move all these to a separate source file
#include <condition_variable>

using namespace mega;
using std::map;
using std::string;

namespace megacmd {

static void printTransferColumnDisplayer(ColumnDisplayer *cd, mega::MegaTransfer *transfer,
                                         const std::string &source, bool printstate=true);

class DownloadId
{
public:
    int mTag; //tag for active/finished transfers in current execution
    std::string mPath; // the path used to initiate the dl


    DownloadId(int tag, const std::string &path)
        : mTag(tag), mPath(path){}

    DownloadId() : DownloadId(-1,""){}


    bool operator<(const DownloadId& r) const
    {
        return std::tie(mTag, mPath)
                < std::tie(r.mTag, r.mPath);
    }


    // this returns an id that is always the same for the same path
    std::string getObjectID() const;

    static bool isObjectID(const string &what);

};


/**
 * @brief The information we want to keep of a transfer
 */
class TransferInfo
{
    long mSubTransfersStarted = 0;
    long mSubTransfersFinishedOk = 0;
    long mSubTransfersFinishedWithFailure = 0;

    map<int, std::unique_ptr<TransferInfo>> mSubTransfersInfo; //subtransferInfo by tag

    int mFinalError = -1;

    bool firstUpdate = true; //TODO: set false when loaded from db!
    string mSourcePath; //full path for files

    std::unique_ptr<MegaTransfer> mTransfer; // a copy of a transfer

    DownloadId mId; //For main transfers

public:

    TransferInfo(MegaApi * api, MegaTransfer *transfer, DownloadId *id = nullptr);

    // updates TransferInfo
    void onTransferUpdate(MegaTransfer *transfer);

    void onTransferUpdate(MegaApi *api, MegaTransfer *transfer);


    void onTransferFinish(MegaTransfer *subtransfer, MegaError *error);

    void onSubTransferStarted(MegaApi *api, MegaTransfer *subtransfer);

    void onSubTransferFinish(MegaTransfer *subtransfer, MegaError *error);

    void onSubTransferUpdate(MegaTransfer *subtransfer);

    void print(OUTSTREAMTYPE &os, std::map<std::string, int> *clflags, std::map<std::string, std::string> *cloptions, bool printHeader = true);
    void addToColumnDisplayer(ColumnDisplayer &cd);

    //MegaTransfer interface
    int getType() const;
    int64_t getStartTime() const;
    long long getTransferredBytes() const;
    long long getTotalBytes() const;
    const char *getPath() const;
    const char *getParentPath() const;
    MegaHandle getNodeHandle() const;
    MegaHandle getParentHandle() const;
    long long getStartPos() const;
    long long getEndPos() const;
    const char *getFileName() const;
    int getNumRetry() const;
    int getMaxRetries() const;
    int getTag() const;
    long long getSpeed() const;
    long long getMeanSpeed() const;
    long long getDeltaSize() const;
    int64_t getUpdateTime() const;
    MegaNode *getPublicMegaNode() const;
    bool isSyncTransfer() const;
    bool isStreamingTransfer() const;
    bool isFinished() const;
    bool isBackupTransfer() const;
    bool isForeignOverquota() const;
    bool isForceNewUpload() const;
    char *getLastBytes() const;
    MegaError getLastError() const;
    const MegaError *getLastErrorExtended() const;
    bool isFolderTransfer() const;
    int getFolderTransferTag() const;
    const char *getAppData() const;
    int getState() const;
    unsigned long long getPriority() const;
    long long getNotificationNumber() const;
    bool getTargetOverride() const;


};

OUTSTREAMTYPE &operator<<(OUTSTREAMTYPE &os, const DownloadId &p);


class DownloadsManager
{
private:
    std::map<DownloadId, std::unique_ptr<TransferInfo>> mTransfers; //here's the ownership of the objects

    std::map<int, std::map<DownloadId, std::unique_ptr<TransferInfo>>::iterator> mActiveTransfers; //All active transfers added
    std::map<int, std::map<DownloadId, std::unique_ptr<TransferInfo>>::iterator> mFinishedTransfers; //All finished transfers in current run

    //Map betwen OID -> TransferInfo
    // since 2 transfer can have the same OID, it will be keep the last recently updated
    std::map<std::string, std::map<DownloadId, std::unique_ptr<TransferInfo>>::iterator> mTransfersInMemory;

public:
    static DownloadsManager& Instance();

    std::map<DownloadId, std::unique_ptr<TransferInfo>>::iterator addNewTopLevelTransfer(MegaApi *api, MegaTransfer *transfer,
                                                                                         int tag, const std::string &path);

    void onTransferStart(MegaApi *api, MegaTransfer *transfer);

    void onTransferUpdate(MegaTransfer *transfer);

    void onTransferFinish(::mega::MegaTransfer *transfer, ::mega::MegaError* error);

    void printAll(OUTSTREAMTYPE &os, map<std::string, int> *clflags, map<std::string, std::string> *cloptions);
    void printOne(OUTSTREAMTYPE &os, std::string objectIDorTag, map<string, int> *clflags, map<string, string> *cloptions);
};


/**
 * @brief Extract parameters from a serialized string
 *
 */
class ParamReader
{
private:
    char *mStart; //starting of the buffer
    char *mP; //position of the buffer
    char *mEnd; //end of the buffer

    //TODO: rename the following (yieldOwnership? expectedToYield?)
    bool mDuplicateConstCharPointers; //whether to dup const char * or just use pointers to the buffer

public:
    ParamReader(const string &params, bool duplicateConstCharPointer = false);

    void advance(size_t size);
    void advanceString(); //advances while != \0;
    const char *read(size_t size);

    size_t sizeRead()
    {
        assert (mP>=mStart);
        return mP-mStart;
    }

    friend ParamReader& operator>> (ParamReader&, MegaTransfer *&);
//    friend ParamReader& operator>> (ParamReader&, MegaError *&);
    friend ParamReader& operator>> (ParamReader&, const char *&);
    friend ParamReader& operator>> (ParamReader&, char *&);
    friend ParamReader& operator>> (ParamReader&, string &);

    template <typename T>
    friend ParamReader& operator>> (ParamReader&, std::unique_ptr<T>&);

    template <typename T>
    friend ParamReader& operator>> (ParamReader&, T &);
};


template <typename T>
ParamReader& operator>>(ParamReader& x, std::unique_ptr<T>& n)
{
#ifdef DEBUG
    auto prev = x.mDuplicateConstCharPointers;
    x.mDuplicateConstCharPointers = true;
#endif

    T *pointer = nullptr;

    x >> pointer;
    n.reset(pointer);

#ifdef DEBUG
    x.mDuplicateConstCharPointers = prev;
#endif

    return x;
}


template <typename T>
ParamReader& operator>> (ParamReader& x, T &o)
{
    if (x.mP + + sizeof(T) <= x.mEnd)
    {
        memcpy(&o,x.mP, sizeof(T));
        x.advance(sizeof(T));
    }
    return x;
}

class ParamWriter
{
private:
    string &mOut;

public:
    ParamWriter(string &where);
    friend ParamWriter& operator<<(ParamWriter&, MegaTransfer *&);
    friend ParamWriter& operator<<(ParamWriter&, const char *);
    friend ParamWriter& operator<<(ParamWriter&, char *);
    friend ParamWriter& operator<<(ParamWriter&, const string&);


    void write(const char *buffer, size_t size);

    template <typename T>
    friend ParamWriter& operator<<(ParamWriter&, const T &s);
};

template <typename T>
ParamWriter& operator<<(ParamWriter& x, const T &s)
{
//    static_assert(!std::is_pointer<T>::value, "Param writer not implemented for this type of pointer");

    x.mOut.append(reinterpret_cast<const char *>(&s), sizeof(T));

    return x;
}



class MegaErrorWithCtor : public MegaError
{
public:
    MegaErrorWithCtor() : MegaError(API_OK){};
};

class MegaTransferHeldInString : public MegaTransfer
{
private:
    string mSerialized;

    int mType;
    const char * mTransferString = nullptr; //TODO: review this one
//    const char * mtoString = nullptr;
//    const char * m__str__ = nullptr;
//    const char * m__toString = nullptr;
    int64_t mStartTime;
    long long mTransferredBytes;
    long long mTotalBytes;
    const char * mPath = nullptr;
    const char * mParentPath = nullptr;
    MegaHandle mNodeHandle;
    MegaHandle mParentHandle;
    long long mStartPos;
    long long mEndPos;
    const char * mFileName = nullptr;
//    MegaTransferListener * mListener = nullptr;
    int mNumRetry;
    int mMaxRetries;
    int mTag;
    long long mSpeed;
    long long mMeanSpeed;
    long long mDeltaSize;
    int64_t mUpdateTime;
//    MegaNode * mPublicMegaNode = nullptr;
    bool misSyncTransfer;
    bool misBackupTransfer;
    bool misForeignOverquota;
    bool misForceNewUpload;
    bool misStreamingTransfer;
    bool misFinished;
    char * mLastBytes = nullptr;
//    MegaError mLastError;
//    const MegaError * mLastErrorExtended = nullptr;
    bool misFolderTransfer;
    int mFolderTransferTag;
    const char * mAppData = nullptr;
    int mState;
    unsigned long long mPriority;
    long long mNotificationNumber;
    bool mTarOverride;


public:
    MegaTransferHeldInString(const string &s, size_t *size_read = nullptr);

    virtual MegaTransfer *copy() override
    {
        return new MegaTransferHeldInString(mSerialized);
    }
    virtual int getType() const override
    {
        return mType;
    }
    virtual const char *getTransferString() const override
    {
        return mTransferString;
    }
    virtual const char *toString() const override
    {
        assert(false && "unimplemented");
        return nullptr;
    }
    virtual const char *__str__() const override
    {
        assert(false && "unimplemented");
        return nullptr;
    }
    virtual const char *__toString() const override
    {
        assert(false && "unimplemented");
        return nullptr;
    }
    virtual int64_t getStartTime() const override
    {
        return mStartTime;
    }
    virtual long long getTransferredBytes() const override
    {
        return mTransferredBytes;
    }
    virtual long long getTotalBytes() const override
    {
        return mTotalBytes;
    }
    virtual const char *getPath() const override
    {
        return mPath;
    }
    virtual const char *getParentPath() const override
    {
        return mParentPath;
    }
    virtual MegaHandle getNodeHandle() const override
    {
        return mNodeHandle;
    }
    virtual MegaHandle getParentHandle() const override
    {
        return mParentHandle;
    }
    virtual long long getStartPos() const override
    {
        return mStartPos;
    }
    virtual long long getEndPos() const override
    {
        return mEndPos;
    }
    virtual const char *getFileName() const override
    {
        return mFileName;
    }
    virtual MegaTransferListener *getListener() const override
    {
        assert(false && "unimplemented");
        return nullptr;
    }
    virtual int getNumRetry() const override
    {
        return mNumRetry;
    }
    virtual int getMaxRetries() const override
    {
        return mMaxRetries;
    }
    virtual int getTag() const override
    {
        return mTag;
    }
    virtual long long getSpeed() const override
    {
        return mSpeed;
    }
    virtual long long getMeanSpeed() const override
    {
        return mMeanSpeed;
    }
    virtual long long getDeltaSize() const override
    {
        return mDeltaSize;
    }
    virtual int64_t getUpdateTime() const override
    {
        return mUpdateTime;
    }
    virtual MegaNode *getPublicMegaNode() const override
    {
        assert(false && "unimplemented");
        return nullptr;
    }
    virtual bool isSyncTransfer() const override
    {
        return misSyncTransfer;
    }
    virtual bool isBackupTransfer() const override
    {
        return misBackupTransfer;
    }
    virtual bool isForeignOverquota() const override
    {
        return misForeignOverquota;
    }
    virtual bool isForceNewUpload() const override
    {
        return misForceNewUpload;
    }
    virtual bool isStreamingTransfer() const override
    {
        return misStreamingTransfer;
    }
    virtual bool isFinished() const override
    {
        return misFinished;
    }
    virtual char *getLastBytes() const override
    {
        return mLastBytes;
    }
    virtual MegaError getLastError() const override
    {
        assert(false && "unimplemented");
        return MegaErrorWithCtor(); //TODO: remove this and do actually serialize the error (not too costly)
    }
    virtual const MegaError *getLastErrorExtended() const override
    {
        assert(false && "unimplemented");
        return nullptr;
    }
    virtual bool isFolderTransfer() const override
    {
        return misFolderTransfer;
    }
    virtual int getFolderTransferTag() const override
    {
        return mFolderTransferTag;
    }
    virtual const char *getAppData() const override
    {
        return mAppData;
    }
    virtual int getState() const override
    {
        return mState;
    }
    virtual unsigned long long getPriority() const override
    {
        return mPriority;
    }
    virtual long long getNotificationNumber() const override
    {
        return mNotificationNumber;
    }
    virtual bool getTargetOverride() const override
    {
        return mTarOverride;
    }
};





}//end namespace
