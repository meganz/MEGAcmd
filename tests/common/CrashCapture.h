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

namespace megacmd {
namespace test {

/**
 * @brief Installs process-wide crash capture.
 *
 * On Windows this takes over every route by which the process can die without
 * running to the end of main: unhandled structured exceptions on any thread,
 * uncaught C++ exceptions, abort(), pure virtual calls, invalid CRT parameters
 * and debug assertions. Each of them writes a minidump plus a symbolised stack
 * of every thread, then terminates the process with a status that names the
 * cause.
 *
 * It cannot see the deaths Windows does not route through user-mode handlers,
 * chiefly the fail fast that the heap manager raises on detecting corruption.
 * Those leave only an exit status, which is why the pipeline reports it.
 *
 * Everything is written to the directory named by MEGACMD_TEST_DUMP_DIR, or to
 * the current directory when that variable is not set. MEGACMD_TEST_DUMP_TYPE
 * selects "mini" (stacks and modules), "full" (the whole heap, which can reach
 * several GB) or, by default, the memory the stacks and registers point at.
 * MEGACMD_TEST_DEBUG_HEAP takes "delayfree" or "checkalways" to stop the debug
 * heap recycling freed blocks, which makes a use-after-free fault instead of
 * quietly succeeding.
 *
 * Call it as the first statement of main, before anything else can die.
 *
 * A no-op on other platforms.
 */
void installCrashCapture();

/**
 * @brief Puts the unhandled exception filter back if something took it over.
 *
 * A library loaded after installCrashCapture can replace the filter without
 * saying so, and the symptom is a process that vanishes with no output. This
 * restores it and reports on standard error when it had in fact been displaced.
 *
 * A no-op on other platforms.
 */
void reassertCrashCapture();

/**
 * @brief Arms a watchdog that captures the state of every thread if shutdown
 * takes longer than @p seconds, then kills the process.
 *
 * Meant to bracket the server stop and join, so that a shutdown which hangs
 * produces the same evidence as one which crashes instead of consuming the whole
 * job timeout. Keep it comfortably below any external per-run timeout, or that
 * will kill the process first and there will be no evidence at all.
 *
 * Zero or a negative @p seconds selects defaultShutdownWatchdogSeconds(). The
 * MEGACMD_TEST_SHUTDOWN_WATCHDOG_SECS variable overrides the argument, and can
 * be set to zero to switch the watchdog off.
 *
 * A no-op on other platforms.
 */
void armShutdownWatchdog(int seconds);

/**
 * @brief Disarms the shutdown watchdog after a shutdown that completed.
 *
 * A no-op on other platforms.
 */
void disarmShutdownWatchdog();

/**
 * @brief Blocks while another thread is being captured, and never returns if one
 * is.
 *
 * A capture always ends by terminating the process. Without this, a thread that
 * aborts during the teardown can be captured while the main thread walks on and
 * exits cleanly, so the process reports success and the crash is invisible from
 * the outside. Call it on the normal exit path, before anything records that the
 * run succeeded.
 *
 * A no-op on other platforms.
 */
void waitForCrashCaptureToFinish();

/**
 * @brief The watchdog period used when armShutdownWatchdog is not given one.
 *
 * Zero on platforms where the watchdog does nothing.
 */
int defaultShutdownWatchdogSeconds();

} // namespace test
} // namespace megacmd
