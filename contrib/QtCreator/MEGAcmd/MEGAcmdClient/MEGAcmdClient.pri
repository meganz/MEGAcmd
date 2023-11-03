include(../MEGAcmdCommon.pri)

MEGACMD_BASE_PATH = $$PWD/$$MEGACMD_BASE_PATH_RELATIVE

SOURCES += \
    $$MEGACMD_BASE_PATH/src/client/megacmdclient.cpp \
    $$MEGACMD_BASE_PATH/src/megacmdshell/megacmdshellcommunications.cpp \
    $$MEGACMD_BASE_PATH/src/megacmdshell/megacmdshellcommunicationsnamedpipes.cpp

HEADERS += \
    $$MEGACMD_BASE_PATH/src/megacmdclient.h \
    $$MEGACMD_BASE_PATH/src/megacmdshell/megacmdshellcommunications.h \
    $$MEGACMD_BASE_PATH/src/megacmdshell/megacmdshellcommunicationsnamedpipes.h
