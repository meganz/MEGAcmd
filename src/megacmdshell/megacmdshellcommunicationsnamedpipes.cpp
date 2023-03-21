/**
 * @file src/megacmdshellcommunicationsnamedpipes.cpp
 * @brief MEGAcmd: Communications module to connect to server using NamedPipes
 *
 * (c) 2013 by Mega Limited, Auckland, New Zealand
 *
 * This file is part of the MEGAcmd.
 *
 * MEGAcmd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * @copyright Simplified (2-clause) BSD License.
 *
 * You should have received a copy of the license along with this
 * program.
 *
 * This file is also distributed under the terms of the GNU General
 * Public License, see http://www.gnu.org/copyleft/gpl.txt for details.
 */

#ifdef _WIN32
#include "megacmdshellcommunicationsnamedpipes.h"

#include <iostream>
#include <thread>
#include <sstream>

#include <shlobj.h> //SHGetFolderPath
#include <Shlwapi.h> //PathAppend
#include <Aclapi.h> //GetSecurityInfo
#include <Sddl.h> //ConvertSidToStringSid

#include <algorithm>

#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#ifndef _O_U16TEXT
#define _O_U16TEXT 0x00020000
#endif
#ifndef _O_U8TEXT
#define _O_U8TEXT 0x00040000
#endif

