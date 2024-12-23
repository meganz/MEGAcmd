/**
 * @file src/comunicationsmanagerfilesockets.cpp
 * @brief MEGAcmd: Communications manager using Network Sockets
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
#include "megacmdcommonutils.h"
#ifndef WIN32

#include "comunicationsmanagerfilesockets.h"
#include "megacmdutils.h"
#include <sys/ioctl.h>
#include <sys/resource.h>

#ifdef __MACH__
#define MSG_NOSIGNAL 0
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

using namespace mega;

namespace megacmd {

ComunicationsManagerFileSockets::ComunicationsManagerFileSockets()
{
    count = 0;
    initialize();
}

int ComunicationsManagerFileSockets::initialize()
{
    auto socketPath = getOrCreateSocketPath(true);
    struct sockaddr_un addr;

    if (socketPath.empty())
    {
        LOG_fatal  << "Could not create runtime directory for socket file: " << strerror(errno);
    }
    if (socketPath.size() >= (ARRAYSIZE(addr.sun_path) - 1))
    {
        LOG_fatal << "Server socket path is too long: '" << socketPath << "'. Exceeds " << ARRAYSIZE(addr.sun_path) - 1 << " max allowance";
        return -1;
    }

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        LOG_fatal << "ERROR opening socket";
    }
    if (fcntl(sockfd, F_SETFD, FD_CLOEXEC) == -1)
    {
        LOG_err << "ERROR setting CLOEXEC to socket: " << errno;
    }

    socklen_t saddrlen = sizeof( addr );
    memset(&addr, 0, sizeof( addr ));
    addr.sun_family = AF_UNIX;

    strncpy(addr.sun_path, socketPath.c_str(), socketPath.size());
    unlink(addr.sun_path);

    LOG_debug << "Binding socket to path " << socketPath;
    if (::bind(sockfd, (struct sockaddr *)&addr, saddrlen))
    {
        if (errno == EADDRINUSE)
        {
            LOG_warn << "ERROR on binding socket: " << socketPath << ": Already in use.";
        }
        else
        {
            LOG_fatal << "ERROR on binding socket: " << socketPath << ": " << strerror(errno);
        }
        close(sockfd);
        sockfd = -1;
    }
    else
    {
        LOG_debug << "Listening for commands at socket path " << socketPath;
        int returned = listen(sockfd, 150);
        if (returned)
        {
            LOG_fatal << "ERROR on listen socket initializing communications manager: " << socketPath << ": " << strerror(errno);
            close(sockfd);
            return errno;
        }
    }
    return 0;
}

bool ComunicationsManagerFileSockets::receivedPetition()
{
    return FD_ISSET(sockfd, &fds);
}

int ComunicationsManagerFileSockets::waitForPetition()
{
    FD_ZERO(&fds);
    if (sockfd)
    {
        FD_SET(sockfd, &fds);
    }
    int rc = select(FD_SETSIZE, &fds, NULL, NULL, NULL);
    if (rc < 0)
    {
        if (errno != EINTR)  //syscall
        {
            LOG_fatal << "Error at select: " << errno;
            return errno;
        }
    }
    return 0;
}

void ComunicationsManagerFileSockets::stopWaiting()
{
#ifdef _WIN32
    shutdown(sockfd,SD_BOTH);
#else
    LOG_verbose << "Shutting down main socket ";

    if (shutdown(sockfd,SHUT_RDWR) == -1)
    { //shutdown failed. we need to send something to the blocked thread so as to wake up from select

        int clientsocket = socket(AF_UNIX, SOCK_STREAM, 0);
        auto socketPath = getOrCreateSocketPath(false);
        LOG_info << "listening at " << socketPath;
        if (clientsocket < 0 )
        {
            LOG_err << "ERROR opening client socket to exit select: " << errno;
            close (sockfd);
        }
        else
        {
            if (fcntl(clientsocket, F_SETFD, FD_CLOEXEC) == -1)
            {
                LOG_err << "ERROR setting CLOEXEC to socket: " << errno;
            }

            struct sockaddr_un addr;
            if (socketPath.size() >= (ARRAYSIZE(addr.sun_path) - 1))
            {
                LOG_fatal << "Server socket path is too long: '" << socketPath << "'";
                return;
            }
            memset(&addr, 0, sizeof( addr ));
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, socketPath.c_str(), socketPath.size());

            if (::connect(clientsocket, (struct sockaddr*)&addr, sizeof( addr )) != -1)
            {
                if (send(clientsocket,"no matter",1,MSG_NOSIGNAL) == -1)
                {
                    LOG_err << "ERROR sending via client socket to exit select: " << errno;
                    close (sockfd);
                }

                close(clientsocket);
            }
            else
            {
                LOG_err << "ERROR connecting client socket to exit select: " << errno;
                close (sockfd);
            }
        }
    }
    LOG_verbose << "Main socket shut down";
#endif
}


CmdPetition* ComunicationsManagerFileSockets::registerStateListener(std::unique_ptr<CmdPetition> &&inf)
{
    const int socket = ((CmdPetitionPosixSockets*) inf.get())->outSocket;
    LOG_debug << "Registering state listener petition with socket: " << socket;

#ifndef NDEBUG
    // let's not be gentle with state listener sockets to prevent frozen clients from stopping the server
    const timeval timeout { .tv_sec = 10, .tv_usec = 0};
    if (setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        LOG_err << "ERROR setting state listener socket timeout: " << errno;
    }
#endif
    return ComunicationsManager::registerStateListener(std::move(inf));
}

int ComunicationsManagerFileSockets::getMaxStateListeners() const
{
    static int maxListenersAllowed = [this](){
        struct rlimit limit;
        if (getrlimit(RLIMIT_NOFILE, &limit) != 0)
        {
            LOG_err << "Failed to get ulimit -n (errno: " << errno << "); falling back to max state listeners default";
            return ComunicationsManager::getMaxStateListeners();
        }
        int systemNumFilesLimit = static_cast<int>(limit.rlim_cur);
        int maxListeners = systemNumFilesLimit - std::max(100, static_cast<int>(systemNumFilesLimit * 0.20)); // leave 20% or 100 file descriptors for libraries and other fds:
        maxListeners = std::min(maxListeners, static_cast<int>(FD_SETSIZE * 0.4)); // we don't want to use fd with numbers > 1024: select will not digest them well: lets play a safe 60% margin.

        return std::max(2/*minimum requirement*/, maxListeners); // maxListeners may be negative based on above calculations (unexpected). Let's play our chances of survival despite that.
    }();

    return maxListenersAllowed;
}

