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

packagesExist(libpcrecpp) | macx {
LIBS += -lpcrecpp
CONFIG += USE_PCRE
}

CONFIG += USE_MEDIAINFO
CONFIG += USE_LIBUV
DEFINES += ENABLE_BACKUPS

unix:!macx {
        exists(/usr/include/ffmpeg-mega)|exists(mega/bindings/qt/3rdparty/include/ffmpeg)|packagesExist(libavcodec) {
            CONFIG += USE_FFMPEG
        }
}
else {
    CONFIG += USE_FFMPEG
}

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

macx {
    HEADERS += ../../../../src/megacmdplatform.h
    OBJECTIVE_SOURCES += ../../../../src/megacmdplatform.mm
    ICON = app.icns
#    QMAKE_INFO_PLIST = Info_MEGA.plist

    CONFIG += USE_OPENSSL
    DEFINES += USE_OPENSSL

    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6
    QMAKE_CXXFLAGS -= -stdlib=libc++
    QMAKE_LFLAGS -= -stdlib=libc++
    CONFIG -= c++11

    LIBS += -framework Cocoa -framework SystemConfiguration -framework CoreFoundation -framework Foundation -framework Security
    LIBS += -lncurses
    QMAKE_CXXFLAGS += -g
}

include(../../../../sdk/bindings/qt/sdk.pri)
DEFINES -= USE_QT
DEFINES -= MEGA_QT_LOGGING

SOURCES -= src/gfx/qt.cpp
SOURCES -= bindings/qt/QTMegaRequestListener.cpp
SOURCES -= bindings/qt/QTMegaTransferListener.cpp
SOURCES -= bindings/qt/QTMegaGlobalListener.cpp
SOURCES -= bindings/qt/QTMegaSyncListener.cpp
SOURCES -= bindings/qt/QTMegaListener.cpp
SOURCES -= bindings/qt/QTMegaEvent.cpp


win32 {
    QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
    QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO
}
else {
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
}
