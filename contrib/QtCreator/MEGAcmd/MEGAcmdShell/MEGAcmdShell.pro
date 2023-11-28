CONFIG(debug, debug|release) {
    CONFIG -= debug release
    CONFIG += debug
}
CONFIG(release, debug|release) {
    CONFIG -= debug release
    CONFIG += release
    DEFINES += NDEBUG
}

CONFIG -= qt

TARGET = MEGAcmdShell
TEMPLATE = app
CONFIG += console

include(../MEGAcmdCommon.pri)
MEGACMD_BASE_PATH = $$PWD/$$MEGACMD_BASE_PATH_RELATIVE

SOURCES += \
    $$MEGACMD_BASE_PATH/src/megacmdshell/megacmdshell.cpp \
    $$MEGACMD_BASE_PATH/src/megacmdshell/megacmdshellcommunications.cpp \
    $$MEGACMD_BASE_PATH/src/megacmdshell/megacmdshellcommunicationsnamedpipes.cpp


HEADERS += \
    $$MEGACMD_BASE_PATH/src/megacmdshell/megacmdshell.h \
    $$MEGACMD_BASE_PATH/src/megacmdshell/megacmdshellcommunications.h \
    $$MEGACMD_BASE_PATH/src/megacmdshell/megacmdshellcommunicationsnamedpipes.h \
    $$MEGACMD_BASE_PATH/sdk/include/mega/thread.h

win32{
    RC_FILE = icon.rc
    QMAKE_LFLAGS += /LARGEADDRESSAWARE
    QMAKE_LFLAGS_WINDOWS += /SUBSYSTEM:WINDOWS,5.01
    QMAKE_LFLAGS_CONSOLE += /SUBSYSTEM:CONSOLE,5.01
    DEFINES += PSAPI_VERSION=1
    DEFINES += UNICODE _UNICODE NTDDI_VERSION=0x06000000 _WIN32_WINNT=0x0600
}

win32 {
    DEFINES += USE_READLINE_STATIC
}

macx {
    !vcpkg {
        INCLUDEPATH += $$MEGACMD_BASE_PATH/sdk/bindings/qt/3rdparty/include
        LIBS += $$MEGACMD_BASE_PATH/sdk/bindings/qt/3rdparty/libs/libreadline.a
    }
    LIBS += -framework Cocoa -framework SystemConfiguration -framework CoreFoundation -framework Foundation -framework Security
    LIBS += -lncurses
}
