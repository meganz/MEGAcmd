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

#ifdef __linux__
#include <sched.h>
#endif


#include <chrono>
#include <cassert>

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


#ifdef WIN32
/**
 * @brief Google tests outputing needs stdout mode to be _O_TEXT
 * otherwise low level printing will fail.
 * This is a custom listener to wrapp writing routines
 */
struct CustomTestEventListener : public ::testing::EmptyTestEventListener {
    std::unique_ptr<TestEventListener> mDefault;
    void OnTestStart(const ::testing::TestInfo& test_info) override {
        megacmd::WindowsNarrowStdoutGuard smg;
        mDefault->OnTestStart(test_info);
    }

    void OnTestPartResult(const ::testing::TestPartResult& result) override {
        megacmd::WindowsNarrowStdoutGuard smg;
        mDefault->OnTestPartResult(result);
    }

    void OnTestEnd(const ::testing::TestInfo& test_info) override {
        megacmd::WindowsNarrowStdoutGuard smg;
        mDefault->OnTestEnd(test_info);
    }
};
#endif
int main (int argc, char *argv[])
{

#ifdef WIN32
    // set default mode to U8TEXT.
    // PROs: we could actually get rid of _setmode to U8TEXT in WindowsUtf8StdoutGuard
    // CONS: any low level printing to stdout will crash if the mode is not _O_TEXT. at least gtest print do
    // Todo: do some testing to write odd number of chasr & flush
    auto OUTPUT_MODE = _O_U8TEXT;
    auto oldModeStdout = _setmode(_fileno(stdout), OUTPUT_MODE);
    auto oldModeStderr = _setmode(_fileno(stderr), OUTPUT_MODE);

    // TODO: this is required for non megacmd shell executables
    // TODO: cat use case (figure out how to make it work
    // TODO: environment variables to control all this
    megacmd::InterceptStreamBuffer intercept_cout(std::cout, std::wcout);
    megacmd::InterceptStreamBuffer intercept_cer(std::cerr, std::wcerr);
#endif

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

        constexpr bool logToCout = false;
        auto createDefaultStream = [] { return new megacmd::LoggedStreamDefaultFile(); };
        megacmd::executeServer(1, args.data(), createDefaultStream, logToCout, mega::MegaApi::LOG_LEVEL_MAX, mega::MegaApi::LOG_LEVEL_MAX);
    });

    auto waitReturn = serverWaitingPromise.get_future().wait_for(std::chrono::seconds(10));
    UNUSED(waitReturn);
    assert(waitReturn != std::future_status::timeout);

#ifdef WIN32
    // Set custom gtests event listener to control the output to stdout
    ::testing::TestEventListeners& listeners =
        ::testing::UnitTest::GetInstance()->listeners();
    auto customListener = new CustomTestEventListener();
    customListener->mDefault.reset(listeners.Release(listeners.default_result_printer()));
    listeners.Append(customListener);
#endif

    auto exitCode = RUN_ALL_TESTS();

    megacmd::stopServer();
    serverThread.join();

#ifdef _WIN32
    // We use a file to pass the exit code to Jenkins,
    // since it fails to get the actual value otherwise
    std::ofstream("exit_code.txt") << exitCode;
#endif

    return exitCode;
}
