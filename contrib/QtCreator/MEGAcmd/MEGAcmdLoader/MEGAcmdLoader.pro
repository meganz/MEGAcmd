CONFIG -= qt

CONFIG(debug, debug|release) {
    CONFIG -= debug release
    CONFIG += debug
}
CONFIG(release, debug|release) {
    CONFIG -= debug release
    CONFIG += release
    DEFINES += NDEBUG
}

TARGET = MEGAcmdLoader
TEMPLATE = app
CONFIG += console

SOURCES += ../../../../src/loader/megacmdloader.cpp

win32 {
    QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
    QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO
}

macx {
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9

    QMAKE_CXXFLAGS += -g
}
