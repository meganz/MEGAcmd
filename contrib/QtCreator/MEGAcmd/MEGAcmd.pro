CONFIG(debug, debug|release) {
    CONFIG -= debug release
    CONFIG += debug
}
CONFIG(release, debug|release) {
    CONFIG -= debug release
    CONFIG += release
    DEFINES += NDEBUG
}

TEMPLATE = subdirs
SUBDIRS =  MEGAcmdShell MEGAcmdClient \
     MEGAcmdServer

macx {
    SUBDIRS += MEGAcmdLoader
}
