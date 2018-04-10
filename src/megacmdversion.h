#ifndef MEGACMDVERSION_H
#define MEGACMDVERSION_H

#ifndef MEGACMD_MAJOR_VERSION
#define MEGACMD_MAJOR_VERSION 0
#endif
#ifndef MEGACMD_MINOR_VERSION
#define MEGACMD_MINOR_VERSION 9
#endif
#ifndef MEGACMD_MICRO_VERSION
#define MEGACMD_MICRO_VERSION 9
#endif

#ifndef MEGACMD_CODE_VERSION
#define MEGACMD_CODE_VERSION (MEGACMD_MICRO_VERSION*100+MEGACMD_MINOR_VERSION*10000+MEGACMD_MAJOR_VERSION*1000000)
#endif

const char * const megacmdchangelog =
        "Webdav: Serve a MEGA location as a WEBDAV server.""\n"
        "Streaming: Webdav command can also be used for HTTP(S) streaming.""\n"
        "Added thumbnails for video files.""\n"
        "Listing -l now list unix-like columned summary (extended info is now -a)""\n"
        "du & ls -v is now --versions""\n"
        "Fixed --mtime restrictions in find""\n"
        "Added support for compiling in NAS systems""\n"
        "Minor fixes & adjustements"
        ;


#endif // VERSION_H


