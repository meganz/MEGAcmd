#!/bin/zsh -e

Usage () {
    echo "Usage: installer_mac.sh [[--build | --build-cmake] | [--sign] | [--create-dmg] | [--notarize] | [--full-pkg | --full-pkg-cmake]]"
    echo "    --build          : Builds the app and creates the bundle using qmake."
    echo "    --build-cmake    : Idem but using cmake"
    echo "    --sign           : Sign the app"
    echo "    --create-dmg     : Create the dmg package"
    echo "    --notarize       : Notarize package against Apple systems."
    echo "    --full-pkg       : Implies and overrides all the above using qmake"
    echo "    --full-pkg-cmake : Idem but using cmake"
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
ID_BUNDLE=mega.mac
MOUNTDIR=tmp
RESOURCES=installer/resourcesDMG

SERVER_PREFIX=MEGAcmdServer/
CLIENT_PREFIX=MEGAcmdClient/
SHELL_PREFIX=MEGAcmdShell/
LOADER_PREFIX=MEGAcmdLoader/
UPDATER_PREFIX=MEGAcmdUpdater/

full_pkg=0
full_pkg_cmake=0
build=0
build_cmake=0
sign=0
createdmg=0
notarize=0

while [ "$1" != "" ]; do
    case $1 in
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

if [ ${build} -eq 1 -o ${build_cmake} -eq 1 ]; then
    if [ -z "${MEGAQTPATH}" ] || [ ! -d "${MEGAQTPATH}/bin" ]; then
        echo "Please set MEGAQTPATH env variable to a valid QT installation path!"
        exit 1;
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
        AVCODEC_VERSION=libavcodec.58.dylib
        AVFORMAT_VERSION=libavformat.58.dylib
        AVUTIL_VERSION=libavutil.56.dylib
        SWSCALE_VERSION=libswscale.5.dylib
        CARES_VERSION=libcares.2.dylib
        CURL_VERSION=libcurl.dylib

        AVCODEC_PATH=${VCPKGPATH}/vcpkg/installed/x64-osx-mega/lib/$AVCODEC_VERSION
        AVFORMAT_PATH=${VCPKGPATH}/vcpkg/installed/x64-osx-mega/lib/$AVFORMAT_VERSION
        AVUTIL_PATH=${VCPKGPATH}/vcpkg/installed/x64-osx-mega/lib/$AVUTIL_VERSION
        SWSCALE_PATH=${VCPKGPATH}/vcpkg/installed/x64-osx-mega/lib/$SWSCALE_VERSION
        CARES_PATH=${VCPKGPATH}/vcpkg/installed/x64-osx-mega/lib/$CARES_VERSION
        CURL_PATH=${VCPKGPATH}/vcpkg/installed/x64-osx-mega/lib/$CURL_VERSION
    fi

    # Clean previous build
    rm -rf Release_x64
    mkdir Release_x64
    cd Release_x64

    # Build binaries
    if [ ${build_cmake} -eq 1 ]; then
        #cmake -DUSE_THIRDPARTY_FROM_VCPKG=1 -DUSE_PREBUILT_3RDPARTY=0 -DCMAKE_PREFIX_PATH=${MEGAQTPATH} -DVCPKG_TRIPLET=x64-osx-mega -DMega3rdPartyDir=${VCPKGPATH} -S ../contrib/cmake
        #cmake --build ./ --target MEGAcmd -j`sysctl -n hw.ncpu`
        #cmake --build ./ --target MEGAclient -j`sysctl -n hw.ncpu`
        #cmake --build ./ --target MEGAcmdShell -j`sysctl -n hw.ncpu`
        #cmake --build ./ --target MEGAcmdLoader -j`sysctl -n hw.ncpu`
        #cmake --build ./ --target MEGAcmdUpdater -j`sysctl -n hw.ncpu`
        SERVER_PREFIX=""
        CLIENT_PREFIX=""
        SHELL_PREFIX=""
        LOADER_PREFIX=""
        UPDATER_PREFIX=""
    else
        [ ! -f ../../sdk/include/mega/config.h ] && cp ../../sdk/contrib/official_build_configs/macos/config.h ../../sdk/include/mega/config.h
        ${MEGAQTPATH}/bin/qmake "THIRDPARTY_VCPKG_BASE_PATH=${VCPKGPATH}" -r ../../contrib/QtCreator/MEGAcmd/ -spec macx-clang CONFIG+=release CONFIG+=x86_64 -nocache
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

        [ ! -f MEGAcmdServer/MEGAcmd.app/Contents/Frameworks/$AVCODEC_VERSION ] && cp -L $AVCODEC_PATH MEGAcmdServer/MEGAcmd.app/Contents/Frameworks/
        [ ! -f MEGAcmdServer/MEGAcmd.app/Contents/Frameworks/$AVFORMAT_VERSION ] && cp -L $AVFORMAT_PATH MEGAcmdServer/MEGAcmd.app/Contents/Frameworks/
        [ ! -f MEGAcmdServer/MEGAcmd.app/Contents/Frameworks/$AVUTIL_VERSION ] && cp -L $AVUTIL_PATH MEGAcmdServer/MEGAcmd.app/Contents/Frameworks/
        [ ! -f MEGAcmdServer/MEGAcmd.app/Contents/Frameworks/$SWSCALE_VERSION ] && cp -L $SWSCALE_PATH MEGAcmdServer/MEGAcmd.app/Contents/Frameworks/
        [ ! -f MEGAcmdServer/MEGAcmd.app/Contents/Frameworks/$CARES_VERSION ] && cp -L $CARES_PATH MEGAcmdServer/MEGAcmd.app/Contents/Frameworks/
        [ ! -f MEGAcmdServer/MEGAcmd.app/Contents/Frameworks/$CURL_VERSION ] && cp -L $CURL_PATH MEGAcmdServer/MEGAcmd.app/Contents/Frameworks/

        if [ ! -f MEGAcmdServer/MEGAcmd.app/Contents/Frameworks/$AVCODEC_VERSION ]  \
            || [ ! -f MEGAcmdServer/MEGAcmd.app/Contents/Frameworks/$AVFORMAT_VERSION ]  \
            || [ ! -f MEGAcmdServer/MEGAcmd.app/Contents/Frameworks/$AVUTIL_VERSION ]  \
            || [ ! -f MEGAcmdServer/MEGAcmd.app/Contents/Frameworks/$SWSCALE_VERSION ];
        then
            echo "Error copying FFmpeg libs to app bundle."
            exit 1
        fi
    fi

    #MEGASYNC_VERSION=`grep "const QString Preferences::VERSION_STRING" ../src/MEGASync/control/Preferences.cpp | awk -F '"' '{print $2}'`
    #/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString $MEGASYNC_VERSION" "${MSYNC_PREFIX}$APP_NAME.app/Contents/Info.plist"

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
fi

if [ "$sign" = "1" ]; then
	cd Release_x64
	cp -R $APP_NAME.app ${APP_NAME}_unsigned.app
	echo "Signing 'APPBUNDLE'"
	codesign --force --verify --verbose --preserve-metadata=entitlements --options runtime --sign "Developer ID Application: Mega Limited" --deep $APP_NAME.app
	echo "Checking signature"
	spctl -vv -a $APP_NAME.app
	cd ..
fi

if [ "$createdmg" = "1" ]; then
	cd Release_x64
	[ -f $APP_NAME.dmg ] && rm $APP_NAME.dmg
	echo "DMG CREATION PROCESS..."
	echo "Creating temporary Disk Image (1/7)"
	#Create a temporary Disk Image
	/usr/bin/hdiutil create -srcfolder $APP_NAME.app/ -volname $APP_NAME -ov $APP_NAME-tmp.dmg -fs HFS+ -format UDRW >/dev/null

	echo "Attaching the temporary image (2/7)"
	#Attach the temporary image
	mkdir $MOUNTDIR
	/usr/bin/hdiutil attach $APP_NAME-tmp.dmg -mountroot $MOUNTDIR >/dev/null

	echo "Copying resources (3/7)"
	#Copy the background, the volume icon and DS_Store files
	unzip -d $MOUNTDIR/$APP_NAME ../$RESOURCES.zip
	/usr/bin/SetFile -a C $MOUNTDIR/$APP_NAME

	echo "Adding symlinks (4/7)"
	#Add a symbolic link to the Applications directory
	ln -s /Applications/ $MOUNTDIR/$APP_NAME/Applications

	echo "Detaching temporary Disk Image (5/7)"
	#Detach the temporary image
	/usr/bin/hdiutil detach $MOUNTDIR/$APP_NAME >/dev/null

	echo "Compressing Image (6/7)"
	#Compress it to a new image
	/usr/bin/hdiutil convert $APP_NAME-tmp.dmg -format UDZO -o $APP_NAME.dmg >/dev/null

	echo "Deleting temporary image (7/7)"
	#Delete the temporary image
	rm $APP_NAME-tmp.dmg
	rmdir $MOUNTDIR
	cd ..
fi

if [ "$notarize" = "1" ]; then

	cd Release_x64
	if [ ! -f $APP_NAME.dmg ];then
		echo ""
		echo "There is no dmg to be notarized."
		echo ""
		exit 1
	fi

	rm querystatus.txt staple.txt || :

	echo "NOTARIZATION PROCESS..."
	echo "Getting USERNAME for notarization commands (1/7)"

	AC_USERNAME=$(security find-generic-password -s AC_PASSWORD | grep  acct | cut -d '"' -f 4)
	if [[ -z "$AC_USERNAME" ]]; then
		echo "Error USERNAME not found for notarization process. You should add item named AC_PASSWORD with and account value matching the username to macOS keychain"
		false
	fi

	echo "Sending dmg for notarization (2/7)"
	xcrun altool --notarize-app -t osx -f $APP_NAME.dmg --primary-bundle-id $ID_BUNDLE -u $AC_USERNAME -p "@keychain:AC_PASSWORD" --output-format xml 2>&1 > staple.txt
	RUUID=$(cat staple.txt | grep RequestUUID -A 1 | tail -n 1 | awk -F "[<>]" '{print $3}')
	echo $RUUID
	if [ ! -z "$RUUID" ] ; then
		echo "Received UUID for notarization request. Checking state... (3/7)"
		attempts=60
		while [ $attempts -gt 0 ]
		do
			echo "Querying state of notarization..."
			xcrun altool --notarization-info $RUUID -u $AC_USERNAME -p "@keychain:AC_PASSWORD" --output-format xml  2>&1 > querystatus.txt
			RUUIDQUERY=$(cat querystatus.txt | grep RequestUUID -A 1 | tail -n 1 | awk -F "[<>]" '{print $3}')
			if [[ "$RUUID" != "$RUUIDQUERY" ]]; then
				echo "UUIDs missmatch"
				false
			fi

			STATUS=$(cat querystatus.txt  | grep -i ">Status<" -A 1 | tail -n 1  | awk -F "[<>]" '{print $3}')

			if [[ $STATUS == "invalid" ]]; then
				echo "INVALID status. Check file querystatus.txt for further information"
				echo $STATUS
				break
			elif [[ $STATUS == "success" ]]; then
				echo "Notarized ok. Stapling dmg file..."
				xcrun stapler staple -v $APP_NAME.dmg
				echo "Stapling ok"

				#Mount dmg volume to check if app bundle is notarized
				echo "Checking signature and notarization"
				mkdir $MOUNTDIR || :
				hdiutil attach $APP_NAME.dmg -mountroot $MOUNTDIR >/dev/null
				spctl -v -a $MOUNTDIR/$APP_NAME/$APP_NAME.app
				hdiutil detach $MOUNTDIR/$APP_NAME >/dev/null
				rmdir $MOUNTDIR
				break
			else
				echo $STATUS
			fi

			attempts=$((attempts - 1))
			sleep 30
		done

		if [[ $attempts -eq 0 ]]; then
			echo "Notarization still in process, timed out waiting for the process to end"
			false
		fi
	fi
	cd ..
fi

echo "DONE"
