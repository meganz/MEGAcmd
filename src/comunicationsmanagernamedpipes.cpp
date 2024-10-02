/**
 * @file src/comunicationsmanagernamedPipes.cpp
 * @brief MegaCMD: Communications manager using Network NamedPipes
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
 */
#ifdef _WIN32

#include "comunicationsmanagernamedpipes.h"
#include "megacmdutils.h"
#include "megacmdcommonutils.h"


#include <winsock2.h>
#include <windows.h>
#include <Lmcons.h> //getusername

using std::wstring;

#define ERRNO WSAGetLastError()

using namespace mega;


namespace megacmd {
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

    HANDLE thepipe = INVALID_HANDLE_VALUE;
    wstring nameOfPipe;
    int attempts = 10;
    bool namedPipesucceded = false;
    while (--attempts && !namedPipesucceded)
    {
        nameOfPipe += getNamedPipeName();

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
                ackStateListenersAndRemoveClosed();
            }
            if (attempts !=10)
            {
                LOG_fatal << "ERROR opening namedPipe ID=" << pipeId << " errno: " << ERRNO << ". Attempts: " << attempts;
            }
            sleepMilliSeconds(500);
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
    mtx = new std::mutex();
    informerMutex = new std::mutex();
    initialize();
}

int ComunicationsManagerNamedPipes::initialize()
{
    petitionready = false;

    wstring nameOfPipe = getNamedPipeName();
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
            sleepMilliSeconds(1000);
            pipeGeneral = INVALID_HANDLE_VALUE;
            return ERRNO;
        }

    }
    petitionready = true;
    return 0;
}

void ComunicationsManagerNamedPipes::stopWaiting()
{
    wstring nameOfPipe = getNamedPipeName();

    DeleteFile(nameOfPipe.c_str()); // without this, CloseHandle will hang, and loop will be stuck in ConnectNamedPipe

    if (pipeGeneral != INVALID_HANDLE_VALUE)
    {
        CloseHandle(pipeGeneral);
    }
}

CmdPetition* ComunicationsManagerNamedPipes::registerStateListener(std::unique_ptr<CmdPetition> inf)
{
    LOG_debug << "Registering state listener petition with namedPipe: " << ((CmdPetitionNamedPipes*) inf.get())->outNamedPipe;
    return ComunicationsManager::registerStateListener(std::move(inf));
}

//TODO: implement unregisterStateListener, not 100% necesary, since when a state listener is not accessible it is unregistered (to deal with sudden deaths).
// also, unregistering might not be straight forward since we need to correlate the thread doing the unregistration with the one who registered.


/**
 * @brief returnAndClosePetition
 * I will clean struct and close the namedPipe within
 */
void ComunicationsManagerNamedPipes::returnAndClosePetition(std::unique_ptr<CmdPetition> inf, OUTSTRINGSTREAM *s, int outCode)
{
    HANDLE outNamedPipe = ((CmdPetitionNamedPipes*) inf.get())->outNamedPipe;

    LOG_verbose << "Output to write in namedPipe " << *(long*)(&outNamedPipe);

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
            sleepMilliSeconds(500);
        }
        else
        {
            connectsucceeded = true;
        }
    }

    if (!connectsucceeded)
    {
        LOG_fatal << "Return and close: Unable to connect on outnamedPipe " << outNamedPipe << " error: " << ERRNO;
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
}


