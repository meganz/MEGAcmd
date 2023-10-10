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

#include "MegaCmdTestingTools.h"
#include "TestUtils.h"

#include "megacmdcommonutils.h"
#include "megacmd.h"

#include <gtest/gtest.h>
#include "Instruments.h"

#include <chrono>
#include <future>

class BasicGenericTest : public ::testing::Test
{
    std::thread mServerThread;
    void SetUp() override
    {
        mServerThread = std::thread([](){
            char **args = new char*[2];
            args[0]=(char *)"argv0_INTEGRATION_TESTS";
            args[1] = NULL;
            megacmd::executeServer(1, args);
        });

        using TI = TestInstruments;

        std::promise<void> serverWaitingPromise;
        TI::Instance().onEventOnce(TI::Event::SERVER_ABOUT_TO_START_WAITING_FOR_PETITIONS,
                [&serverWaitingPromise]() {
                    serverWaitingPromise.set_value();
                });

        ASSERT_NE(serverWaitingPromise.get_future().wait_for(std::chrono::seconds(10))
                    , std::future_status::timeout);
    }

    void TearDown() override
    {
        megacmd::stopServer();
        mServerThread.join();
    }
};

class NOINTERACTIVEBasicTest : public BasicGenericTest{};

TEST_F(NOINTERACTIVEBasicTest, BasicGenericTest)
{
    bool t = true;

    ASSERT_EQ(true, t);
}
