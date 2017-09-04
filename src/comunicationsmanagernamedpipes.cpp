/**
 * @file examples/megacmd/comunicationsmanagernamedPipes.cpp
 * @brief MegaCMD: Communications manager using Network NamedPipes
 *
 * (c) 2013-2016 by Mega Limited, Auckland, New Zealand
 *
 * This file is part of the MEGA SDK - Client Access Engine.
 *
 * Applications using the MEGA API must present a valid application key
 * and comply with the the rules set forth in the Terms of Service.
 *
 * The MEGA SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * @copyright Simplified (2-clause) BSD License.
 *
 * You should have received a copy of the license along with this
 * program.
 */
#ifdef _WIN32

#include "comunicationsmanagernamedpipes.h"
#include "megacmdutils.h"


#include <windows.h>
#include <Lmcons.h> //getusername

#define ERRNO WSAGetLastError()


using namespace mega;


bool namedPipeValid(HANDLE namedPipe)
{
    return namedPipe != INVALID_HANDLE_VALUE;
}

bool ComunicationsManagerNamedPipes::ended;

int ComunicationsManagerNamedPipes::get_next_comm_id()
{
    mtx->lock();
    ++count;
    mtx->unlock();
    return count;
}

HANDLE ComunicationsManagerNamedPipes::doCreatePipe(wstring nameOfPipe)
{
    return CreateNamedPipeW(
           nameOfPipe.c_str(), // name of the pipe
           /*PIPE_ACCESS_DUPLEX | FILE_FLAG_WRITE_THROUGH, // 2-way pipe, synchronous
           PIPE_TYPE_BYTE, // send data as a byte stream
            */
                PIPE_ACCESS_DUPLEX,
                PIPE_TYPE_MESSAGE, // this treats the pipe as if FILE_FLAG_WRITE_THROUGH anyhow
           1, // only allow 1 instance of this pipe
           0, // no outbound buffer
           0, // no inbound buffer
           0, // use default wait time
           NULL // use default security attributes
       );
}

HANDLE ComunicationsManagerNamedPipes::create_new_namedPipe(int *pipeId)
{

    *pipeId = get_next_comm_id();

    HANDLE thepipe;
    wstring nameOfPipe;
    int attempts = 10;
    bool namedPipesucceded = false;
    while (--attempts && !namedPipesucceded)
    {
        wchar_t username[UNLEN+1];
        DWORD username_len = UNLEN+1;
        GetUserNameW(username, &username_len);

        nameOfPipe += L"\\\\.\\pipe\\megacmdpipe";
        nameOfPipe += L"_";
        nameOfPipe += username;

        if (*pipeId)
        {
    #ifdef __MINGW32__
            wostringstream wos;
            wos << *pipeId;
            nameOfPipe += wos.str();
    #else
            nameOfPipe += std::to_wstring(*pipeId);
    #endif
        }

        // Create a pipe to send data
        thepipe = doCreatePipe(nameOfPipe);

        if (!namedPipeValid(thepipe))
        {
            if (errno == EMFILE) //TODO: review possible out
            {
                LOG_verbose << " Trying to reduce number of used files by sending ACK to listeners to discard disconnected ones.";
                string sack="ack";
                informStateListeners(sack);
            }
            if (attempts !=10)
            {
                LOG_fatal << "ERROR opening namedPipe ID=" << pipeId << " errno: " << ERRNO << ". Attempts: " << attempts;
            }
            sleepMicroSeconds(500);
        }
        else
        {
            namedPipesucceded = true;
        }
    }
    if (!namedPipeValid(thepipe))
    {
        return INVALID_HANDLE_VALUE;
    }

    return thepipe;
}


ComunicationsManagerNamedPipes::ComunicationsManagerNamedPipes()
{
    count = 0;
    mtx = new MegaMutex();
    initialize();
}

int ComunicationsManagerNamedPipes::initialize()
{
    mtx->init(false);

    petitionready = false;

    wchar_t username[UNLEN+1];
    DWORD username_len = UNLEN+1;
    GetUserNameW(username, &username_len);
    wstring nameOfPipe (L"\\\\.\\pipe\\megacmdpipe");
    nameOfPipe += L"_";
    nameOfPipe += username;

    pipeGeneral = doCreatePipe(nameOfPipe);

    if (!namedPipeValid(pipeGeneral))
    {
        if (ERRNO == ERROR_ACCESS_DENIED)
        {
            LOG_fatal << "ERROR initiating communications. Couldn't create NamedPipe: Access denied. Ensure there is no other instance running.";
        }
        else
        {
            LOG_fatal << "ERROR initiating communications. Couldn't create NamedPipe: " << ERRNO;
        }
        Sleep(6000);
        exit(1);
        return -1;
    }

    return 0;
}