void ComunicationsManagerNamedPipes::sendPartialOutput(CmdPetition *inf, OUTSTRING *s)
{
    if (inf->clientDisconnected)
    {
        return;
    }

    HANDLE outNamedPipe = ((CmdPetitionNamedPipes *)inf)->outNamedPipe;

    bool connectsucceeded = false;
    int attempts = 10;
    while (--attempts && !connectsucceeded)
    {
        if (!ConnectNamedPipe(outNamedPipe, NULL))
        {
            if (ERRNO == ERROR_PIPE_CONNECTED)
            {
                //cerr << "Client arrived first when connecting to namedPipe " << outNamedPipe << endl;
                connectsucceeded = true;
                break;
            }
            else
            {
                cerr << "ERROR on connecting to namedPipe " << outNamedPipe << ". errno: " << ERRNO << ". Attempts: " << attempts << endl;
            }
            sleepMilliSeconds(500);
        }
        else
        {
            connectsucceeded = true;
        }
    }

    if (!connectsucceeded)
    {
        cerr << "sendPartialOutput: Unable to connect on outnamedPipe " << outNamedPipe << " error: " << ERRNO << endl;
        if (errno == ERROR_NO_DATA) //TODO: pipe disconnected error?
        {
            std::cerr << "WARNING: Client disconnected, the rest of the output will be discarded" << endl;
            inf->clientDisconnected = true;
        }
        return;
    }

    int outCode = MCMD_PARTIALOUT;
    DWORD n;
    if (!WriteFile(outNamedPipe,(const char*)&outCode, sizeof(outCode), &n, NULL))
    {
        LOG_err << "ERROR writing output Code to namedPipe: " << ERRNO;
        if (errno == ERROR_NO_DATA) //TODO: pipe disconnected error?
        {
            std::cerr << "WARNING: Client disconnected, the rest of the output will be discarded" << endl;
            inf->clientDisconnected = true;
        }
        return;
    }

    string sutf8;
    localwtostring(s,&sutf8);

    size_t size = sutf8.size() > 1 ? sutf8.size() : 1; // client does not like empty responses
    if (!WriteFile(outNamedPipe,(const char*)&size, sizeof(size), &n, NULL))
    {
        LOG_err << "ERROR writing output Code to namedPipe: " << ERRNO;
        return;
    }
    if (!WriteFile(outNamedPipe,sutf8.data(), DWORD(size), &n, NULL))
    {
        LOG_err << "ERROR writing to namedPipe: " << ERRNO;
    }
}


void ComunicationsManagerNamedPipes::sendPartialOutput(CmdPetition *inf, char *s, size_t size)
{
    HANDLE outNamedPipe = ((CmdPetitionNamedPipes *)inf)->outNamedPipe;

    bool connectsucceeded = false;
    int attempts = 10;
    while (--attempts && !connectsucceeded)
    {
        if (!ConnectNamedPipe(outNamedPipe, NULL))
        {
            if (ERRNO == ERROR_PIPE_CONNECTED)
            {
                //cerr << "Client arrived first when connecting to namedPipe " << outNamedPipe << endl;
                connectsucceeded = true;
                break;
            }
            else
            {
                cerr << "ERROR on connecting to namedPipe " << outNamedPipe << ". errno: " << ERRNO << ". Attempts: " << attempts << endl;
            }
            sleepMilliSeconds(500);
        }
        else
        {
            connectsucceeded = true;
        }
    }

    if (!connectsucceeded)
    {
        cerr << "sendPartialOutput: Unable to connect on outnamedPipe " << outNamedPipe << " error: " << ERRNO << endl;
        if (errno == ERROR_NO_DATA)
        {
            std::cerr << "WARNING: Client disconnected, the rest of the output will be discarded" << endl;
            inf->clientDisconnected = true;
        }
        return;
    }

    int outCode = MCMD_PARTIALOUT;
    DWORD n;
    if (!WriteFile(outNamedPipe,(const char*)&outCode, sizeof(outCode), &n, NULL))
    {
        LOG_err << "ERROR writing output Code to namedPipe: " << ERRNO;
        if (errno == ERROR_NO_DATA)
        {
            std::cerr << "WARNING: Client disconnected, the rest of the output will be discarded" << endl;
            inf->clientDisconnected = true;
        }
        return;
    }

    size_t thesize = size > 1 ? size : 1; // client does not like empty responses
    if (!WriteFile(outNamedPipe,(const char*)&thesize, sizeof(thesize), &n, NULL))
    {
        LOG_err << "ERROR writing output Code to namedPipe: " << ERRNO;
        return;
    }
    if (!WriteFile(outNamedPipe,s, DWORD(thesize), &n, NULL))
    {
        LOG_err << "ERROR writing to namedPipe: " << ERRNO;
    }
}

