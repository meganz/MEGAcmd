#ifndef MEGACMDVERSION_H
#define MEGACMDVERSION_H

#ifndef MEGACMD_MAJOR_VERSION
#define MEGACMD_MAJOR_VERSION 1
#endif
#ifndef MEGACMD_MINOR_VERSION
#define MEGACMD_MINOR_VERSION 4
#endif
#ifndef MEGACMD_MICRO_VERSION
#define MEGACMD_MICRO_VERSION 0
#endif

#ifndef MEGACMD_BUILD_ID
#define MEGACMD_BUILD_ID 0
#endif

#ifndef MEGACMD_CODE_VERSION
#define MEGACMD_CODE_VERSION (MEGACMD_BUILD_ID+MEGACMD_MICRO_VERSION*100+MEGACMD_MINOR_VERSION*10000+MEGACMD_MAJOR_VERSION*1000000)
#endif
namespace megacmd {

const char * const megacmdchangelog =
        "Fix issues with backups timestamps in Raspbian and other OS""\n"
        "Allow uploads when reached bandwidth overquota""\n"
        "Fix for syncing mounted drives in Windows""\n"
        "Fix issues in sync resumption for MacOS""\n"
        "Improvements in filename escaping""\n"
        "Improvements in transfers cancellation""\n"
        "Improve uploads stalling in Windows""\n"
        "Fix crash in MEGAcmd shell when typing CTRL+D while inserting 2FA code""\n"
        "Warn incoming Windows XP deprecation""\n"
        "Improvements in information messages""\n"
        "Fix issues with too long paths in MEGAcmd server""\n"
        "Prepare for over storage quota announcements""\n"
        "Other fixes & adjustments"
        ;

}//end namespace
#endif // VERSION_H

