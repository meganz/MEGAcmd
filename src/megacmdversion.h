#ifndef MEGACMDVERSION_H
#define MEGACMDVERSION_H

#ifndef MEGACMD_MAJOR_VERSION
#define MEGACMD_MAJOR_VERSION 1
#endif
#ifndef MEGACMD_MINOR_VERSION
#define MEGACMD_MINOR_VERSION 0
#endif
#ifndef MEGACMD_MICRO_VERSION
#define MEGACMD_MICRO_VERSION 0
#endif

#ifndef MEGACMD_CODE_VERSION
#define MEGACMD_CODE_VERSION (MEGACMD_MICRO_VERSION*100+MEGACMD_MINOR_VERSION*10000+MEGACMD_MAJOR_VERSION*1000000)
#endif

const char * const megacmdchangelog =
        "added FTP support (See \"ftp\")""\n"
        "renew path parsing & improved completion with special characters""\n"
        "fix truncated redirected output in MacOS""\n"
        "added support for account cancelation(See \"cancel\")""\n"
        "cp now allows multiple sourth paths and regular expresions""\n"
        "du path display size variable now""\n"
        "output error code always positive now""\n"
        "new command \"errorcode\" to translate error code into string""\n"
        "allow password protected links for PRO users""\n"
        "password changing no longer requires old one""\n"
        "webdav now allows stop serving all locations""\n"
        "added \"graphisc\" command to turn off thumbnails/previews generation""\n"
        "support login and password change using 2FA""\n"
        "limit one instance of server""\n"
        "Many more minor fixes & adjustements"
        ;


#endif // VERSION_H


