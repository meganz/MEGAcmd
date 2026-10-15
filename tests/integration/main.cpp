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

#include <gtest/gtest.h>

#include "megacmd.h"
#include "megacmdlogger.h"
#include "megacmd_rotating_logger.h"
#include "Instruments.h"

#include "CrashCapture.h"
#include "TestUtils.h"

#ifdef __linux__
#include <sched.h>
#endif


#include <chrono>
#include <cassert>
#include <fstream>

#ifndef UNUSED
    #define UNUSED(x) (void)(x)
#endif


#ifdef __linux__
static void setUpUnixSignals()
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
}
#endif

int main (int argc, char *argv[])
{
    // First thing, so that every later failure produces a dump and a stack
    // instead of a process that simply disappears.
    megacmd::test::installCrashCapture();

#ifdef __linux__
    if (getenv("MEGA_INTEGRATION_TEST_ENFORCE_SINGLE_CPU"))
    {
        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(1, &set);
        if (sched_setaffinity(getpid(), sizeof(set), &set) == -1)
        {
            std::cerr << "sched_setaffinity error: " << errno << std::endl;
            exit(errno);
        }
    }

    setUpUnixSignals();
#endif

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

    using TI = TestInstruments;

    std::promise<void> serverWaitingPromise;
    TI::Instance().onEventOnce(TI::Event::SERVER_ABOUT_TO_START_WAITING_FOR_PETITIONS,
                               [&serverWaitingPromise]() {
        serverWaitingPromise.set_value();
    });

    std::thread serverThread([] {

        std::vector<char*> args{
            (char *)"argv0_INTEGRATION_TESTS",
            nullptr
        };

        megacmd::LogConfig logConfig;
        logConfig.mSdkLogLevel = mega::MegaApi::LOG_LEVEL_MAX;
        logConfig.mCmdLogLevel = mega::MegaApi::LOG_LEVEL_MAX;
        logConfig.mLogToCout = false;
        logConfig.mJsonLogs = true;

        // Which thread wrote each record. Cheap, and the only way to read a
        // teardown that races an SDK thread.
        logConfig.mLogThreadId = true;

        // Draining on every record costs a lock and a notify per record from
        // every SDK thread, which is itself capable of moving the timing of a
        // race. Opt in, so a run meant to be comparable with a normal one can
        // leave it alone.
        if (getenv("MEGACMD_TEST_FLUSH_EVERY_RECORD"))
        {
            logConfig.mFlushOnLevel = mega::MegaApi::LOG_LEVEL_MAX;
        }

        auto createDefaultStream = [] { return new megacmd::FileRotatingLoggedStream(megacmd::MegaCmdLogger::getDefaultFilePath()); };
        megacmd::executeServer(1, args.data(), createDefaultStream, logConfig);
    });

    auto waitReturn = serverWaitingPromise.get_future().wait_for(std::chrono::seconds(10));
    UNUSED(waitReturn);
    assert(waitReturn != std::future_status::timeout);

    auto exitCode = RUN_ALL_TESTS();

#ifdef _WIN32
    // Written before the shutdown, so that a run whose tests all passed but
    // whose teardown died can be told apart from a run that failed its tests.
    std::ofstream("tests_done.txt") << exitCode;
#endif

    // Zero picks the built-in default, and MEGACMD_TEST_SHUTDOWN_WATCHDOG_SECS
    // overrides both, so the pipeline can retune this without a rebuild.
    megacmd::test::armShutdownWatchdog(0);

    megacmd::stopServer();
    serverThread.join();

    megacmd::test::disarmShutdownWatchdog();

    // If some other thread died during the shutdown, its capture must finish and
    // take the process with it. Writing the success marker first would report a
    // clean run for a process that crashed.
    megacmd::test::waitForCrashCaptureToFinish();

#ifdef _WIN32
    // We use a file to pass the exit code to Jenkins,
    // since it fails to get the actual value otherwise
    std::ofstream("exit_code.txt") << exitCode;
#endif

    return exitCode;
}
