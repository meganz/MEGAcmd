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

#ifndef _WIN32
#include <sys/file.h> // LOCK_EX and LOCK_NB
#endif

#define CONFIGURATIONSTOREDBYVERSION -2
class ConfigurationManager
{
private:
    static std::string configFolder;
#if !defined(_WIN32) && defined(LOCK_EX) && defined(LOCK_NB)
    static int fd;
#endif

    static void loadConfigDir();


public:
    static std::map<std::string, sync_struct *> configuredSyncs;
    static std::map<std::string, backup_struct *> configuredBackups;

    static std::string session;

    static std::set<std::string> excludedNames;

    static void loadConfiguration(bool debug);
    static void clearConfigurationFile();
    static bool lockExecution();
    static void unlockExecution();

    static void loadsyncs();
    static void loadbackups();

    static void saveSyncs(std::map<std::string, sync_struct *> *syncsmap);
    static void saveBackups(std::map<std::string, backup_struct *> *backupsmap);

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

    template<typename T>
    static void savePropertyValueList(const char* property, std::list<T> value, char separator = ';')
    {
        std::ostringstream os;
        typename std::list<T>::iterator it = value.begin();
        for (; it != value.end(); ++it){
            os << (*it);

            if ( std::distance( it, value.end() ) != 1 ) // not last
            {
                os << separator;
            }
        }
        saveProperty(property,os.str().c_str());
    }

    template<typename T>
    static void savePropertyValueSet(const char* property, std::set<T> value, char separator = ';')
    {
        std::ostringstream os;
        typename std::set<T>::iterator it = value.begin();
        for (; it != value.end(); ++it){
            os << (*it);

            if ( std::distance( it, value.end() ) != 1 ) // not last
            {
                os << separator;
            }
        }
        saveProperty(property,os.str().c_str());
    }

    static std::string getConfigurationSValue(std::string propertyName);
    template <typename T>
    static T getConfigurationValue(std::string propertyName, T defaultValue)
    {
        std::string propValue = getConfigurationSValue(propertyName);
        if (!propValue.size()) return defaultValue;

        T i;
        std::istringstream is(propValue);
        is >> i;
        return i;
    }

    template <typename T>
    static std::list<T> getConfigurationValueList(std::string propertyName, char separator = ';')
    {
        std::list<T> toret;

        std::string propValue = getConfigurationSValue(propertyName);
        if (!propValue.size())
        {
            return toret;
        }
        size_t possep;
        do {
            possep = propValue.find(separator);

            std::string current = propValue.substr(0,possep);

            if (possep != std::string::npos && ((possep + 1 ) != propValue.size()))
            {
                propValue = propValue.substr(possep + 1);
            }
            else
            {
                possep = std::string::npos;
            }

            if (current.size())
            {
                T i;
                std::istringstream is(current);
                is >> i;
                toret.push_back(i);
            }
        } while (possep != std::string::npos);

        return toret;
    }

    static std::list<std::string> getConfigurationValueList(std::string propertyName, char separator = ';')
    {
        std::list<std::string> toret;

        std::string propValue = getConfigurationSValue(propertyName);
        if (!propValue.size())
        {
            return toret;
        }
        size_t possep;
        do {
            possep = propValue.find(separator);

            std::string current = propValue.substr(0,possep);

            if (possep != std::string::npos && ((possep + 1 ) != propValue.size()))
            {
                propValue = propValue.substr(possep + 1);
            }
            else
            {
                possep = std::string::npos;
            }

            if (current.size())
            {
                toret.push_back(current);
            }
        } while (possep != std::string::npos);

        return toret;
    }

    template <typename T>
    static std::set<T> getConfigurationValueSet(std::string propertyName, char separator = ';')
    {
        std::set<T> toret;

        std::string propValue = getConfigurationSValue(propertyName);
        if (!propValue.size())
        {
            return toret;
        }
        size_t possep;
        do {
            possep = propValue.find(separator);

            std::string current = propValue.substr(0,possep);

            if (possep != std::string::npos && ((possep + 1 ) != propValue.size()))
            {
                propValue = propValue.substr(possep + 1);
            }
            else
            {
                possep = std::string::npos;
            }

            if (current.size())
            {
                T i;
                std::istringstream is(current);
                is >> i;
                toret.insert(i);
            }
        } while (possep != std::string::npos);

        return toret;
    }

    static std::string getConfigFolder();

    static void unloadConfiguration();

};


#endif // CONFIGURATIONMANAGER_H
