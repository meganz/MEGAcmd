CONFIG(debug, debug|release) {
    CONFIG -= debug release
    CONFIG += debug
}
CONFIG(release, debug|release) {
    CONFIG -= debug release
    CONFIG += release
}

MEGACMD_BASE_PATH_RELATIVE = ../../../../..
MEGACMD_BASE_PATH = $$PWD/$$MEGACMD_BASE_PATH_RELATIVE

TARGET = test_unit
TEMPLATE = app

CONFIG -= qt
CONFIG += object_parallel_to_source
CONFIG += console

DEFINES += MEGACMD_TESTING_CODE

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

    debug:LIBS += -lgtestd
    !debug:LIBS += -lgtest
}
else {
    exists(/opt/gtest/gtest-1.10.0/lib) {
        LIBS += -L/opt/gtest/gtest-1.10.0/lib
        INCLUDEPATH += /opt/gtest/gtest-1.10.0/include
    }
    LIBS += -lgtest
    LIBS += -lpthread
}

CONFIG -= c++11
QMAKE_CXXFLAGS-=-std=c++11
CONFIG += c++17
QMAKE_CXXFLAGS+=-std=c++17

QMAKE_CXXFLAGS += "-fsanitize=address"
QMAKE_LFLAGS += "-fsanitize=address"
QMAKE_CXXFLAGS_DEBUG += "-fsanitize=address"

INCLUDEPATH += \
$$MEGACMD_BASE_PATH_RELATIVE/sdk/include \
$$MEGACMD_BASE_PATH_RELATIVE/src \

include(../MEGAcmdTest_common/MEGAcmdTest_common.pri)

SOURCES += \
$$MEGACMD_BASE_PATH_RELATIVE/tests/unit/StringUtilsTests.cpp \
$$MEGACMD_BASE_PATH_RELATIVE/tests/unit/main.cpp

#Dependencies:
SOURCES += \
$$MEGACMD_BASE_PATH_RELATIVE/src/megacmdcommonutils.cpp

