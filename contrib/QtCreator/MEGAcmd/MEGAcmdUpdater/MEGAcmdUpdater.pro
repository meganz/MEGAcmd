CONFIG -= qt
MEGASDK_BASE_PATH = $$PWD/../../../../sdk

CONFIG(debug, debug|release) {
    CONFIG -= debug release
    CONFIG += debug
}
CONFIG(release, debug|release) {
    CONFIG -= debug release
    CONFIG += release
    DEFINES += NDEBUG
}

TARGET = MEGAcmdUpdater
TEMPLATE = app
!win32 {
CONFIG += console
}

include(../vcpkg_inclusion.pri)

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
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.12
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
    vcpkg:LIBS += -lurlmon -lShlwapi -lShell32 -lAdvapi32 -lcryptopp-static
    else:LIBS += -lurlmon -lShlwapi -lShell32 -lAdvapi32 -lcryptoppmt

    QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
    QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO

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
