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

packagesExist(libpcrecpp){
DEFINES += USE_PCRE
LIBS += -lpcrecpp
CONFIG += USE_PCRE
}

CONFIG += USE_MEDIAINFO

win32 {
    SOURCES += ../../../../sdk/src/wincurl/console.cpp
    SOURCES += ../../../../sdk/src/wincurl/consolewaiter.cpp
    SOURCES += ../../../../src/comunicationsmanagernamedpipes.cpp
    HEADERS += ../../../../src/comunicationsmanagernamedpipes.h
}
else {
    SOURCES += ../../../../sdk/src/posix/console.cpp
    SOURCES += ../../../../sdk/src/posix/consolewaiter.cpp

    DEFINES += USE_PTHREAD

}

SOURCES += ../../../../src/megacmd.cpp \
    ../../../../src/listeners.cpp \
    ../../../../src/megacmdexecuter.cpp \
    ../../../../src/megacmdlogger.cpp \
    ../../../../src/megacmdsandbox.cpp \
    ../../../../src/configurationmanager.cpp \
    ../../../../src/comunicationsmanager.cpp \
    ../../../../src/megacmdutils.cpp


HEADERS += ../../../../src/megacmd.h \
    ../../../../src/megacmdexecuter.h \
    ../../../../src/listeners.h \
    ../../../../src/megacmdlogger.h \
    ../../../../src/megacmdsandbox.h \
    ../../../../src/configurationmanager.h \
    ../../../../src/comunicationsmanager.h \
    ../../../../src/megacmdutils.h \
    ../../../../src/megacmdversion.h \
    ../../../../src/megacmdplatform.h

    SOURCES +=../../../../src/comunicationsmanagerportsockets.cpp
    HEADERS +=../../../../src/comunicationsmanagerportsockets.h

win32 {
    LIBS += -lshell32
    RC_FILE = icon.rc
    QMAKE_LFLAGS += /LARGEADDRESSAWARE
    QMAKE_LFLAGS_WINDOWS += /SUBSYSTEM:WINDOWS,5.01
    QMAKE_LFLAGS_CONSOLE += /SUBSYSTEM:CONSOLE,5.01
    DEFINES += PSAPI_VERSION=1
    DEFINES += UNICODE _UNICODE NTDDI_VERSION=0x05010000 _WIN32_WINNT=0x0501
}
else {
    SOURCES +=../../../../src/comunicationsmanagerfilesockets.cpp
    HEADERS +=../../../../src/comunicationsmanagerfilesockets.h
}
include(../../../../sdk/bindings/qt/sdk.pri)
DEFINES -= USE_QT
DEFINES -= MEGA_QT_LOGGING
DEFINES += USE_FREEIMAGE
SOURCES -= src/thread/qtthread.cpp
win32{
    SOURCES += src/thread/win32thread.cpp
}
else{
    SOURCES += src/thread/posixthread.cpp
    LIBS += -lpthread
}
SOURCES -= src/gfx/qt.cpp
SOURCES += src/gfx/freeimage.cpp
SOURCES -= bindings/qt/QTMegaRequestListener.cpp
SOURCES -= bindings/qt/QTMegaTransferListener.cpp
SOURCES -= bindings/qt/QTMegaGlobalListener.cpp
SOURCES -= bindings/qt/QTMegaSyncListener.cpp
SOURCES -= bindings/qt/QTMegaListener.cpp
SOURCES -= bindings/qt/QTMegaEvent.cpp

macx {
    HEADERS += ../../../../src/megacmdplatform.h
    OBJECTIVE_SOURCES += ../../../../src/megacmdplatform.mm
    ICON = app.icns
#    QMAKE_INFO_PLIST = Info_MEGA.plist
    DEFINES += USE_PTHREAD
    INCLUDEPATH += ../../../../sdk/bindings/qt/3rdparty/include/FreeImage/Source
    LIBS += $$PWD/../../../../sdk/bindings/qt/3rdparty/libs/libfreeimage.a
    INCLUDEPATH += ../../../../sdk/bindings/qt/3rdparty/include/pcre
    LIBS += $$PWD/../../../../sdk/bindings/qt/3rdparty/libs/libpcre.a
    LIBS += $$PWD/../../../../sdk/bindings/qt/3rdparty/libs/libpcrecpp.a
    DEFINES += USE_PCRE
    CONFIG += USE_PCRE

    LIBS += -framework Cocoa -framework SystemConfiguration -framework CoreFoundation -framework Foundation -framework Security
    LIBS += -lncurses
    QMAKE_CXXFLAGS += -g
}
else {
    LIBS += -lfreeimage
}

win32 {
    QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
    QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO
}
else {
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
}
