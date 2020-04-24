/**
 * @file src/megacmdsandbox.cpp
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

#include "megacmdsandbox.h"

using namespace mega;

namespace megacmd {
bool MegaCmdSandbox::isOverquota() const
{
    return overquota;
}

void MegaCmdSandbox::setOverquota(bool value)
{
    overquota = value;
}

void MegaCmdSandbox::resetSandBox()
{
    this->overquota = false;
    this->istemporalbandwidthvalid = false;
    this->temporalbandwidth = 0;
    this->temporalbandwithinterval = 0;
    this->lastQuerytemporalBandwith = m_time();
    this->timeOfOverquota = m_time();
    this->secondsOverQuota = 0;
    this->storageStatus = 0;
    this->receivedStorageSum = 0;
    this->totalStorage = 0;
    this->timeOfPSACheck = 0;
    this->lastPSAnumreceived = -1;
    if (reasonblocked.size()) removeGreetingStatusAllListener(string("message:").append(reasonblocked));
    this->reasonblocked = "";
    this->reasonPending = false;
}

std::string MegaCmdSandbox::getReasonblocked()
{
    std::unique_lock<std::mutex> ul(reasonBlockedMutex);

    if (reasonPending)
    {
        std::future<std::string> f = reasonPromise.get_future();
        ul.unlock();
        try
        {
            return f.get();
        }
        catch(std::future_error)
        {
            ul.lock();
            //already retrieved or broken promise (promise is replaced): another state. This is unlikely
        }
    }

    return reasonblocked;
}

void MegaCmdSandbox::setReasonPendingPromise()
{
    std::lock_guard<std::mutex> g(reasonBlockedMutex);
    reasonPending = true;
    this->reasonPromise = std::promise<std::string>();
}

void MegaCmdSandbox::doSetReasonBlocked(const std::string &value)
{
    broadcastMessage(value);
    if (reasonblocked.size()) removeGreetingStatusAllListener(string("message:").append(reasonblocked));
    if (value.size()) appendGreetingStatusAllListener(string("message:").append(value));
    reasonblocked = value;
}

void MegaCmdSandbox::setReasonblocked(const std::string &value)
{
    std::lock_guard<std::mutex> g(reasonBlockedMutex);
    doSetReasonBlocked(value);
}

void MegaCmdSandbox::setPromisedReasonblocked(const std::string &value)
{
    std::lock_guard<std::mutex> g(reasonBlockedMutex);

    doSetReasonBlocked(value);
    reasonPending = false;
    this->reasonPromise.set_value(value);
}

MegaCmdSandbox::MegaCmdSandbox()
{
    this->overquota = false;
    this->istemporalbandwidthvalid = false;
    this->temporalbandwidth = 0;
    this->temporalbandwithinterval = 0;
    this->lastQuerytemporalBandwith = m_time();
    this->timeOfOverquota = m_time();
    this->secondsOverQuota = 0;
    this->storageStatus = 0;
    this->receivedStorageSum = 0;
    this->totalStorage = 0;
    this->timeOfPSACheck = 0;
    this->lastPSAnumreceived = -1;
}

}//end namespace
