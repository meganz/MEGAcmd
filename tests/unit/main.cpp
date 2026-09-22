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

#include "CrashCapture.h"
#include "TestUtils.h"

int main (int argc, char *argv[])
{
    // First thing, so that a crash produces a dump and a stack instead of a
    // process that simply disappears.
    megacmd::test::installCrashCapture();

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

    const int exitCode = RUN_ALL_TESTS();

    // If some other thread died while the tests ran, its capture must finish and
    // take the process with it. Returning from here first would report a clean
    // run for a process that crashed.
    megacmd::test::waitForCrashCaptureToFinish();

    return exitCode;
}
