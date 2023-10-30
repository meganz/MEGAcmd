# This guard is needed to avoid a double-include of this file in the test
# projects (since they include both the client and the server .pri files)
isEmpty(MEGACMD_COMMON_INCLUDED) {
MEGACMD_COMMON_INCLUDED = true

include(vcpkg_inclusion.pri)

packagesExist(libpcrecpp) | macx {
    LIBS += -lpcrecpp
	CONFIG += USE_PCRE
}

CONFIG += USE_MEDIAINFO
CONFIG += USE_LIBUV
DEFINES += ENABLE_BACKUPS
CONFIG += USE_CONSOLE
unix:!macx {
    exists(/usr/include/fpdfview.h) {
	    CONFIG += USE_PDFIUM
	}
}
else {
    CONFIG += USE_PDFIUM
}

win32 {
CONFIG += noreadline
CONFIG += USE_AUTOCOMPLETE
DEFINES += NO_READLINE
CONFIG+=c++17
}

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

win32 {
    DEFINES += NOMINMAX

    LIBS += -lole32 -loleaut32 -lshell32 -llz32 -ltaskschd
    QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
    QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO
}
else {
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter

    DEFINES += USE_PTHREAD

    LIBS += -lpthread
    LIBS += -lpcre
}

MEGACMD_BASE_PATH_RELATIVE = ../../../..
MEGACMD_COMMON_BASE_PATH = $$PWD/../../..

macx {
    HEADERS +=  $$MEGACMD_COMMON_BASE_PATH/src/megacmdplatform.h
	OBJECTIVE_SOURCES += $$MEGACMD_COMMON_BASE_PATH/src/megacmdplatform.mm
    ICON = app.icns
    QMAKE_INFO_PLIST = Info_MEGA.plist

    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.12

    LIBS += -framework Cocoa -framework SystemConfiguration -framework CoreFoundation -framework Foundation -framework Security
    LIBS += -lncurses
    QMAKE_CXXFLAGS += -g
}

include($$MEGACMD_COMMON_BASE_PATH/sdk/bindings/qt/sdk.pri)
DEFINES -= USE_QT
DEFINES -= MEGA_QT_LOGGING

SOURCES -= src/gfx/qt.cpp
SOURCES -= bindings/qt/QTMegaRequestListener.cpp
SOURCES -= bindings/qt/QTMegaTransferListener.cpp
SOURCES -= bindings/qt/QTMegaGlobalListener.cpp
SOURCES -= bindings/qt/QTMegaSyncListener.cpp
SOURCES -= bindings/qt/QTMegaListener.cpp
SOURCES -= bindings/qt/QTMegaEvent.cpp

CONFIG(FULLREQUIREMENTS) {
    DEFINES += REQUIRE_HAVE_FFMPEG
	DEFINES += REQUIRE_HAVE_LIBUV
	#DEFINES += REQUIRE_HAVE_LIBRAW
	#DEFINES += REQUIRE_ENABLE_CHAT
	DEFINES += REQUIRE_ENABLE_BACKUPS
	#DEFINES += REQUIRE_ENABLE_WEBRTC
	#DEFINES += REQUIRE_ENABLE_EVT_TLS
	DEFINES += REQUIRE_USE_MEDIAINFO
	DEFINES += REQUIRE_USE_PCRE
}

SOURCES += \
    $$MEGACMD_COMMON_BASE_PATH/src/megacmdcommonutils.cpp

HEADERS += \
    $$MEGACMD_COMMON_BASE_PATH/src/megacmdcommonutils.h

}
