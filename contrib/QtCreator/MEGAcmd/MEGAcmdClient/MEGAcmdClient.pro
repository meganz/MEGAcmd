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
SOURCES += ../../../../sdk/src/thread/win32thread.cpp \
    ../../../../sdk/src/logging.cpp
HEADERS +=  ../../../../sdk/include/mega/win32thread.h \
    ../../../../sdk/include/mega/logging.h
}
else {
SOURCES += ../../../../sdk/src/thread/cppthread.cpp \
    ../../../../sdk/src/thread/posixthread.cpp \
    ../../../../sdk/src/logging.cpp

HEADERS +=  ../../../../sdk/include/mega/posixthread.h \
    ../../../../sdk/include/mega/thread/cppthread.h \
    ../../../../sdk/include/mega/logging.h
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
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9
    QMAKE_CXXFLAGS -= -stdlib=libc++
    QMAKE_LFLAGS -= -stdlib=libc++
    CONFIG -= c++11

    QMAKE_CXXFLAGS += -g
}

