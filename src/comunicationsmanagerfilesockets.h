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
        shutdown(outSocket, SHUT_RDWR);
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
    int sockfd;
    char buffer[1024];
    struct sockaddr_in serv_addr, cli_addr;

    // to get next socket id
    int count;
    std::mutex mtx;
    std::mutex informerMutex;

    void sendPartialOutputImpl(CmdPetition *inf, char *s, size_t size, bool binaryContents, bool sendAsError);
public:
    ComunicationsManagerFileSockets();

    int initialize();

    bool receivedPetition();

    int waitForPetition() override;

    virtual void stopWaiting();

    CmdPetition* registerStateListener(std::unique_ptr<CmdPetition> &&inf) override;

    int getMaxStateListeners() const override;

    /**
     * @brief returnAndClosePetition
     * I will clean struct and close the socket within
     */
    void returnAndClosePetition(std::unique_ptr<CmdPetition> inf, OUTSTRINGSTREAM *s, int) override;

    void sendPartialOutput(CmdPetition *inf, OUTSTRING *s) override;
    void sendPartialOutput(CmdPetition *inf, char *s, size_t size, bool binaryContents = false) override;
    void sendPartialError(CmdPetition *inf, OUTSTRING *s) override;
    void sendPartialError(CmdPetition *inf, char *s, size_t size, bool binaryContents = false) override;

    int informStateListener(CmdPetition *inf, const std::string &s) override;


    /**
     * @brief getPetition
     * @return pointer to new CmdPetitionPosix. Petition returned must be properly deleted (this can be calling returnAndClosePetition)
     */
    std::unique_ptr<CmdPetition> getPetition() override;

    virtual int getConfirmation(CmdPetition *inf, std::string message);

    virtual std::string getUserResponse(CmdPetition *inf, std::string message);

    ~ComunicationsManagerFileSockets();
};

}//end namespace
#endif
#endif // COMUNICATIONSMANAGERPOSIX_H
