/**
 * @file examples/megacmd/comunicationsmanagerportsockets.cpp
 * @brief MEGAcmd: Communications manager using Network Sockets
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

#include "comunicationsmanagerportsockets.h"
#include "megacmdutils.h"

#ifdef _WIN32
#include <windows.h>
#define ERRNO WSAGetLastError()
#else
#define ERRNO errno
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

#ifndef EADDRINUSE
#define EADDRINUSE WSAEADDRINUSE
#endif

using namespace mega;

void closeSocket(int socket){
#ifdef _WIN32
    closesocket(socket);
#else
    close(socket);
#endif
}

bool socketValid(int socket)
{
#ifdef _WIN32
    return socket != INVALID_SOCKET;
#else
    return socket >= 0;
#endif
}

#ifdef _WIN32
    bool ComunicationsManagerPortSockets::ended;
#endif


int ComunicationsManagerPortSockets::get_next_comm_id()
{
    mtx->lock();
    ++count;
    mtx->unlock();
    return count;
}

int ComunicationsManagerPortSockets::create_new_socket(int *sockId)
{
    int thesock;
    int attempts = 10;
    bool socketsucceded = false;
    while (--attempts && !socketsucceded)
    {
        thesock = socket(AF_INET, SOCK_STREAM, 0);

        if (!socketValid(thesock))
        {
            if (errno == EMFILE)
            {
                LOG_verbose << " Trying to reduce number of used files by sending ACK to listeners to discard disconnected ones.";
                string sack="ack";
                informStateListeners(sack);
            }
            if (attempts !=10)
            {
                LOG_fatal << "ERROR opening socket ID=" << sockId << " errno: " << errno << ". Attempts: " << attempts;
            }
            sleepMicroSeconds(500);
        }
        else
        {
            socketsucceded = true;
        }
    }
    if (!socketValid(thesock))
    {
        return -1;
    }

    int portno=MEGACMDINITIALPORTNUMBER;

    *sockId = get_next_comm_id();
    portno += *sockId;

    struct sockaddr_in addr;

    memset(&addr, 0, sizeof( addr ));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
    addr.sin_port = htons(portno);

    socklen_t saddrlength = sizeof( addr );

    bool bindsucceeded = false;

    attempts = 10;
    while (--attempts && !bindsucceeded)
    {
        if (::bind(thesock, (struct sockaddr*)&addr, saddrlength) == SOCKET_ERROR)
        {
            if (errno == EADDRINUSE)
            {
                LOG_warn << "ERROR on binding socket: Already in use. Attempts: " << attempts;
            }
            else
            {
                LOG_fatal << "ERROR on binding socket port " << portno << " errno: " << errno << ". Attempts: " << attempts;
            }
            sleepMicroSeconds(500);
        }
        else
        {
            bindsucceeded = true;
        }
    }

    if (bindsucceeded)
    {
        if (thesock)
        {
            int returned = listen(thesock, 150);
            if (returned == SOCKET_ERROR)
            {
                LOG_fatal << "ERROR on listen socket: " << ERRNO;
            }
        }
        return thesock;
    }

    return 0;
}


ComunicationsManagerPortSockets::ComunicationsManagerPortSockets()
{
    count = 0;
    mtx = new MegaMutex();
    initialize();
}

int ComunicationsManagerPortSockets::initialize()
{
    mtx->init(false);
#if _WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    /* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
    wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        LOG_fatal << "ERROR initializing WSA";
    }
#endif

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (!socketValid(sockfd))
    {
        LOG_fatal << "ERROR opening socket";
    }

    int portno=MEGACMDINITIALPORTNUMBER;

    struct sockaddr_in addr;

    memset(&addr, 0, sizeof( addr ));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
    addr.sin_port = htons(portno);

    socklen_t saddrlength = sizeof( addr );

#ifdef _WIN32
    DWORD reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) != 0) {
         printf("SO_REUSEADDR setsockopt failed with: %d\n", WSAGetLastError());
    }
#endif

    if (::bind(sockfd, (struct sockaddr*)&addr, saddrlength) == SOCKET_ERROR)
    {
        if (ERRNO == EADDRINUSE)
        {
            LOG_fatal << "ERROR on binding socket at port: " << portno << ": Already in use.";
        }
        else
        {
            LOG_fatal << "ERROR on binding socket at port: " << portno << ": " << ERRNO;
        }
        sockfd = -1;

    }
    else
    {
        int returned = listen(sockfd, 150);
        if (returned == SOCKET_ERROR)
        {
            LOG_fatal << "ERROR on listen socket initializing communications manager  at port: " << portno << ": " << ERRNO;
            return ERRNO;
        }
#if _WIN32
    sockfd_event_handle = WSACreateEvent();
    if (WSAEventSelect( sockfd, sockfd_event_handle, FD_ACCEPT) == SOCKET_ERROR )
    {
        LOG_fatal << "Error at WSAEventSelect: " << ERRNO;
    }
    WSAResetEvent(sockfd_event_handle);


    ended=false;

#endif
    }

    return 0;
}

bool ComunicationsManagerPortSockets::receivedPetition()
{
    return FD_ISSET(sockfd, &fds);
}

int ComunicationsManagerPortSockets::waitForPetition()
{
    FD_ZERO(&fds);
    if (sockfd)
    {
        FD_SET(sockfd, &fds);
    }
    int rc = select(FD_SETSIZE, &fds, NULL, NULL, NULL);
    if (rc == SOCKET_ERROR)
    {
        if (ERRNO != EINTR)  //syscall
        {
            LOG_fatal << "Error at select: " << ERRNO;
            return ERRNO;
        }
    }
    return 0;
}

void ComunicationsManagerPortSockets::stopWaiting()
{
#ifdef _WIN32
    shutdown(sockfd,SD_BOTH);
#else
    shutdown(sockfd,SHUT_RDWR);
#endif
}

void ComunicationsManagerPortSockets::registerStateListener(CmdPetition *inf)
{
    LOG_debug << "Registering state listener petition with socket: " << ((CmdPetitionPortSockets *) inf)->outSocket;
    ComunicationsManager::registerStateListener(inf);
}

//TODO: implement unregisterStateListener, not 100% necesary, since when a state listener is not accessible it is unregistered (to deal with sudden deaths).
// also, unregistering might not be straight forward since we need to correlate the thread doing the unregistration with the one who registered.


/**
 * @brief returnAndClosePetition
 * I will clean struct and close the socket within
 */
