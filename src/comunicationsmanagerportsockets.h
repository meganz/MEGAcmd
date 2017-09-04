/**
 * @file examples/megacmd/comunicationsmanagerportsockets.h
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
#ifndef COMUNICATIONSMANAGERPORTSOCKETS_H
#define COMUNICATIONSMANAGERPORTSOCKETS_H

#include "comunicationsmanager.h"

#include <sys/types.h>
#ifdef _WIN32
#include <WinSock2.h>
#else
#include <sys/socket.h>
#endif
#define MEGACMDINITIALPORTNUMBER 12300


void closeSocket(int socket);

class CmdPetitionPortSockets: public CmdPetition
{
public:
    int outSocket;
    int acceptedOutSocket;
    CmdPetitionPortSockets(){
        acceptedOutSocket = -1;
    }
    virtual ~CmdPetitionPortSockets()
    {
        closeSocket(outSocket);
        if (acceptedOutSocket != -1)
        {
            closeSocket(acceptedOutSocket);
        }
    }
};

OUTSTREAMTYPE &operator<<(OUTSTREAMTYPE &os, CmdPetitionPortSockets &p);

class ComunicationsManagerPortSockets : public ComunicationsManager
{
private:
    fd_set fds;

    // sockets and asociated variables
    int sockfd, newsockfd;
#ifdef _WIN32
    HANDLE sockfd_event_handle;
    static bool ended;
#endif
    socklen_t clilen;
    char buffer[1024];
    struct sockaddr_in serv_addr, cli_addr;

    // to get next socket id
    int count;
    mega::MegaMutex *mtx;

    /**
     * @brief create_new_socket
     * The caller is responsible for deleting the newly created socket
     * @return
     */
    int create_new_socket(int *sockId);

public:
    ComunicationsManagerPortSockets();

    int initialize();

    bool receivedPetition();

    int waitForPetition();

    virtual void stopWaiting();

    int get_next_comm_id();

    void registerStateListener(CmdPetition *inf);

    /**
     * @brief returnAndClosePetition
     * I will clean struct and close the socket within
     */
    void returnAndClosePetition(CmdPetition *inf, OUTSTRINGSTREAM *s, int);

    int informStateListener(CmdPetition *inf, std::string &s);

    /**
     * @brief getPetition
     * @return pointer to new CmdPetitionPosix. Petition returned must be properly deleted (this can be calling returnAndClosePetition)
     */
    CmdPetition *getPetition();

    virtual bool getConfirmation(CmdPetition *inf, std::string message);

    /**
     * @brief get_petition_details
     * @return a string describing details of the petition
     */
    std::string get_petition_details(CmdPetition *inf);

    ~ComunicationsManagerPortSockets();
};


#endif // COMUNICATIONSMANAGERPOSIX_H
