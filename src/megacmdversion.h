#ifndef MEGACMDVERSION_H
#define MEGACMDVERSION_H

#ifndef MEGACMD_MAJOR_VERSION
#define MEGACMD_MAJOR_VERSION 0
#endif
#ifndef MEGACMD_MINOR_VERSION
#define MEGACMD_MINOR_VERSION 9
#endif
#ifndef MEGACMD_MICRO_VERSION
#define MEGACMD_MICRO_VERSION 8
#endif

#ifndef MEGACMD_CODE_VERSION
#define MEGACMD_CODE_VERSION (MEGACMD_MICRO_VERSION*100+MEGACMD_MINOR_VERSION*10000+MEGACMD_MAJOR_VERSION*1000000)
#endif

const char * const megacmdchangelog =
        "Backups: added the possibility to configure periodic backups""\n"
        "Full version support: listing, see space ocupied, accessing, copying, removing, ...""\n"
        "Added \"deleteversions\" command to remove versions (all or by path)""\n"
        "Permissions: you can change default permission for files and folders""\n"
        "Persistence of settings: speedlimit, permissions, https""\n"
        "Speedlimit takes units and allows human-readable output""\n"
        "Support for video metadata""\n"
        "Minor fixes and doc improvements"
        ;


#endif // VERSION_H


