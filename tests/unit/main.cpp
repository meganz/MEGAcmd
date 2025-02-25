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

#include "megacmdcommonutils.h"

#include "megaapi.h"
#include <gtest/gtest.h>

#include "TestUtils.h"

int main (int argc, char *argv[])
{
    mega::MegaApi::setLogToConsole(true);
    mega::MegaApi::setLogLevel(mega::MegaApi::LOG_LEVEL_MAX);

    testing::InitGoogleTest(&argc, argv);

#ifdef WIN32
    megacmd::Instance<megacmd::WindowsConsoleController> windowsConsoleController;

    // Set custom gtests event listener to control the output to stdout
    ::testing::TestEventListeners& listeners =
        ::testing::UnitTest::GetInstance()->listeners();
    auto customListener = new CustomTestEventListener();
    customListener->mDefault.reset(listeners.Release(listeners.default_result_printer()));
    listeners.Append(customListener);
#endif

    return RUN_ALL_TESTS();
}
