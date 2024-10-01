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
struct CmdPetitionPosixSockets: public CmdPetition
{
    int outSocket = -1;

    virtual ~CmdPetitionPosixSockets()
    {
        close(outSocket);
    }

    std::string getPetitionDetails() const override
    {
        return "socket output: " + std::to_string(outSocket);
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

    int waitForPetition() override;

    virtual void stopWaiting();

    bool registerStateListener(CmdPetition *inf) override;

    int getMaxStateListeners() const override;

    /**
     * @brief returnAndClosePetition
     * I will clean struct and close the socket within
     */
    void returnAndClosePetition(CmdPetition *inf, OUTSTRINGSTREAM *s, int);

    virtual void sendPartialOutput(CmdPetition *inf, OUTSTRING *s);

    int informStateListener(CmdPetition *inf, const std::string &s) override;


    /**
     * @brief getPetition
     * @return pointer to new CmdPetitionPosix. Petition returned must be properly deleted (this can be calling returnAndClosePetition)
     */
    CmdPetition *getPetition();

    virtual int getConfirmation(CmdPetition *inf, std::string message);

    virtual std::string getUserResponse(CmdPetition *inf, std::string message);

    ~ComunicationsManagerFileSockets();
};

}//end namespace
#endif
#endif // COMUNICATIONSMANAGERPOSIX_H
