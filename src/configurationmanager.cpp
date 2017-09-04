/**
 * @file examples/megacmd/configurationmanager.cpp
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

#include "configurationmanager.h"

#include <fstream>

#ifdef _WIN32
#include <shlobj.h> //SHGetFolderPath
#include <Shlwapi.h> //PathAppend
#else
#include <pwd.h>  //getpwuid_r
#endif

using namespace std;
using namespace mega;

bool is_file_exist(const char *fileName)
{
    std::ifstream infile(fileName);
    return infile.good();
}

string ConfigurationManager::configFolder;
map<string, sync_struct *> ConfigurationManager::configuredSyncs;
string ConfigurationManager::session;
map<string, sync_struct *> ConfigurationManager::loadedSyncs;


std::string ConfigurationManager::getConfigFolder()
{
    return configFolder;
}

void ConfigurationManager::loadConfigDir()
{
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

    string localConfigFolder;
    MegaFileSystemAccess *fsAccess = new MegaFileSystemAccess();
    fsAccess->setdefaultfolderpermissions(0700);
    fsAccess->path2local(&configFolder, &localConfigFolder);
    if (!is_file_exist(configFolder.c_str()) && !fsAccess->mkdirlocal(&localConfigFolder, true))
    {
        LOG_err << "Config folder not created";
    }
    delete fsAccess;
}


void ConfigurationManager::saveSession(const char*session)
{
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

void ConfigurationManager::saveSyncs(map<string, sync_struct *> *syncsmap)
{
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

void ConfigurationManager::unloadConfiguration()
{
    map<string, sync_struct *>::iterator itr;
    for (itr = configuredSyncs.begin(); itr != configuredSyncs.end(); )
    {
        sync_struct *thesync = ((sync_struct*)( *itr ).second );
        configuredSyncs.erase(itr++);
        delete thesync;
    }

    for (itr = loadedSyncs.begin(); itr != loadedSyncs.end(); )
    {
        sync_struct *thesync = ((sync_struct*)( *itr ).second );
        loadedSyncs.erase(itr++);
        delete thesync;
    }
}


void ConfigurationManager::loadsyncs()
{
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

            while (!( fi.peek() == EOF ))
            {
                sync_struct *thesync = new sync_struct;
                //Load syncs
                fi.read((char*)&thesync->fingerprint, sizeof( long long ));
                fi.read((char*)&thesync->handle, sizeof( MegaHandle ));
                size_t lengthLocalPath;
                fi.read((char*)&lengthLocalPath, sizeof( size_t ));
                thesync->localpath.resize(lengthLocalPath);
                fi.read((char*)thesync->localpath.c_str(), sizeof( char ) * lengthLocalPath);

                if (configuredSyncs.find(thesync->localpath) != configuredSyncs.end())
                {
                    delete configuredSyncs[thesync->localpath];
                }
                configuredSyncs[thesync->localpath] = thesync;
            }

            if (fi.bad())
            {
                LOG_err << "fail with sync file  at the end";
            }

            fi.close();
        }
    }
}

void ConfigurationManager::loadConfiguration(bool debug)
{
    stringstream sessionfile;
    if (!configFolder.size())
    {
        loadConfigDir();
    }
    if (configFolder.size())
    {
        sessionfile << configFolder << "/" << "session";

        if (debug)
            cout << "Session file: " << sessionfile.str() << endl;

        ifstream fi(sessionfile.str().c_str(), ios::in);
        if (fi.is_open())
        {
            string line;
            getline(fi, line);
            {
                session = line;
                if (debug)
                    cout << "Session read from configuration: " << line.substr(0, 5) << "..." << endl;
            }
            fi.close();
        }
    }
    else
    {
        if (debug)
            cout  << "Couldnt access configuration folder " << endl;
    }
}
