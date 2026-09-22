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

#include <atomic>
#include <chrono>
#include <future>
#include <thread>

#include <gtest/gtest.h>

#include "TestUtils.h"
#include "deferred_single_trigger.h"

namespace {

using namespace std::chrono_literals;

// A shot armed with no wait fires as soon as its thread gets to run.
constexpr auto noWait = 0s;
constexpr auto waitLongerThanTheTest = 60s;

constexpr auto giveUp = 5s;
constexpr auto longEnoughToFire = 200ms;

}

TEST(DeferredSingleTriggerTest, shotFires)
{
    std::promise<void> fired;
    DeferredSingleTrigger trigger(noWait);

    trigger.triggerDeferredSingleShot([&fired] { fired.set_value(); });

    EXPECT_EQ(fired.get_future().wait_for(giveUp), std::future_status::ready);
}

// The server stops its deferred work before deleting the api the callbacks use,
// so once stop() returns nothing may be pending and nothing may be running.
TEST(DeferredSingleTriggerTest, stop)
{
    G_SUBTEST << "Cancels a pending shot";
    {
        std::promise<void> fired;
        DeferredSingleTrigger trigger(waitLongerThanTheTest);

        trigger.triggerDeferredSingleShot([&fired] { fired.set_value(); });
        trigger.stop();

        EXPECT_EQ(fired.get_future().wait_for(longEnoughToFire), std::future_status::timeout);
    }

    G_SUBTEST << "Waits for a shot already running";
    {
        std::promise<void> started;
        std::atomic_bool finished(false);
        DeferredSingleTrigger trigger(noWait);

        trigger.triggerDeferredSingleShot([&started, &finished]
        {
            started.set_value();
            std::this_thread::sleep_for(longEnoughToFire);
            finished = true;
        });
        ASSERT_EQ(started.get_future().wait_for(giveUp), std::future_status::ready);

        trigger.stop();

        EXPECT_TRUE(finished);
    }

    G_SUBTEST << "Ignores a later trigger";
    {
        std::promise<void> fired;
        DeferredSingleTrigger trigger(noWait);

        trigger.stop();
        trigger.triggerDeferredSingleShot([&fired] { fired.set_value(); });

        EXPECT_EQ(fired.get_future().wait_for(longEnoughToFire), std::future_status::timeout);
    }

    G_SUBTEST << "Can be called more than once, and before the destructor";
    {
        DeferredSingleTrigger trigger(noWait);

        trigger.stop();
        trigger.stop();
    }
}
