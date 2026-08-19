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

#include "CrashCapture.h"

#ifdef _WIN32

// fopen is not used here, but the CRT deprecation warnings for the string
// handling below are noise in a file whose whole job is to run once, at a point
// where the process is already dying.
#ifndef _CRT_SECURE_NO_WARNINGS
    #define _CRT_SECURE_NO_WARNINGS
#endif

#include <windows.h>

// The project is compiled with UNICODE defined, which makes dbghelp.h redirect
// its whole API to the wide variants: SymInitialize becomes SymInitializeW,
// SYMBOL_INFO becomes SYMBOL_INFOW, and so on. Everything here deals in narrow
// strings, so the mapping is switched off for this one include.
#pragma push_macro("UNICODE")
#pragma push_macro("_UNICODE")
#undef UNICODE
#undef _UNICODE
#include <dbghelp.h>
#pragma pop_macro("_UNICODE")
#pragma pop_macro("UNICODE")

#include <tlhelp32.h>

#include <crtdbg.h>
#include <csignal>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <initializer_list>
#include <new>
#include <typeinfo>

#pragma comment(lib, "dbghelp.lib")

#if defined(_M_X64)
    #define MEGACMD_STACKWALK_MACHINE IMAGE_FILE_MACHINE_AMD64
#elif defined(_M_IX86)
    #define MEGACMD_STACKWALK_MACHINE IMAGE_FILE_MACHINE_I386
#elif defined(_M_ARM64)
    #define MEGACMD_STACKWALK_MACHINE IMAGE_FILE_MACHINE_ARM64
#else
    #define MEGACMD_STACKWALK_MACHINE 0
#endif

