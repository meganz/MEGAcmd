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

#pragma once

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cassert>

class DeferredSingleTrigger
{
    std::unique_ptr<std::thread> mThread;
    std::condition_variable mConditionVariable;
    std::mutex mConditionVariableMutex;
    std::mutex mThreadMutex;
    bool mDestroyed;
    size_t mCounter;
    std::chrono::seconds mSecondsToWait;

    size_t incrementCounter()
    {
        // Since the counter is used in the CV predicate, it must be protected with the CV mutex
        std::unique_lock<std::mutex> lock(mConditionVariableMutex);
        return ++mCounter;
    }

    void destroy()
    {
        // Since the destroyed flag is used in the CV predicate, it must be protected with the CV mutex
        std::unique_lock<std::mutex> lock(mConditionVariableMutex);
        assert(!mDestroyed);
        mDestroyed = true;
    }

    bool isDestroyed()
    {
        // Since the destroyed flag is used in the CV predicate, it must be protected with the CV mutex
        std::unique_lock<std::mutex> lock(mConditionVariableMutex);
        return mDestroyed;
    }

    std::unique_lock<std::mutex> ensureThreadIsStopped()
    {
        std::unique_lock<std::mutex> lock(mThreadMutex);
        if (mThread)
        {
            mConditionVariable.notify_one();
            mThread->join();
            mThread.reset();
        }
        return lock;
    }

public:
    DeferredSingleTrigger(std::chrono::seconds secondsToWait) :
        mDestroyed(false),
        mCounter(0),
        mSecondsToWait(secondsToWait) {}

    ~DeferredSingleTrigger()
    {
        destroy();
        ensureThreadIsStopped();
    }

    template<typename CB>
    void triggerDeferredSingleShot(CB&& callback)
    {
        const size_t id = incrementCounter();
        std::unique_lock<std::mutex> lock = ensureThreadIsStopped();

        if (isDestroyed())
        {
            // Without this check, we might spawn a new thread after the destructor is done
            // (i.e., if the lock above is waiting on the destructor's lock)
            return;
        }

        mThread.reset(new std::thread([this, id, callback] () {
            std::unique_lock<std::mutex> lock(mConditionVariableMutex);

            bool timeout = mConditionVariable.wait_for(lock, mSecondsToWait, [this, id]
            {
                assert(mCounter >= id);
                return mDestroyed || id != mCounter;
            });

            if (!timeout)
            {
                callback();
            }
        }));
    }
};
