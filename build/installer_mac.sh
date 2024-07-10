#!/bin/zsh -e

Usage () {
    echo "Usage: installer_mac.sh [--arch [arm64|x86_64|universal]] [--build] | [--sign] | [--create-dmg] | [--notarize] | [--full-pkg]"
    echo "    --build                          : Builds the app and creates the bundle using cmake"
    echo "    --sign                           : Sign the app"
    echo "    --create-dmg                     : Create the dmg package"
    echo "    --notarize                       : Notarize package against Apple systems."
    echo "    --full-pkg                       : Implies and overrides all the above"
    echo "    --arch [arm64|x86_64|universal]  : Arch target. It will use the host arch if none specified."
    echo ""
    echo "Environment variables needed to build:"
    echo "    VCPKGPATH : Point it to a directory containing a valid vcpkg installation"
    echo ""
}

if [ $# -eq 0 ]; then
   Usage
   exit 1
fi

APP_NAME=MEGAcmd
VOLUME_NAME="Install MEGAcmd"
MOUNTDIR=tmp
RESOURCES=installer/resourcesDMG

SERVER_PREFIX=MEGAcmdServer/
CLIENT_PREFIX=MEGAcmdClient/
SHELL_PREFIX=MEGAcmdShell/
LOADER_PREFIX=MEGAcmdLoader/
UPDATER_PREFIX=MEGAcmdUpdater/

host_arch=`uname -m`
target_arch=${host_arch}
full_pkg=0
build=0
sign=0
createdmg=0
notarize=0

sign_time=0
dmg_time=0
notarize_time=0
total_time=0

while [ "$1" != "" ]; do
    case $1 in
        --arch )
            shift
            target_arch="${1}"
            if [ "${target_arch}" != "arm64" ] && [ "${target_arch}" != "x86_64" ] && [ "${target_arch}" != "universal" ]; then Usage; echo "Error: Invalid arch value."; exit 1; fi
            ;;
        --build )
            build=1
            ;;
        --sign )
            sign=1
            ;;
        --create-dmg )
            createdmg=1
            ;;
        --notarize )
            notarize=1
            ;;
        --full-pkg )
            full_pkg=1
            ;;
        -h | --help )
            Usage
            exit
            ;;
        * )
            Usage
            echo "Unknown parameter: ${1}"
            exit 1
    esac
    shift
done

if [ ${full_pkg} -eq 1 ]; then
    build=1
    sign=1
    createdmg=1
    notarize=1
fi

if [ ${build} -ne 1 -a ${sign} -ne 1 -a ${createdmg} -ne 1 -a ${notarize} -ne 1 ]; then
   Usage
   echo "Error: No action selected. Nothing to do."
   exit 1
fi

build_archs=${target_arch}
if [ "$target_arch" = "universal" ]; then
    build_archs=(arm64 x86_64)
fi

for build_arch in $build_archs; do
    eval build_time_${build_arch}=0
done

