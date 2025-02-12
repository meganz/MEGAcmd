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

ComunicationsManager::ComunicationsManager()
{
}

bool ComunicationsManager::receivedPetition()
{
    return false;
}

CmdPetition* ComunicationsManager::registerStateListener(std::unique_ptr<CmdPetition> &&inf)
{
    std::lock_guard<std::recursive_mutex> g(mStateListenersMutex);

    // Always remove closed listeners before a new one comes in
    ackStateListenersAndRemoveClosed();

    if (stateListenersPetitions.size() >= getMaxStateListeners())
    {
        LOG_warn << "Max number of state listeners (" << stateListenersPetitions.size() << ") reached";
        return nullptr;
    }

    stateListenersPetitions.emplace_back(std::move(inf));
    return stateListenersPetitions.back().get();
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
        if (informStateListener(it->get(), s + (char) 0x1F) < 0)
        {
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
            if (informStateListener(it->get(), s + (char) 0x1F) < 0)
            {
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

void ComunicationsManager::returnAndClosePetition(std::unique_ptr<CmdPetition> inf, OUTSTRINGSTREAM *s, int outCode)
{
}


/**
 * @brief getPetition
 * @return pointer to new CmdPetition. Petition returned must be properly deleted (this can be calling returnAndClosePetition)
 */
std::unique_ptr<CmdPetition> ComunicationsManager::getPetition()
{
    return std::make_unique<CmdPetition>();
}

int ComunicationsManager::getConfirmation(CmdPetition *inf, string message)
{
    return MCMDCONFIRM_NO;
}

std::string ComunicationsManager::getUserResponse(CmdPetition *inf, string message)
{
    return string();
}

void CmdPetition::setLine(std::string_view line)
{
    mLine = line;
}

std::string_view CmdPetition::getLine() const
{
    return mLine;
}

std::string_view CmdPetition::getUniformLine() const
{
    return ltrim(std::string_view(mLine), 'X');
}

std::string CmdPetition::getRedactedLine() const
{
    if (!startsWith(getUniformLine(), "login"))
    {
        return mLine;
    }

    constexpr std::string_view passStr = "*********";

    std::vector<std::string> tokens = split(mLine, " ");
    std::vector<int> nonFlagIndices;

    for (int i = 1; i < tokens.size(); ++i)
    {
        auto& token = tokens[i];
        if (startsWith(token, "--auth-code="))
        {
            token = "--auth-code=";
            token += passStr;
        }
        else if (startsWith(token, "--auth-key="))
        {
            token = "--auth-key=";
            token += passStr;
        }
        else if (startsWith(token, "--password="))
        {
            token = "--password=";
            token += passStr;
        }

        if (!startsWith(token, "-"))
        {
            nonFlagIndices.push_back(i);
        }
    }

    if (nonFlagIndices.size() == 2)
    {
        // With two non-flag tokens, the first is the email and the second is the password
        tokens[nonFlagIndices[1]] = passStr;
    }
    else if (nonFlagIndices.size() == 1)
    {
        // With one non-flag token, hide only if it's the session
        const std::string &token = tokens[nonFlagIndices[0]];
        const bool isSession = (token.find('@') == std::string::npos && token.find('#') == std::string::npos);
        if (isSession)
        {
            tokens[nonFlagIndices[0]] = passStr;
        }
    }

    return joinStrings(tokens, " ", false);
}

bool CmdPetition::isFromCmdShell() const
{
    return startsWith(mLine, "X");
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
