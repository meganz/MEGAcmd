#!/bin/zsh -e

Usage () {
    echo "Usage: installer_mac.sh [[--arch [arm64|x86_64]] [--build | --build-cmake] | [--sign] | [--create-dmg] | [--notarize] | [--full-pkg | --full-pkg-cmake]]"
    echo "    --arch [arm64|x86_64]  : Arch target. It will build for the host arch if not defined."
    echo "    --build                : Builds the app and creates the bundle using qmake."
    echo "    --build-cmake          : Idem but using cmake"
    echo "    --sign                 : Sign the app"
    echo "    --create-dmg           : Create the dmg package"
    echo "    --notarize             : Notarize package against Apple systems."
    echo "    --full-pkg             : Implies and overrides all the above using qmake"
    echo "    --full-pkg-cmake       : Idem but using cmake"
    echo ""
    echo "Environment variables needed to build:"
    echo "    MEGAQTPATH : Point it to a valid Qt installation path"
    echo "    VCPKGPATH : Point it to a directory containing a valid vcpkg installation"
    echo ""
    echo "Note: --build and --build-cmake are mutually exclusive."
    echo "      --full-pkg and --full-pkg-cmake are mutually exclusive."
    echo ""
}

if [ $# -eq 0 ]; then
   Usage
   exit 1
fi

APP_NAME=MEGAcmd
VOLUME_NAME="InstallMEGAcmd"
ID_BUNDLE=mega.mac
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
full_pkg_cmake=0
build=0
build_cmake=0
sign=0
createdmg=0
notarize=0

build_time=0
sign_time=0
dmg_time=0
notarize_time=0
total_time=0

while [ "$1" != "" ]; do
    case $1 in
        --arch )
            shift
            target_arch="${1}"
            if [ "${target_arch}" != "arm64" ] && [ "${target_arch}" != "x86_64" ]; then Usage; echo "Error: Invalid arch value."; exit 1; fi
            ;;
        --build )
            build=1
            if [ ${build_cmake} -eq 1 ]; then Usage; echo "Error: --build and --build-cmake are mutually exclusive."; exit 1; fi
            ;;
        --build-cmake )
            build_cmake=1
            if [ ${build} -eq 1 ]; then Usage; echo "Error: --build and --build-cmake are mutually exclusive."; exit 1; fi
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
            if [ ${full_pkg_cmake} -eq 1 ]; then Usage; echo "Error: --full-pkg and --full-pkg-cmake are mutually exclusive."; exit 1; fi
            full_pkg=1
            ;;
        --full-pkg-cmake )
            if [ ${full_pkg} -eq 1 ]; then Usage; echo "Error: --full-pkg and --full-pkg-cmake are mutually exclusive."; exit 1; fi
            full_pkg_cmake=1
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
    build_cmake=0
    sign=1
    createdmg=1
    notarize=1
fi

if [ ${full_pkg_cmake} -eq 1 ]; then
    build=0
    build_cmake=1
    sign=1
    createdmg=1
    notarize=1
fi

if [ ${build} -ne 1 -a ${build_cmake} -ne 1 -a ${sign} -ne 1 -a ${createdmg} -ne 1 -a ${notarize} -ne 1 ]; then
   Usage
   echo "Error: No action selected. Nothing to do."
   exit 1
fi