if [ ${build} -eq 1 ]; then
    for build_arch in $build_archs; do

    build_time_start=`date +%s`

    if [ -z "${VCPKGPATH}" ] || [ ! -d "${VCPKGPATH}/vcpkg/installed" ]; then
        echo "Please set VCPKGPATH env variable to a directory containing a valid vcpkg installation!"
        exit 1;
    fi

    echo "Building with:"
    echo "  VCPKGPATH  : ${VCPKGPATH}"

    # Clean previous build
    [ -z "${SKIP_REMOVAL}" ]  && rm -rf Release_${build_arch}
    mkdir Release_${build_arch} || true
    cd Release_${build_arch}

    # Build binaries
    # Detect crosscompilation and set CMAKE_OSX_ARCHITECTURES.
    if  [ "${build_arch}" != "${host_arch}" ]; then
        CMAKE_EXTRA="-DCMAKE_OSX_ARCHITECTURES=${build_arch}"
    fi

    cmake -B build/build-cmake-Release -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_VERBOSE_MAKEFILE=ON ${CMAKE_EXTRA}
    cmake --build build/build-cmake-Release --target mega-cmd -j $(sysctl -n hw.ncpu)
    cmake --build build/build-cmake-Release --target mega-cmd -j $(sysctl -n hw.ncpu)
    cmake --build build/build-cmake-Release --target mega-cmd-server -j $(sysctl -n hw.ncpu)
    cmake --build build/build-cmake-Release --target mega-cmd-updater -j $(sysctl -n hw.ncpu)
    SERVER_PREFIX=""
    CLIENT_PREFIX=""
    SHELL_PREFIX=""
    LOADER_PREFIX=""
    UPDATER_PREFIX=""

    # create folder structure
    mkdir -p ${APP_NAME}.app/Contents/Frameworks
    mkdir -p ${APP_NAME}.app/Contents/MacOS
    mkdir -p ${APP_NAME}.app/Contents/Resources

    # place (and strip) executables
    typeset -A execOriginTarget
    execOriginTarget[mega-exec]=mega-exec
    execOriginTarget[mega-cmd]=MEGAcmdShell
    execOriginTarget[mega-cmd-updater]=MEGAcmdUpdater
    execOriginTarget[mega-cmd-server]=mega-cmd

    for k in "${(@k)execOriginTarget}"; do
        dsymutil $k -o $execOriginTarget[$k].dSYM
        cp $k ${APP_NAME}.app/Contents/MacOS/$execOriginTarget[$k]
        strip ${APP_NAME}.app/Contents/MacOS/$execOriginTarget[$k]
        # have dylibs use relative paths:
         for i in $(otool -L ${APP_NAME}.app/Contents/MacOS/$execOriginTarget[$k] | grep dylib | grep -v "\t/usr/lib"  | awk '{print $1}'); do
            install_name_tool -change $i "@executable_path/../Frameworks/"$(basename $i) ${APP_NAME}.app/Contents/MacOS/$execOriginTarget[$k];
         done
    done

    # copy initialize script (it will simple launch MEGAcmdLoader in a terminal) as the new main executable: MEGAcmd
    cp ../installer/MEGAcmd.sh ${APP_NAME}.app/Contents/MacOS/MEGAcmd

    # place commands and completion scripts
    cp ../../src/client/mega-* ${APP_NAME}.app/Contents/MacOS/
    cp ../../src/client/megacmd_completion.sh  ${APP_NAME}.app/Contents/MacOS/

    # place dylibs
     for i in $(otool -L ./mega-cmd ./mega-cmd-server ./mega-cmd-updater ./mega-exec | grep dylib | grep -v "\t/usr/lib" | sort | uniq | grep -o "lib[a-x0-9\.]*dylib"); do
        # copy dylib into its place
        libinAppPath=${APP_NAME}.app/Contents/Frameworks/$i
        cp "${VCPKGPATH}"/vcpkg/installed/${build_arch//x86_64/x64}-osx-mega/lib/$i $libinAppPath;

        # have their ids matching the relative one that will be set in the executable
        install_name_tool -id "@executable_path/../Frameworks/"$(basename $i) ${libinAppPath};

         # and have their linked dylibs use relative paths too:
         for l in $(otool -L ${libinAppPath} | grep dylib | grep "^\t" | grep -v "\t/usr/lib" | grep -v "executable_path" | awk '{print $1}'); do
            install_name_tool -change $l "@executable_path/../Frameworks/"$(basename $l) ${libinAppPath};
         done
     done;

    # place icns
    cp ../../contrib/QtCreator/MEGAcmd/MEGAcmdServer/app.icns ${APP_NAME}.app/Contents/Resources/app.icns

    #place PkgInfo
    echo 'APPL????' > ${APP_NAME}.app/Contents/PkgInfo

    #place empty empty.lproj
    touch ${APP_NAME}.app/Contents/Resources/empty.lproj

    #place Info.plist
    cp ../installer/Info.plist ${APP_NAME}.app/Contents/Info.plist


    # update version into Info.plist
    MEGACMD_VERSION=`grep -o "[0-9][0-9]*$" ../../src/megacmdversion.h | head -n 3 | paste -s -d '.' -`
    /usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString $MEGACMD_VERSION" "$APP_NAME.app/Contents/Info.plist"

    otool -L ${APP_NAME}.app/Contents/MacOS/mega-cmd
    otool -L ${APP_NAME}.app/Contents/MacOS/mega-exec
    otool -L ${APP_NAME}.app/Contents/MacOS/MEGAcmdLoader || true
    otool -L ${APP_NAME}.app/Contents/MacOS/MEGAcmdUpdater
    otool -L ${APP_NAME}.app/Contents/MacOS/MEGAcmdShell

    cd ..

    eval build_time_${build_arch}=`expr $(date +%s) - $build_time_start`
    unset build_time_start
    done
fi

if [ "$target_arch" = "universal" ]; then
    # Remove Release_universal folder (despite SKIP_REMOVAL, since it does not contain previous compilation results)
    rm -rf Release_${target_arch} || true
    for i in `find Release_arm64/MEGAcmd.app -type d`; do mkdir -p $i ${i/Release_arm64/Release_universal}; done
    # copy x86_64 files
    for i in `find Release_x86_64/MEGAcmd.app -type f`; do
        cp $i ${i/Release_x86_64/Release_universal}
    done

    # replace them by arm64
    for i in `find Release_arm64/MEGAcmd.app -type f`; do
        cp $i ${i/Release_arm64/Release_universal};
    done

    # merge binaries:
    for i in `find Release_arm64/MEGAcmd.app -type f ! -size 0 ! -name app.icns | perl -lne 'print if -B'`; do
        lipo -create $i ${i/Release_arm64/Release_x86_64} -output ${i/Release_arm64/Release_universal} || true;
    done
fi

if [ "$sign" = "1" ]; then
    sign_time_start=`date +%s`
    cd Release_${target_arch}
    cp -R $APP_NAME.app ${APP_NAME}_unsigned.app
    echo "Signing 'APPBUNDLE'"
    codesign --force --verify --verbose --preserve-metadata=entitlements --options runtime --sign "Developer ID Application: Mega Limited" --deep $APP_NAME.app
    echo "Checking signature"
    spctl -vv -a $APP_NAME.app
    cd ..
    sign_time=`expr $(date +%s) - $sign_time_start`
fi

if [ "$createdmg" = "1" ]; then
    dmg_time_start=`date +%s`
    cd Release_${target_arch}
    [ -f $APP_NAME.dmg ] && rm $APP_NAME.dmg
    echo "DMG CREATION PROCESS..."
    echo "Creating temporary Disk Image (1/7)"
    #Create a temporary Disk Image
    /usr/bin/hdiutil create -srcfolder $APP_NAME.app/ -volname $VOLUME_NAME -ov $APP_NAME-tmp.dmg -fs HFS+ -format UDRW >/dev/null

    echo "Attaching the temporary image (2/7)"
    #Attach the temporary image
    mkdir $MOUNTDIR
    /usr/bin/hdiutil attach $APP_NAME-tmp.dmg -mountroot $MOUNTDIR >/dev/null

    echo "Copying resources (3/7)"
    #Copy the background, the volume icon and DS_Store files
    unzip -d $MOUNTDIR/$VOLUME_NAME ../$RESOURCES.zip
    /usr/bin/SetFile -a C $MOUNTDIR/$VOLUME_NAME

    echo "Adding symlinks (4/7)"
    #Add a symbolic link to the Applications directory
    ln -s /Applications/ $MOUNTDIR/$VOLUME_NAME/Applications

    # Delete unnecessary file system events log if possible
    echo "Deleting .fseventsd"
    rm -rf $MOUNTDIR/$VOLUME_NAME/.fseventsd || true

    echo "Detaching temporary Disk Image (5/7)"
    #Detach the temporary image
    /usr/bin/hdiutil detach $MOUNTDIR/$VOLUME_NAME >/dev/null

    echo "Compressing Image (6/7)"
    #Compress it to a new image
    /usr/bin/hdiutil convert $APP_NAME-tmp.dmg -format UDZO -o $APP_NAME.dmg >/dev/null

    echo "Deleting temporary image (7/7)"
    #Delete the temporary image
    rm $APP_NAME-tmp.dmg
    rmdir $MOUNTDIR
    cd ..
    dmg_time=`expr $(date +%s) - $dmg_time_start`
fi

if [ "$notarize" = "1" ]; then
    notarize_time_start=`date +%s`
    cd Release_${target_arch}
    if [ ! -f $APP_NAME.dmg ];then
        echo ""
        echo "There is no dmg to be notarized."
        echo ""
        exit 1
    fi

    echo "Sending dmg for notarization (1/3)"

    xcrun notarytool submit $APP_NAME.dmg  --keychain-profile "AC_PASSWORD" --wait 2>&1 | tee notarylog.txt
    echo >> notarylog.txt

    xcrun stapler staple -v $APP_NAME.dmg 2>&1 | tee -a notarylog.txt

    echo "Stapling ok (2/3)"

    #Mount dmg volume to check if app bundle is notarized
    echo "Checking signature and notarization (3/3)"
    mkdir $MOUNTDIR || :
    hdiutil attach $APP_NAME.dmg -mountroot $MOUNTDIR >/dev/null
    spctl --assess -vv -a $MOUNTDIR/$VOLUME_NAME/$APP_NAME.app
    hdiutil detach $MOUNTDIR/$VOLUME_NAME >/dev/null
    rmdir $MOUNTDIR

    cd ..
    notarize_time=`expr $(date +%s) - $notarize_time_start`
fi

echo ""
if [ ${build} -eq 1 ]; then for build_arch in $build_archs; do echo "Build ${build_arch}:   ${(P)$(echo build_time_${build_arch})} s";  done; fi
if [ ${sign} -eq 1 ]; then echo "Sign:         ${sign_time} s"; fi
if [ ${createdmg} -eq 1 ]; then echo "dmg:          ${dmg_time} s"; fi
if [ ${notarize} -eq 1 ]; then echo "Notarization: ${notarize_time} s"; fi
echo ""
echo "DONE in       "`expr $(for build_arch in $build_archs; do echo "${(P)$(echo build_time_${build_arch})} + "; done) ${sign_time} + ${dmg_time} + ${notarize_time}`" s"