namespace megacmd {
namespace test {
namespace {

// Exit statuses for the deaths that have no structured exception code of their
// own. They are outside the range the tests themselves can return, so the status
// alone identifies what happened.
constexpr UINT kUncaughtCxxException = 0xE0000101;
constexpr UINT kPureVirtualCall      = 0xE0000102;
constexpr UINT kInvalidParameter     = 0xE0000103;
constexpr UINT kDebugAssertion       = 0xE0000104;
constexpr UINT kAborted              = 0xE0000105;
constexpr UINT kCaptureWatchdog      = 0xE0000106;
constexpr UINT kShutdownHung         = 0xE0000107;

// Declared in ntstatus.h, which cannot be included after windows.h without a
// wall of macro redefinitions.
constexpr DWORD kStatusHeapCorruption     = 0xC0000374;
constexpr DWORD kStatusStackBufferOverrun  = 0xC0000409;
constexpr DWORD kCxxExceptionCode         = 0xE06D7363;

// How long the capture itself is allowed to take. Walking the stacks of
// suspended threads can deadlock against whatever lock the crashing thread was
// holding, and a hung capture would burn the whole job timeout, so a second
// watchdog kills the process once the dump is safely on disk.
constexpr DWORD CAPTURE_DEADLINE_MS = 120 * 1000;

constexpr int DEFAULT_SHUTDOWN_WATCHDOG_SECS = 300;

constexpr size_t PATH_ROOM = MAX_PATH * 2;
constexpr size_t LINE_ROOM = 4096;

// Everything the capture path needs is worked out up front, in installCrashCapture,
// so that a crash caused by a broken heap does not have to allocate to report
// itself.
char gDumpPath[PATH_ROOM] = {};
char gDumpPartialPath[PATH_ROOM] = {};
char gReportPath[PATH_ROOM] = {};
char gSummaryPath[PATH_ROOM] = {};
char gSymbolSearchPath[PATH_ROOM * 2] = {};

LONG gCapturing = 0;
bool gSymbolsReady = false;
HANDLE gReportHandle = INVALID_HANDLE_VALUE;
HANDLE gShutdownDone = nullptr;
HANDLE gShutdownWatchdog = nullptr;
DWORD gShutdownWatchdogMs = 0;
DWORD gDeadlineThreadId = 0;
LPTOP_LEVEL_EXCEPTION_FILTER gPreviousFilter = nullptr;

// Bypasses the CRT. The console controller puts stderr into _O_U8TEXT mode, in
// which a narrow write is read back as UTF-16 and an odd length trips the CRT's
// invalid-parameter handler, which this file has hooked. Writing to the handle
// avoids the text mode, the stdio locks and that whole loop.
void writeToStandardError(const char* text, size_t length)
{
    const HANDLE handle = GetStdHandle(STD_ERROR_HANDLE);
    if (handle && handle != INVALID_HANDLE_VALUE)
    {
        DWORD written = 0;
        WriteFile(handle, text, static_cast<DWORD>(length), &written, nullptr);
    }
}

void report(const char* format, ...)
{
    // Static, because the capture must not allocate. Only ever entered by the
    // one thread that won gCapturing.
    static char line[LINE_ROOM];

    va_list args;
    va_start(args, format);
    const int written = vsnprintf(line, sizeof(line), format, args);
    va_end(args);

    const size_t length = written < 0 ? strlen(line) : static_cast<size_t>(
        written < static_cast<int>(sizeof(line)) ? written : sizeof(line) - 1);

    if (gReportHandle != INVALID_HANDLE_VALUE)
    {
        DWORD done = 0;
        WriteFile(gReportHandle, line, static_cast<DWORD>(length), &done, nullptr);
    }

    writeToStandardError(line, length);
}

size_t copyInto(char* destination, size_t room, const char* text)
{
    const size_t length = strlen(text);
    const size_t fits = length < room ? length : (room ? room - 1 : 0);
    if (room)
    {
        memcpy(destination, text, fits);
        destination[fits] = '\0';
    }
    return fits;
}

void environmentVariable(const char* name, char* destination, size_t room)
{
    const DWORD written = GetEnvironmentVariableA(name, destination, static_cast<DWORD>(room));
    if (written == 0 || written >= room)
    {
        if (room)
        {
            destination[0] = '\0';
        }
    }
}

const char* exceptionName(DWORD code)
{
    switch (code)
    {
        case EXCEPTION_ACCESS_VIOLATION:         return "EXCEPTION_ACCESS_VIOLATION";
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
        case EXCEPTION_BREAKPOINT:               return "EXCEPTION_BREAKPOINT";
        case EXCEPTION_DATATYPE_MISALIGNMENT:    return "EXCEPTION_DATATYPE_MISALIGNMENT";
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
        case EXCEPTION_ILLEGAL_INSTRUCTION:      return "EXCEPTION_ILLEGAL_INSTRUCTION";
        case EXCEPTION_IN_PAGE_ERROR:            return "EXCEPTION_IN_PAGE_ERROR";
        case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "EXCEPTION_INT_DIVIDE_BY_ZERO";
        case EXCEPTION_INVALID_DISPOSITION:      return "EXCEPTION_INVALID_DISPOSITION";
        case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
        case EXCEPTION_PRIV_INSTRUCTION:         return "EXCEPTION_PRIV_INSTRUCTION";
        case EXCEPTION_SINGLE_STEP:              return "EXCEPTION_SINGLE_STEP";
        case EXCEPTION_STACK_OVERFLOW:           return "EXCEPTION_STACK_OVERFLOW";
        case kStatusHeapCorruption:              return "STATUS_HEAP_CORRUPTION";
        case kStatusStackBufferOverrun:          return "STATUS_STACK_BUFFER_OVERRUN (fail fast)";
        case kCxxExceptionCode:                  return "C++ exception (0xE06D7363)";
        default:                                 return "unknown exception code";
    }
}

// The debug CRT and the Windows heap fill released memory with recognisable
// patterns, so the faulting address usually says outright whether the target had
// already been freed.
const char* addressInterpretation(ULONG_PTR address)
{
    const ULONG_PTR middle = (address >> 16) & 0xFFFF;
    const ULONG_PTR low = address & 0xFFFFFFFF;

    if (address == 0)
    {
        return " (null)";
    }
    if (middle == 0xDDDD || low == 0xDDDDDDDD)
    {
        return " (debug heap fill 0xDD: memory already freed)";
    }
    if (middle == 0xCDCD || low == 0xCDCDCDCD)
    {
        return " (debug heap fill 0xCD: memory never initialised)";
    }
    if (low == 0xFEEEFEEE)
    {
        return " (heap fill 0xFEEEFEEE: memory returned to the heap)";
    }
    if (low == 0xBAADF00D)
    {
        return " (heap fill 0xBAADF00D: allocated but never written)";
    }
    if (address < 0x10000)
    {
        return " (low address: most likely a member access through a null pointer)";
    }
    return "";
}

// Done once at startup. SymInitialize with module enumeration takes long enough
// in a Debug build with this many DLLs that running it from the crash path let a
// normal process exit finish first, and the stack was never written.
void loadSymbolMachinery()
{
    SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES |
                  SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_NO_PROMPTS);

