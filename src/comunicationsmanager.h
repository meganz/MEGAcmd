/**
 * @file src/comunicationsmanager.h
 * @brief MEGAcmd: Communications manager non supporting non-interactive mode
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

#ifndef COMUNICATIONSMANAGER_H
#define COMUNICATIONSMANAGER_H

#include "megacmd.h"
#include "megacmdcommonutils.h"

namespace megacmd {
class CmdPetition
{
public:
    char *line;
    mega::MegaThread *petitionThread;
    int clientID = -27;
    bool clientDisconnected;

    CmdPetition()
    {
        line = NULL;
        petitionThread = NULL;
        clientDisconnected = false;
    }

    char *getLine()
    {
        return line;
    }
    virtual ~CmdPetition()
    {
        if (line != NULL)
        {
            free(line);
        }
    }
    mega::MegaThread *getPetitionThread() const;
    void setPetitionThread(mega::MegaThread *value);
    virtual std::string getPetitionDetails() const { return {}; }
};

OUTSTREAMTYPE &operator<<(OUTSTREAMTYPE &os, CmdPetition const &p);

#ifdef _WIN32
std::ostream &operator<<(std::ostream &os, CmdPetition const &p);
#endif

class ComunicationsManager
{
private:
    fd_set fds;

    std::recursive_mutex mStateListenersMutex;
    std::vector<CmdPetition *> stateListenersPetitions;

public:
    ComunicationsManager();

    virtual bool receivedPetition();

    virtual bool registerStateListener(CmdPetition *inf);

    virtual int waitForPetition();

    virtual void stopWaiting();

    virtual int get_next_comm_id();

    virtual int getMaxStateListeners() const;

    void ackStateListenersAndRemoveClosed();

    /**
     * @brief returnAndClosePetition
     * It will clean struct and close the socket within
     */
    virtual void returnAndClosePetition(CmdPetition *inf, OUTSTRINGSTREAM *s, int);

    virtual void sendPartialOutput(CmdPetition *inf, OUTSTRING *s);
    virtual void sendPartialOutput(CmdPetition *inf, char *s, size_t size);


    /**
     * @brief Sends an status message (e.g. prompt:who@/new/prompt:) to all registered listeners
     * @param s
     * @returns if state listeners left
     */
    bool informStateListeners(const std::string &s);

    void informStateListenerByClientId(const std::string &s, int clientID);

    /**
     * @brief informStateListener
     * @param inf This contains the petition that originated the register. It should contain the implementation details that identify a listener
     *   (e.g. In a socket implementation, the socket identifier)
     * @param s
     * @return -1 if connection closed by listener (removal required)
     */
    virtual int informStateListener(CmdPetition *inf, const std::string &s);


    /**
     * @brief getPetition
     * @return pointer to new CmdPetition. Petition returned must be properly deleted (this can be calling returnAndClosePetition)
     */
    virtual CmdPetition *getPetition();

    virtual int getConfirmation(CmdPetition *inf, std::string message);
    virtual std::string getUserResponse(CmdPetition *inf, std::string message);

    virtual ~ComunicationsManager();
};

} //end namespace
#endif // COMUNICATIONSMANAGER_H
