CONFIG(debug, debug|release) {
    CONFIG -= debug release
    CONFIG += debug
}
CONFIG(release, debug|release) {
    CONFIG -= debug release
    CONFIG += release
}

MEGASDK_BASE_PATH = $$PWD/../../../../sdk

TARGET = MEGAcmdUpdateGenerator
TEMPLATE = app
CONFIG += console

SOURCES += $$MEGASDK_BASE_PATH/src/crypto/cryptopp.cpp \
            $$MEGASDK_BASE_PATH/src/base64.cpp \
            $$MEGASDK_BASE_PATH/src/utils.cpp \
            $$MEGASDK_BASE_PATH/src/logging.cpp

LIBS += -lcryptopp

win32 {
    release {
        LIBS += -L"$$MEGASDK_BASE_PATH/bindings/qt/3rdparty/libs/x32"
    }
    else {
        LIBS += -L"$$MEGASDK_BASE_PATH/bindings/qt/3rdparty/libs/x32d"
    }

    INCLUDEPATH += $$[QT_INSTALL_PREFIX]/src/3rdparty/zlib #TODO: figure out path to this
    #Probably: INCLUDEPATH += $$MEGASDK_BASE_PATH/bindings/qt/3rdparty/include/zlib

    LIBS += -lws2_32
    DEFINES += USE_CURL
}

macx {
   LIBS += -L$$MEGASDK_BASE_PATH/bindings/qt/3rdparty/libs
}

DEFINES += USE_CRYPTOPP
DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD $$MEGASDK_BASE_PATH/bindings/qt/3rdparty/include \
                $$MEGASDK_BASE_PATH/bindings/qt/3rdparty/include/cryptopp \
                $$MEGASDK_BASE_PATH/bindings/qt/3rdparty/include/cares \
                $$MEGASDK_BASE_PATH/include/

win32 {
    INCLUDEPATH += $$MEGASDK_BASE_PATH/include/mega/wincurl
}

unix {
    INCLUDEPATH += $$MEGASDK_BASE_PATH/include/mega/posix
}

SOURCES += ../../../../src/updategenerator/MEGAUpdateGenerator.cpp