    gSymbolsReady = SymInitialize(GetCurrentProcess(),
                                  gSymbolSearchPath[0] ? gSymbolSearchPath : nullptr,
                                  TRUE) != FALSE;
}

// Cheap: only picks up modules loaded since startup.
void refreshSymbols()
{
    if (gSymbolsReady)
    {
        SymRefreshModuleList(GetCurrentProcess());
        return;
    }

    // Startup did not manage it, so there is nothing to lose by trying here.
    loadSymbolMachinery();
}

// Describes one frame into a static buffer, and returns it so the innermost one
// can go into the summary.
const char* describeFrame(DWORD64 address)
{
    static char described[1024];

    const HANDLE process = GetCurrentProcess();

    char moduleName[64] = "?";
    IMAGEHLP_MODULE64 module = {};
    module.SizeOfStruct = sizeof(module);
    if (SymGetModuleInfo64(process, address, &module))
    {
        copyInto(moduleName, sizeof(moduleName), module.ModuleName);
    }

    alignas(SYMBOL_INFO) char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {};
    auto* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolBuffer);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    DWORD64 symbolOffset = 0;
    if (SymFromAddr(process, address, &symbolOffset, symbol))
    {
        DWORD lineOffset = 0;
        IMAGEHLP_LINE64 line = {};
        line.SizeOfStruct = sizeof(line);
        if (SymGetLineFromAddr64(process, address, &lineOffset, &line))
        {
            snprintf(described, sizeof(described), "%s!%s+0x%llx  [%s:%lu]",
                     moduleName, symbol->Name,
                     static_cast<unsigned long long>(symbolOffset),
                     line.FileName, line.LineNumber);
        }
        else
        {
            snprintf(described, sizeof(described), "%s!%s+0x%llx",
                     moduleName, symbol->Name,
                     static_cast<unsigned long long>(symbolOffset));
        }
    }
    else
    {
        snprintf(described, sizeof(described), "%s!0x%llx (no symbol)",
                 moduleName, static_cast<unsigned long long>(address));
    }

    return described;
}

// An abort gives no EXCEPTION_POINTERS, so the walk starts inside this file and
// the first frames are always the handler itself followed by the CRT's abort
// machinery. Naming any of those in the summary is useless: the culprit is the
// first frame below them that belongs to the program.
bool isPlumbingFrame(const char* described)
{
    static const char* const plumbing[] = {
        "captureAndDie", "onDebugReport", "onAbort", "onTerminate", "onPureCall",
        "onInvalidParameter", "onUnhandledException", "walkStack", "describeFrame",
        "shutdownWatchdogProc", "captureDeadlineProc", "describeExceptionInFlight",
        "ucrtbased!", "ucrtbase!", "VCRUNTIME", "vcruntime", "!abort", "!terminate",
        "CrtDbgReport", "_wassert", "KERNELBASE!", "kernelbase!", "ntdll!",
    };

    for (const char* marker : plumbing)
    {
        if (strstr(described, marker))
        {
            return true;
        }
    }
    return false;
}

// Copies the innermost interesting described frame into @p innermost, for the
// summary.
void walkStack(HANDLE thread, const CONTEXT& startingContext,
               char* innermost, size_t innermostRoom)
{
    // StackWalk64 writes to the context it is given.
    CONTEXT context = startingContext;

    STACKFRAME64 frame = {};
#if defined(_M_X64)
    frame.AddrPC.Offset = context.Rip;
    frame.AddrFrame.Offset = context.Rbp;
    frame.AddrStack.Offset = context.Rsp;
#elif defined(_M_IX86)
    frame.AddrPC.Offset = context.Eip;
    frame.AddrFrame.Offset = context.Ebp;
    frame.AddrStack.Offset = context.Esp;
#elif defined(_M_ARM64)
    frame.AddrPC.Offset = context.Pc;
    frame.AddrFrame.Offset = context.Fp;
    frame.AddrStack.Offset = context.Sp;
#endif
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Mode = AddrModeFlat;

    bool any = false;
    for (int depth = 0; depth < 128; ++depth)
    {
        if (!StackWalk64(MEGACMD_STACKWALK_MACHINE, GetCurrentProcess(), thread, &frame,
                         &context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64,
                         nullptr))
        {
            break;
        }
        if (frame.AddrPC.Offset == 0)
        {
            break;
        }

        const char* described = describeFrame(frame.AddrPC.Offset);
        if (innermost && innermostRoom && innermost[0] == '\0' &&
            !isPlumbingFrame(described))
        {
            copyInto(innermost, innermostRoom, described);
        }
        any = true;

        report("  %02d  0x%016llx  %s\n", depth,
               static_cast<unsigned long long>(frame.AddrPC.Offset), described);
    }

    if (!any)
    {
        report("  (no frames could be walked)\n");
    }
}

void walkEveryOtherThread()
{
    const DWORD ownProcess = GetCurrentProcessId();
    const DWORD ownThread = GetCurrentThreadId();

    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
    {
        report("\ncould not enumerate the other threads: error %lu\n", GetLastError());
        return;
    }

    THREADENTRY32 entry = {};
    entry.dwSize = sizeof(entry);
    if (!Thread32First(snapshot, &entry))
    {
        CloseHandle(snapshot);
        return;
    }

    do
    {
        if (entry.th32OwnerProcessID != ownProcess || entry.th32ThreadID == ownThread)
        {
            continue;
        }

        // Never suspend the thread that is holding the deadline: freezing it
        // would disarm the only thing that can rescue a stuck capture.
        if (entry.th32ThreadID == gDeadlineThreadId)
        {
            continue;
        }

        const HANDLE thread = OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION |
                                         THREAD_SUSPEND_RESUME, FALSE, entry.th32ThreadID);
        if (!thread)
        {
            report("\nthread %lu: could not be opened (error %lu)\n",
                   entry.th32ThreadID, GetLastError());
            continue;
        }

        report("\nthread %lu:\n", entry.th32ThreadID);

        if (SuspendThread(thread) == static_cast<DWORD>(-1))
        {
            report("  (could not be suspended: error %lu)\n", GetLastError());
            CloseHandle(thread);
            continue;
        }

        CONTEXT context = {};
        context.ContextFlags = CONTEXT_FULL;
        if (GetThreadContext(thread, &context))
        {
            walkStack(thread, context, nullptr, 0);
        }
        else
        {
            report("  (context unavailable: error %lu)\n", GetLastError());
        }

        ResumeThread(thread);
        CloseHandle(thread);
    }
    while (Thread32Next(snapshot, &entry));

