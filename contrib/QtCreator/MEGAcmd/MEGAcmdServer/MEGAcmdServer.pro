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

win32 {
TARGET = MEGAcmdServer
}
else {
TARGET = MEGAcmd
}
TEMPLATE = app
CONFIG += console
CONFIG += USE_MEGAAPI
CONFIG -= USE_PDFIUM

include(../MEGAcmdCommon.pri)

SOURCES += ../../../../src/megacmd.cpp \
    ../../../../src/listeners.cpp \
    ../../../../src/megacmdexecuter.cpp \
    ../../../../src/megacmdlogger.cpp \
    ../../../../src/megacmdsandbox.cpp \
    ../../../../src/configurationmanager.cpp \
    ../../../../src/comunicationsmanager.cpp \
    ../../../../src/megacmdutils.cpp \
    ../../../../src/megacmdcommonutils.cpp


HEADERS += ../../../../src/megacmd.h \
    ../../../../src/megacmdexecuter.h \
    ../../../../src/listeners.h \
    ../../../../src/megacmdlogger.h \
    ../../../../src/megacmdsandbox.h \
    ../../../../src/configurationmanager.h \
    ../../../../src/comunicationsmanager.h \
    ../../../../src/megacmdutils.h \
    ../../../../src/megacmdcommonutils.h \
    ../../../../src/megacmdversion.h \
    ../../../../src/megacmdplatform.h

win32 {
    SOURCES += ../../../../src/comunicationsmanagernamedpipes.cpp
    HEADERS += ../../../../src/comunicationsmanagernamedpipes.h

    RC_FILE = icon.rc
    QMAKE_LFLAGS += /LARGEADDRESSAWARE
    QMAKE_LFLAGS_WINDOWS += /SUBSYSTEM:WINDOWS,5.01
    QMAKE_LFLAGS_CONSOLE += /SUBSYSTEM:CONSOLE,5.01
    DEFINES += PSAPI_VERSION=1
    DEFINES += UNICODE _UNICODE NTDDI_VERSION=0x06000000 _WIN32_WINNT=0x0600
}
else {
    SOURCES +=../../../../src/comunicationsmanagerfilesockets.cpp
    HEADERS +=../../../../src/comunicationsmanagerfilesockets.h
}
