#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/types.h>
#include <pwd.h>
#include <sys/file.h> //flock
#include <errno.h>
#endif

#include <iostream>
#include <sstream>
#include <fstream>

#include <time.h>
#include <unistd.h>

#include "UpdateTask.h"
#include "Preferences.h"

using namespace std;

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

#if !defined(_WIN32) && defined(LOCK_EX) && defined(LOCK_NB)
    static int fdMcmdUpdaterLockFile;
#endif

string getLockFile()
{
    string configFolder;
#ifdef _WIN32
   TCHAR szPath[MAX_PATH];
    if (!SUCCEEDED(GetModuleFileName(NULL, szPath , MAX_PATH)))
    {
        cerr << "Couldnt get EXECUTABLE folder" << endl;
    }
#else
    const char *homedir = NULL;

    homedir = getenv("HOME");
    if (!homedir)
    {
        struct passwd pd;
        struct passwd* pwdptr = &pd;
        struct passwd* tempPwdPtr;
        char pwdbuffer[200];
        int pwdlinelen = sizeof( pwdbuffer );

        if (( getpwuid_r(22, pwdptr, pwdbuffer, pwdlinelen, &tempPwdPtr)) != 0)
        {
            cerr << "Couldnt get HOME folder" << endl;
            return string();
        }
        else
        {
            homedir = pwdptr->pw_dir;
        }
    }
    configFolder = homedir;
#endif
    if (configFolder.size())
    {
        stringstream lockfile;
        lockfile << configFolder << "/" << "lockMCMDUpdater";
        return lockfile.str();
    }
    return string();
}

bool lockExecution()
{
    string thelockfile = getLockFile();
    if (thelockfile.size())
    {

    #ifdef _WIN32
        string wlockfile;
        MegaApi::utf8ToUtf16(thelockfile.c_str(),&wlockfile);
        OFSTRUCT offstruct;
        if (LZOpenFileW((LPWSTR)wlockfile.data(), &offstruct, OF_CREATE | OF_READWRITE |OF_SHARE_EXCLUSIVE ) == HFILE_ERROR)
        {
            return false;
        }
    #elif defined(LOCK_EX) && defined(LOCK_NB)
        fdMcmdUpdaterLockFile = open(thelockfile.c_str(), O_RDWR | O_CREAT, 0666); // open or create lockfile
        //check open success...
        if (flock(fdMcmdUpdaterLockFile, LOCK_EX | LOCK_NB))
        {
            return false;
        }

        if (fcntl(fdMcmdUpdaterLockFile, F_SETFD, FD_CLOEXEC) == -1)
        {
            cerr << "ERROR setting CLOEXEC to lock fdMcmdUpdaterLockFile: " << errno << endl;
        }

    #else
        ifstream fi(thelockfile.c_str());
        if(!fi.fail()){
            return false;
        }
        if (fi.is_open())
        {
            fi.close();
        }
        ofstream fo(thelockfile.c_str());
        if (fo.is_open())
        {
            fo.close();
        }
    #endif

        return true;
    }
    else
    {
        cerr << "ERROR figuring out lock file " << endl;

        return false;
    }
}


void unlockExecution()
{
    string thelockfile = getLockFile();

    if (thelockfile.size())
    {
#if !defined(_WIN32) && defined(LOCK_EX) && defined(LOCK_NB)
        flock(fdMcmdUpdaterLockFile, LOCK_UN | LOCK_NB);
        close(fdMcmdUpdaterLockFile);
#endif
        unlink(thelockfile.c_str());
    }
    else
    {
        cerr << "ERROR figuring out lock file " << endl;
    }
}


int main(int argc, char **argv)
{
    vector<const char*> args(argv + 1, argv + argc);

    bool doNotInstall = extractarg(args, "--do-not-install");
    bool emergencyupdate = extractarg(args, "--normal-update");
    bool skiplockcheck = extractarg(args, "--skip-lock-check");

    if (!lockExecution() && !skiplockcheck)
    {
        cerr << "Another instance of MEGAcmd Updater is running. Execute with --skip-lock-check to force running (NOT RECOMMENDED)" << endl;
        return 0;
    }

    time_t currentTime = time(NULL);
    cout << "Process started at " << ctime(&currentTime) << endl;
    srand(currentTime);

    UpdateTask updater;
    bool updated = updater.checkForUpdates(!emergencyupdate, doNotInstall);

    currentTime = time(NULL);
    cout << "Process finished at " << ctime(&currentTime) << endl;
    unlockExecution();
    return updated;
}