bool ComunicationsManagerNamedPipes::receivedPetition()
{
    return petitionready;
}

int ComunicationsManagerNamedPipes::waitForPetition()
{
    petitionready = false;
    if (!ConnectNamedPipe(pipeGeneral, NULL))
    {
        if (ERRNO == ERROR_PIPE_CONNECTED)
        {
            LOG_debug << "Client arrived first when connecting to pipeGeneral.";
        }
        else
        {
            LOG_fatal << "ERROR on connecting to namedPipe. errno: " << ERRNO;
            sleepMicroSeconds(1000);
            pipeGeneral = INVALID_HANDLE_VALUE;
            return false;
        }

    }
    petitionready = true;
    return true;
}

void ComunicationsManagerNamedPipes::stopWaiting()
{
    wstring nameOfPipe;
    wchar_t username[UNLEN+1];
    DWORD username_len = UNLEN+1;
    GetUserNameW(username, &username_len);

    nameOfPipe += L"\\\\.\\pipe\\megacmdpipe";
    nameOfPipe += L"_";
    nameOfPipe += username;
    DeleteFile(nameOfPipe.c_str()); // without this, CloseHandle will hang, and loop will be stuck in ConnectNamedPipe

    CloseHandle(pipeGeneral);
}

void ComunicationsManagerNamedPipes::registerStateListener(CmdPetition *inf)
{
    LOG_debug << "Registering state listener petition with namedPipe: " << ((CmdPetitionNamedPipes *) inf)->outNamedPipe;
    ComunicationsManager::registerStateListener(inf);
}

//TODO: implement unregisterStateListener, not 100% necesary, since when a state listener is not accessible it is unregistered (to deal with sudden deaths).
// also, unregistering might not be straight forward since we need to correlate the thread doing the unregistration with the one who registered.


/**
 * @brief returnAndClosePetition
 * I will clean struct and close the namedPipe within
 */
void ComunicationsManagerNamedPipes::returnAndClosePetition(CmdPetition *inf, OUTSTRINGSTREAM *s, int outCode)
{
    HANDLE outNamedPipe = ((CmdPetitionNamedPipes *)inf)->outNamedPipe;

    LOG_verbose << "Output to write in namedPipe " << outNamedPipe << ": <<" << s->str() << ">>";

    bool connectsucceeded = false;
    int attempts = 10;
    while (--attempts && !connectsucceeded)
    {
        if (!ConnectNamedPipe(outNamedPipe, NULL))
        {
            if (ERRNO == ERROR_PIPE_CONNECTED)
            {
                LOG_verbose << "Client arrived first when connecting to namedPipe " << outNamedPipe;
                connectsucceeded = true;
                break;
            }
            else
            {
                LOG_fatal << "ERROR on connecting to namedPipe " << outNamedPipe << ". errno: " << ERRNO << ". Attempts: " << attempts;
            }
            sleepMicroSeconds(500);
        }
        else
        {
            connectsucceeded = true;
        }
    }

    if (!connectsucceeded)
    {
        LOG_fatal << "Return and close: Unable to connect on outnamedPipe " << outNamedPipe << " error: " << ERRNO;
        delete inf;
        return;
    }

    OUTSTRING sout = s->str();

    DWORD n;
    if (!WriteFile(outNamedPipe,(const char*)&outCode, sizeof(outCode), &n, NULL))
    {
        LOG_err << "ERROR writing output Code to namedPipe: " << ERRNO;
    }

    string sutf8;
    localwtostring(&sout,&sutf8);
    if (!WriteFile(outNamedPipe,sutf8.data(), max(1,(int)sutf8.size()), &n, NULL)) // client does not like empty responses
    {
        LOG_err << "ERROR writing to namedPipe: " << ERRNO;
    }
    DisconnectNamedPipe(outNamedPipe);
    delete inf;
}

