/**
 * @file src/configurationmanager.cpp
 * @brief MEGAcmd: configuration manager
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

#include "configurationmanager.h"
#include "megacmdversion.h"
#include "megacmdutils.h"
#include "listeners.h"
#include "updater/Preferences.h"
#include <fstream>

#ifdef _WIN32
#include <shlobj.h> //SHGetFolderPath
#include <Shlwapi.h> //PathAppend
#include <lzexpand.h>
#else
#include <pwd.h>  //getpwuid_r
#include <sys/file.h> //flock
#endif

#ifdef _WIN32
#define PATH_MAX_LOCAL_BACKUP MAX_PATH
#else
#define PATH_MAX_LOCAL_BACKUP PATH_MAX
#endif

#if defined(_WIN32)
#define unlink _unlink
#endif

using namespace mega;

namespace megacmd {
using namespace std;
bool is_file_exist(const char *fileName)
{
    std::ifstream infile(fileName);
    return infile.good();
}

string ConfigurationManager::configFolder;
bool ConfigurationManager::hasBeenUpdated = false;
#if !defined(_WIN32) && defined(LOCK_EX) && defined(LOCK_NB)
int ConfigurationManager::fd;
#endif
map<string, sync_struct *> ConfigurationManager::oldConfiguredSyncs;
string ConfigurationManager::session;
std::set<std::string> ConfigurationManager::excludedNames;
map<std::string, backup_struct *> ConfigurationManager::configuredBackups;
std::recursive_mutex ConfigurationManager::settingsMutex;

std::string ConfigurationManager::getConfigFolder()
{
    if (!configFolder.size())
    {
        ConfigurationManager::loadConfigDir();
    }
    return configFolder;
}

bool ConfigurationManager::getHasBeenUpdated()
{
    return hasBeenUpdated;
}

static const char* const persistentmcmdconfigurationkeys[] =
{
    "autoupdate", "updaterregistered"
};

void ConfigurationManager::loadConfigDir()
{
    std::lock_guard<std::recursive_mutex> g(settingsMutex);

#ifdef _WIN32

   TCHAR szPath[MAX_PATH];
    if (!SUCCEEDED(GetModuleFileName(NULL, szPath , MAX_PATH)))
    {
        LOG_fatal << "Couldnt get EXECUTABLE folder";
    }
    else
    {
        if (SUCCEEDED(PathRemoveFileSpec(szPath)))
        {
            if (PathAppend(szPath,TEXT(".megaCmd")))
            {
                MegaApi::utf16ToUtf8(szPath, lstrlen(szPath), &configFolder);
            }
        }
    }
#else
    const char *homedir = NULL;

    homedir = getenv("HOME");
    if (!homedir)
    {
        struct passwd pd;
        struct passwd* pwdptr = &pd;
        struct passwd* tempPwdPtr;
        char pwdbuffer[200];
        int pwdlinelen = sizeof( pwdbuffer );

        if (( getpwuid_r(22, pwdptr, pwdbuffer, pwdlinelen, &tempPwdPtr)) != 0)
        {
            LOG_fatal << "Couldnt get HOME folder";
            return;
        }
        else
        {
            homedir = pwdptr->pw_dir;
        }
    }
    stringstream sconfigDir;
    sconfigDir << homedir << "/" << ".megaCmd";
    configFolder = sconfigDir.str();
#endif

    MegaFileSystemAccess *fsAccess = new MegaFileSystemAccess();
    fsAccess->setdefaultfolderpermissions(0700);
    LocalPath localConfigFolder = LocalPath::fromAbsolutePath(configFolder);
    constexpr bool isHidden = true;
    constexpr bool reportExisting = false;
    if (!is_file_exist(configFolder.c_str()) && !fsAccess->mkdirlocal(localConfigFolder, isHidden, reportExisting))
    {
        LOG_err << "Config folder not created";
    }
    delete fsAccess;
}


void ConfigurationManager::saveSession(const char*session)
{
    std::lock_guard<std::recursive_mutex> g(settingsMutex);

    stringstream sessionfile;
    if (!configFolder.size())
    {
        loadConfigDir();
    }
    if (configFolder.size())
    {
        sessionfile << configFolder << "/" << "session";
        LOG_debug << "Session file: " << sessionfile.str();

        ofstream fo(sessionfile.str().c_str(), ios::out);

        if (fo.is_open())
        {
            if (session)
                fo << session;
            fo.close();
        }
    }
    else
    {
        LOG_err << "Couldnt access configuration folder ";
    }
}

void ConfigurationManager::saveProperty(const char *property, const char *value)
{
    std::lock_guard<std::recursive_mutex> g(settingsMutex);

    if (!configFolder.size())
    {
        loadConfigDir();
    }
    if (configFolder.size())
    {
        bool found = false;
        stringstream configFile;
        configFile << configFolder << "/" << "megacmd.cfg";

        stringstream formerlines;
        ifstream infile(configFile.str().c_str());
        string line;
        while (getline(infile, line))
        {
            if (line.length() > 0 && line[0] != '#')
            {
                string key;
                size_t pos = line.find("=");
                if (pos != string::npos)
                {
                    key = line.substr(0, pos);
                    rtrimProperty(key, ' ');

                    if (!strcmp(key.c_str(), property))
                    {
                        if (!found)
                        {
                            formerlines << property << "=" << value << endl;
                        }
                        found = true;
                    }
                    else
                    {
                        formerlines << line << endl;
                    }
                }
                else
                {
                    formerlines << line << endl;
                }
            }
        }
        ofstream fo(configFile.str().c_str());

        if (fo.is_open())
        {
            fo << formerlines.str();
            if (!found)
            {
                fo << property << "=" << value << endl;
            }
            fo.close();
        }
    }
}

void ConfigurationManager::migrateSyncConfig(MegaApi *api)
{
    bool informed = false;

    std::map<sync_struct*, std::unique_ptr<MegaCmdListener>> listeners;

    {
        std::lock_guard<std::recursive_mutex> g(settingsMutex);
        for (auto itr = oldConfiguredSyncs.begin();
             itr != oldConfiguredSyncs.end(); itr++)
        {
            if (!informed)
            {
                LOG_debug << "copying sync config";
                informed = true;
            }

            sync_struct *thesync = ((sync_struct*)( *itr ).second );

            auto newListenerPair = listeners.emplace(thesync, ::make_unique<MegaCmdListener>(api));

            api->copySyncDataToCache(thesync->localpath.c_str(), thesync->handle, nullptr,
                                     thesync->fingerprint, thesync->active, false, newListenerPair.first->second.get());
        }
    }

    for (auto &lPair : listeners)
    {
        auto &thesync = lPair.first;
        auto &listener = lPair.second;
        listener->wait();
        auto e = listener->getError();

        if (e && e->getErrorCode() == API_OK)
        {
            removeSyncConfig(thesync);
        }
        else
        {
            LOG_err << " fail to copy old sync cache data: " << thesync->localpath;
        }
    }

}

void ConfigurationManager::removeSyncConfig(sync_struct *syncToRemove)
{
    std::lock_guard<std::recursive_mutex> g(settingsMutex);
    for (map<string, sync_struct *>::iterator itr = oldConfiguredSyncs.begin();
         itr != oldConfiguredSyncs.end(); itr++)
    {
        sync_struct *thesync = ((sync_struct*)( *itr ).second );

        if (thesync == syncToRemove)
        {
            oldConfiguredSyncs.erase(itr);
            delete thesync;
            ConfigurationManager::saveSyncs(&ConfigurationManager::oldConfiguredSyncs);

            return;
        }
    }
}

void ConfigurationManager::saveSyncs(map<string, sync_struct *> *syncsmap)
{
    std::lock_guard<std::recursive_mutex> g(settingsMutex);

    stringstream syncsfile;
    if (!configFolder.size())
    {
        loadConfigDir();
    }
    if (configFolder.size())
    {
        syncsfile << configFolder << "/" << "syncs";
        LOG_debug << "Syncs file: " << syncsfile.str();

        ofstream fo(syncsfile.str().c_str(), ios::out | ios::binary);

        if (fo.is_open())
        {
            map<string, sync_struct *>::iterator itr;
            int i = 0;
            if (syncsmap)
            {
                for (itr = syncsmap->begin(); itr != syncsmap->end(); ++itr, i++)
                {
                    sync_struct *thesync = ((sync_struct*)( *itr ).second );

                    long long controlvalue = CONFIGURATIONSTOREDBYVERSION;
                    fo.write((char*)&controlvalue, sizeof( long long ));
                    int versionmcmd = MEGACMD_CODE_VERSION;
                    fo.write((char*)&versionmcmd, sizeof( int ));

                    fo.write((char*)&thesync->fingerprint, sizeof( long long ));
                    fo.write((char*)&thesync->handle, sizeof( MegaHandle ));
                    const char * localPath = thesync->localpath.c_str();
                    size_t lengthLocalPath = thesync->localpath.size();
                    fo.write((char*)&lengthLocalPath, sizeof( size_t ));
                    fo.write((char*)localPath, sizeof( char ) * lengthLocalPath);
                }
            }

            fo.close();
        }
    }
    else
    {
        LOG_err << "Couldnt access configuration folder ";
    }
}

void ConfigurationManager::saveBackups(map<string, backup_struct *> *backupsmap)
{
    std::lock_guard<std::recursive_mutex> g(settingsMutex);

    stringstream backupsfile;
    if (!configFolder.size())
    {
        loadConfigDir();
    }
    if (configFolder.size())
    {
        backupsfile << configFolder << "/" << "backups";
        LOG_debug << "Backups file: " << backupsfile.str();

        ofstream fo(backupsfile.str().c_str(), ios::out | ios::binary);

        if (fo.is_open())
        {
            map<string, backup_struct *>::iterator itr;
            int i = 0;
            if (backupsmap)
            {
                for (itr = backupsmap->begin(); itr != backupsmap->end(); ++itr, i++)
                {
                    backup_struct *thebackup = ((backup_struct*)( *itr ).second );

                    int versionmcmd = MEGACMD_CODE_VERSION;
                    fo.write((char*)&versionmcmd, sizeof( int ));

                    fo.write((char*)&thebackup->handle, sizeof( MegaHandle ));
                    const char * localPath = thebackup->localpath.c_str();
                    size_t lengthLocalPath = thebackup->localpath.size();
                    fo.write((char*)&lengthLocalPath, sizeof( size_t ));
                    fo.write((char*)localPath, sizeof( char ) * lengthLocalPath);

                    fo.write((char*)&thebackup->numBackups, sizeof( int ));
                    fo.write((char*)&thebackup->period, sizeof( int64_t ));

                    const char * speriod = thebackup->speriod.c_str();
                    size_t lengthLocalPeriod = thebackup->speriod.size();
                    fo.write((char*)&lengthLocalPeriod, sizeof( size_t ));
                    fo.write((char*)speriod, sizeof( char ) * lengthLocalPeriod);


                }
            }

            fo.close();
        }
    }
    else
    {
        LOG_err << "Couldnt access configuration folder ";
    }
}

void ConfigurationManager::addExcludedName(string excludedName)
{
    std::lock_guard<std::recursive_mutex> g(settingsMutex);
    LOG_verbose << "Adding: " << excludedName << " to exclusion list";
    excludedNames.insert(excludedName);
    saveExcludedNames();
}

void ConfigurationManager::removeExcludedName(string excludedName)
{
    std::lock_guard<std::recursive_mutex> g(settingsMutex);
    LOG_verbose << "Removing: " << excludedName << " from exclusion list";
    excludedNames.erase(excludedName);
    saveExcludedNames();
}

void ConfigurationManager::saveExcludedNames()
{
    std::lock_guard<std::recursive_mutex> g(settingsMutex);
    stringstream excludedNamesFile;
    if (!configFolder.size())
    {
        loadConfigDir();
    }
    if (configFolder.size())
    {
        excludedNamesFile << configFolder << "/" << "excluded";
        LOG_debug << "Exclusion file: " << excludedNamesFile.str();

        ofstream fo(excludedNamesFile.str().c_str(), ios::out | ios::binary);

        if (fo.is_open())
        {
            for (set<string>::iterator it=ConfigurationManager::excludedNames.begin(); it!=ConfigurationManager::excludedNames.end(); ++it)
            {
                fo << *it << endl;
            }
            fo.close();
        }
    }
    else
    {
        LOG_err << "Couldnt access configuration folder ";
    }
}

void ConfigurationManager::loadExcludedNames()
{
    std::lock_guard<std::recursive_mutex> g(settingsMutex);
    stringstream excludedNamesFile;
    if (!configFolder.size())
    {
        loadConfigDir();
    }
    if (configFolder.size())
    {
        excludedNamesFile << configFolder << "/" << "excluded";
        LOG_debug << "Excluded file: " << excludedNamesFile.str();

        if (!is_file_exist(excludedNamesFile.str().c_str()) && !oldConfiguredSyncs.size()) //do not add defaults if syncs already configured
        {
            excludedNames.insert(".*");
            excludedNames.insert("desktop.ini");
            excludedNames.insert("Thumbs.db");
            excludedNames.insert("~*");
            saveExcludedNames();
        }

        ifstream fi(excludedNamesFile.str().c_str(), ios::in | ios::binary);

        if (fi.is_open())
        {
            if (fi.fail())
            {
                LOG_err << "fail with sync file";
            }

            if (fi.bad())
            {
                LOG_err << "fail with sync file  at the end";
            }

            string excludedName;
            while (std::getline(fi, excludedName))
            {
                excludedNames.insert(excludedName);
            }

            fi.close();
        }
    }
}

void ConfigurationManager::unloadConfiguration()
{
    std::lock_guard<std::recursive_mutex> g(settingsMutex);

    for (map<string, sync_struct *>::iterator itr = oldConfiguredSyncs.begin(); itr != oldConfiguredSyncs.end(); )
    {
        sync_struct *thesync = ((sync_struct*)( *itr ).second );
        oldConfiguredSyncs.erase(itr++);
        delete thesync;
    }

    for (map<string, backup_struct *>::iterator itr = configuredBackups.begin(); itr != configuredBackups.end(); )
    {
        backup_struct *thebackup = ((backup_struct*)( *itr ).second );
        configuredBackups.erase(itr++);
        delete thebackup;
    }
    ConfigurationManager::session = string();
    ConfigurationManager::excludedNames.clear();
}

void ConfigurationManager::loadsyncs()
{
    std::lock_guard<std::recursive_mutex> g(settingsMutex);
    stringstream syncsfile;
    if (!configFolder.size())
    {
        loadConfigDir();
    }
    if (configFolder.size())
    {
        syncsfile << configFolder << "/" << "syncs";
        LOG_debug << "Syncs file: " << syncsfile.str();

        ifstream fi(syncsfile.str().c_str(), ios::in | ios::binary);

        if (fi.is_open())
        {
            if (fi.fail())
            {
                LOG_err << "fail with sync file";
            }

            bool updateSavedFormatRequired = false;
            while (!( fi.peek() == EOF ))
            {
                int versioncodeStoredValues;

                sync_struct *thesync = new sync_struct;
                //Load syncs
                fi.read((char*)&thesync->fingerprint, sizeof( long long ));
                if (thesync->fingerprint == CONFIGURATIONSTOREDBYVERSION)
                {
                    fi.read((char*)&versioncodeStoredValues, sizeof(int));
                }
                else
                {
                    versioncodeStoredValues = 90500;
                }

                if (versioncodeStoredValues > 90500)
                {
                    fi.read((char*)&thesync->fingerprint, sizeof( long long ));
                }

                fi.read((char*)&thesync->handle, sizeof( MegaHandle ));
                size_t lengthLocalPath;
                fi.read((char*)&lengthLocalPath, sizeof( size_t ));
                thesync->localpath.resize(lengthLocalPath);
                fi.read((char*)thesync->localpath.c_str(), sizeof( char ) * lengthLocalPath);

                if (oldConfiguredSyncs.find(thesync->localpath) != oldConfiguredSyncs.end())
                {
                    delete oldConfiguredSyncs[thesync->localpath];
                }
                oldConfiguredSyncs[thesync->localpath] = thesync;

                if (versioncodeStoredValues != MEGACMD_CODE_VERSION)
                {
                    updateSavedFormatRequired = true;
                }
            }
            if (updateSavedFormatRequired)
            {
                ConfigurationManager::saveSyncs(&ConfigurationManager::oldConfiguredSyncs);
            }

            if (fi.bad())
            {
                LOG_err << "fail with sync file  at the end";
            }

            fi.close();
        }
    }
}


void ConfigurationManager::loadbackups()
{
    std::lock_guard<std::recursive_mutex> g(settingsMutex);
    stringstream backupsfile;
    if (!configFolder.size())
    {
        loadConfigDir();
    }
    if (configFolder.size())
    {
        backupsfile << configFolder << "/" << "backups";
        LOG_debug << "Backups file: " << backupsfile.str();

        ifstream fi(backupsfile.str().c_str(), ios::in | ios::binary);

        if (fi.is_open())
        {
            if (fi.fail())
            {
                LOG_err << "fail with backup file";
            }

            while (!( fi.peek() == EOF ))
            {
                backup_struct *thebackup = new backup_struct;
                //Load backups
                int versionmcmd;
                fi.read((char*)&versionmcmd, sizeof( int ));

                fi.read((char*)&thebackup->handle, sizeof( MegaHandle ));
                size_t lengthLocalPath;
                fi.read((char*)&lengthLocalPath, sizeof( size_t ));
                if (lengthLocalPath && lengthLocalPath <= PATH_MAX_LOCAL_BACKUP)
                {
                    thebackup->localpath.resize(lengthLocalPath);
                    fi.read((char*)thebackup->localpath.c_str(), sizeof( char ) * lengthLocalPath);

                    fi.read((char*)&thebackup->numBackups, sizeof( int ));
                    fi.read((char*)&thebackup->period, sizeof( int64_t ));

                    size_t lengthLocalPeriod;
                    fi.read((char*)&lengthLocalPeriod, sizeof( size_t ));
                    if (lengthLocalPeriod && lengthLocalPeriod <= PATH_MAX_LOCAL_BACKUP)
                    {
                        thebackup->speriod.resize(lengthLocalPeriod);
                        fi.read((char*)thebackup->speriod.c_str(), sizeof( char ) * lengthLocalPeriod);

                    }
                    if (configuredBackups.find(thebackup->localpath) != configuredBackups.end())
                    {
                        delete configuredBackups[thebackup->localpath];
                    }

                    thebackup->id = -1; //id will be set upon resumption
                    thebackup->tag = -1; //tag will be set upon resumption

                    configuredBackups[thebackup->localpath] = thebackup;
                }
                else
                {
                    LOG_err << " Failed to restore backup info";
                }
            }

            if (fi.bad())
            {
                LOG_err << "fail with backup file  at the end";
            }

            fi.close();
        }
    }
}

void ConfigurationManager::loadConfiguration(bool debug)
{
    std::lock_guard<std::recursive_mutex> g(settingsMutex);

    if (!configFolder.size())
    {
        loadConfigDir();
    }
    if (configFolder.size())
    {
        //SESSION
        stringstream sessionfile;
        sessionfile << configFolder << "/" << "session";

        if (debug)
        {
            std::cout << "Session file: " << sessionfile.str() << std::endl;
        }

        ifstream fi(sessionfile.str().c_str(), ios::in);
        if (fi.is_open())
        {
            string line;
            getline(fi, line);
            {
                session = line;
                if (debug)
                {
                    std::cout << "Session read from configuration: " << line.substr(0, 5) << "..." << std::endl;
                }
            }
            fi.close();
        }


        //Check if version has been updated.
        stringstream versionionfile;
        versionionfile << configFolder << "/" << VERSION_FILE_NAME;

        // Get latest version if any.
        string latestVersion;
        ifstream fiVer(versionionfile.str().c_str());
        if (fiVer.is_open())
        {
            fiVer >> latestVersion;
            fiVer.close();
        }

        if (!latestVersion.empty() && (MEGACMD_CODE_VERSION > stol(latestVersion)))
        {
            hasBeenUpdated = true;
            if (debug)
            {
                std::cout << "MEGAcmd has been updated." << std::endl;
            }
        }

        // Store current version.
        ofstream fo(versionionfile.str().c_str(), ios::out);
        if (fo.is_open())
        {
            fo << MEGACMD_CODE_VERSION;
            fo.close();
        }
    }
    else
    {
        if (debug)
        {
            std::cout << "Couldnt access configuration folder " << std::endl;
        }
    }
}

bool ConfigurationManager::lockExecution()
{
    if (!configFolder.size())
    {
        loadConfigDir();
    }
    if (configFolder.size())
    {
        stringstream lockfile;
#ifdef _WIN32
        lockfile << "\\\\?\\";
        lockfile << configFolder << "\\" << "lockMCMD";
#else
        lockfile << configFolder << "/" << "lockMCMD";
#endif
        LOG_err << "Lock file: " << lockfile.str();

#ifdef _WIN32
        string wlockfile;
        MegaApi::utf8ToUtf16(lockfile.str().c_str(), &wlockfile);
        if (CreateFileW((LPCWSTR)(wlockfile).data(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL) == INVALID_HANDLE_VALUE)
        {
            return false;
        }
#elif defined(LOCK_EX) && defined(LOCK_NB)
        fd = open(lockfile.str().c_str(), O_RDWR | O_CREAT, 0666); // open or create lockfile
        //check open success...
        if (flock(fd, LOCK_EX | LOCK_NB))
        {
            return false;
        }

        if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)
        {
            LOG_err << "ERROR setting CLOEXEC to lock fd: " << errno;
        }

#else
        ifstream fi(thelockfile.c_str());
        if(!fi.fail()){
            return false;
        }
        if (fi.is_open())
        {
            fi.close();
        }
        ofstream fo(thelockfile.c_str());
        if (fo.is_open())
        {
            fo.close();
        }
#endif

        return true;
    }
    else
    {
        LOG_err  << "Couldnt access configuration folder ";
    }
    return false;
}

void ConfigurationManager::unlockExecution()
{
    if (!configFolder.size())
    {
        loadConfigDir();
    }
    if (configFolder.size())
    {
        stringstream lockfile;
        lockfile << configFolder << "/" << "lockMCMD";

#if !defined(_WIN32) && defined(LOCK_EX) && defined(LOCK_NB)
        flock(fd, LOCK_UN | LOCK_NB);
        close(fd);
#endif
        unlink(lockfile.str().c_str());
    }
    else
    {
        LOG_err  << "Couldnt access configuration folder ";
    }
}
string ConfigurationManager::getConfigurationSValue(string propertyName)
{
    std::lock_guard<std::recursive_mutex> g(settingsMutex);

    if (!configFolder.size())
    {
        loadConfigDir();
    }
    if (configFolder.size())
    {
        stringstream configFile;
        configFile << configFolder << "/" << "megacmd.cfg";
        return getPropertyFromFile(configFile.str().c_str(),propertyName.c_str());
    }
    return string();
}

void ConfigurationManager::clearConfigurationFile()
{
    std::lock_guard<std::recursive_mutex> g(settingsMutex);

    if (!configFolder.size())
    {
        loadConfigDir();
    }
    if (configFolder.size())
    {
        stringstream configFile;
        configFile << configFolder << "/" << "megacmd.cfg";

        stringstream formerlines;
        ifstream infile(configFile.str().c_str());
        string line;
        while (getline(infile, line))
        {
            size_t pos;
            if (line.length() > 0 && line[0] != '#' && (pos = line.find("=")) != string::npos)
            {
                string key;
                key = line.substr(0, pos);
                rtrimProperty(key, ' ');

                for (unsigned int i = 0; i < sizeof(persistentmcmdconfigurationkeys)/sizeof(persistentmcmdconfigurationkeys[0]); i++)
                {
                    if (!strcmp(key.c_str(), persistentmcmdconfigurationkeys[i]))
                    {
                        formerlines << line << endl;
                    }
                }
            }
            else
            {
                formerlines << line << endl;
            }
        }
        ofstream fo(configFile.str().c_str());

        if (fo.is_open())
        {
            fo << formerlines.str();
            fo.close();
        }
    }
}
}//end namespace