if [ ${build} -eq 1 -o ${build_cmake} -eq 1 ]; then
    build_time_start=`date +%s`

    if [ ${build_cmake} -ne 1 ]; then
      if [ -z "${MEGAQTPATH}" ] || [ ! -d "${MEGAQTPATH}/bin" ]; then
        echo "Please set MEGAQTPATH env variable to a valid QT installation path!"
        exit 1;
      fi
    fi
    if [ -z "${VCPKGPATH}" ] || [ ! -d "${VCPKGPATH}/vcpkg/installed" ]; then
        echo "Please set VCPKGPATH env variable to a directory containing a valid vcpkg installation!"
        exit 1;
    fi

    MEGAQTPATH="$(cd "$MEGAQTPATH" && pwd -P)"
    echo "Building with:"
    echo "  MEGAQTPATH : ${MEGAQTPATH}"
    echo "  VCPKGPATH  : ${VCPKGPATH}"

    if [ ${build_cmake} -ne 1 ]; then
        CARES_VERSION=libcares.2.dylib
        CURL_VERSION=libcurl.dylib

        CARES_PATH=${VCPKGPATH}/vcpkg/installed/${target_arch//x86_64/x64}-osx-mega/lib/$CARES_VERSION
        CURL_PATH=${VCPKGPATH}/vcpkg/installed/${target_arch//x86_64/x64}-osx-mega/lib/$CURL_VERSION
    fi

    # Clean previous build
    [ -z "${SKIP_REMOVAL}" ]  && rm -rf Release_${target_arch}
    mkdir Release_${target_arch} || true
    cd Release_${target_arch}

    # Build binaries
    if [ ${build_cmake} -eq 1 ]; then
        # Detect crosscompilation and set CMAKE_OSX_ARCHITECTURES.
        if  [ "${target_arch}" != "${host_arch}" ]; then
            CMAKE_EXTRA="-DCMAKE_OSX_ARCHITECTURES=${target_arch}"
        fi

#        cmake -DUSE_THIRDPARTY_FROM_VCPKG=1  -DCMAKE_PREFIX_PATH=${MEGAQTPATH} -DVCPKG_TRIPLET=x64-osx-mega -DMega3rdPartyDir=${VCPKGPATH} -DCMAKE_BUILD_TYPE=RelWithDebInfo ${CMAKE_EXTRA} -S ..//cmake
        cmake -DUSE_THIRDPARTY_FROM_VCPKG=1  -DCMAKE_PREFIX_PATH=${MEGAQTPATH} -DMega3rdPartyDir=${VCPKGPATH} -DCMAKE_BUILD_TYPE=RelWithDebInfo ${CMAKE_EXTRA} -S ..//cmake
        cmake --build ./ --target mega-cmd -j`sysctl -n hw.ncpu`
        cmake --build ./ --target mega-exec -j`sysctl -n hw.ncpu`
        cmake --build ./ --target mega-cmd-server -j`sysctl -n hw.ncpu`
        cmake --build ./ --target mega-cmd-updater -j`sysctl -n hw.ncpu`
        SERVER_PREFIX=""
        CLIENT_PREFIX=""
        SHELL_PREFIX=""
        LOADER_PREFIX=""
        UPDATER_PREFIX=""
    else
	#qmake build (should be deprecated)
        cp ../../sdk/contrib/official_build_configs/macos/config.h ../../sdk/include/mega/config.h
#        ${MEGAQTPATH}/bin/qmake "THIRDPARTY_VCPKG_BASE_PATH=${VCPKGPATH}" -r ../../contrib/QtCreator/MEGAcmd/ -spec macx-clang CONFIG+=release CONFIG+=x86_64 -nocache
        ${MEGAQTPATH}/bin/qmake "THIRDPARTY_VCPKG_BASE_PATH=${VCPKGPATH}" -r ../../contrib/QtCreator/MEGAcmd/ -spec macx-clang CONFIG+=release -nocache
        make -j`sysctl -n hw.ncpu`
    fi

    # Prepare MEGAcmd (server, mega-cmd) bundle
    cp -R ${SERVER_PREFIX}MEGAcmd.app ${SERVER_PREFIX}MEGAcmd_orig.app
    ${MEGAQTPATH}/bin/macdeployqt ${SERVER_PREFIX}MEGAcmd.app -no-strip
    dsymutil ${SERVER_PREFIX}MEGAcmd.app/Contents/MacOS/MEGAcmd -o MEGAcmd.app.dSYM
    strip ${SERVER_PREFIX}MEGAcmd.app/Contents/MacOS/MEGAcmd

    # Prepare MEGAclient (mega-exec) bundle
    ${MEGAQTPATH}/bin/macdeployqt ${CLIENT_PREFIX}MEGAclient.app -no-strip
    dsymutil ${CLIENT_PREFIX}MEGAclient.app/Contents/MacOS/MEGAclient -o MEGAclient.dSYM
    strip ${CLIENT_PREFIX}MEGAclient.app/Contents/MacOS/MEGAclient

    # Prepare MEGAcmdSell bundle
    ${MEGAQTPATH}/bin/macdeployqt ${SHELL_PREFIX}MEGAcmdShell.app -no-strip
    dsymutil ${SHELL_PREFIX}MEGAcmdShell.app/Contents/MacOS/MEGAcmdShell -o MEGAcmdShell.dSYM
    strip ${SHELL_PREFIX}MEGAcmdShell.app/Contents/MacOS/MEGAcmdShell

    # Prepare MEGAcmdLoader bundle
    ${MEGAQTPATH}/bin/macdeployqt ${LOADER_PREFIX}MEGAcmdLoader.app -no-strip
    dsymutil ${LOADER_PREFIX}MEGAcmdLoader.app/Contents/MacOS/MEGAcmdLoader -o MEGAcmdLoader.dSYM
    strip ${LOADER_PREFIX}MEGAcmdLoader.app/Contents/MacOS/MEGAcmdLoader

    # Prepare MEGAcmdUpdater bundle
    ${MEGAQTPATH}/bin/macdeployqt ${UPDATER_PREFIX}MEGAcmdUpdater.app -no-strip
    dsymutil ${UPDATER_PREFIX}MEGAcmdUpdater.app/Contents/MacOS/MEGAcmdUpdater -o MEGAcmdUpdater.dSYM
    strip ${UPDATER_PREFIX}MEGAcmdUpdater.app/Contents/MacOS/MEGAcmdUpdater

    # Copy client scripts and completion into package contents
    cp ../../src/client/mega-* ${SERVER_PREFIX}MEGAcmd.app/Contents/MacOS/
    cp ../../src/client/megacmd_completion.sh  ${SERVER_PREFIX}MEGAcmd.app/Contents/MacOS/

    # Rename exec MEGAcmd (aka: MEGAcmdServer) to mega-cmd and add it to package contents
    mv ${SERVER_PREFIX}MEGAcmd.app/Contents/MacOS/MEGAcmd ${SERVER_PREFIX}MEGAcmd.app/Contents/MacOS/mega-cmd
    # Copy initialize script (it will simple launch MEGAcmdLoader in a terminal) as the new main executable: MEGAcmd
    cp ../installer/MEGAcmd.sh ${SERVER_PREFIX}MEGAcmd.app/Contents/MacOS/MEGAcmd
    # Rename MEGAClient to mega-exec and add it to package contents
    mv ${CLIENT_PREFIX}MEGAclient.app/Contents/MacOS/MEGAclient ${SERVER_PREFIX}MEGAcmd.app/Contents/MacOS/mega-exec
    # Add MEGAcmdLoader into package contents
    mv ${LOADER_PREFIX}MEGAcmdLoader.app/Contents/MacOS/MEGAcmdLoader ${SERVER_PREFIX}MEGAcmd.app/Contents/MacOS/MEGAcmdLoader
    # Add MEGAcmdUpdater into package contents
    mv ${UPDATER_PREFIX}MEGAcmdUpdater.app/Contents/MacOS/MEGAcmdUpdater ${SERVER_PREFIX}MEGAcmd.app/Contents/MacOS/MEGAcmdUpdater
    # Add MEGAcmdShell into package contents
    mv ${SHELL_PREFIX}MEGAcmdShell.app/Contents/MacOS/MEGAcmdShell ${SERVER_PREFIX}MEGAcmd.app/Contents/MacOS/MEGAcmdShell


    if [ ${build_cmake} -ne 1 ]; then

        # Remove unneded Qt stuff
        rm -rf ${SERVER_PREFIX}MEGAcmd.app/Contents/Plugins
        rm -rf ${SERVER_PREFIX}MEGAcmd.app/Contents/Resources/qt.conf

        [ ! -f MEGAcmdServer/MEGAcmd.app/Contents/Frameworks/$CARES_VERSION ] && cp -L $CARES_PATH MEGAcmdServer/MEGAcmd.app/Contents/Frameworks/
        [ ! -f MEGAcmdServer/MEGAcmd.app/Contents/Frameworks/$CURL_VERSION ] && cp -L $CURL_PATH MEGAcmdServer/MEGAcmd.app/Contents/Frameworks/

        if [ ! -f MEGAcmdServer/MEGAcmd.app/Contents/Frameworks/$CURL_VERSION ]  \
            || [ ! -f MEGAcmdServer/MEGAcmd.app/Contents/Frameworks/$CARES_VERSION ];
        then
            echo "Error copying libs to app bundle."
            exit 1
        fi
    fi

    MEGACMD_VERSION=`grep -o "[0-9][0-9]*$" ../../src/megacmdversion.h | head -n 3 | paste -s -d '.' -`
    /usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString $MEGACMD_VERSION" "${SERVER_PREFIX}$APP_NAME.app/Contents/Info.plist"

    if [ ${build_cmake} -ne 1 ]; then
        rm -r $APP_NAME.app || :
        mv ${SERVER_PREFIX}$APP_NAME.app ./
    fi

    otool -L MEGAcmd.app/Contents/MacOS/mega-cmd
    otool -L MEGAcmd.app/Contents/MacOS/mega-exec
    otool -L MEGAcmd.app/Contents/MacOS/MEGAcmdLoader
    otool -L MEGAcmd.app/Contents/MacOS/MEGAcmdUpdater
    otool -L MEGAcmd.app/Contents/MacOS/MEGAcmdShell

    cd ..

    build_time=`expr $(date +%s) - $build_time_start`
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
if [ ${build} -eq 1 -o ${build_cmake} -eq 1 ]; then echo "Build:        ${build_time} s"; fi
if [ ${sign} -eq 1 ]; then echo "Sign:         ${sign_time} s"; fi
if [ ${createdmg} -eq 1 ]; then echo "dmg:          ${dmg_time} s"; fi
if [ ${notarize} -eq 1 ]; then echo "Notarization: ${notarize_time} s"; fi
echo ""
echo "DONE in       "`expr ${build_time} + ${sign_time} + ${dmg_time} + ${notarize_time}`" s"
