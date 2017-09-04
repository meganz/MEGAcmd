#ifndef MEGACMDVERSION_H
#define MEGACMDVERSION_H

#ifndef MEGACMD_MAJOR_VERSION
#define MEGACMD_MAJOR_VERSION 0
#endif
#ifndef MEGACMD_MINOR_VERSION
#define MEGACMD_MINOR_VERSION 9
#endif
#ifndef MEGACMD_MICRO_VERSION
#define MEGACMD_MICRO_VERSION 4
#endif

#ifndef MEGACMD_CODE_VERSION
#define MEGACMD_CODE_VERSION (MEGACMD_MICRO_VERSION*100+MEGACMD_MINOR_VERSION*10000+MEGACMD_MAJOR_VERSION*1000000)
#endif

const char * const megacmdchangelog =
        "Separated MEGAcmd into interactive shell (MEGAcmdShell) and server(MEGAcmdServer)""\n"
        "Added transfers management with \"transfer\" command""\n"
        "Uploads and downloads now support background mode with \"-q\"""\n"
        "Added confirmation on folder removal (interactive & non-interactive modes)""\n"
        "PCRE are now optional if available in all the commands with \"--use-pcre\"""\n"
        "Server initiated automatically in interactive and non-interactive mode""\n"
        "Added unicode support for Windows""\n"
        "Refurbished communications and secured non-interactive mode in Windows""\n"
        "Implemented copy (cp) to user's inbox""\n"
        "Several fixes and commands improvements"
        ;


#endif // VERSION_H
