include(../../vcpkg_inclusion.pri)

MEGACMD_BASE_PATH_RELATIVE = ../../../../..
MEGACMD_BASE_PATH = $$PWD/$$MEGACMD_BASE_PATH_RELATIVE

INCLUDEPATH += \
    $$MEGACMD_BASE_PATH/tests/common/ \

SOURCES += \
    $$MEGACMD_BASE_PATH/tests/common/Instruments.cpp \
    $$MEGACMD_BASE_PATH/tests/common/TestUtils.cpp \

HEADERS += \
    $$MEGACMD_BASE_PATH/tests/common/Instruments.h \
    $$MEGACMD_BASE_PATH/tests/common/TestUtils.h \
