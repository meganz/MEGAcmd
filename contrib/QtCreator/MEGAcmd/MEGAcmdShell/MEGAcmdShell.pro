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

SOURCES += ../../../../src/megacmdshell/megacmdshell.cpp \
    ../../../../src/megacmdshell/megacmdshellcommunications.cpp \
    ../../../../src/megacmdshell/megacmdshellcommunicationsnamedpipes.cpp \
    ../../../../src/megacmdcommonutils.cpp


HEADERS += ../../../../src/megacmdshell/megacmdshell.h \
    ../../../../src/megacmdshell/megacmdshellcommunications.h \
    ../../../../src/megacmdshell/megacmdshellcommunicationsnamedpipes.h \
    ../../../../sdk/include/mega/thread.h \
    ../../../../src/megacmdcommonutils.h


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
        INCLUDEPATH += $$PWD/../../../../sdk/bindings/qt/3rdparty/include
        LIBS += $$PWD/../../../../sdk/bindings/qt/3rdparty/libs/libreadline.a
    }
    LIBS += -framework Cocoa -framework SystemConfiguration -framework CoreFoundation -framework Foundation -framework Security
    LIBS += -lncurses
}
