/**
 * @file examples/megacmd/configurationmanager.h
 * @brief MEGAcmd: configuration manager
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

#ifndef CONFIGURATIONMANAGER_H
#define CONFIGURATIONMANAGER_H

#include "megacmd.h"
#include <map>

class ConfigurationManager
{
private:
    static std::string configFolder;

    static void loadConfigDir();


public:
    static std::map<std::string, sync_struct *> configuredSyncs;
    static std::map<std::string, sync_struct *> loadedSyncs;
    static std::string session;

    static void loadConfiguration(bool debug);
    static void loadsyncs();

    static void saveSyncs(std::map<std::string, sync_struct *> *syncsmap);

    static void saveSession(const char*session);
    static std::string getConfigFolder();

    static void unloadConfiguration();

};


#endif // CONFIGURATIONMANAGER_H
