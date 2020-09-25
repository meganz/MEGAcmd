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

INCLUDEPATH += $$MEGASDK_BASE_PATH/bindings/qt/3rdparty/include

macx {    
    OBJECTIVE_SOURCES +=  ../../../../src/updater/MacUtils.mm
    DEFINES += _DARWIN_FEATURE_64_BIT_INODE CRYPTOPP_DISABLE_ASM
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9
    LIBS += -L$$MEGASDK_BASE_PATH/bindings/qt/3rdparty/libs/
    LIBS += -framework Cocoa -framework SystemConfiguration -framework CoreFoundation -framework Foundation -framework Security
    QMAKE_CXXFLAGS += -g
    LIBS += -lcryptopp
}

win32 {
    LIBS += -llz32 -luser32

    contains(CONFIG, BUILDX64) {
       release {
            LIBS += -L"$$MEGASDK_BASE_PATH/bindings/qt/3rdparty/libs/x64"
        }
        else {
            LIBS += -L"$$MEGASDK_BASE_PATH/bindings/qt/3rdparty/libs/x64d"
        }
    }

    !contains(CONFIG, BUILDX64) {
        release {
            LIBS += -L"$$MEGASDK_BASE_PATH/bindings/qt/3rdparty/libs/x32"
        }
        else {
            LIBS += -L"$$MEGASDK_BASE_PATH/bindings/qt/3rdparty/libs/x32d"
        }
    }

    DEFINES += UNICODE _UNICODE NTDDI_VERSION=0x06000000 _WIN32_WINNT=0x0600
    LIBS += -lurlmon -lShlwapi -lShell32 -lAdvapi32 -lcryptoppmt

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