void ComunicationsManagerPortSockets::returnAndClosePetition(CmdPetition *inf, OUTSTRINGSTREAM *s, int outCode)
{
    LOG_verbose << "Output to write in socket " << ((CmdPetitionPortSockets *)inf)->outSocket << ": <<" << s->str() << ">>";
    sockaddr_in cliAddr;
    socklen_t cliLength = sizeof( cliAddr );
    int connectedsocket = ((CmdPetitionPortSockets *)inf)->acceptedOutSocket;
    if (connectedsocket == SOCKET_ERROR)
    {
        connectedsocket = accept(((CmdPetitionPortSockets *)inf)->outSocket, (struct sockaddr*)&cliAddr, &cliLength);
        ((CmdPetitionPortSockets *)inf)->acceptedOutSocket = connectedsocket; //So that it gets closed in destructor
    }
    if (connectedsocket == SOCKET_ERROR)
    {
        LOG_fatal << "Return and close: Unable to accept on outsocket " << ((CmdPetitionPortSockets *)inf)->outSocket << " error: " << ERRNO;
        delete inf;
        return;
    }

    OUTSTRING sout = s->str();
#ifdef __MACH__
#define MSG_NOSIGNAL 0
#elif _WIN32
#define MSG_NOSIGNAL 0
#endif
    int n = send(connectedsocket, (const char*)&outCode, sizeof( outCode ), MSG_NOSIGNAL);
    if (n == SOCKET_ERROR)
    {
        LOG_err << "ERROR writing output Code to socket: " << ERRNO;
    }

#ifdef _WIN32
   string sutf8;
   localwtostring(&sout,&sutf8);
   n = send(connectedsocket, sutf8.data(), sutf8.size(), MSG_NOSIGNAL);
#else
   n = send(connectedsocket, sout.data(), max(1,(int)sout.size()), MSG_NOSIGNAL); //TODO: test this max and do it for windows
#endif

    if (n == SOCKET_ERROR)
    {
        LOG_err << "ERROR writing to socket: " << ERRNO;
    }
    closeSocket(connectedsocket);
    closeSocket(((CmdPetitionPortSockets *)inf)->outSocket);
    delete inf;
}

