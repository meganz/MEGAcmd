/**
 * @file src/megacmdsandbox.h
 * @brief MegaCMD: A sandbox class to store status variables
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

#ifndef MEGACMDSANDBOX_H
#define MEGACMDSANDBOX_H

#include <mega.h>
#include <ctime>
#include <string>
#include <future>
#include "megacmdexecuter.h"

namespace megacmd {
class MegaCmdExecuter;


class NodesCurrentPromise
{
    using Promise_t = std::promise<bool>;
    using Future_t = std::future<bool>;
    std::mutex mMutex;
    std::shared_ptr<Promise_t> mAccountUpToDatePromise;
    bool pendingCompletion = false;


public:
    void initiatePromise()
    {
        std::lock_guard<std::mutex> g(mMutex);
        assert(!pendingCompletion);
        mAccountUpToDatePromise = std::make_shared<Promise_t>();
        pendingCompletion = true;
    }

    void setCompleted()
    {
        std::lock_guard<std::mutex> g(mMutex);
        mAccountUpToDatePromise->set_value(true);
    }

    Future_t getFuture()
    {
        std::lock_guard<std::mutex> g(mMutex);
        assert(mAccountUpToDatePromise);
        return mAccountUpToDatePromise->get_future();
    }

    void reset()
    {
        std::lock_guard<std::mutex> g(mMutex);
        if (mAccountUpToDatePromise && pendingCompletion)
        {
            mAccountUpToDatePromise->set_value(false);
            pendingCompletion = false;
        }
    }

    void fulfil()
    {
        std::lock_guard<std::mutex> g(mMutex);
        if (mAccountUpToDatePromise && pendingCompletion)
        {
            mAccountUpToDatePromise->set_value(true);
            pendingCompletion = false;
        }
    }

    ~NodesCurrentPromise()
    {
        reset();
    }
};

class MegaCmdSandbox
{
private:
    bool overquota;
    std::mutex reasonBlockedMutex;
    std::string reasonblocked;
    bool reasonPending = false;
    std::promise<std::string> reasonPromise;

    void doSetReasonBlocked(const std::string &value);

public:
    NodesCurrentPromise mNodesCurrentPromise;

    bool istemporalbandwidthvalid;
    long long temporalbandwidth;
    long long temporalbandwithinterval;
    ::mega::m_time_t lastQuerytemporalBandwith;
    ::mega::m_time_t timeOfOverquota;
    ::mega::m_time_t secondsOverQuota;

    ::mega::m_time_t timeOfPSACheck;
    int lastPSAnumreceived;

    int storageStatus;
    long long receivedStorageSum;
    long long totalStorage;

    MegaCmdExecuter * cmdexecuter = nullptr;

public:
    MegaCmdSandbox();
    bool isOverquota() const;
    void setOverquota(bool value);
    void resetSandBox();

    std::string getReasonblocked();
    void setReasonPendingPromise();
    void setReasonblocked(const std::string &value);
    void setPromisedReasonblocked(const std::string &value);


};

}//end namespace
#endif // MEGACMDSANDBOX_H
