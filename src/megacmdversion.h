#ifndef MEGACMDVERSION_H
#define MEGACMDVERSION_H

#ifndef MEGACMD_MAJOR_VERSION
#define MEGACMD_MAJOR_VERSION 1
#endif
#ifndef MEGACMD_MINOR_VERSION
#define MEGACMD_MINOR_VERSION 6
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
        "Ease establishing shares""\n"
        "Improvements shares management"
        ;

}//end namespace
#endif // VERSION_H

