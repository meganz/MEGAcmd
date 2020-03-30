#ifdef _WIN32
#include <winsock2.h>
#include <Windows.h>
#include <lzexpand.h>
#include <shlwapi.h>
#else
#include <sys/types.h>
#include <pwd.h>
#include <sys/file.h> //flock
#include <errno.h>
#include <unistd.h>
#endif

#include <iostream>
#include <sstream>
#include <fstream>

#include <time.h>

#include "UpdateTask.h"
#include "Preferences.h"

using namespace std;

namespace megacmdupdater {
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


#ifdef _WIN32
void utf8ToUtf16(const char* utf8data, string* utf16string)
{
    if(!utf8data)
    {
        utf16string->clear();
        utf16string->append("", 1);
        return;
    }

    int size = strlen(utf8data) + 1;

    // make space for the worst case
    utf16string->resize(size * sizeof(wchar_t));

    // resize to actual result
    utf16string->resize(sizeof(wchar_t) * MultiByteToWideChar(CP_UTF8, 0, utf8data, size, (wchar_t*)utf16string->data(),
                                                              utf16string->size() / sizeof(wchar_t) + 1));
    if (utf16string->size())
    {
        utf16string->resize(utf16string->size() - 1);
    }
    else
    {
        utf16string->append("", 1);
    }
}
// convert Windows Unicode to UTF-8
void utf16ToUtf8(const wchar_t* utf16data, int utf16size, string* utf8string)
{
    if(!utf16size)
    {
        utf8string->clear();
        return;
    }

    utf8string->resize((utf16size + 1) * 4);

    utf8string->resize(WideCharToMultiByte(CP_UTF8, 0, utf16data,
        utf16size,
        (char*)utf8string->data(),
        utf8string->size() + 1,
        NULL, NULL));
}
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
    else
    {
        if (SUCCEEDED(PathRemoveFileSpec(szPath)))
        {
            utf16ToUtf8(szPath, lstrlen(szPath), &configFolder);
        }
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
        utf8ToUtf16(thelockfile.c_str(),&wlockfile);
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

}//end of namespace

using namespace megacmdupdater;

#ifdef _WIN32
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <winsock2.h>
#include <windows.h>
#include <locale>
#include <codecvt>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine,int iCmdShow)
{
    int argc;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(),&argc);
    vector<const char*> args;
    for (int i = 0; i < argc; i++)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convert;
        args.push_back(convert.to_bytes(argv[i]).c_str());
    }

#else
int main(int argc, char *argv[])
{
    vector<const char*> args;
    if (argc > 1)
    {
        args = vector<const char*>(argv + 1, argv + argc);
    }
#endif

    bool doNotInstall = extractarg(args, "--do-not-install");
    bool emergencyupdate = extractarg(args, "--emergency-update");
    bool skiplockcheck = extractarg(args, "--skip-lock-check");

#ifdef _WIN32
    if(!emergencyupdate)
    {
        AllocConsole();
        freopen("CONOUT$", "w+", stdout);
    }
#endif

    if (!lockExecution() && !skiplockcheck)
    {
        cerr << "Another instance of MEGAcmd Updater is running. Execute with --skip-lock-check to force running (NOT RECOMMENDED)" << endl;
        return 0;
    }

    time_t currentTime = time(NULL);
    cout << "Process started at " << ctime(&currentTime) << endl;
    srand(currentTime);

    UpdateTask updater;
    bool updated = updater.checkForUpdates(emergencyupdate, doNotInstall);

    currentTime = time(NULL);
    cout << "Process finished at " << ctime(&currentTime) << endl;
    unlockExecution();
    return updated;
}
