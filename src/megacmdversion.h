#ifndef MEGACMDVERSION_H
#define MEGACMDVERSION_H

#ifndef MEGACMD_MAJOR_VERSION
#define MEGACMD_MAJOR_VERSION 0
#endif
#ifndef MEGACMD_MINOR_VERSION
#define MEGACMD_MINOR_VERSION 9
#endif
#ifndef MEGACMD_MICRO_VERSION
#define MEGACMD_MICRO_VERSION 6
#endif

#ifndef MEGACMD_CODE_VERSION
#define MEGACMD_CODE_VERSION (MEGACMD_MICRO_VERSION*100+MEGACMD_MINOR_VERSION*10000+MEGACMD_MAJOR_VERSION*1000000)
#endif

const char * const megacmdchangelog =
        "Added transfer resumption""\n"
        "Added file versioning for modified files(webclient can browse them)""\n"
        "Added time and size constrains for find""\n"
        "Reformated sync output""\n"
        "Added exclusions to syncs. Default: .* ~* desktop.ini Thumbs.db""\n"
        "Fixed detection of invalid TIMEVAL when no unit specified""\n"
        "Improved sync display format""\n"
        "Fix some \"get\" cases with \"/\" involved in Windows""\n"
        "Fix in email validation""\n"
        "Fix segfault in userattr""\n"
        "Added \"--in\" & \"--out\" to showpcr""\n"
        "Added masterkey command to show master key""\n"
        "All/None options in confirmation for deletion""\n"
        "Added multi-transfer progress (one single progress bar)""\n"
        "Fix mkdir loop""\n"
        "Minor fixes and improvements in error management""\n"
        "Changed name for server in windows to MEGAcmdServer.exe"
        ;


#endif // VERSION_H