/**
 * @brief returnAndClosePetition
 * I will clean struct and close the socket within
 */
void ComunicationsManagerFileSockets::returnAndClosePetition(std::unique_ptr<CmdPetition> inf, OUTSTRINGSTREAM *s, int outCode)
{
    const int socket = ((CmdPetitionPosixSockets *) inf.get())->outSocket;
    assert(socket != -1);

    LOG_verbose << "Output to write in socket " << socket;
    if (socket == -1)
    {
        LOG_fatal << "Return and close: not valid outsocket " << socket;
        return;
    }

    string sout = s->str();

    auto n = send(socket, &outCode, sizeof(outCode), MSG_NOSIGNAL);
    if (n < 0)
    {
        LOG_err << "ERROR writing output Code to socket: " << errno;
    }
    n = send(socket, sout.data(), max(static_cast<size_t>(1), sout.size()), MSG_NOSIGNAL); // for some reason without the max recv never quits in the client for empty responses
    if (n < 0)
    {
        LOG_err << "ERROR writing to socket: " << errno;
    }
}

void ComunicationsManagerFileSockets::sendPartialOutput(CmdPetition *inf, OUTSTRING *s)
{
    if (inf->clientDisconnected)
    {
        return;
    }

    int connectedsocket = ((CmdPetitionPosixSockets *)inf)->outSocket;
    assert(connectedsocket != -1);
    if (connectedsocket == -1)
    {
        std::cerr << "Return and close: no valid outsocket " << ((CmdPetitionPosixSockets *)inf)->outSocket << endl;
        return;
    }

    if (s->size())
    {
        size_t size = s->size();

        int outCode = MCMD_PARTIALOUT;
        auto n = send(connectedsocket, (void*)&outCode, sizeof( outCode ), MSG_NOSIGNAL);
        if (n < 0)
        {
            std::cerr << "ERROR writing MCMD_PARTIALOUT to socket: " << errno << endl;
            if (errno == EPIPE)
            {
                std::cerr << "WARNING: Client disconnected, the rest of the output will be discarded" << endl;
                inf->clientDisconnected = true;
            }
            return;
        }
        n = send(connectedsocket, (void*)&size, sizeof( size ), MSG_NOSIGNAL);
        if (n < 0)
        {
            std::cerr << "ERROR writing size of partial output to socket: " << errno << endl;
            return;
        }


        n = send(connectedsocket, s->data(), size, MSG_NOSIGNAL); // for some reason without the max recv never quits in the client for empty responses

        if (n < 0)
        {
            std::cerr << "ERROR writing to socket partial output: " << errno << endl;
            return;
        }
    }
}

