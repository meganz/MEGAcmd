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
        "added \"cat\" command to read text files (and potentially stream any file)""\n"
        "added update capabilities for Windows & MacOS (automatic updates are enabled by default)""\n"
        "added \"media-info\" command to show some information of multimedia files""\n"
        "added \"df\" command to show storage info""\n"
        "added tree-like listing command: \"tree\" or \"ls --tree\"""\n"
        "shown progress in non-interactive mode""\n"
        "improvements in progress and transfers results information""\n"
        "width output adjustments in non-interactive mode""\n"
        "output streamed partially from server to clients""\n"
        "added --time-format option to commands displaying times, to allow other formats""\n"
        "2FA login auth code can be passed as parameter now""\n"
        "transfer now differentiate backup transfers""\n"
        "backup command completion for local paths now only looks for folders""\n"
        "backup transfers are no longer cached (no reason to: backups are considered failed in such case)""\n"
        "backup fix some halts and output improvements""\n"
        "added Public Service Announcements (PSA) support""\n"
        "killsession now allows multiple parameters""\n"
        "fix \"clear\" in some linuxes""\n"
        "add support for spaces in password prompts""\n"
        "many more minor fixes & adjustments"
        ;


#endif // VERSION_H


