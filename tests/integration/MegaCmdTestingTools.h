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

#include <gtest/gtest.h>

#include "megacmdcommonutils.h"
#include "megacmd.h"
#include "client/megacmdclient.h"

#include <gtest/gtest.h>
#include "Instruments.h"

#include <chrono>
#include <future>

struct ClientResponse
{
    int mStatus = -1;
    OUTSTRING mOut;
    OUTSTRING mErr;
};

ClientResponse executeInClient(std::list<std::string> command, bool nonInteractive = true);
void ensureLoggedIn();
void ensureReadStructure();

class BasicGenericTest : public ::testing::Test
{
};

class LoggedInTest : public BasicGenericTest
{
    void SetUp() override
    {
        ensureLoggedIn();
    }
};

class ReadTest : public LoggedInTest
{
    void SetUp() override
    {
        ensureReadStructure();
    }
};
