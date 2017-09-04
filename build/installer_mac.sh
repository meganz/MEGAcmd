#!/bin/sh -e

Usage () {
    echo "Usage: installer_mac.sh [[--sign] | [--create-dmg]]"
}

APP_NAME=MEGAcmd
MOUNTDIR=tmp
RESOURCES=installer/resourcesDMG
QTBASE=/QT/qt5/qtbase

sign=0
createdmg=0

while [ "$1" != "" ]; do
    case $1 in
        --sign )		sign=1
                                ;;
        --create-dmg )		createdmg=1
                                ;;
        -h | --help )           Usage
                                exit
                                ;;
        * )                     Usage
                                exit 1
    esac
    shift
done

rm -rf Release_x64
mkdir Release_x64
cd Release_x64
$QTBASE/bin/qmake -r ../../../../contrib/QtCreator/MEGAcmd/ -spec macx-g++ CONFIG+=release CONFIG+=x86_64 -nocache
make -j4

#After building, we will have 4 folders (one per project: MEGAcmdServer, MEGAcmdClient, MEGAcmdLoader & MEGAcmdShell)
# we will include the stuff from the other 3 into MEGAcmdServer folder

#backup stuff
cp -R MEGAcmdServer/MEGAcmd.app MEGAcmdServer/MEGAcmd_orig.app

#do macdeploy on the server .app
$QTBASE/bin/macdeployqt MEGAcmdServer/MEGAcmd.app

#get debug symbols and strip executable for MEGAcmdServer
dsymutil MEGAcmdServer/MEGAcmd.app/Contents/MacOS/MEGAcmd -o MEGAcmd.app.dSYM
strip MEGAcmdServer/MEGAcmd.app/Contents/MacOS/MEGAcmd

#get debug symbols and strip executable for MEGAClient
dsymutil MEGAcmdClient/MEGAclient.app/Contents/MacOS/MEGAclient -o MEGAclient.dSYM
strip MEGAcmdClient/MEGAclient.app/Contents/MacOS/MEGAclient

#get debug symbols and strip executable for MEGAcmdShell
dsymutil MEGAcmdShell/MEGAcmdShell.app/Contents/MacOS/MEGAcmdShell -o MEGAcmdShell.dSYM
strip MEGAcmdShell/MEGAcmdShell.app/Contents/MacOS/MEGAcmdShell

#get debug symbols and strip executable for MEGAcmdLoader
dsymutil MEGAcmdLoader/MEGAcmdLoader.app/Contents/MacOS/MEGAcmdLoader -o MEGAcmdLoader.dSYM
strip MEGAcmdLoader/MEGAcmdLoader.app/Contents/MacOS/MEGAcmdLoader

#copy client scripts and completion into package contents
cp ../../client/mega-* MEGAcmdServer/MEGAcmd.app/Contents/MacOS/
cp ../../client/megacmd_completion.sh  MEGAcmdServer/MEGAcmd.app/Contents/MacOS/

#rename exec MEGAcmd (aka: MEGAcmdServer) to mega-cmd and add it to package contents
mv MEGAcmdServer/MEGAcmd.app/Contents/MacOS/MEGAcmd MEGAcmdServer/MEGAcmd.app/Contents/MacOS/mega-cmd

#copy initialize script (it will simple launch MEGAcmdLoader in a terminal) as the new main executable: MEGAcmd
cp ../installer/MEGAcmd.sh  MEGAcmdServer/MEGAcmd.app/Contents/MacOS/MEGAcmd

#rename MEGAClient to mega-exec and add it to package contents
mv MEGAcmdClient/MEGAclient.app/Contents/MacOS/MEGAclient MEGAcmdServer/MEGAcmd.app/Contents/MacOS/mega-exec

#add MEGAcmdLoader into package contents
mv MEGAcmdLoader/MEGAcmdLoader.app/Contents/MacOS/MEGAcmdLoader MEGAcmdServer/MEGAcmd.app/Contents/MacOS/MEGAcmdLoader

#add MEGAcmdShell into package contents
mv MEGAcmdShell/MEGAcmdShell.app/Contents/MacOS/MEGAcmdShell MEGAcmdServer/MEGAcmd.app/Contents/MacOS/MEGAcmdShell

#move the resulting package to ./
mv MEGAcmdServer/MEGAcmd.app ./MEGAcmd.app

if [ "$sign" = "1" ]; then
	cp -R $APP_NAME.app ${APP_NAME}_unsigned.app
	echo "Signing 'APPBUNDLE'"
	codesign --force --verify --verbose --sign "Developer ID Application: Mega Limited" --deep $APP_NAME.app
	echo "Checking signature"
	spctl -vv -a $APP_NAME.app
fi

if [ "$createdmg" = "1" ]; then
	echo "DMG CREATION PROCESS..."
	echo "Creating temporary Disk Image (1/7)"
	#Create a temporary Disk Image
	/usr/bin/hdiutil create -srcfolder $APP_NAME.app/ -volname $APP_NAME -ov $APP_NAME-tmp.dmg -format UDRW >/dev/null

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
fi

echo "Cleaning"
rm -rf MEGAcmdServer
rm -rf MEGAcmdClient
rm -rf MEGAcmdShell

echo "DONE"


