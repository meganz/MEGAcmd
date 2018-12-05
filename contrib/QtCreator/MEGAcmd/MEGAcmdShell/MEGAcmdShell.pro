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

win32 {
DEFINES += NO_READLINE
include(../../../../sdk/bindings/qt/sdk.pri) #This is required to have console.cpp included: avoiding this is rather complicated
HEADERS +=     ../../../../sdk/include/mega/win32/autocomplete.h
}

SOURCES += ../../../../src/megacmdshell/megacmdshell.cpp \
    ../../../../src/megacmdshell/megacmdshellcommunications.cpp \
    ../../../../src/megacmdshell/megacmdshellcommunicationsnamedpipes.cpp \
    ../../../../src/megacmdcommonutils.cpp


HEADERS += ../../../../src/megacmdshell/megacmdshell.h \
    ../../../../src/megacmdshell/megacmdshellcommunications.h \
    ../../../../src/megacmdshell/megacmdshellcommunicationsnamedpipes.h \
    ../../../../sdk/include/mega/thread.h \
    ../../../../src/megacmdcommonutils.h


INCLUDEPATH += ../../../../sdk/include

win32{
    RC_FILE = icon.rc
    QMAKE_LFLAGS += /LARGEADDRESSAWARE
    QMAKE_LFLAGS_WINDOWS += /SUBSYSTEM:WINDOWS,5.01
    QMAKE_LFLAGS_CONSOLE += /SUBSYSTEM:CONSOLE,5.01
    DEFINES += PSAPI_VERSION=1
    DEFINES += UNICODE _UNICODE NTDDI_VERSION=0x05010000 _WIN32_WINNT=0x0501
}


win32{
    DEFINES += USE_WIN32THREAD
}
else{
    ## Disable de following to work with posix threads
    #DEFINES+=USE_CPPTHREAD
    #CONFIG += c++11

    DEFINES+=USE_PTHREAD
    LIBS += -lpthread

    LIBS += -lpcre
}

win32 {
SOURCES += ../../../../sdk/src/win32/autocomplete.cpp \
    ../../../../sdk/src/win32/console.cpp \
    ../../../../sdk/src/thread/win32thread.cpp \
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
DEFINES += USE_READLINE_STATIC
}


DEFINES -= USE_QT

macx {
    INCLUDEPATH += $$PWD/../../../../sdk/bindings/qt/3rdparty/include
    LIBS += $$PWD/../../../../sdk/bindings/qt/3rdparty/libs/libreadline.a
    LIBS += -framework Cocoa -framework SystemConfiguration -framework CoreFoundation -framework Foundation -framework Security
    LIBS += -lncurses
    QMAKE_CXXFLAGS += -g
}
else {
    unix {
        LIBS += -lreadline
    }
}

win32 {
    QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
    QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO

    INCLUDEPATH += $$PWD/../../../../sdk/bindings/qt/3rdparty/include

    DEFINES += __STDC_LIMIT_MACROS #this is required to include <thread> or <mutex>

    LIBS +=  -lshlwapi -lws2_32 -luser32 -ladvapi32 -lshell32

    contains(CONFIG, BUILDX64) {
       release {
            LIBS += -L"$$PWD/../../../../sdk/bindings/qt/3rdparty/libs/x64"
        }
        else {
            LIBS += -L"$$PWD/../../../../sdk/bindings/qt/3rdparty/libs/x64d"
        }
    }


    !contains(CONFIG, BUILDX64) {
        release {
            LIBS += -L"$$PWD/../../../../sdk/bindings/qt/3rdparty/libs/x32"
        }
        else {
            LIBS += -L"$$PWD/../../../../sdk/bindings/qt/3rdparty/libs/x32d"
        }
    }
}
else {
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
}