int ComunicationsManagerNamedPipes::informStateListener(CmdPetition *inf, const string &s)
{
    std::lock_guard<std::mutex> g(*informerMutex);

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
    if (!WriteFile(outNamedPipe, s.data(), DWORD(s.size()), &n, NULL))
    {
        if (ERRNO == 32 || ERRNO == 109 || (ERRNO == 232 && s == "ack")) //namedPipe closed | pipe has been ended
        {
            LOG_debug << "namedPipe closed. Client probably disconnected. Original petition: " << inf->line;
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
std::unique_ptr<CmdPetition> ComunicationsManagerNamedPipes::getPetition()
{
    auto inf = std::make_unique<CmdPetitionNamedPipes>();

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
        inf->line = "ERROR";
        return inf;
    }

    string receivedutf8;

    localwtostring(&wread,&receivedutf8);

    int namedPipe_id = 0; // this value shouldn't matter
    inf->outNamedPipe = create_new_namedPipe(&namedPipe_id);
    if (!namedPipeValid(inf->outNamedPipe) || !namedPipe_id)
    {
        LOG_fatal << "ERROR creating output namedPipe at getPetition";
        inf->line = "ERROR";
        return inf;
    }

    if(!WriteFile(pipeGeneral,(const char*)&namedPipe_id, sizeof( namedPipe_id ), &n, NULL))
    {
        LOG_fatal << "ERROR writing to namedPipe at getPetition: ERRNO = " << ERRNO;
        inf->line = "ERROR";
        return inf;
    }

    if (!DisconnectNamedPipe(pipeGeneral) )
    {
        LOG_fatal << " Error disconnecting from general pip. errno: " << ERRNO;
    }

    inf->line = receivedutf8;

    return inf;
}

int ComunicationsManagerNamedPipes::getConfirmation(CmdPetition *inf, string message)
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

    int response = MCMDCONFIRM_NO;
    if (!ReadFile(outNamedPipe,(char *)&response, sizeof(response), &n, NULL))
    {
        LOG_err << "ERROR receiving confirmation response: " << ERRNO;
    }
    return response;
}


string ComunicationsManagerNamedPipes::getUserResponse(CmdPetition *inf, string message)
{
    HANDLE outNamedPipe = ((CmdPetitionNamedPipes *)inf)->outNamedPipe;

    int outCode = MCMD_REQSTRING;
    DWORD n;
    if (!WriteFile(outNamedPipe, (const char *)&outCode, sizeof( outCode ), &n, NULL))
    {
        LOG_err << "ERROR writing output Code to namedPipe: " << ERRNO;
    }

    if (!WriteFile(outNamedPipe, message.data(), max(1,(int)message.size()), &n, NULL) )
    {
        LOG_err << "ERROR writing to namedPipe: " << ERRNO;
    }

    wstring wread;
    wchar_t wbuffer[1024]= {};

    //ZeroMemory( wbuffer, sizeof(wbuffer));
    bool readok = ReadFile(outNamedPipe, wbuffer, 1023*sizeof(wchar_t), &n, NULL );
    while(readok && n == 1023*sizeof(wchar_t))
    {
        DWORD total_available_bytes;
        if (FALSE == PeekNamedPipe(outNamedPipe,0,0,0,&total_available_bytes,0))
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

        readok = ReadFile(outNamedPipe, wbuffer, 1023*sizeof(wchar_t), &n, NULL );
    }
    if (readok)
    {
        wbuffer[n/sizeof(wchar_t)]=0;
        wread.append(wbuffer);
    }

    if (!readok)
    {
        LOG_err << "Failed to read user response from named pipe. errno: L" << ERRNO;
        return "FAILED";
    }

    string receivedutf8;
    localwtostring(&wread,&receivedutf8);

    return receivedutf8;
}

ComunicationsManagerNamedPipes::~ComunicationsManagerNamedPipes()
{
    delete mtx;
    delete informerMutex;
}
}//end namespace
#endif