int ComunicationsManagerPortSockets::informStateListener(CmdPetition *inf, string &s)
{
    LOG_verbose << "Inform State Listener: Output to write in socket " << ((CmdPetitionPortSockets *)inf)->outSocket << ": <<" << s << ">>";

    sockaddr_in cliAddr;
    socklen_t cliLength = sizeof( cliAddr );

    static map<int,int> connectedsockets;

    int connectedsocket = -1;
    if (connectedsockets.find(((CmdPetitionPortSockets *)inf)->outSocket) == connectedsockets.end())
    {
        //select with timeout and accept non-blocking, so that things don't get stuck
        fd_set set;
        FD_ZERO(&set);
        FD_SET(((CmdPetitionPortSockets *)inf)->outSocket, &set);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 4000000;
        int rv = select(((CmdPetitionPortSockets *)inf)->outSocket+1, &set, NULL, NULL, &timeout);
        if(rv == -1)
        {
            LOG_err << "Informing state listener: Unable to select on outsocket " << ((CmdPetitionPortSockets *)inf)->outSocket << " error: " << errno;
            return -1;
        }
        else if(rv == 0)
        {
            LOG_warn << "Informing state listener: timeout in select on outsocket " << ((CmdPetitionPortSockets *)inf)->outSocket;
        }
        else
        {
#ifndef _WIN32
            int oldfl = fcntl(sockfd, F_GETFL);
            fcntl(((CmdPetitionPortSockets *)inf)->outSocket, F_SETFL, oldfl | O_NONBLOCK);
#endif
            connectedsocket = accept(((CmdPetitionPortSockets *)inf)->outSocket, (struct sockaddr*)&cliAddr, &cliLength);
#ifndef _WIN32
            fcntl(((CmdPetitionPortSockets *)inf)->outSocket, F_SETFL, oldfl);
#endif
        }
        connectedsockets[((CmdPetitionPortSockets *)inf)->outSocket] = connectedsocket;
    }
    else
    {
        connectedsocket = connectedsockets[((CmdPetitionPortSockets *)inf)->outSocket];
    }

    if (connectedsocket == -1)
    {
        if (errno == 32) //socket closed
        {
            LOG_debug << "Unregistering no longer listening client. Original petition: " << *inf;
            closeSocket(connectedsocket);
            connectedsockets.erase(((CmdPetitionPortSockets *)inf)->outSocket);
            return -1;
        }
        else
        {
            LOG_err << "Informing state listener: Unable to accept on outsocket " << ((CmdPetitionPortSockets *)inf)->outSocket << " error: " << errno;
        }
        return 0;
    }

#ifdef __MACH__
#define MSG_NOSIGNAL 0
#endif

    int n = send(connectedsocket, s.data(), s.size(), MSG_NOSIGNAL);
    if (n < 0)
    {
        if (errno == 32) //socket closed
        {
            LOG_debug << "Unregistering no longer listening client. Original petition: " << *inf;
            connectedsockets.erase(((CmdPetitionPortSockets *)inf)->outSocket);
            return -1;
        }
        else
        {
            LOG_err << "ERROR writing to socket: " << errno;
        }
    }

    return 0;
}

/**
 * @brief getPetition
 * @return pointer to new CmdPetitionPosix. Petition returned must be properly deleted (this can be calling returnAndClosePetition)
 */
CmdPetition * ComunicationsManagerPortSockets::getPetition()
{
    CmdPetitionPortSockets *inf = new CmdPetitionPortSockets();

    clilen = sizeof( cli_addr );

    newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);

    if (!socketValid(newsockfd))
    {
        if (ERRNO == EMFILE)
        {
            LOG_fatal << "ERROR on accept at getPetition: TOO many open files.";
            //send state listeners an ACK command to see if they are responsive and close them otherwise
            string sack = "ack";
            informStateListeners(sack);
        }
        else
        {
            LOG_fatal << "ERROR on accept at getPetition: " << errno;
        }

        sleepSeconds(1);
        inf->line = strdup("ERROR");
        return inf;
    }

    memset(buffer, 0, 1024);

