/**
 * @file examples/megacmd/comunicationsmanager.h
 * @brief MEGAcmd: Communications manager non supporting non-interactive mode
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

#ifndef COMUNICATIONSMANAGER_H
#define COMUNICATIONSMANAGER_H

#include "megacmd.h"

static const int MAXCMDSTATELISTENERS = 300;

class CmdPetition
{
    public:
        char * line;
        mega::MegaThread * petitionThread;
        int clientID;

        CmdPetition()
        {
            line = NULL;
            petitionThread = NULL;
        }

        char *getLine()
        {
            return line;
        }
        virtual ~CmdPetition()
        {
            if ( line != NULL )
            {
                free(line);
            }
        }
        mega::MegaThread *getPetitionThread() const;
        void setPetitionThread(mega::MegaThread *value);

};

OUTSTREAMTYPE &operator<<(OUTSTREAMTYPE &os, CmdPetition const &p);

#ifdef _WIN32
std::ostream &operator<<(std::ostream &os, CmdPetition const &p);
#endif

class ComunicationsManager
{
private:
    fd_set fds;
    std::vector<CmdPetition *> stateListenersPetitions;

public:
    ComunicationsManager();

    virtual bool receivedPetition();

    void registerStateListener(CmdPetition *inf);

    virtual int waitForPetition();

    virtual void stopWaiting();

    virtual int get_next_comm_id();

    /**
     * @brief returnAndClosePetition
     * It will clean struct and close the socket within
     */
    virtual void returnAndClosePetition(CmdPetition *inf, OUTSTRINGSTREAM *s, int);

    /**
     * @brief Sends an status message (e.g. prompt:who@/new/prompt:) to all registered listeners
     * @param s
     */
    void informStateListeners(std::string &s);

    void informStateListenerByClientId(std::string &s, int clientID);

    /**
     * @brief informStateListener
     * @param inf This contains the petition that originated the register. It should contain the implementation details that identify a listener
     *   (e.g. In a socket implementation, the socket identifier)
     * @param s
     * @return -1 if connection closed by listener (removal required)
     */
    virtual int informStateListener(CmdPetition *inf, std::string &s);



    /**
     * @brief getPetition
     * @return pointer to new CmdPetition. Petition returned must be properly deleted (this can be calling returnAndClosePetition)
     */
    virtual CmdPetition *getPetition();

    virtual bool getConfirmation(CmdPetition *inf, std::string message);

    /**
     * @brief get_petition_details
     * @return a std::string describing details of the petition
     */
    virtual std::string get_petition_details(CmdPetition *inf);

    virtual ~ComunicationsManager();
};

#endif // COMUNICATIONSMANAGER_H