    CloseHandle(snapshot);
}

MINIDUMP_TYPE dumpType()
{
    char requested[32] = {};
    environmentVariable("MEGACMD_TEST_DUMP_TYPE", requested, sizeof(requested));

    if (strcmp(requested, "mini") == 0)
    {
        return static_cast<MINIDUMP_TYPE>(MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules);
    }
    if (strcmp(requested, "full") == 0)
    {
        return static_cast<MINIDUMP_TYPE>(MiniDumpWithFullMemory | MiniDumpWithHandleData |
                                          MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules |
                                          MiniDumpWithFullMemoryInfo);
    }

    // The default keeps the memory the stacks and registers point at, so the
    // object the teardown tripped over can be inspected after the fact even
    // though it had already been freed, without the size of a full heap dump.
    return static_cast<MINIDUMP_TYPE>(MiniDumpWithDataSegs | MiniDumpWithHandleData |
                                      MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules |
                                      MiniDumpWithFullMemoryInfo |
                                      MiniDumpWithIndirectlyReferencedMemory);
}

// The first thing the capture does. No allocation, no stdio, a path worked out
// before the process was in trouble.
void writeMiniDump(EXCEPTION_POINTERS* pointers)
{
    // Written under a partial name and renamed only once it is complete. A file
    // created here and never filled, because the capture was killed part way,
    // would otherwise be indistinguishable from a real dump: that is exactly what
    // made one run report a reproduction it had not had. A leftover .partial is
    // itself the useful signal that a capture began and did not finish.
    const HANDLE file = CreateFileA(gDumpPartialPath, GENERIC_WRITE, 0, nullptr,
                                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        report("could not create %s: error %lu\n", gDumpPartialPath, GetLastError());
        return;
    }

    MINIDUMP_EXCEPTION_INFORMATION information = {};
    information.ThreadId = GetCurrentThreadId();
    information.ExceptionPointers = pointers;
    information.ClientPointers = FALSE;

    const BOOL written = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file,
                                           dumpType(),
                                           pointers ? &information : nullptr, nullptr, nullptr);

    LARGE_INTEGER size = {};
    GetFileSizeEx(file, &size);
    CloseHandle(file);

    if (written && MoveFileExA(gDumpPartialPath, gDumpPath, MOVEFILE_REPLACE_EXISTING))
    {
        report("minidump written to %s (%.1f MB)\n", gDumpPath,
               static_cast<double>(size.QuadPart) / (1024.0 * 1024.0));
    }
    else if (written)
    {
        report("minidump written but %s could not be renamed to %s: error %lu\n",
               gDumpPartialPath, gDumpPath, GetLastError());
    }
    else
    {
        report("MiniDumpWriteDump failed for %s: error %lu\n",
               gDumpPartialPath, GetLastError());
        DeleteFileA(gDumpPartialPath);
    }
}