int ComunicationsManagerFileSockets::informStateListener(CmdPetition *inf, const string &s)
{
    std::lock_guard<std::mutex> g(informerMutex);
    LOG_verbose << "Inform State Listener: Output to write in socket " << ((CmdPetitionPosixSockets *)inf)->outSocket << ": <<" << s << ">>";

    int connectedsocket = ((CmdPetitionPosixSockets *)inf)->outSocket;
    assert(connectedsocket != -1);

    if (connectedsocket == -1)
    {
        LOG_err << "Informing state listener: Not valid on outsocket " << ((CmdPetitionPosixSockets *)inf)->outSocket;
        return 0;
    }

#ifdef __MACH__
#define MSG_NOSIGNAL 0
#endif

    auto n = send(connectedsocket, s.data(), s.size(), MSG_NOSIGNAL);
    if (n < 0)
    {
        if (errno == EPIPE) //socket closed
        {
            LOG_verbose << "Unregistering no longer listening client. Original petition: " << inf->line;
            return -1;
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK) // timed out
        {
            LOG_warn << "Unregistering timed out listening client. Original petition: " << inf->line;
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
std::unique_ptr<CmdPetition> ComunicationsManagerFileSockets::getPetition()
{
    auto inf = std::make_unique<CmdPetitionPosixSockets>();
    static socklen_t clilen = sizeof(cli_addr);

    int newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, &clilen);

    if (newsockfd < 0)
    {
        if (errno == EMFILE)
        {
            LOG_fatal << "ERROR on accept at getPetition: TOO many open files.";
            ackStateListenersAndRemoveClosed();
        }
        else
        {
            LOG_fatal << "ERROR on accept at getPetition: " << errno;
        }

        sleep(1);
        inf->line = "ERROR";
        return inf;
    }

    if (fcntl(newsockfd, F_SETFD, FD_CLOEXEC) == -1)
    {
        LOG_err << "ERROR setting CLOEXEC to socket: " << errno;
    }

    string wholepetition;

    int n = read(newsockfd, buffer, 1023);
    while(n == 1023)
    {
        unsigned long int total_available_bytes;
        if (-1 == ioctl(newsockfd, FIONREAD, &total_available_bytes))
        {
            LOG_err << "Failed to PeekNamedPipe. errno: " << errno;
            break;
        }

        if (total_available_bytes == 0)
        {
            break;
        }

        buffer[n] = '\0';
        wholepetition.append(buffer);
        n = read(newsockfd, buffer, 1023);
    }
    if (n >=0 )
    {
        buffer[n] = '\0';
        wholepetition.append(buffer);
    }
    if (n < 0)
    {
        LOG_fatal << "ERROR reading from socket at getPetition: " << errno;
        inf->line = "ERROR";
        close(newsockfd);
        return inf;
    }

    inf->outSocket = newsockfd;
    inf->line = wholepetition;

    return inf;
}

int ComunicationsManagerFileSockets::getConfirmation(CmdPetition *inf, string message)
{
    int connectedsocket = ((CmdPetitionPosixSockets *)inf)->outSocket;
    assert(connectedsocket != -1);
    if (connectedsocket == -1)
    {
        LOG_fatal << "Getting Confirmation: invalid outsocket " << ((CmdPetitionPosixSockets *)inf)->outSocket;
        return false;
    }

    int outCode = MCMD_REQCONFIRM;
    auto n = send(connectedsocket, (void*)&outCode, sizeof( outCode ), MSG_NOSIGNAL);
    if (n < 0)
    {
        LOG_err << "ERROR writing output Code to socket: " << errno;
    }
    n = send(connectedsocket, message.data(), max(1,(int)message.size()), MSG_NOSIGNAL); // for some reason without the max recv never quits in the client for empty responses
    if (n < 0)
    {
        LOG_err << "ERROR writing to socket: " << errno;
    }

    int response;
    n = recv(connectedsocket,&response, sizeof(response), MSG_NOSIGNAL);

    return response;
}

string ComunicationsManagerFileSockets::getUserResponse(CmdPetition *inf, string message)
{
    int connectedsocket = ((CmdPetitionPosixSockets *)inf)->outSocket;
    assert(connectedsocket != -1);
    if (connectedsocket == -1)
    {
        LOG_fatal << "Getting Confirmation: Invalid outsocket " << ((CmdPetitionPosixSockets *)inf)->outSocket;
        return "FAILED";
    }

    int outCode = MCMD_REQSTRING;
    auto n = send(connectedsocket, (void*)&outCode, sizeof( outCode ), MSG_NOSIGNAL);
    if (n < 0)
    {
        LOG_err << "ERROR writing output Code to socket: " << errno;
    }
    n = send(connectedsocket, message.data(), max(static_cast<size_t>(1), message.size()), MSG_NOSIGNAL); // for some reason without the max recv never quits in the client for empty responses
    if (n < 0)
    {
        LOG_err << "ERROR writing to socket: " << errno;
    }

    string response;
    int BUFFERSIZE = 1024;
    char buffer[1025];
    do{
        n = recv(connectedsocket, buffer, BUFFERSIZE, MSG_NOSIGNAL);
        if (n)
        {
            buffer[n] = '\0';
            response += buffer;
        }
    } while(n == BUFFERSIZE && n != SOCKET_ERROR);

    return response;
}

ComunicationsManagerFileSockets::~ComunicationsManagerFileSockets()
{
}
}//end namespace

#endif
