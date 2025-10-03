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

#include "megacmdcommonutils.h"
#include "configurationmanager.h"
#include "megacmdversion.h"
#include "megacmdutils.h"
#include "listeners.h"
#include "updater/Preferences.h"
#include "sync_ignore.h"

#include <fstream>

#ifndef ERRNO
#ifdef _WIN32
#include <windows.h>
#define ERRNO WSAGetLastError()
#else
#define ERRNO errno
#endif
#endif


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

using namespace mega;
using namespace std;

namespace megacmd {

static const char* const LOCK_FILE_NAME = "lockMCMD";

#ifdef WIN32
HANDLE ConfigurationManager::mLockFileHandle;
#elif defined(LOCK_EX) && defined(LOCK_NB)
int ConfigurationManager::fd;
#endif
map<string, sync_struct *> ConfigurationManager::oldConfiguredSyncs;
string ConfigurationManager::session;
map<std::string, backup_struct *> ConfigurationManager::configuredBackups;
std::recursive_mutex ConfigurationManager::settingsMutex;

fs::path ConfigurationManager::getConfigFolder()
{
    if (mConfigFolder.empty())
    {
        ConfigurationManager::loadConfigDir();
    }
    return mConfigFolder;
}

bool ConfigurationManager::getHasBeenUpdated()
{
    return hasBeenUpdated;
}

static const char* const persistentmcmdconfigurationkeys[] =
{
    "autoupdate", "updaterregistered"
};

void ConfigurationManager::createFolderIfNotExisting(const fs::path &folder)
{
    std::error_code ec;
    auto createdOk = fs::create_directory(folder, ec);
    bool alreadyExisting = !createdOk && !ec;
    if (alreadyExisting)
    {
        return;
    }

    LOG_debug << "Creating directory " << folder;

    if (!createdOk)
    {
        LOG_err << "Directory " << folder << " creation failed: " << errorCodeStr(ec);
        return;
    }

#ifndef WIN32
    fs::permissions(folder, fs::perms::owner_all, fs::perm_options::replace, ec);
    if (!ec)
    {
        LOG_warn << "Failed to set permissions on new folder " << folder << ": " << errorCodeStr(ec);
    }
#endif
}

void ConfigurationManager::loadConfigDir()
{
    auto dirs = PlatformDirectories::getPlatformSpecificDirectories();
    std::lock_guard<std::recursive_mutex> g(settingsMutex);
    mConfigFolder = dirs->configDirPath();
    if (mConfigFolder.empty())
    {
        LOG_fatal << "Could not get config directory path";
        return;
    }

    createFolderIfNotExisting(mConfigFolder);
}

fs::path ConfigurationManager::getAndCreateConfigDir()
{
    auto dirs = PlatformDirectories::getPlatformSpecificDirectories();
    const fs::path configDir = dirs->configDirPath();
    if (configDir.empty())
    {
        LOG_fatal << "Could not get config directory path";
        throw std::runtime_error("Could not get config directory path");
    }

    createFolderIfNotExisting(configDir);
    return configDir;
}

fs::path ConfigurationManager::getAndCreateRuntimeDir()
{
    auto dirs = PlatformDirectories::getPlatformSpecificDirectories();
    const fs::path runtimeDir = dirs->runtimeDirPath();
    if (runtimeDir.empty())
    {
        LOG_fatal << "Could not get runtime directory path";
        throw std::runtime_error("Could not get runtime directory path");
    }

    createFolderIfNotExisting(runtimeDir);
    return runtimeDir;
}

fs::path ConfigurationManager::getConfigFolderSubdir(const fs::path& subdirName)
{
    auto dirs = PlatformDirectories::getPlatformSpecificDirectories();
    const fs::path configDir = dirs->configDirPath();
    assert(!configDir.empty());

    fs::path configSubDir = configDir / subdirName;

    createFolderIfNotExisting(configSubDir);

    return configSubDir;
}

void ConfigurationManager::saveSession(const char*session)
{
    std::lock_guard<std::recursive_mutex> g(settingsMutex);
    auto configDir = getAndCreateConfigDir();
    assert(!configDir.empty());

    const fs::path sessionFilePath = configDir / "session";
    LOG_debug << "Session file: " << sessionFilePath;

    ofstream fo(sessionFilePath, ios::out);
    if (fo.is_open())
    {
        if (session)
        {
            fo << session;
        }
        fo.close();
    }
}

string ConfigurationManager::saveProperty(const char *property, const char *value)
{
    std::lock_guard<std::recursive_mutex> g(settingsMutex);

    if (mConfigFolder.empty())
    {
        loadConfigDir();
    }
    assert(!mConfigFolder.empty());

    bool found = false;
    const fs::path configFilePath = mConfigFolder / "megacmd.cfg";

    stringstream formerlines;

    ifstream infile(configFilePath);
    string line, prevValue;
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

                if (!strcmp(key.c_str(), property) && !found)
                {
                    formerlines << property << "=" << value << endl;
                    prevValue = line.substr(pos + 1);
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

    ofstream fo(configFilePath);
    if (fo.is_open())
    {
        fo << formerlines.str();
        if (!found)
        {
            fo << property << "=" << value << endl;
        }
        fo.close();
    }
    return prevValue;
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

            auto newListenerPair = listeners.emplace(thesync, std::make_unique<MegaCmdListener>(api));

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

    if (mConfigFolder.empty())
    {
        loadConfigDir();
    }
    if (!mConfigFolder.empty())
    {
        const fs::path syncsFilePath = mConfigFolder / "syncs";
        LOG_debug << "Syncs file: " << syncsFilePath;

        ofstream fo(syncsFilePath, ios::out | ios::binary);

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

    if (mConfigFolder.empty())
    {
        loadConfigDir();
    }
    if (!mConfigFolder.empty())
    {
        const fs::path backupsFilePath = mConfigFolder / "backups";
        LOG_debug << "Backups file: " << backupsFilePath;

        ofstream fo(backupsFilePath, ios::out | ios::binary);

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

void ConfigurationManager::transitionLegacyExclusionRules(MegaApi& api)
{
    // Note that using `MegaApi::exportLegacyExclusionRules` here won't simplify much.
    // Since we still need to manually load all legacy rules into a vector, call `setLegacyExcludedNames`,
    // then call `exportLegacyExclusionRules` to generate a .megaignore file, and finally move it manually
    // to our location and add the .default prefix.
    // Since we need to have all the custom mega ignore functionality for the `mega-ignore` command anyway,
    // and since we also need to manually read the legacy file, it's just easier to rely on our custom
    // method to generate the default file.

    const fs::path defaultMegaIgnorePath = MegaIgnoreFile::getDefaultPath();
    if (fs::exists(defaultMegaIgnorePath))
    {
        LOG_debug << "Default .megaignore file already exists. Skipping legacy transition";
        return;
    }

    const fs::path excludeFilePath = mConfigFolder / "excluded";
    const fs::path hiddenExcludeFilePath = mConfigFolder / ".excluded";
    if (!fs::exists(excludeFilePath))
    {
        LOG_debug << "Missing legacy exclude file. Skipping transition";
        return;
    }

    LOG_debug << "Transitioning legacy exclusion rules from " << excludeFilePath << " to " << defaultMegaIgnorePath;

    std::ifstream excludeFile(excludeFilePath);
    if (!excludeFile.is_open() || excludeFile.fail())
    {
        LOG_err << "There was an error opening legacy exclude file " << excludeFilePath;
        return;
    }

    MegaIgnoreFile megaIgnoreFile(defaultMegaIgnorePath);
    if (!megaIgnoreFile.isValid())
    {
        LOG_err << "There was an error opening default .megaignore file " << defaultMegaIgnorePath;
        return;
    }

    sendEvent(StatsManager::MegacmdEvent::TRANSITIONING_PRE_SRW_EXCLUSIONS, &api, false);

    std::set<string> excludeFilters;
    std::vector<string> excludePatterns;
    for (std::string line; getline(excludeFile, line);)
    {
        if (line.empty())
        {
            continue;
        }
        excludePatterns.push_back(line);

        string filter = SyncIgnore::getFilterFromLegacyPattern(line);
        if (!MegaIgnoreFile::isValidFilter(filter))
        {
            LOG_warn << "Found invalid pattern \"" << line << "\" in legacy exclude file";
            continue;
        }
        excludeFilters.insert(filter);
    }

    // This ensures transition works for active syncs
    api.setLegacyExcludedNames(&excludePatterns);

    // This ensures transition works in case there are no existing syncs
    megaIgnoreFile.addFilters(excludeFilters);

    LOG_debug << "Transition of legacy exclusion rules completed successfully. Hiding legacy exclude file";
    try
    {
        fs::rename(excludeFilePath, hiddenExcludeFilePath);
    }
    catch (const fs::filesystem_error& e)
    {
        LOG_err << "Could not hide legacy exclude file " << excludeFilePath << " (error: " << e.what() << ")";
    }

    string message = "Your legacy sync exclusion rules have been ported to \"" +
                     pathAsUtf8(defaultMegaIgnorePath) + "\"\n" +
                     "See \"%mega-%sync-ignore\" for more info.";
    broadcastMessage(message, true);
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
}

void ConfigurationManager::loadsyncs()
{
    std::lock_guard<std::recursive_mutex> g(settingsMutex);
    if (mConfigFolder.empty())
    {
        loadConfigDir();
    }
    if (!mConfigFolder.empty())
    {
        const fs::path syncsFilePath = mConfigFolder / "syncs";
        LOG_debug << "Syncs file: " << syncsFilePath;

        ifstream fi(syncsFilePath, ios::in | ios::binary);

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
    if (mConfigFolder.empty())
    {
        loadConfigDir();
    }
    if (!mConfigFolder.empty())
    {
        const fs::path backupsFilePath = mConfigFolder / "backups";
        LOG_debug << "Backups file: " << backupsFilePath;

        ifstream fi(backupsFilePath, ios::in | ios::binary);

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

#ifdef _WIN32
    WindowsUtf8StdoutGuard utf8Guard;
#endif

    // SESSION
    const fs::path configDir = getAndCreateConfigDir();
    if (!configDir.empty())
    {
        const fs::path sessionFilePath = configDir / "session";
        if (debug)
        {
            COUT << "Session file: " << sessionFilePath << std::endl;
        }

        ifstream fi(sessionFilePath, ios::in);
        if (fi.is_open())
        {
            string line;
            getline(fi, line);
            {
                session = line;
                if (debug)
                {
                    COUT << "Session read from configuration: " << line.substr(0, 5) << "..." << std::endl;
                }
            }
            fi.close();
        }

        // Check if version has been updated.
        if (mConfigFolder.empty())
        {
            loadConfigDir();
        }
        const fs::path versionFilePath = mConfigFolder / VERSION_FILE_NAME;

        // Get latest version if any.
        string latestVersion;
        ifstream fiVer(versionFilePath);
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
                COUT << "MEGAcmd has been updated." << std::endl;
            }
        }

        // Store current version.
        ofstream fo(versionFilePath, ios::out);
        if (fo.is_open())
        {
            fo << MEGACMD_CODE_VERSION;
            fo.close();
        }
        else
        {
            CERR << "Could not write MEGAcmd version to " << versionFilePath << std::endl;
        }
    }
    else
    {
        if (debug)
        {
            COUT << "Couldnt access data folder " << std::endl;
        }
    }
}

bool ConfigurationManager::lockExecution()
{
    // Check for legacy lock (in config path)
    auto dirs = PlatformDirectories::getPlatformSpecificDirectories();
    auto configDir = dirs->configDirPath();
    auto runtimeDir = getAndCreateRuntimeDir();
    if (runtimeDir != configDir)
    {
        const fs::path lockFileAtConfigDir = configDir / LOCK_FILE_NAME;
        if (fs::is_regular_file(lockFileAtConfigDir))
        {
            LOG_warn << "Found a lock file from a previous MEGAcmd version. Will try to acquire the lock and remove it";
            bool legacyLockResult = lockExecution(configDir);
            if (!legacyLockResult)
            {
                LOG_err << "Failed to acquire lock from a previous version. You may need to ensure there is "
                           "no MEGAcmd Server running and try again or manually remove lock file: " <<  lockFileAtConfigDir;
                return false;
            }
            if (!unlockExecution(configDir))
            {
                LOG_err << "Failed to release lock from a previous version. You may need to ensure there is "
                           "no MEGAcmd Server running and try again or manually remove lock file: " <<  lockFileAtConfigDir;
                return false;
            }
        }
    }

    return lockExecution(runtimeDir);
}

bool ConfigurationManager::lockExecution(const fs::path &lockFileFolder)
{
    assert(!lockFileFolder.empty());

    const fs::path lockFilePath = lockFileFolder / LOCK_FILE_NAME;
    LOG_info << "Lock file: " << lockFilePath;

#ifdef _WIN32
    mLockFileHandle = CreateFileW(lockFilePath.wstring().c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (mLockFileHandle == INVALID_HANDLE_VALUE)
    {
        LOG_err << "ERROR creating lock file: " << ERRNO;

        return false;
    }
#elif defined(LOCK_EX) && defined(LOCK_NB)
    fd = open(lockFilePath.string().c_str(), O_RDWR | O_CREAT, 0666); // open or create lockfile
    //check open success...
    if (flock(fd, LOCK_EX | LOCK_NB))
    {
        LOG_err << "ERROR locking lock fd: " << errno;
        close(fd);
        return false;
    }

    if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)
    {
        LOG_err << "ERROR setting CLOEXEC to lock fd: " << errno;
    }

#else
    ifstream fi(lockFilePath);
    if(!fi.fail())
    {
        close(fd);
        return false;
    }
    if (fi.is_open())
    {
        fi.close();
    }
    ofstream fo(lockFilePath);
    if (fo.is_open())
    {
        fo.close();
    }
#endif

    return true;
}

bool ConfigurationManager::unlockExecution()
{
    return unlockExecution(getAndCreateRuntimeDir());
}

bool ConfigurationManager::unlockExecution(const fs::path &lockFileFolder)
{
    assert(!lockFileFolder.empty());

#ifdef _WIN32
    CloseHandle(mLockFileHandle);
#elif defined(LOCK_EX) && defined(LOCK_NB)
    flock(fd, LOCK_UN | LOCK_NB);
    close(fd);
#endif

    const fs::path lockFilePath = lockFileFolder / LOCK_FILE_NAME;
    if (std::error_code ec; !fs::remove(lockFilePath, ec))
    {
        LOG_err << "Failed to remove lock file, error = " << errorCodeStr(ec);
        return false;
    }
    return true;
}

string ConfigurationManager::getConfigurationSValue(string propertyName)
{
    std::lock_guard<std::recursive_mutex> g(settingsMutex);

    if (mConfigFolder.empty())
    {
        loadConfigDir();
    }
    if (!mConfigFolder.empty())
    {
        const fs::path configFilePath = mConfigFolder / "megacmd.cfg";
        return getPropertyFromFile(configFilePath, propertyName.c_str());
    }
    return "";
}

void ConfigurationManager::clearConfigurationFile()
{
    std::lock_guard<std::recursive_mutex> g(settingsMutex);

    if (mConfigFolder.empty())
    {
        loadConfigDir();
    }
    if (!mConfigFolder.empty())
    {
        const fs::path configFilePath = mConfigFolder / "megacmd.cfg";
        ifstream infile(configFilePath);

        stringstream formerlines;
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

        ofstream fo(configFilePath);
        if (fo.is_open())
        {
            fo << formerlines.str();
            fo.close();
        }
    }
}

ConfiguratorMegaApiHelper::ConfiguratorMegaApiHelper()
{
    auto configSetterSyncULLCb = [this](auto cb)
    {
        return [this, cb](MegaApi *api, const std::string &/*name*/, const char *value)
        {
            try
            {
                return cb(api, std::stoull(value));
            }
            catch(...)
            {
                return false;
            }
            return true;
        };
    };

    auto confGetter = [](::mega::MegaApi */*api*/,const char *key){ return  ConfigurationManager::getConfigurationValueOpt<std::string>(key); };

    auto validatorULL = [](std::optional<unsigned long long> minOpt = {}, std::optional<unsigned long long> maxOpt = {})
    {
        return [minOpt, maxOpt](const char *value){
            if (value && value[0] == '-')
            {
                return false;
            }

            try
            {
                auto v = std::stoull(value);
                if (maxOpt && v > *maxOpt)
                {
                    return false;
                }
                if (minOpt && v < *minOpt)
                {
                    return false;
                }
                return true;
            }
            catch (...)
            {
                return false;
            }
        };
    };

    mConfigurators.emplace_back("max_nodes_in_cache", "Max nodes loaded in memory",
                                "This controls the number of nodes that the SDK stores in memory.",
                                configSetterSyncULLCb([](MegaApi *api, auto value){ api->setLRUCacheSize(value); return true; }),
                                confGetter,
                                std::nullopt/*megaApiGetter*/,
                                validatorULL());

    mConfigurators.emplace_back("exported_folders_sdks", "Number of additional SDK instances loaded at startup",
                                "This controls the number of SDK instances that are created at startup in order to download or import contents from exported folder links. "
                                "Default 5. Min 0. Max 20. If set to 0, you will not be able to download or import from folder links.",
                                configSetterSyncULLCb([](MegaApi *api, auto value){ return true;/*TODO: implement reactiveness to value changes*/ }),
                                confGetter,
                                std::nullopt/*megaApiGetter*/,
                                validatorULL(0, 20));
}

const std::vector<ConfiguratorMegaApiHelper::ValueConfigurator> & ConfiguratorMegaApiHelper::getConfigurators()
{
    return mConfigurators;
}

}//end namespace