DWORD WINAPI captureDeadlineProc(LPVOID)
{
    Sleep(CAPTURE_DEADLINE_MS);

    // Kill first. Anything that touches stdio here could block on the very lock
    // that the stuck capture is waiting for, and then nothing rescues the run.
    TerminateProcess(GetCurrentProcess(), kCaptureWatchdog);
    return 0;
}

void describeCause(const char* what, EXCEPTION_POINTERS* pointers,
                   char* summary, size_t summaryRoom)
{
    if (pointers && pointers->ExceptionRecord)
    {
        const EXCEPTION_RECORD& record = *pointers->ExceptionRecord;

        snprintf(summary, summaryRoom, "%s: %s (0x%08lx) at 0x%016llx",
                 what, exceptionName(record.ExceptionCode), record.ExceptionCode,
                 static_cast<unsigned long long>(
                     reinterpret_cast<ULONG_PTR>(record.ExceptionAddress)));
        report("%s\n", summary);

        if ((record.ExceptionCode == EXCEPTION_ACCESS_VIOLATION ||
             record.ExceptionCode == EXCEPTION_IN_PAGE_ERROR) &&
            record.NumberParameters >= 2)
        {
            const ULONG_PTR operation = record.ExceptionInformation[0];
            const ULONG_PTR address = record.ExceptionInformation[1];

            const char* verb = operation == 0 ? "reading" :
                               operation == 1 ? "writing" :
                               operation == 8 ? "executing" : "accessing";

            char detail[256];
            snprintf(detail, sizeof(detail), " %s address 0x%016llx%s",
                     verb, static_cast<unsigned long long>(address),
                     addressInterpretation(address));
            report(" %s\n", detail);

            const size_t used = strlen(summary);
            if (used + 1 < summaryRoom)
            {
                copyInto(summary + used, summaryRoom - used, detail);
            }
        }
    }
    else
    {
        copyInto(summary, summaryRoom, what);
        report("%s\n", what);
    }
}

// A single line the pipeline can echo without parsing the whole report.
void writeSummary(const char* summary)
{
    const HANDLE file = CreateFileA(gSummaryPath, GENERIC_WRITE, 0, nullptr,
                                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return;
    }

    DWORD done = 0;
    WriteFile(file, summary, static_cast<DWORD>(strlen(summary)), &done, nullptr);
    WriteFile(file, "\n", 1, &done, nullptr);
    CloseHandle(file);
}

