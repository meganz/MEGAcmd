#ifndef PREFERENCES_H
#define PREFERENCES_H


const char CLIENT_KEY[] = "BdARkQSQ";
const char USER_AGENT[] = "MEGA/MEGAcmdUpdaterTask";

#ifdef _WIN32
const char UPDATE_CHECK_URL[]  = "http://g.static.mega.co.nz/upd/wcmd/v.txt";
const char EMERGENCY_UPDATE_CHECK_URL[]  = "http://g.static.mega.co.nz/eupd/wcmd/v.txt";
#else
const char UPDATE_CHECK_URL[]  = "http://g.static.mega.co.nz/upd/mcmd/v.txt";
const char EMERGENCY_UPDATE_CHECK_URL[]  = "http://g.static.mega.co.nz/eupd/mcmd/v.txt";

const char APP_DIR_BUNDLE[] = "/Applications/MEGAcmd.app/";
#endif

//TODO: update Keys
const char UPDATE_PUBLIC_KEY[] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXX";
const char UPDATE_FILENAME[] = "v.txt";
const char UPDATE_FOLDER_NAME[] = "eupdate";
const char BACKUP_FOLDER_NAME[] = "ebackup";
const char VERSION_FILE_NAME[] = "megacmd.version";
#endif // PREFERENCES_H
