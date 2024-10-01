/**
 * @file src/comunicationsmanager.cpp
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

#include "comunicationsmanager.h"

using namespace mega;

namespace megacmd {
OUTSTREAMTYPE &operator<<(OUTSTREAMTYPE &os, const CmdPetition& p)
{
    return os << p.line;
}

#ifdef _WIN32
std::ostream &operator<<(std::ostream &os, const CmdPetition& p)
{
    return os << p.line;
}
#endif

ComunicationsManager::ComunicationsManager()
{
}

bool ComunicationsManager::receivedPetition()
{
    return false;
}

bool ComunicationsManager::registerStateListener(CmdPetition *inf)
{
    std::lock_guard<std::recursive_mutex> g(mStateListenersMutex);

    // Always remove closed listeners before a new one comes in
    ackStateListenersAndRemoveClosed();

    if (stateListenersPetitions.size() >= getMaxStateListeners())
    {
        LOG_warn << "Max number of state listeners (" << stateListenersPetitions.size() << ") reached";
        return false;
    }

    stateListenersPetitions.push_back(inf);
    return true;
}

int ComunicationsManager::waitForPetition()
{
    return 0;
}

void ComunicationsManager::stopWaiting()
{
}

int ComunicationsManager::get_next_comm_id()
{
    return 0;
}

int ComunicationsManager::getMaxStateListeners() const
{
    return 200; // default value
}

void ComunicationsManager::ackStateListenersAndRemoveClosed()
{
    informStateListeners("ack");
}

bool ComunicationsManager::informStateListeners(const string &s)
{
    std::lock_guard<std::recursive_mutex> g(mStateListenersMutex);
    for (auto it = stateListenersPetitions.begin(); it != stateListenersPetitions.end();)
    {
        if (informStateListener(*it, s + (char) 0x1F) < 0)
        {
            delete *it;
            it = stateListenersPetitions.erase(it);
            continue;
        }
        ++it;
    }
    return !stateListenersPetitions.empty();
}

void ComunicationsManager::informStateListenerByClientId(const string &s, int clientID)
{
    std::lock_guard<std::recursive_mutex> g(mStateListenersMutex);
    for (auto it = stateListenersPetitions.begin(); it != stateListenersPetitions.end(); ++it)
    {
        if (clientID == (*it)->clientID)
        {
            if (informStateListener(*it, s + (char) 0x1F) < 0)
            {
                delete *it;
                stateListenersPetitions.erase(it);
            }
            return;
        }
    }
}

int ComunicationsManager::informStateListener(CmdPetition *inf, const string &s)
{
    return 0;
}

void ComunicationsManager::returnAndClosePetition(CmdPetition *inf, OUTSTRINGSTREAM *s, int outCode)
{
    delete inf;
    return;
}

void ComunicationsManager::sendPartialOutput(CmdPetition *inf, OUTSTRING *s)
{
    return;
}

void ComunicationsManager::sendPartialOutput(CmdPetition *inf, char *s, size_t size)
{
    return;
}



/**
 * @brief getPetition
 * @return pointer to new CmdPetition. Petition returned must be properly deleted (this can be calling returnAndClosePetition)
 */
CmdPetition * ComunicationsManager::getPetition()
{
    CmdPetition *inf = new CmdPetition();
    return inf;
}

int ComunicationsManager::getConfirmation(CmdPetition *inf, string message)
{
    return MCMDCONFIRM_NO;
}

std::string ComunicationsManager::getUserResponse(CmdPetition *inf, string message)
{
    return string();
}

ComunicationsManager::~ComunicationsManager()
{
    std::lock_guard<std::recursive_mutex> g(mStateListenersMutex);
    for (std::vector< CmdPetition * >::iterator it = stateListenersPetitions.begin(); it != stateListenersPetitions.end();)
    {
        delete *it;
        it = stateListenersPetitions.erase(it);
    }
}

std::string CmdPetition::getUniformLine() const
{
    std::string str = line;
    return ltrim(str, 'X');
}

MegaThread *CmdPetition::getPetitionThread() const
{
    return petitionThread;
}

void CmdPetition::setPetitionThread(MegaThread *value)
{
    petitionThread = value;
}
}//end namespace