// Every route into this function ends with the process dead. The order is chosen
// so the minidump lands on disk before anything that could allocate or block.
[[noreturn]] void captureAndDie(const char* what, UINT exitCode, EXCEPTION_POINTERS* pointers)
{
    if (InterlockedCompareExchange(&gCapturing, 1, 0) != 0)
    {
        // A second thread died while the first was being captured. Let the first
        // one finish; the deadline thread is already running and will take the
        // process down if it cannot.
        for (;;)
        {
            Sleep(1000);
        }
    }

    if (const HANDLE deadline = CreateThread(nullptr, 0, captureDeadlineProc, nullptr, 0,
                                             &gDeadlineThreadId))
    {
        CloseHandle(deadline);
    }
    else
    {
        const char* warning = "crash capture: the deadline thread could not be started, "
                              "so a stuck capture will not be cut short\n";
        writeToStandardError(warning, strlen(warning));
    }

    // The dump first, on a path that was worked out at startup, so a broken heap
    // cannot cost us the one artefact that always pays for itself.
    writeMiniDump(pointers);

    gReportHandle = CreateFileA(gReportPath, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    SYSTEMTIME now = {};
    GetSystemTime(&now);

    report("\n"
           "================ MEGACMD TEST CRASH CAPTURE ================\n"
           "%04u-%02u-%02u %02u:%02u:%02u.%03u UTC  pid %lu  thread %lu\n",
           now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond,
           now.wMilliseconds, GetCurrentProcessId(), GetCurrentThreadId());

    static char summary[1024];
    describeCause(what, pointers, summary, sizeof(summary));

    refreshSymbols();

    report("\nfaulting thread %lu:\n", GetCurrentThreadId());

    CONTEXT context = {};
    if (pointers && pointers->ContextRecord)
    {
        context = *pointers->ContextRecord;
    }
    else
    {
        RtlCaptureContext(&context);
    }

    static char innermost[1024] = {};
    walkStack(GetCurrentThread(), context, innermost, sizeof(innermost));

    if (innermost[0])
    {
        const size_t used = strlen(summary);
        if (used + 5 < sizeof(summary))
        {
            copyInto(summary + used, sizeof(summary) - used, " in ");
            const size_t withIn = strlen(summary);
            copyInto(summary + withIn, sizeof(summary) - withIn, innermost);
        }
    }

    report("\n---- every other thread ----\n");
    walkEveryOtherThread();

    report("\nsummary: %s\n"
           "================ END OF CRASH CAPTURE ================\n", summary);

    if (gReportHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(gReportHandle);
        gReportHandle = INVALID_HANDLE_VALUE;
    }

    writeSummary(summary);

    TerminateProcess(GetCurrentProcess(), exitCode);

    // TerminateProcess on the current process does not return.
    for (;;)
    {
        Sleep(1000);
    }
}

LONG WINAPI onUnhandledException(EXCEPTION_POINTERS* pointers)
{
    const DWORD code = pointers && pointers->ExceptionRecord
                           ? pointers->ExceptionRecord->ExceptionCode
                           : static_cast<DWORD>(EXCEPTION_ACCESS_VIOLATION);

    captureAndDie("unhandled structured exception", code, pointers);
    return EXCEPTION_EXECUTE_HANDLER;
}

// MSVC keeps the terminate handler per thread, so std::set_terminate in
// installCrashCapture only covers the thread that called it. An uncaught C++
// exception on any other thread therefore reaches the CRT's own default handler,
// which calls abort() and leaves nothing but "abort() has been called". Asking
// for the exception in flight recovers the one thing that message throws away.
size_t describeExceptionInFlight(char* into, size_t room)
{
    if (!std::current_exception())
    {
        return 0;
    }

    try
    {
        std::rethrow_exception(std::current_exception());
    }
    catch (const std::exception& exception)
    {
        return static_cast<size_t>(snprintf(into, room,
            ", with an uncaught C++ exception in flight on this thread: %s: %s",
            typeid(exception).name(), exception.what()));
    }
    catch (...)
    {
        return static_cast<size_t>(snprintf(into, room,
            ", with an uncaught C++ exception in flight on this thread, of a type "
            "that does not derive from std::exception"));
    }
}

void onTerminate()
{
    static char what[1024];
    copyInto(what, sizeof(what), "uncaught C++ exception");

    if (std::current_exception())
    {
        try
        {
            std::rethrow_exception(std::current_exception());
        }
        catch (const std::exception& exception)
        {
            snprintf(what, sizeof(what), "uncaught C++ exception: %s", exception.what());
        }
        catch (...)
        {
            copyInto(what, sizeof(what),
                     "uncaught C++ exception of a type that does not derive from std::exception");
        }
    }
    else
    {
        copyInto(what, sizeof(what), "std::terminate with no exception in flight");
    }

    captureAndDie(what, kUncaughtCxxException, nullptr);
}

void onPureCall()
{
    captureAndDie("pure virtual function call, which means a virtual call on a "
                  "partially destroyed object",
                  kPureVirtualCall, nullptr);
}

void __cdecl onInvalidParameter(const wchar_t* expression, const wchar_t* function,
                                const wchar_t* file, unsigned int line, uintptr_t)
{
    static char what[1024];
    snprintf(what, sizeof(what),
             "invalid parameter passed to a CRT function: %ls in %ls at %ls:%u",
             expression ? expression : L"?", function ? function : L"?",
             file ? file : L"?", line);

    captureAndDie(what, kInvalidParameter, nullptr);
}

void onAbort(int)
{
    static char what[1024];
    const int used = snprintf(what, sizeof(what), "abort()");
    if (used > 0 && static_cast<size_t>(used) < sizeof(what))
    {
        describeExceptionInFlight(what + used, sizeof(what) - static_cast<size_t>(used));
    }

    captureAndDie(what, kAborted, nullptr);
}

#ifdef _DEBUG
int onDebugReport(int reportType, char* message, int* returnValue)
{
    if (reportType == _CRT_WARN)
    {
        // Warnings are noise, not deaths.
        return FALSE;
    }

    static char what[1024];
    int used = snprintf(what, sizeof(what), "debug %s: %s",
                        reportType == _CRT_ASSERT ? "assertion" : "error",
                        message ? message : "(no message)");

    // Trim the newline the CRT puts on its abort message, so the addition below
    // reads as one sentence.
    while (used > 0 && (what[used - 1] == '\n' || what[used - 1] == '\r'))
    {
        what[--used] = '\0';
    }

    if (used > 0 && static_cast<size_t>(used) < sizeof(what))
    {
        describeExceptionInFlight(what + used, sizeof(what) - static_cast<size_t>(used));
    }

    if (returnValue)
    {
        *returnValue = 0;
    }

    captureAndDie(what, kDebugAssertion, nullptr);
}

// Holding freed blocks back keeps the debug fill pattern in place, so a read
// through a dangling pointer faults on recognisable memory instead of quietly
// finding a recycled object. It costs memory, since nothing is ever returned to
// the heap, which is why it is opt in.
void hardenDebugHeap()
{
    char mode[32] = {};
    environmentVariable("MEGACMD_TEST_DEBUG_HEAP", mode, sizeof(mode));

    if (mode[0] == '\0' || strcmp(mode, "off") == 0)
    {
        return;
    }

    int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF;

    if (strcmp(mode, "delayfree") == 0 || strcmp(mode, "checkalways") == 0)
    {
        flags |= _CRTDBG_DELAY_FREE_MEM_DF;
    }
    if (strcmp(mode, "checkalways") == 0)
    {
        flags |= _CRTDBG_CHECK_ALWAYS_DF;
    }

    _CrtSetDbgFlag(flags);

    char note[128];
    snprintf(note, sizeof(note), "crash capture: debug heap hardening is %s\n", mode);
    writeToStandardError(note, strlen(note));
}
#endif

DWORD WINAPI shutdownWatchdogProc(LPVOID)
{
    if (WaitForSingleObject(gShutdownDone, gShutdownWatchdogMs) == WAIT_OBJECT_0)
    {
        return 0;
    }

    static char what[256];
    snprintf(what, sizeof(what),
             "shutdown did not finish within %lu seconds, so it is stuck rather than crashed",
             gShutdownWatchdogMs / 1000);

    captureAndDie(what, kShutdownHung, nullptr);
}

void workOutArtefactPaths()
{
    char directory[PATH_ROOM] = {};
    environmentVariable("MEGACMD_TEST_DUMP_DIR", directory, sizeof(directory));
    if (directory[0] == '\0')
    {
        copyInto(directory, sizeof(directory), ".");
    }

    size_t end = strlen(directory);
    while (end > 1 && (directory[end - 1] == '\\' || directory[end - 1] == '/'))
    {
        directory[--end] = '\0';
    }
    CreateDirectoryA(directory, nullptr);

    const DWORD pid = GetCurrentProcessId();
    snprintf(gDumpPath, sizeof(gDumpPath), "%s\\crash-%lu.dmp", directory, pid);
    snprintf(gDumpPartialPath, sizeof(gDumpPartialPath),
             "%s\\crash-%lu.dmp.partial", directory, pid);
    snprintf(gReportPath, sizeof(gReportPath), "%s\\crash-%lu-report.txt", directory, pid);
    snprintf(gSummaryPath, sizeof(gSummaryPath), "%s\\crash-%lu-summary.txt", directory, pid);

    // The build leaves the pdb next to the exe, and the pipeline may point
    // _NT_SYMBOL_PATH at more.
    char exePath[MAX_PATH] = {};
    if (GetModuleFileNameA(nullptr, exePath, static_cast<DWORD>(sizeof(exePath))))
    {
        if (char* lastSeparator = strrchr(exePath, '\\'))
        {
            *lastSeparator = '\0';
            copyInto(gSymbolSearchPath, sizeof(gSymbolSearchPath), exePath);
        }
    }

    char extra[PATH_ROOM] = {};
    environmentVariable("_NT_SYMBOL_PATH", extra, sizeof(extra));
    if (extra[0])
    {
        const size_t used = strlen(gSymbolSearchPath);
        if (used)
        {
            copyInto(gSymbolSearchPath + used, sizeof(gSymbolSearchPath) - used, ";");
        }
        const size_t withSeparator = strlen(gSymbolSearchPath);
        copyInto(gSymbolSearchPath + withSeparator,
                 sizeof(gSymbolSearchPath) - withSeparator, extra);
    }
}

} // namespace