int ComunicationsManagerNamedPipes::informStateListener(CmdPetition *inf, string &s)
{
    LOG_verbose << "Inform State Listener: Output to write in namedPipe " << ((CmdPetitionNamedPipes *)inf)->outNamedPipe << ": <<" << s << ">>";
    HANDLE outNamedPipe = ((CmdPetitionNamedPipes *)inf)->outNamedPipe;

    if (!ConnectNamedPipe(outNamedPipe, NULL))
    {
        if (ERRNO == ERROR_PIPE_CONNECTED)
        {
            LOG_debug << "Client arrived first when connecting to namedPipe to inform state. " << outNamedPipe;
        }
        else if(ERRNO == ERROR_NO_DATA)
        {
            LOG_debug << "Client probably disconnected: " << outNamedPipe;
            return -1;
        }
        else
        {
            LOG_fatal << "ERROR on connecting to namedPipe " << outNamedPipe << ". errno: " << ERRNO;
            return -1;
        }
    }

    DWORD n;
    if (!WriteFile(outNamedPipe, s.data(), s.size(), &n, NULL))
    {
        if (ERRNO == 32) //namedPipe closed
        {
            LOG_debug << "namedPipe closed. Client probably disconnected. Original petition: " << *inf;
            return -1;
        }
        else
        {
            LOG_err << "ERROR writing to namedPipe to inform state: " << ERRNO;
        }
    }

    return 0;
}

/**
 * @brief getPetition
 * @return pointer to new CmdPetitionPosix. Petition returned must be properly deleted (this can be calling returnAndClosePetition)
 */
CmdPetition * ComunicationsManagerNamedPipes::getPetition()
{
    CmdPetitionNamedPipes *inf = new CmdPetitionNamedPipes();

    wstring wread;
    wchar_t wbuffer[1024]= {};

    DWORD n;
    //ZeroMemory( wbuffer, sizeof(wbuffer));
    bool readok = ReadFile(pipeGeneral, wbuffer, 1023*sizeof(wchar_t), &n, NULL );
    while(readok && n == 1023*sizeof(wchar_t))
    {
        DWORD total_available_bytes;
        if (FALSE == PeekNamedPipe(pipeGeneral,0,0,0,&total_available_bytes,0))
        {
            LOG_err << "Failed to PeekNamedPipe. errno: L" << ERRNO;
            break;
        }
        if (total_available_bytes == 0)
        {
            break;
        }
        wbuffer[n/sizeof(wchar_t)]=0;
        wread.append(wbuffer);

        readok = ReadFile(pipeGeneral, wbuffer, 1023*sizeof(wchar_t), &n, NULL );
    }
    if (readok)
    {
        wbuffer[n/sizeof(wchar_t)]=0;
        wread.append(wbuffer);
    }

    if (!readok)
    {
        LOG_err << "Failed to read petition from named pipe. errno: L" << ERRNO;
        inf->line = strdup("ERROR");
        return inf;
    }

    string receivedutf8;

    localwtostring(&wread,&receivedutf8);

    int namedPipe_id = 0; // this value shouldn't matter
    inf->outNamedPipe = create_new_namedPipe(&namedPipe_id);
    if (!namedPipeValid(inf->outNamedPipe) || !namedPipe_id)
    {
        LOG_fatal << "ERROR creating output namedPipe at getPetition";
        inf->line = strdup("ERROR");
        return inf;
    }

    if(!WriteFile(pipeGeneral,(const char*)&namedPipe_id, sizeof( namedPipe_id ), &n, NULL))
    {
        LOG_fatal << "ERROR writing to namedPipe at getPetition: ERRNO = " << ERRNO;
        inf->line = strdup("ERROR");
        return inf;
    }

    if (!DisconnectNamedPipe(pipeGeneral) )
    {
        LOG_fatal << " Error disconnecting from general pip. errno: " << ERRNO;
    }

    inf->line = strdup(receivedutf8.c_str());

    return inf;
}

bool ComunicationsManagerNamedPipes::getConfirmation(CmdPetition *inf, string message)
{
    HANDLE outNamedPipe = ((CmdPetitionNamedPipes *)inf)->outNamedPipe;

    int outCode = MCMD_REQCONFIRM;
    DWORD n;
    if (!WriteFile(outNamedPipe, (const char *)&outCode, sizeof( outCode ), &n, NULL))
    {
        LOG_err << "ERROR writing output Code to namedPipe: " << ERRNO;
    }

    if (!WriteFile(outNamedPipe, message.data(), max(1,(int)message.size()), &n, NULL) )
    {
        LOG_err << "ERROR writing to namedPipe: " << ERRNO;
    }

    bool response = false;
    if (!ReadFile(outNamedPipe,(char *)&response, sizeof(response), &n, NULL))
    {
        LOG_err << "ERROR receiving confirmation response: " << ERRNO;
    }
    return response;
}

string ComunicationsManagerNamedPipes::get_petition_details(CmdPetition *inf)
{
    ostringstream os;
    os << "namedPipe output: " << ((CmdPetitionNamedPipes *)inf)->outNamedPipe;
    return os.str();
}

ComunicationsManagerNamedPipes::~ComunicationsManagerNamedPipes()
{
    delete mtx;
}
#endif
