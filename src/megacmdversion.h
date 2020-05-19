#ifndef MEGACMDVERSION_H
#define MEGACMDVERSION_H

#ifndef MEGACMD_MAJOR_VERSION
#define MEGACMD_MAJOR_VERSION 1
#endif
#ifndef MEGACMD_MINOR_VERSION
#define MEGACMD_MINOR_VERSION 3
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
        "Fix --path-display-size for commands that use it and improve display for \"transfer\" & \"sync\"""\n"
        "Support for blocked accounts with instructions to unblock them""\n"
        "Fix crash in libcryptopp for Ubuntu 19.10 and onwards""\n"
        "Fix issues with failed transfers""\n"
        "Speed up sync engine startup for windows""\n"
        "Other fixes & adjustments"
        ;

}//end namespace
#endif // VERSION_H


