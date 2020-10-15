win32:THIRDPARTY_VCPKG_BASE_PATH =  C:/Users/build/MEGA/build-MEGAsync/3rdParty_MSVC2017_20200529
win32:contains(QMAKE_TARGET.arch, x86_64):VCPKG_TRIPLET = x64-windows-mega
win32:!contains(QMAKE_TARGET.arch, x86_64):VCPKG_TRIPLET = x86-windows-mega

macx:THIRDPARTY_VCPKG_BASE_PATH = $$PWD/../../../../3rdParty
macx:VCPKG_TRIPLET = x64-osx

message("THIRDPARTY_VCPKG_BASE_PATH: $$THIRDPARTY_VCPKG_BASE_PATH")
message("VCPKG_TRIPLET: $$VCPKG_TRIPLET")

THIRDPARTY_VCPKG_PATH = $$THIRDPARTY_VCPKG_BASE_PATH/vcpkg/installed/$$VCPKG_TRIPLET
exists($$THIRDPARTY_VCPKG_PATH) {
   CONFIG += vcpkg
}
vcpkg:debug:message("Building DEBUG with VCPKG 3rdparty at $$THIRDPARTY_VCPKG_PATH")
vcpkg:release:message("Building RELEASE with VCPKG 3rdparty at $$THIRDPARTY_VCPKG_PATH")
!vcpkg:message("vcpkg not used")

CONFIG -= qt
MEGASDK_BASE_PATH = $$PWD/../../../../sdk

CONFIG(debug, debug|release) {
    CONFIG -= debug release
    CONFIG += debug
}
CONFIG(release, debug|release) {
    CONFIG -= debug release
    CONFIG += release
}

TARGET = MEGAcmdUpdater
TEMPLATE = app
!win32 {
CONFIG += console
}

HEADERS += ../../../../src/updater/UpdateTask.h \
    ../../../../src/updater/Preferences.h \
    ../../../../src/updater/MacUtils.h

SOURCES += ../../../../src/updater/MegaUpdater.cpp \
    ../../../../src/updater/UpdateTask.cpp

vcpkg:INCLUDEPATH += $$THIRDPARTY_VCPKG_PATH/include
else:INCLUDEPATH += $$MEGASDK_BASE_PATH/bindings/qt/3rdparty/include

macx {    
    OBJECTIVE_SOURCES +=  ../../../../src/updater/MacUtils.mm
    DEFINES += _DARWIN_FEATURE_64_BIT_INODE CRYPTOPP_DISABLE_ASM
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9
    !vcpkg:LIBS += -L$$MEGASDK_BASE_PATH/bindings/qt/3rdparty/libs/
    vcpkg:debug:LIBS += -L$$THIRDPARTY_VCPKG_PATH/debug/lib/
    vcpkg:release:LIBS += -L$$THIRDPARTY_VCPKG_PATH/lib/
    LIBS += -framework Cocoa -framework SystemConfiguration -framework CoreFoundation -framework Foundation -framework Security
    QMAKE_CXXFLAGS += -g
    LIBS += -lcryptopp
}

win32 {
    LIBS += -llz32 -luser32

    contains(CONFIG, BUILDX64) {
       release {
            vcpkg:LIBS += -L"$$THIRDPARTY_VCPKG_PATH/lib"
            else:LIBS += -L"$$MEGASDK_BASE_PATH/bindings/qt/3rdparty/libs/x64"
        }
        else {
            vcpkg:LIBS += -L"$$THIRDPARTY_VCPKG_PATH/debug/lib"
            else:LIBS += -L"$$MEGASDK_BASE_PATH/bindings/qt/3rdparty/libs/x64d"
        }
    }

    !contains(CONFIG, BUILDX64) {
        release {
            vcpkg:LIBS += -L"$$THIRDPARTY_VCPKG_PATH/lib"
            else:LIBS += -L"$$MEGASDK_BASE_PATH/bindings/qt/3rdparty/libs/x32"
        }
        else {
            vcpkg:LIBS += -L"$$THIRDPARTY_VCPKG_PATH/debug/lib"
            else:LIBS += -L"$$MEGASDK_BASE_PATH/bindings/qt/3rdparty/libs/x32d"
        }
    }

    DEFINES += UNICODE _UNICODE NTDDI_VERSION=0x06000000 _WIN32_WINNT=0x0600
    vcpkg:LIBS += -lurlmon -lShlwapi -lShell32 -lAdvapi32 -lcryptopp-staticcrt
    else:LIBS += -lurlmon -lShlwapi -lShell32 -lAdvapi32 -lcryptoppmt

    QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
    QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO

    QMAKE_CXXFLAGS_RELEASE += -MT
    QMAKE_CXXFLAGS_DEBUG += -MTd

    QMAKE_CXXFLAGS_RELEASE -= -MD
    QMAKE_CXXFLAGS_DEBUG -= -MDd

    RC_FILE = icon.rc
    QMAKE_LFLAGS += /LARGEADDRESSAWARE
    QMAKE_LFLAGS_WINDOWS += /SUBSYSTEM:WINDOWS,5.01
    QMAKE_LFLAGS_CONSOLE += /SUBSYSTEM:CONSOLE,5.01
    DEFINES += PSAPI_VERSION=1
}

unix:!macx {
   exists($$MEGASDK_BASE_PATH/bindings/qt/3rdparty/libs/libcryptopp.a) {
    LIBS +=  $$MEGASDK_BASE_PATH/bindings/qt/3rdparty/libs/libcryptopp.a
   }
   else {
    LIBS += -lcryptopp
   }
}