namespace megacmd {

bool MegaCmdShellCommunicationsNamedPipes::confirmResponse; //TODO: do all this only in parent class
bool MegaCmdShellCommunicationsNamedPipes::stopListener;
mega::Thread *MegaCmdShellCommunicationsNamedPipes::listenerThread;
HANDLE MegaCmdShellCommunicationsNamedPipes::newNamedPipe;

bool MegaCmdShellCommunicationsNamedPipes::namedPipeValid(HANDLE namedPipe)
{
    return namedPipe != INVALID_HANDLE_VALUE;
}

void MegaCmdShellCommunicationsNamedPipes::closeNamedPipe(HANDLE namedPipe){
    CloseHandle(namedPipe);
}

using namespace std;

BOOL GetLogonSID (PSID *ppsid)
{
   BOOL bSuccess = FALSE;
   DWORD dwIndex;
   DWORD dwLength = 0;
   PTOKEN_GROUPS ptg = NULL;

   HANDLE hToken = NULL;
   DWORD dwErrorCode = 0;

     // Open the access token associated with the calling process.
     if (OpenProcessToken(
                          GetCurrentProcess(),
                          TOKEN_QUERY,
                          &hToken
                          ) == FALSE)
     {
       dwErrorCode = GetLastError();
       wprintf(L"OpenProcessToken failed. GetLastError returned: %d\n", dwErrorCode);
       return HRESULT_FROM_WIN32(dwErrorCode);
     }



// Verify the parameter passed in is not NULL.
    if (NULL == ppsid)
        goto Cleanup;

// Get required buffer size and allocate the TOKEN_GROUPS buffer.

   if (!GetTokenInformation(
         hToken,         // handle to the access token
         TokenGroups,    // get information about the token's groups
         (LPVOID) ptg,   // pointer to TOKEN_GROUPS buffer
         0,              // size of buffer
         &dwLength       // receives required buffer size
      ))
   {
      if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
         goto Cleanup;

      ptg = (PTOKEN_GROUPS)HeapAlloc(GetProcessHeap(),
         HEAP_ZERO_MEMORY, dwLength);

      if (ptg == NULL)
         goto Cleanup;
   }

// Get the token group information from the access token.

   if (!GetTokenInformation(
         hToken,         // handle to the access token
         TokenGroups,    // get information about the token's groups
         (LPVOID) ptg,   // pointer to TOKEN_GROUPS buffer
         dwLength,       // size of buffer
         &dwLength       // receives required buffer size
         ))
   {
      goto Cleanup;
   }

// Loop through the groups to find the logon SID.

   for (dwIndex = 0; dwIndex < ptg->GroupCount; dwIndex++)
      if ((ptg->Groups[dwIndex].Attributes & SE_GROUP_LOGON_ID)
             ==  SE_GROUP_LOGON_ID)
      {
      // Found the logon SID; make a copy of it.

         dwLength = GetLengthSid(ptg->Groups[dwIndex].Sid);
         *ppsid = (PSID) HeapAlloc(GetProcessHeap(),
                     HEAP_ZERO_MEMORY, dwLength);
         if (*ppsid == NULL)
             goto Cleanup;
         if (!CopySid(dwLength, *ppsid, ptg->Groups[dwIndex].Sid))
         {
             HeapFree(GetProcessHeap(), 0, (LPVOID)*ppsid);
             goto Cleanup;
         }
         break;
      }

   bSuccess = TRUE;

Cleanup:

// Free the buffer for the token groups.

   if (ptg != NULL)
      HeapFree(GetProcessHeap(), 0, (LPVOID)ptg);

   return bSuccess;
}

bool MegaCmdShellCommunicationsNamedPipes::isFileOwnerCurrentUser(HANDLE hFile)
{
    DWORD dwRtnCode = 0;
    PSID pSidOwner = NULL;
    BOOL bRtnBool = TRUE;
    LPWSTR AcctName = NULL;
    LPTSTR DomainName = NULL;
    DWORD dwAcctName = 0, dwDomainName = 0;
    SID_NAME_USE eUse = SidTypeUnknown;
    PSECURITY_DESCRIPTOR pSD = NULL;

    // Get the owner SID of the file.
    dwRtnCode = GetSecurityInfo(
                hFile,
                SE_FILE_OBJECT,
                OWNER_SECURITY_INFORMATION,
                &pSidOwner,
                NULL,
                NULL,
                NULL,
                &pSD);

    // Check GetLastError for GetSecurityInfo error condition.
    if (dwRtnCode != ERROR_SUCCESS)
    {
        cerr << "GetSecurityInfo error = " << ERRNO << endl;
        return false;
    }

    // First call to LookupAccountSid to get the buffer sizes.
    bRtnBool = LookupAccountSidW(
                NULL,           // local computer
                pSidOwner,
                AcctName,
                (LPDWORD)&dwAcctName,
                DomainName,
                (LPDWORD)&dwDomainName,
                &eUse);

    // Reallocate memory for the buffers.
    AcctName = (LPWSTR)GlobalAlloc(GMEM_FIXED, dwAcctName * sizeof(wchar_t));

    if (AcctName == NULL)
    {
        cerr << "GlobalAlloc error = " << ERRNO << endl;
        return false;
    }

    DomainName = (LPTSTR)GlobalAlloc(GMEM_FIXED,dwDomainName * sizeof(wchar_t));

    if (DomainName == NULL)
    {
        cerr << "GlobalAlloc error = " << ERRNO << endl;
        return false;
    }

    // Second call to LookupAccountSid to get the account name.
    bRtnBool = LookupAccountSidW(
                NULL,                   // name of local or remote computer
                pSidOwner,              // security identifier
                AcctName,               // account name buffer
                (LPDWORD)&dwAcctName,   // size of account name buffer
                DomainName,             // domain name
                (LPDWORD)&dwDomainName, // size of domain name buffer
                &eUse);                 // SID type

    if (bRtnBool == FALSE)
    {
        if (ERRNO == ERROR_NONE_MAPPED)
        {
            cerr << "Account owner not found for specified SID." << endl;
        }
        else
        {
            cerr << "Error in LookupAccountSid: " << ERRNO << endl;
        }
        return false;
    }

    wchar_t username[UNLEN+1];
    DWORD username_len = UNLEN+1;

    GetUserNameW(username, &username_len);


    LPWSTR stringSIDOwner;
    if (ConvertSidToStringSidW(pSidOwner, &stringSIDOwner))
    {
        if (!wcscmp(username, AcctName) || ( 
#ifndef __MINGW32__
            IsUserAnAdmin() && 
#endif
            !wcscmp(stringSIDOwner, L"S-1-5-32-544"))) // owner == user  or   owner == administrators and current process running as admin
        {
            LocalFree(stringSIDOwner);
            return true;
        }
        else
        {
            std::wcerr << L"Unmatched owner - current user -> " << AcctName << L" - " << username;
#ifndef __MINGW32__
            std::wcerr << L" IsUserAdmin=" << IsUserAnAdmin();
#endif
            std::wcerr << L" SIDOwner=" << stringSIDOwner << endl;
            LocalFree(stringSIDOwner);
            return false;
        }
    }
    return false;


}

HANDLE MegaCmdShellCommunicationsNamedPipes::doOpenPipe(wstring nameOfPipe)
{
    wchar_t username[UNLEN+1];
    DWORD username_len = UNLEN+1;
    GetUserNameW(username, &username_len);
    wstring serverPipeName(L"\\\\.\\pipe\\megacmdpipe");
    serverPipeName += L"_";
    serverPipeName += username;

    if (nameOfPipe != serverPipeName)
    {
        if (!WaitNamedPipeW(nameOfPipe.c_str(),16000)) //TODO: use a real time and report timeout error.
        {
            OUTSTREAM << "ERROR WaitNamedPipe: " << nameOfPipe << " . errno: " << ERRNO << endl;
        }
    }

    HANDLE theNamedPipe = CreateFileW(
                nameOfPipe.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,/*FILE_FLAG_WRITE_THROUGH,*/
                NULL
                );

    if (namedPipeValid(theNamedPipe))
    {
        if (!isFileOwnerCurrentUser(theNamedPipe))
        {
            OUTSTREAM << "ERROR: Pipe owner does not match current user!" << endl;
            return INVALID_HANDLE_VALUE;
        }
    }

    return theNamedPipe;
}

HANDLE MegaCmdShellCommunicationsNamedPipes::createNamedPipe(int number, bool initializeserver)
{
    wstring nameOfPipe;

    wchar_t username[UNLEN+1];
    DWORD username_len = UNLEN+1;
    GetUserNameW(username, &username_len);

    nameOfPipe += L"\\\\.\\pipe\\megacmdpipe";
    nameOfPipe += L"_";
    nameOfPipe += username;

    if (number)
    {
#ifdef __MINGW32__
        wostringstream wos;
        wos << number;
        nameOfPipe += wos.str();
#else
        nameOfPipe += std::to_wstring(number);
#endif
    }

    // Open the named pipe
    HANDLE theNamedPipe = doOpenPipe(nameOfPipe);

    if (!namedPipeValid(theNamedPipe))
    {
        if (!number && initializeserver)
        {
            if (ERRNO == ERROR_PIPE_BUSY)
            {
                int attempts = 10;
                while (--attempts && !namedPipeValid(theNamedPipe))
                {
                    theNamedPipe = doOpenPipe(nameOfPipe);
                    Sleep(200*(10-attempts));
                }
                if (!namedPipeValid(theNamedPipe))
                {
                    if (ERRNO == ERROR_PIPE_BUSY)
                    {
                        OUTSTREAM << "Failed to access server: "  << ERRNO << endl;
                    }
                    else
                    {
                        OUTSTREAM << "Failed to access server. Server busy." << endl;
                    }
                }
            }
            else if (ERRNO != ERROR_FILE_NOT_FOUND)
            {
                OUTSTREAM << "Unexpected failure to access server: "  << ERRNO << endl;
            }
            else
            {
                //launch server
                cerr << "MEGAcmd Server not running. Initiating in the background..." << endl;
                STARTUPINFO si;
                PROCESS_INFORMATION pi;
                ZeroMemory( &si, sizeof(si) );
                ZeroMemory( &pi, sizeof(pi) );

#ifndef NDEBUG
                LPCWSTR t = TEXT("..\\MEGAcmdServer\\debug\\MEGAcmdServer.exe");
                if (true)
                {
#else

                wchar_t foldercontainingexec[MAX_PATH+1];
                bool okgetcontaningfolder = false;
                if (S_OK != SHGetFolderPathW(NULL,CSIDL_LOCAL_APPDATA,NULL,0,(LPWSTR)foldercontainingexec))
                {
                    if(S_OK != SHGetFolderPathW(NULL,CSIDL_COMMON_APPDATA,NULL,0,(LPWSTR)foldercontainingexec))
                    {
                        cerr << " Could not get LOCAL nor COMMON App Folder : " << ERRNO << endl;
                    }
                    else
                    {
                        okgetcontaningfolder = true;
                    }
                }
                else
                {
                    okgetcontaningfolder = true;
                }

                if (okgetcontaningfolder)
                {
                    wstring fullpathtoexec(foldercontainingexec);
                    fullpathtoexec+=L"\\MEGAcmd\\MEGAcmdServer.exe";

                    LPCWSTR t = fullpathtoexec.c_str();
#endif
                    LPWSTR t2 = (LPWSTR) t;
                    si.cb = sizeof(si);
//                    si.wShowWindow = SW_SHOWNOACTIVATE | SW_SHOWMINIMIZED;
                    si.wShowWindow = SW_HIDE;
                    si.dwFlags = STARTF_USESHOWWINDOW;
                    if (!CreateProcess( t,t2,NULL,NULL,TRUE,
                                        CREATE_NEW_CONSOLE,
                                        NULL,NULL,
                                        &si,&pi) )
                    {
                        COUT << "Unable to execute: " << t << " errno = : " << ERRNO << endl;
                    }

//                    Sleep(2000); // Give it a initial while to start.
                }

                //try again:
                int attempts = 10;
                int waitimet = 1500;
                theNamedPipe = INVALID_HANDLE_VALUE;
                while ( attempts && !namedPipeValid(theNamedPipe))
                {
                    Sleep(waitimet/1000);
                    waitimet=waitimet*2;
                    attempts--;
                    theNamedPipe = doOpenPipe(nameOfPipe);
                }
                if (attempts < 0)
                {
                    cerr << "Unable to connect to " << (number?("response namedPipe N "+number):"MEGAcmd server") << ": error=" << ERRNO << endl;

                    cerr << "Please ensure MEGAcmdServer is running" << endl;
                    return INVALID_HANDLE_VALUE;
                }
                else
                {
                    serverinitiatedfromshell = true;
                    registerAgainRequired = true;
                }
            }
        }
        else if (initializeserver) // initializeserver=false when closing server. we dont need to state the error
        {
            OUTSTREAM << "ERROR opening namedPipe: " << nameOfPipe << ": " << ERRNO << endl;
        }
        return theNamedPipe;
    }
    return theNamedPipe;
}

MegaCmdShellCommunicationsNamedPipes::MegaCmdShellCommunicationsNamedPipes()
{
#ifdef _WIN32
    setlocale(LC_ALL, "en-US");
#endif

    serverinitiatedfromshell = false;
    registerAgainRequired = false;

    stopListener = false;
    listenerThread = NULL;
    redirectedstdout = false;
}

int MegaCmdShellCommunicationsNamedPipes::executeCommandW(wstring wcommand, string (*readresponse)(const char *), OUTSTREAMTYPE &output, bool interactiveshell)
{
    return executeCommand("", readresponse, output, interactiveshell, wcommand);
}

/**
 * @brief Determines it outputing to console (true) or pipe/file (false)
 * @return
 */
bool outputtobinaryorconsole(void)
{
    HANDLE stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (stdoutHandle == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD fileType = GetFileType(stdoutHandle);

    if ((fileType == FILE_TYPE_UNKNOWN) && (GetLastError() != ERROR_SUCCESS)) {
        return false;
    }

    fileType &= ~(FILE_TYPE_REMOTE);

    if (fileType == FILE_TYPE_CHAR) {
        DWORD consoleMode;
        BOOL result = GetConsoleMode(stdoutHandle, &consoleMode);

        if ((result == FALSE) && (GetLastError() == ERROR_INVALID_HANDLE)) {
            return false;
        } else {
            return true;
        }
    } else {
        return true;
    }
}

int MegaCmdShellCommunicationsNamedPipes::executeCommand(string command, std::string (*readresponse)(const char *), OUTSTREAMTYPE &output, bool interactiveshell, wstring wcommand)
{
    HANDLE theNamedPipe = createNamedPipe(0,command.compare(0,4,"exit")
                                          && command.compare(0,4,"quit")
                                          && command.compare(0,7,"sendack"));
    if (!namedPipeValid(theNamedPipe))
    {
        return -1;
    }

    if (interactiveshell)
    {
        command="X"+command;
    }

//    //unescape \uXXXX sequences
//    command=unescapeutf16escapedseqs(command.c_str());

    //get local wide chars string (utf8 -> utf16)
    if (!wcommand.size())
    {
        stringtolocalw(command.c_str(),&wcommand);
    }
    else if (interactiveshell)
    {
        wcommand=L"X"+wcommand;
    }

    DWORD n;
    if (!WriteFile(theNamedPipe,(char *)wcommand.data(),DWORD(wcslen(wcommand.c_str())*sizeof(wchar_t)), &n, NULL))
    {
        cerr << "ERROR writing command to namedPipe: " << ERRNO << endl;
        return -1;
    }

    int receiveNamedPipeNum = -1 ;
    if (!ReadFile(theNamedPipe, (char *)&receiveNamedPipeNum, sizeof(receiveNamedPipeNum), &n, NULL) )
    {
        cerr << "ERROR reading output namedPipe" << endl;
        return -1;
    }

    HANDLE newNamedPipe = createNamedPipe(receiveNamedPipeNum);
    if (!namedPipeValid(newNamedPipe))
    {
        return -1;
    }

    int outcode = -1;

    if (!ReadFile(newNamedPipe, (char *)&outcode, sizeof(outcode),&n, NULL))
    {
        cerr << "ERROR reading output code: " << ERRNO << endl;
        return -1;
    }

    bool binaryoutput = !wcommand.compare(0, 3, L"cat") && redirectedstdout;

    while (outcode == MCMD_REQCONFIRM || outcode == MCMD_REQSTRING || outcode == MCMD_PARTIALOUT)
    {
        if (outcode == MCMD_PARTIALOUT)
        {
            size_t partialoutsize;
            if (!ReadFile(newNamedPipe, (char *)&partialoutsize, sizeof(partialoutsize),&n, NULL))
            {
                std::cerr << "Error reading size of partial output: " << ERRNO << std::endl;
                return -1;
            }


            if (partialoutsize > 0)
            {
                megaCmdStdoutputing.lock();
                int oldmode = 0;

                if (binaryoutput)
                {
                    oldmode = _setmode(_fileno(stdout), O_BINARY);
                }

                size_t BUFFERSIZE = 10024;
                char buffer[10025];
                do{
                    BOOL readok;
                    readok = ReadFile(newNamedPipe, buffer, DWORD(std::min(BUFFERSIZE,partialoutsize)),&n,NULL);
                    if (readok)
                    {

                        if (binaryoutput)
                        {
                            std::cout << string(buffer,n) << flush;
                        }
                        else
                        {
                            buffer[n]='\0';

                            wstring wbuffer;
                            stringtolocalw((const char*)&buffer,&wbuffer);
                            int oldmode;
                            oldmode = _setmode(_fileno(stdout), _O_U8TEXT);
                            output << wbuffer << flush;
                            _setmode(_fileno(stdout), oldmode);
                        }
                        partialoutsize-=n;
                    }
                } while(n != 0 && partialoutsize && n !=SOCKET_ERROR);

                if (binaryoutput)
                {
                    _setmode(_fileno(stdout), oldmode);
                }
                megaCmdStdoutputing.unlock();
            }
            else
            {
                cerr << "Invalid size of partial output: " << partialoutsize << endl;
                return -1;
            }

        }
        else
        {

            DWORD BUFFERSIZE = 1024;
            string confirmQuestion;
            char bufferQuestion[1025];
            memset(bufferQuestion,'\0',1025);
            BOOL readok;
            do{
                readok = ReadFile(newNamedPipe, bufferQuestion, BUFFERSIZE, &n, NULL);
                confirmQuestion.append(bufferQuestion);
            } while(n == BUFFERSIZE && readok);

            if (!readok)
            {
                cerr << "ERROR reading confirm question: " << ERRNO << endl;
            }

            if (outcode == MCMD_REQCONFIRM)
            {
                int response = MCMDCONFIRM_NO;

                if (readresponse != NULL)
                {
                    response = readconfirmationloop(confirmQuestion.c_str(), readresponse);
                }

                if (!WriteFile(newNamedPipe, (const char *) &response, sizeof(response), &n, NULL))
                {
                    cerr << "ERROR writing confirm response to namedPipe: " << ERRNO << endl;
                    return -1;
                }
            }
            else // MCMD_REQSTRING
            {
                string response = "FAILED";

                if (readresponse != NULL)
                {
                    response = readresponse(confirmQuestion.c_str());
                }

                wstring wresponse;
                stringtolocalw(response.c_str(),&wresponse);
                if (!WriteFile(newNamedPipe, (char *) wresponse.data(), DWORD(wcslen(wresponse.c_str())*sizeof(wchar_t)), &n, NULL))
                {
                    cerr << "ERROR writing confirm response to namedPipe: " << ERRNO << endl;
                    return -1;
                }
            }
        }
        if (!ReadFile(newNamedPipe, (char *)&outcode, sizeof(outcode),&n, NULL))
        {
            cerr << "ERROR reading output code: " << ERRNO << endl;
            return -1;
        }
    }

    DWORD BUFFERSIZE = 1024;
    char buffer[1025];
    BOOL readok;
    do{
        readok = ReadFile(newNamedPipe, buffer, BUFFERSIZE,&n,NULL);
        if (readok)
        {
            buffer[n]='\0';

            wstring wbuffer;
            stringtolocalw((const char*)&buffer,&wbuffer);
            int oldmode;
//            if (interactiveshell || outputtobinaryorconsole())
//            {
                // In non-interactive mode, at least in powershell, when outputting to a file/pipe, things get rough
                // Powershell tries to interpret the output as a string and would meddle with the UTF16 encoding, resulting
                // in unusable output, So we disable the UTF-16 in such cases (this might cause that the output could be truncated!).
//                oldmode = _setmode(_fileno(stdout), _O_U16TEXT);
//            }
            megaCmdStdoutputing.lock();
            oldmode = _setmode(_fileno(stdout), _O_U8TEXT);
            output << wbuffer << flush;
            _setmode(_fileno(stdout), oldmode);
            megaCmdStdoutputing.unlock();

//            if (interactiveshell || outputtobinaryorconsole() || true)
//            {
//                _setmode(_fileno(stdout), oldmode);
//            }
        }
        if (readok && n == BUFFERSIZE)
        {
            DWORD total_available_bytes;
            if (FALSE == PeekNamedPipe(newNamedPipe,0,0,0,&total_available_bytes,0))
            {
                break;
            }
            if (total_available_bytes == 0)
            {
                break;
            }
        }
    } while(n == BUFFERSIZE && readok);

    if (!readok)
    {
        cerr << "ERROR reading output: " << ERRNO << endl;
        return -1;
    }

    closeNamedPipe(newNamedPipe);
    closeNamedPipe(theNamedPipe);

    return outcode;
}

void *MegaCmdShellCommunicationsNamedPipes::listenToStateChangesEntryNamedPipe(void *slsc)
{
    listenToStateChanges(((sListenStateChangesNamedPipe *)slsc)->receiveNamedPipeNum,((sListenStateChangesNamedPipe *)slsc)->statechangehandle);
    delete ((sListenStateChangesNamedPipe *)slsc);
    return NULL;
}


int MegaCmdShellCommunicationsNamedPipes::listenToStateChanges(int receiveNamedPipeNum, void (*statechangehandle)(string))
{
    newNamedPipe = createNamedPipe(receiveNamedPipeNum);
    if (!namedPipeValid(newNamedPipe))
    {
        return -1;
    }

    int timeout_notified_server_might_be_down = 0;
    while (!stopListener)
    {
        if (!namedPipeValid(newNamedPipe))
        {
            return -1;
        }

        string newstate;

        DWORD BUFFERSIZE = 1024;
        char buffer[1025];
        DWORD n;
        bool readok;
        do{
            readok = ReadFile(newNamedPipe, buffer, BUFFERSIZE, &n, NULL);
            if (readok)
            {
                buffer[n]='\0';
                newstate += buffer;
            }
        } while(n == BUFFERSIZE && readok);

        if (!readok)
        {
            if (ERRNO == ERROR_BROKEN_PIPE)
            {
                if (!stopListener && !updating)
                {
                    cerr << "ERROR reading output (state change): The server probably exited."<< endl;
                }
            }
            else
            {
                cerr << "ERROR reading output (state change): " << ERRNO << endl;
            }
            closeNamedPipe(newNamedPipe);
            return -1;
        }

        if (!n)
        {
            if (!timeout_notified_server_might_be_down)
            {
                timeout_notified_server_might_be_down = 30;
                cerr << endl << "MEGAcmdServer.exe is probably down. Executing anything will try to respawn or reconnect to it";
            }
            timeout_notified_server_might_be_down--;
            if (!timeout_notified_server_might_be_down)
            {
                registerAgainRequired = true;
                closeNamedPipe(newNamedPipe);
                return -1;
            }
            Sleep(1000);
            continue;
        }

        if (statechangehandle != NULL)
        {
            statechangehandle(newstate);
        }
    }

    closeNamedPipe(newNamedPipe);
    return 0;
}

int MegaCmdShellCommunicationsNamedPipes::registerForStateChanges(bool interactive, void (*statechangehandle)(string), bool initiateServer)
{
    if (statechangehandle == NULL)
    {
        registerAgainRequired = false;
        return 0; //Do nth
    }
    HANDLE theNamedPipe = createNamedPipe(0, initiateServer);
    if (!namedPipeValid(theNamedPipe))
    {
        return -1;
    }

    wstring wcommand=interactive?L"Xregisterstatelistener":L"registerstatelistener";

    DWORD n;
    if (!WriteFile(theNamedPipe,(char *)wcommand.data(),DWORD(wcslen(wcommand.c_str())*sizeof(wchar_t)), &n, NULL))
    {
        cerr << "ERROR writing command to namedPipe: " << ERRNO << endl;
        return -1;
    }

    int receiveNamedPipeNum = -1;

    if (!ReadFile(theNamedPipe, (char *)&receiveNamedPipeNum, sizeof(receiveNamedPipeNum), &n, NULL) )
    {
        cerr << "ERROR reading output namedPipe" << endl;
        return -1;
    }

    if (listenerThread != NULL)
    {
        stopListener = true;
        listenerThread->join();
    }

    stopListener = false;

    sListenStateChangesNamedPipe * slsc = new sListenStateChangesNamedPipe();
    slsc->receiveNamedPipeNum = receiveNamedPipeNum;
    slsc->statechangehandle = statechangehandle;
    listenerThread = new MegaThread();
    listenerThread->start(listenToStateChangesEntryNamedPipe,slsc);

    registerAgainRequired = false;

    closeNamedPipe(theNamedPipe);
    return 0;
}

void MegaCmdShellCommunicationsNamedPipes::setResponseConfirmation(bool confirmation)
{
    confirmResponse = confirmation;
}

MegaCmdShellCommunicationsNamedPipes::~MegaCmdShellCommunicationsNamedPipes()
{
    if (listenerThread != NULL) //TODO: use heritage for whatever we can
    {
        stopListener = true;

        executeCommand("sendack");

        listenerThread->join();
    }
    delete (MegaThread *)listenerThread;
}

} //end namespace
#endif
