/**
 * @file src/comunicationsmanagerfilesockets.h
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
#ifndef COMUNICATIONSMANAGERFILESOCKETS_H
#define COMUNICATIONSMANAGERFILESOCKETS_H

#ifndef WIN32

#include "comunicationsmanager.h"

#include <sys/types.h>
#include <sys/socket.h>

namespace megacmd {
class CmdPetitionPosixSockets: public CmdPetition
{
public:
    int outSocket = -1;

    CmdPetitionPosixSockets(){
    }

    virtual ~CmdPetitionPosixSockets()
    {
        close(outSocket);
        outSocket = -1;
    }
};

OUTSTREAMTYPE &operator<<(OUTSTREAMTYPE &os, CmdPetitionPosixSockets &p);

class ComunicationsManagerFileSockets : public ComunicationsManager
{
private:
    fd_set fds;

    // sockets and asociated variables
    int sockfd, newsockfd;
    socklen_t clilen;
    char buffer[1024];
    struct sockaddr_in serv_addr, cli_addr;

    // to get next socket id
    int count;
    std::mutex mtx;
    std::mutex informerMutex;

public:
    ComunicationsManagerFileSockets();

    int initialize();

    bool receivedPetition();

    int waitForPetition();

    virtual void stopWaiting();

    void registerStateListener(CmdPetition *inf);

    /**
     * @brief returnAndClosePetition
     * I will clean struct and close the socket within
     */
    void returnAndClosePetition(CmdPetition *inf, OUTSTRINGSTREAM *s, int);

    virtual void sendPartialOutput(CmdPetition *inf, OUTSTRING *s);

    int informStateListener(CmdPetition *inf, std::string &s);


    /**
     * @brief getPetition
     * @return pointer to new CmdPetitionPosix. Petition returned must be properly deleted (this can be calling returnAndClosePetition)
     */
    CmdPetition *getPetition();

    virtual int getConfirmation(CmdPetition *inf, std::string message);

    virtual std::string getUserResponse(CmdPetition *inf, std::string message);

    /**
     * @brief get_petition_details
     * @return a string describing details of the petition
     */
    std::string get_petition_details(CmdPetition *inf);

    /**
     * @brief getSocketPath
     * @return a string containing the path to the server socket
     */
    static std::string getSocketPath(bool ensure);
    ~ComunicationsManagerFileSockets();
};

}//end namespace
#endif
#endif // COMUNICATIONSMANAGERPOSIX_H
