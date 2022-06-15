#ifndef MEGACMDVERSION_H
#define MEGACMDVERSION_H

#ifndef MEGACMD_MAJOR_VERSION
#define MEGACMD_MAJOR_VERSION 1
#endif
#ifndef MEGACMD_MINOR_VERSION
#define MEGACMD_MINOR_VERSION 5
#endif
#ifndef MEGACMD_MICRO_VERSION
#define MEGACMD_MICRO_VERSION 1
#endif

#ifndef MEGACMD_BUILD_ID
#define MEGACMD_BUILD_ID 0
#endif

#ifndef MEGACMD_CODE_VERSION
#define MEGACMD_CODE_VERSION (MEGACMD_BUILD_ID+MEGACMD_MICRO_VERSION*100+MEGACMD_MINOR_VERSION*10000+MEGACMD_MAJOR_VERSION*1000000)
#endif
namespace megacmd {

const char * const megacmdchangelog =
#ifdef __MACH__
        "Add support for pdfs (uploading them will create thumbnails/previews)""\n"
        "Improve communications with server in POSIX: no longer create multiple sockets""\n"
        "Support resuming session when logged into a folder""\n"
        "Renew and improve sync management and improve status reporting""\n"
        "Commands sync and transfers now allow for selecting --output-cols""\n"
        "Add creation time to ls""\n"
        "Fix issue running from php (due to a dangling file descriptor)""\n"
        "Fix several crashes, leaks and major performance improvements""\n"
#endif
        "Security adjustments"
        ;

}//end namespace
#endif // VERSION_H

