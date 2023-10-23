macx|win32 {
# have VCPKG included if not done already
isEmpty(VCPKG_TRIPLET) {
    MEGACMD_BASE_PATH_RELATIVE = ../../..
    MEGACMD_BASE_PATH = $$PWD/$$MEGACMD_BASE_PATH_RELATIVE

    isEmpty(THIRDPARTY_VCPKG_BASE_PATH){
        THIRDPARTY_VCPKG_BASE_PATH = $$MEGACMD_BASE_PATH/../3rdParty_megacmd
    }

    win32 {
        contains(QMAKE_TARGET.arch, x86_64):VCPKG_TRIPLET = x64-windows-mega
        !contains(QMAKE_TARGET.arch, x86_64):VCPKG_TRIPLET = x86-windows-mega
    }

    macx{
        isEmpty(VCPKG_TRIPLET){
            contains(QT_ARCH, x86_64):VCPKG_TRIPLET = x64-osx-mega
            contains(QT_ARCH, arm64):VCPKG_TRIPLET = arm64-osx-mega
        }
        contains(VCPKG_TRIPLET, arm64-osx-mega):contains(QMAKE_HOST.arch, arm64):QMAKE_APPLE_DEVICE_ARCHS=arm64

        message("Building for macOS $$QT_ARCH in a $$QMAKE_HOST.arch host.")
    }

    unix:!macx:VCPKG_TRIPLET = x64-linux

    message("vcpkg inclusion: THIRDPARTY_VCPKG_BASE_PATH: $$THIRDPARTY_VCPKG_BASE_PATH")
    message("vcpkg inclusion: VCPKG_TRIPLET: $$VCPKG_TRIPLET")

    THIRDPARTY_VCPKG_PATH = $$THIRDPARTY_VCPKG_BASE_PATH/vcpkg/installed/$$VCPKG_TRIPLET
    exists($$THIRDPARTY_VCPKG_PATH) {
       CONFIG += vcpkg
    }
}
else {
    message("vcpkg inclusion: REUSING THIRDPARTY_VCPKG_BASE_PATH: $$THIRDPARTY_VCPKG_BASE_PATH")
    message("vcpkg inclusion: REUSING VCPKG_TRIPLET: $$VCPKG_TRIPLET")
}

#print vcpkg inclusion status
vcpkg:debug:message("vcpkg inclusion: Building DEBUG with VCPKG 3rdparty at $$THIRDPARTY_VCPKG_PATH")
vcpkg:release:message("vcpkg inclusion: Building RELEASE with VCPKG 3rdparty at $$THIRDPARTY_VCPKG_PATH")
!vcpkg:message("vcpkg inclusion: vcpkg not used")

# Now do the actual includes:
vcpkg:INCLUDEPATH += $$THIRDPARTY_VCPKG_PATH/include
release:LIBS += -L"$$THIRDPARTY_VCPKG_PATH/lib"
debug:LIBS += -L"$$THIRDPARTY_VCPKG_PATH/debug/lib"

} #macx|win32
