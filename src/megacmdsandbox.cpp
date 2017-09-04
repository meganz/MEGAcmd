/**
 * @file examples/megacmd/megacmdsandbox.cpp
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

#include "megacmdsandbox.h"

bool MegaCmdSandbox::isOverquota() const
{
    return overquota;
}

void MegaCmdSandbox::setOverquota(bool value)
{
    overquota = value;
}

MegaCmdSandbox::MegaCmdSandbox()
{
    this->overquota = false;
    this->istemporalbandwidthvalid = false;
    this->temporalbandwidth = 0;
    this->temporalbandwithinterval = 0;
    this->lastQuerytemporalBandwith = time(NULL);
    this->timeOfOverquota = time(NULL);
    this->secondsOverQuota = 0;
}

