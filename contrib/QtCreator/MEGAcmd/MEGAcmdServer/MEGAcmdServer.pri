CONFIG += USE_MEGAAPI

include(../MEGAcmdCommon.pri)

MEGACMD_BASE_PATH_RELATIVE = ../../../..
MEGACMD_BASE_PATH = $$PWD/$$MEGACMD_BASE_PATH_RELATIVE

SOURCES += $$MEGACMD_BASE_PATH/src/megacmd.cpp \
    $$MEGACMD_BASE_PATH/src/listeners.cpp \
    $$MEGACMD_BASE_PATH/src/megacmdexecuter.cpp \
    $$MEGACMD_BASE_PATH/src/megacmdlogger.cpp \
    $$MEGACMD_BASE_PATH/src/megacmdsandbox.cpp \
    $$MEGACMD_BASE_PATH/src/configurationmanager.cpp \
    $$MEGACMD_BASE_PATH/src/comunicationsmanager.cpp \
    $$MEGACMD_BASE_PATH/src/megacmdutils.cpp \
    $$MEGACMD_BASE_PATH/src/megacmdcommonutils.cpp

CONFIG(USE_DOWNLOADS_COMMAND) {
DEFINES+=HAVE_DOWNLOADS_COMMAND
SOURCES += $$MEGACMD_BASE_PATH/src/megacmdtransfermanager.cpp
HEADERS += $$MEGACMD_BASE_PATH/src/megacmdtransfermanager.h
}

HEADERS += $$MEGACMD_BASE_PATH/src/megacmd.h \
    $$MEGACMD_BASE_PATH/src/megacmdexecuter.h \
    $$MEGACMD_BASE_PATH/src/listeners.h \
    $$MEGACMD_BASE_PATH/src/megacmdlogger.h \
    $$MEGACMD_BASE_PATH/src/megacmdsandbox.h \
    $$MEGACMD_BASE_PATH/src/configurationmanager.h \
    $$MEGACMD_BASE_PATH/src/comunicationsmanager.h \
    $$MEGACMD_BASE_PATH/src/megacmdutils.h \
    $$MEGACMD_BASE_PATH/src/megacmdcommonutils.h \
    $$MEGACMD_BASE_PATH/src/megacmdversion.h \
    $$MEGACMD_BASE_PATH/src/megacmdplatform.h

win32 {
    SOURCES += $$MEGACMD_BASE_PATH/src/comunicationsmanagernamedpipes.cpp
    HEADERS += $$MEGACMD_BASE_PATH/src/comunicationsmanagernamedpipes.h
}
else {
    SOURCES +=$$MEGACMD_BASE_PATH/src/comunicationsmanagerfilesockets.cpp
    HEADERS +=$$MEGACMD_BASE_PATH/src/comunicationsmanagerfilesockets.h
}