#ifdef _WIN32
    wchar_t wbuffer[1024]= {};
    int n = recv(newsockfd, (char *)wbuffer, 1023, MSG_NOSIGNAL);
    while (n == SOCKET_ERROR && ERRNO == WSAEWOULDBLOCK)
    {
        LOG_verbose << "Unexpected WSAEWOULDBLOCK in socket at getPetition: . Retrying ...";
        Sleep(10);
        n = recv(newsockfd, (char *)wbuffer, 1023, MSG_NOSIGNAL);
    }
    string receivedutf8;
    if (n != SOCKET_ERROR)
    {
        wbuffer[n]='\0';
#ifdef __MINGW32__
        wstring ws=wbuffer;
        localwtostring(&ws,&receivedutf8);
#else
        localwtostring(&wstring(wbuffer),&receivedutf8);
#endif
    }
    else
    {
        LOG_warn << "Received empty command from client at getPetition: ";
    }

#else
    int n = recv(newsockfd, buffer, 1023, MSG_NOSIGNAL);
#endif

    if (n == SOCKET_ERROR)
    {
        LOG_fatal << "ERROR reading from socket at getPetition: " << newsockfd << " errno: " << ERRNO;
        inf->line = strdup("ERROR");
        return inf;
    }

    int socket_id = 0;
    inf->outSocket = create_new_socket(&socket_id);
    if (!socketValid(inf->outSocket) || !socket_id)
    {
        LOG_fatal << "ERROR creating output socket at getPetition";
        inf->line = strdup("ERROR");
        return inf;
    }

    n = send(newsockfd, (const char*)&socket_id, sizeof( socket_id ), MSG_NOSIGNAL);

    if (n == SOCKET_ERROR)
    {
        LOG_fatal << "ERROR writing to socket at getPetition: ERRNO = " << ERRNO;
        inf->line = strdup("ERROR");
        return inf;
    }
    closeSocket(newsockfd);
#if _WIN32
    inf->line = strdup(receivedutf8.c_str());
#else
    inf->line = strdup(buffer);
#endif
    return inf;
}

bool ComunicationsManagerPortSockets::getConfirmation(CmdPetition *inf, string message)
{
    sockaddr_in cliAddr;
    socklen_t cliLength = sizeof( cliAddr );
    int connectedsocket = ((CmdPetitionPortSockets *)inf)->acceptedOutSocket;
    if (connectedsocket == -1)
        connectedsocket = accept(((CmdPetitionPortSockets *)inf)->outSocket, (struct sockaddr*)&cliAddr, &cliLength);
     ((CmdPetitionPortSockets *)inf)->acceptedOutSocket = connectedsocket;
    if (connectedsocket == -1)
    {
        LOG_fatal << "Getting Confirmation: Unable to accept on outsocket " << ((CmdPetitionPortSockets *)inf)->outSocket << " error: " << errno;
        delete inf;
        return false;
    }

    int outCode = MCMD_REQCONFIRM;
    int n = send(connectedsocket, (const char *)&outCode, sizeof( outCode ), MSG_NOSIGNAL);
    if (n < 0)
    {
        LOG_err << "ERROR writing output Code to socket: " << errno;
    }
    n = send(connectedsocket, message.data(), max(1,(int)message.size()), MSG_NOSIGNAL); // for some reason without the max recv never quits in the client for empty responses
    if (n < 0)
    {
        LOG_err << "ERROR writing to socket: " << errno;
    }

    bool response;
    n = recv(connectedsocket,(char *)&response, sizeof(response), MSG_NOSIGNAL);
    return response;
}

string ComunicationsManagerPortSockets::get_petition_details(CmdPetition *inf)
{
    ostringstream os;
    os << "socket output: " << ((CmdPetitionPortSockets *)inf)->outSocket;
    return os.str();
}


ComunicationsManagerPortSockets::~ComunicationsManagerPortSockets()
{
#if _WIN32
   WSACleanup();
   ended = true;
#endif
    delete mtx;
}
