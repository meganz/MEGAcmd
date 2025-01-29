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

#include "megacmd.h"
#include "megaapi.h"


#include "megacmdlogger.h"
#include "megacmd_rotating_logger.h"

#include <vector>

using std::vector;

bool extractarg(vector<const char*>& args, const char *what)
{
    for (int i = int(args.size()); i--; )
    {
        if (!strcmp(args[i], what))
        {
            args.erase(args.begin() + i);
            return true;
        }
    }
    return false;
}

bool extractargparam(vector<const char*>& args, const char *what, std::string& param)
{
    for (int i = int(args.size()) - 1; --i >= 0; )
    {
        if (!strcmp(args[i], what) && args.size() > i)
        {
            param = args[i + 1];
            args.erase(args.begin() + i, args.begin() + i + 2);
            return true;
        }
    }
    return false;
}

std::pair<int/*sdk*/, int/*cmd*/> getLogLevels(std::vector<const char*>& args)
{
    using mega::MegaApi;
    // default log levels
    int sdkLogLevel = MegaApi::LOG_LEVEL_ERROR;
    int cmdLogLevel = MegaApi::LOG_LEVEL_INFO;

    // for !Windows have DEBUG builds use debug level by default
#if defined(DEBUG) && !defined(_WIN32)
    sdkLogLevel = MegaApi::LOG_LEVEL_DEBUG;
    cmdLogLevel = MegaApi::LOG_LEVEL_DEBUG;
#endif

    // read parameters from argumetns
    bool debug = extractarg(args, "--debug");
    bool debugfull = extractarg(args, "--debug-full");
    bool verbose = extractarg(args, "--verbose");
    bool verbosefull = extractarg(args, "--verbose-full");

// overridance by environment variable
    const char *envCmdLogLevel = getenv("MEGACMD_LOGLEVEL");
    if (envCmdLogLevel)
    {
        std::string loglevelenv(envCmdLogLevel);
        debug |= !loglevelenv.compare("DEBUG");
        debugfull |= !loglevelenv.compare("FULLDEBUG");
        verbose |= !loglevelenv.compare("VERBOSE");
        verbosefull |= !loglevelenv.compare("FULLVERBOSE");
    }

    if (debug)
    {
        cmdLogLevel = MegaApi::LOG_LEVEL_DEBUG;
    }
    if (debugfull)
    {
        sdkLogLevel = MegaApi::LOG_LEVEL_DEBUG;
        cmdLogLevel = MegaApi::LOG_LEVEL_DEBUG;
    }
    if (verbose)
    {
        cmdLogLevel = MegaApi::LOG_LEVEL_MAX;
    }
    if (verbosefull)
    {
        sdkLogLevel = MegaApi::LOG_LEVEL_MAX;
        cmdLogLevel = MegaApi::LOG_LEVEL_MAX;
    }

    return {sdkLogLevel, cmdLogLevel};
}

#ifdef _WIN32
// returns true if attempted to uninstall
bool mayUninstall(std::vector<const char*> &args)
{
    bool buninstall = extractarg(args, "--uninstall") || extractarg(args, "/uninstall");
    if (buninstall)
    {
        megacmd::uninstall();
        return true;
    }
    return false;
}
#endif

#ifndef _WIN32
#include <sys/wait.h>
static bool is_pid_running(pid_t pid) {

    while(waitpid(-1, 0, WNOHANG) > 0) {
        // Wait for defunct....
    }

    if (0 == kill(pid, 0))
    {
        return true; // Process exists
    }

    return 0;
}
#endif

// mechanism used for updates/restarts in order to wait for other process
void waitIfRequired(std::vector<const char*> &args)
{
    std::string shandletowait;
    bool dowaitforhandle = extractargparam(args, "--wait-for", shandletowait);
    if (dowaitforhandle)
    {
#ifdef _WIN32
        DWORD processId = atoi(shandletowait.c_str());

        HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);

        cout << "Waiting for former server to end" << endl;
        WaitForSingleObject( processHandle, INFINITE );
        CloseHandle(processHandle);
#else

        pid_t processId = atoi(shandletowait.c_str());

        while (is_pid_running(processId))
        {
            cerr << "Waiting for former MEGAcmd server to end:  " << processId << endl;
            sleep(1);
        }
#endif
    }
}

//TODO: things that will require testing (make them reach Jira release somehow)
// - ALL PARAMETERS (including api url)
// - windows uninstall and updating
// - linux updates (signals and waiting)
// - windows execution with non utf-8 paths
// - linux forking (clients into server)
int main(int argc, char* argv[])
{
#ifdef WIN32
    megacmd::Instance<megacmd::WindowsConsoleController> windowsConsoleController;
#endif

    std::vector<const char*> args;
    if (argc > 1)
    {
        args = std::vector<const char*>(argv + 1, argv + argc);
    }
#ifdef _WIN32
    if (mayUninstall(args))
    {
        return 0;
    }
#endif

    waitIfRequired(args);

    bool logToCout = !extractarg(args, "--do-not-log-to-stdout");
    auto logLevels = getLogLevels(args);
    int sdkLogLevel = logLevels.first;
    int cmdLogLevel = logLevels.second;

    bool skiplockcheck = extractarg(args, "--skip-lock-check");

    std::string debug_api_url;
    extractargparam(args, "--apiurl", debug_api_url);  // only for debugging
    bool disablepkp = extractarg(args, "--disablepkp");  // only for debugging

    auto createLoggedStream = []
    {
        return new megacmd::FileRotatingLoggedStream(megacmd::MegaCmdLogger::getDefaultFilePath());
    };

    return megacmd::executeServer(argc, argv, createLoggedStream,
                                  logToCout, sdkLogLevel, cmdLogLevel,
                                  skiplockcheck, debug_api_url, disablepkp);
}
