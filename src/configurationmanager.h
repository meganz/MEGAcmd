/**
 * @file src/configurationmanager.h
 * @brief MEGAcmd: configuration manager
 *
 * (c) 2013-2016 by Mega Limited, Auckland, New Zealand
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

#ifndef CONFIGURATIONMANAGER_H
#define CONFIGURATIONMANAGER_H

#include "megacmd.h"
#include <map>
#include <set>

#define CONFIGURATIONSTOREDBYVERSION -2
class ConfigurationManager
{
private:
    static std::string configFolder;

    static void loadConfigDir();


public:
    static std::map<std::string, sync_struct *> configuredSyncs;
    static std::string session;

    static long long maxspeedupload;
    static long long maxspeeddownload;
#ifndef _WIN32
    static std::string permissionsFiles;
    static std::string permissionsFolders;
#endif


    static std::set<std::string> excludedNames;

    static void loadConfiguration(bool debug);
    static void clearConfigurationFile();
    static void loadsyncs();

    static void saveSyncs(std::map<std::string, sync_struct *> *syncsmap);

    static void addExcludedName(std::string excludedName);
    static void removeExcludedName(std::string excludedName);
    static void saveExcludedNames();
    /**
     * @brief loadExcludedNames
     * if called for the first time, it will add default excluded names if no sync has been loaded previously
     */
    static void loadExcludedNames();

    static void saveSession(const char*session);

    static void saveProperty(const char* property, const char* value);

    template<typename T>
    static void savePropertyValue(const char* property, T value)
    {
        std::ostringstream os;
        os << value;
        saveProperty(property,os.str().c_str());
    }

    static std::string getConfigFolder();

    static void unloadConfiguration();

};


#endif // CONFIGURATIONMANAGER_H
