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

include(MEGAcmdClient.pri)

SOURCES += $$MEGACMD_BASE_PATH/src/client/megacmd_client_main.cpp

#win32{
#    DEFINES += USE_WIN32THREAD
#}
#else{
#    ## Disable de following to work with posix threads
#    #DEFINES+=USE_CPPTHREAD
#    #CONFIG += c++11

#    DEFINES+=USE_PTHREAD
#}

win32 {
    LIBS +=  -lshlwapi -lws2_32
    LIBS +=  -lshell32 -luser32 -ladvapi32

    RC_FILE = icon.rc
    QMAKE_LFLAGS += /LARGEADDRESSAWARE
    QMAKE_LFLAGS_WINDOWS += /SUBSYSTEM:WINDOWS,5.01
    QMAKE_LFLAGS_CONSOLE += /SUBSYSTEM:CONSOLE,5.01
    DEFINES += PSAPI_VERSION=1
    DEFINES += UNICODE _UNICODE NTDDI_VERSION=0x06000000 _WIN32_WINNT=0x0600
    QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
    QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO
}
else{
    LIBS += -lpthread
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
}

macx {
    !vcpkg{
	    INCLUDEPATH += $$MEGACMD_BASE_PATH/sdk/bindings/qt/3rdparty/include
		LIBS += $$MEGACMD_BASE_PATH/sdk/bindings/qt/3rdparty/libs/libreadline.a
    }
    LIBS += -framework Cocoa -framework SystemConfiguration -framework CoreFoundation -framework Foundation -framework Security
    LIBS += -lncurses
}
