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

TARGET = MEGAclient
TEMPLATE = app
CONFIG += console

SOURCES += ../../../../src/client/megacmdclient.cpp \
    ../../../../src/megacmdshell/megacmdshellcommunications.cpp \
    ../../../../src/megacmdshell/megacmdshellcommunicationsnamedpipes.cpp \
    ../../../../src/megacmdcommonutils.cpp

HEADERS += ../../../../src/megacmdshell/megacmdshellcommunications.h \
    ../../../../src/megacmdshell/megacmdshellcommunicationsnamedpipes.h \
    ../../../../sdk/include/mega/thread.h \
    ../../../../src/megacmdcommonutils.h

INCLUDEPATH += ../../../../sdk/include

win32{
    DEFINES += USE_WIN32THREAD
}
else{
    ## Disable de following to work with posix threads
    #DEFINES+=USE_CPPTHREAD
    #CONFIG += c++11

    DEFINES+=USE_PTHREAD
}

win32 {
    LIBS +=  -lshlwapi -lws2_32
    LIBS +=  -lshell32 -luser32 -ladvapi32

    RC_FILE = icon.rc
    QMAKE_LFLAGS += /LARGEADDRESSAWARE
    QMAKE_LFLAGS_WINDOWS += /SUBSYSTEM:WINDOWS,5.01
    QMAKE_LFLAGS_CONSOLE += /SUBSYSTEM:CONSOLE,5.01
    DEFINES += PSAPI_VERSION=1
    DEFINES += UNICODE _UNICODE NTDDI_VERSION=0x05010000 _WIN32_WINNT=0x0501
    QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
    QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO
}
else{
    LIBS += -lpthread
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
}


macx {
    INCLUDEPATH += $$PWD/../../../../sdk/bindings/qt/3rdparty/include
    LIBS += $$PWD/../../../../sdk/bindings/qt/3rdparty/libs/libreadline.a
    LIBS += -framework Cocoa -framework SystemConfiguration -framework CoreFoundation -framework Foundation -framework Security
    LIBS += -lncurses

    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9

    QMAKE_CXXFLAGS += -g
}


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
    win32 {
        DEFINES += __STDC_CONSTANT_MACROS
    }
}
include(../../../../sdk/bindings/qt/sdk.pri) #This is required to have logging.h included: avoiding this is rather complicated
DEFINES -= USE_QT
DEFINES -= MEGA_QT_LOGGING

SOURCES -= src/gfx/qt.cpp
SOURCES -= bindings/qt/QTMegaRequestListener.cpp
SOURCES -= bindings/qt/QTMegaTransferListener.cpp
SOURCES -= bindings/qt/QTMegaGlobalListener.cpp
SOURCES -= bindings/qt/QTMegaSyncListener.cpp
SOURCES -= bindings/qt/QTMegaListener.cpp
SOURCES -= bindings/qt/QTMegaEvent.cpp

