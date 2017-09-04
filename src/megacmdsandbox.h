/**
 * @file examples/megacmd/megacmdsandbox.h
 * @brief MegaCMD: A sandbox class to store status variables
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

#ifndef MEGACMDSANDBOX_H
#define MEGACMDSANDBOX_H

#include <ctime>

class MegaCmdSandbox
{
private:
    bool overquota;
public:
    bool istemporalbandwidthvalid;
    long long temporalbandwidth;
    long long temporalbandwithinterval;
    time_t lastQuerytemporalBandwith;
    time_t timeOfOverquota;
    time_t secondsOverQuota;
public:
    MegaCmdSandbox();
    bool isOverquota() const;
    void setOverquota(bool value);
};

#endif // MEGACMDSANDBOX_H
