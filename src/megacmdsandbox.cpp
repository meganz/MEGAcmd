/**
 * @file src/megacmdsandbox.cpp
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

#include "megacmdsandbox.h"

using namespace mega;

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
    this->lastQuerytemporalBandwith = m_time();
    this->timeOfOverquota = m_time();
    this->secondsOverQuota = 0;
}

