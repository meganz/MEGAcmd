#ifndef MEGACMDVERSION_H
#define MEGACMDVERSION_H

#ifndef MEGACMD_MAJOR_VERSION
#define MEGACMD_MAJOR_VERSION 1
#endif
#ifndef MEGACMD_MINOR_VERSION
#define MEGACMD_MINOR_VERSION 2
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
        "\"put\" now supports wildcard expressions""\n"
        "support setting a proxy with \"proxy\" command""\n"
        "add support for addressing inshares with //from/""\n"
        "support for files/folders within public links""\n"
        "minor fix for ls --tree autocompletion""\n"
        "discard flags/options after \"--\"""\n"
        "--show-handles option added in ls & find""\n"
        "files/folders can be addressed using their handle H:XXXXXXX""\n"
        "support new links format""\n"
        "fixes in reported used storage""\n"
        "fix crash in find command""\n"
        "do not consider inshares for version storage used""\n"
        "win installer: do not ask for elevate permissions when running in silent mode""\n"
        "improved columned outputs to maximize screen use (syncs & transfers)""\n"
        "improve responsiveness at startup in interactive mode, to avoid hangs when session does not resume"
        "Added mode while logging in that allows certain actions (like setting a proxy)""\n"
        "non-interactive mode will not wait for commands that can be addressed before the session is resumed""\n"

        "speedup cancellation/startup of a huge number of transfers""\n"
        "cloud raid support""\n"
        "speedup improvements in cache and other CPU bottlenecks""\n"
        "many more fixes & adjustments"
        ;

}//end namespace
#endif // VERSION_H


