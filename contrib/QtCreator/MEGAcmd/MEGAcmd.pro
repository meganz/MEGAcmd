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
     MEGAcmdServer MEGAcmdUpdater

macx {
    SUBDIRS += MEGAcmdLoader
    QMAKE_INFO_PLIST = Info_MEGA.plist
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.12
}
