/**
 * @file src/megacmdsandbox.h
 * @brief MegaCMD: A sandbox class to store status variables
 *
 * (c) 2013-2016 by Mega Limited, Auckland, New Zealand
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

class MegaCmdSandbox
{
private:
    bool overquota;
public:
    bool istemporalbandwidthvalid;
    long long temporalbandwidth;
    long long temporalbandwithinterval;
    ::mega::m_time_t lastQuerytemporalBandwith;
    ::mega::m_time_t timeOfOverquota;
    ::mega::m_time_t secondsOverQuota;
public:
    MegaCmdSandbox();
    bool isOverquota() const;
    void setOverquota(bool value);
};

#endif // MEGACMDSANDBOX_H