void installCrashCapture()
{
    workOutArtefactPaths();
    loadSymbolMachinery();

    // No modal dialogs on a machine with nobody in front of it.
    SetErrorMode(GetErrorMode() | SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    // Leave enough stack for the filter to run after a stack overflow. Per
    // thread, so this only covers the one calling here.
    ULONG guarantee = 64 * 1024;
    SetThreadStackGuarantee(&guarantee);

    gPreviousFilter = SetUnhandledExceptionFilter(onUnhandledException);

    // MSVC keeps this per thread, so it only covers the caller. The abort routes
    // below are what catch a terminate on any other thread.
    std::set_terminate(onTerminate);
    _set_purecall_handler(onPureCall);
    _set_invalid_parameter_handler(onInvalidParameter);

    // Keep the abort message, drop the Windows Error Reporting handover, and
    // catch the signal so abort() produces a dump like everything else.
    _set_abort_behavior(_WRITE_ABORT_MSG, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    signal(SIGABRT, onAbort);

#ifdef _DEBUG
    // A debug assertion would otherwise wait forever on a dialog nobody can
    // click. Route the text to stderr and capture it like a crash.
    for (const int reportType : {_CRT_ERROR, _CRT_ASSERT})
    {
        _CrtSetReportMode(reportType, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(reportType, _CRTDBG_FILE_STDERR);
    }
    _CrtSetReportHook(onDebugReport);

    hardenDebugHeap();
#endif

    // Silent when all is well. A crash prints its own banner, and the report
    // carries the paths.
}

void reassertCrashCapture()
{
    // Anything loaded after installCrashCapture can take the filter over without
    // saying so, and the symptom of that is exactly what is being investigated:
    // a process that vanishes with no output. Put it back and say if it moved.
    const LPTOP_LEVEL_EXCEPTION_FILTER displaced =
        SetUnhandledExceptionFilter(onUnhandledException);

    if (displaced != onUnhandledException)
    {
        char note[256];
        snprintf(note, sizeof(note),
                 "crash capture: the unhandled exception filter had been taken over by "
                 "0x%p and has been put back\n", reinterpret_cast<void*>(displaced));
        writeToStandardError(note, strlen(note));
    }
}

void armShutdownWatchdog(int seconds)
{
    char configured[32] = {};
    environmentVariable("MEGACMD_TEST_SHUTDOWN_WATCHDOG_SECS", configured, sizeof(configured));
    if (configured[0])
    {
        seconds = atoi(configured);
    }
    else if (seconds <= 0)
    {
        seconds = DEFAULT_SHUTDOWN_WATCHDOG_SECS;
    }

    if (seconds <= 0)
    {
        return;
    }

    reassertCrashCapture();

    gShutdownWatchdogMs = static_cast<DWORD>(seconds) * 1000;
    gShutdownDone = CreateEventA(nullptr, TRUE, FALSE, nullptr);
    if (!gShutdownDone)
    {
        return;
    }

    gShutdownWatchdog = CreateThread(nullptr, 0, shutdownWatchdogProc, nullptr, 0, nullptr);
}

void waitForCrashCaptureToFinish()
{
    if (InterlockedCompareExchange(&gCapturing, 0, 0) == 0)
    {
        return;
    }

    // Another thread is being captured right now, and a capture always ends by
    // terminating the process. Exiting normally from here would race it and hide
    // the crash behind a status of zero, which is exactly what happened the first
    // time this ran. So stop, and let the capture have the process.
    const char* note = "crash capture: a capture is in flight, holding the exit until it "
                       "is done\n";
    writeToStandardError(note, strlen(note));

    // Bounded, in case the capture's own deadline thread never started.
    for (DWORD waited = 0; waited < CAPTURE_DEADLINE_MS + 30000; waited += 100)
    {
        Sleep(100);
    }

    const char* giving = "crash capture: the in-flight capture never finished\n";
    writeToStandardError(giving, strlen(giving));
    TerminateProcess(GetCurrentProcess(), kCaptureWatchdog);
}

void disarmShutdownWatchdog()
{
    if (gShutdownDone)
    {
        SetEvent(gShutdownDone);
    }

    if (gShutdownWatchdog)
    {
        WaitForSingleObject(gShutdownWatchdog, 5000);
        CloseHandle(gShutdownWatchdog);
        gShutdownWatchdog = nullptr;
    }
}

int defaultShutdownWatchdogSeconds()
{
    return DEFAULT_SHUTDOWN_WATCHDOG_SECS;
}

} // namespace test
} // namespace megacmd

#else // _WIN32

namespace megacmd {
namespace test {

void installCrashCapture() {}
void reassertCrashCapture() {}
void waitForCrashCaptureToFinish() {}
void armShutdownWatchdog(int) {}
void disarmShutdownWatchdog() {}
int defaultShutdownWatchdogSeconds() { return 0; }

} // namespace test
} // namespace megacmd

#endif // _WIN32
