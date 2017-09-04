#!/bin/bash -x

##
 # @file examples/megacmd/build/create_tarball.sh
 # @brief Generates megacmd tarballs and compilation scripts
 #
 # (c) 2013-2014 by Mega Limited, Auckland, New Zealand
 #
 # This file is part of the MEGA SDK - Client Access Engine.
 #
 # Applications using the MEGA API must present a valid application key
 # and comply with the the rules set forth in the Terms of Service.
 #
 # The MEGA SDK is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 #
 # @copyright Simplified (2-clause) BSD License.
 #
 # You should have received a copy of the license along with this
 # program.
##

set -euo pipefail
IFS=$'\n\t'

# make sure the source tree is in "clean" state
cwd=$(pwd)
BASEPATH=$(pwd)/../../../
cd ../../../src
make clean 2> /dev/null || true

#~ make distclean 2> /dev/null || true
#~ cd megacmd
#~ make distclean 2> /dev/null || true
#~ cd mega
#~ make distclean 2> /dev/null || true
#~ rm -fr bindings/qt/3rdparty || true
#~ ./clean.sh || true
cd $cwd

# download software archives
archives=$cwd/archives
rm -fr $archives
mkdir $archives
$BASEPATH/contrib/build_sdk.sh -q -e -g -w -s -v -u -o $archives

# get current version
megacmd_VERSION=$(cat $BASEPATH/examples/megacmd/megacmdversion.h  | grep define | grep _VERSION | grep -v CODE | head -n 3 | awk 'BEGIN{ORS=""; first=1}{if(first){first=0;}else{print ".";}print $3}')
export megacmd_NAME=megacmd-$megacmd_VERSION
rm -rf $megacmd_NAME.tar.gz
rm -rf $megacmd_NAME

echo "megacmd version: $megacmd_VERSION"

# delete previously generated files
rm -fr megacmd/megacmd*.dsc

# fix version number in template files and copy to appropriate directories
sed -e "s/megacmd_VERSION/$megacmd_VERSION/g" templates/megacmd/megacmd.spec > megacmd/megacmd.spec
sed -e "s/megacmd_VERSION/$megacmd_VERSION/g" templates/megacmd/megacmd.dsc > megacmd/megacmd.dsc
sed -e "s/megacmd_VERSION/$megacmd_VERSION/g" templates/megacmd/PKGBUILD > megacmd/PKGBUILD

# read the last generated ChangeLog version
version_file="version"

if [ -s "$version_file" ]; then
    last_version=$(cat "$version_file")
else
    last_version="none"
fi

if [ "$last_version" != "$megacmd_VERSION" ]; then
    # add RPM ChangeLog entry
    changelog="megacmd/megacmd.changes"
    changelogold="megacmd/megacmd.changes.old"
    if [ -f $changelog ]; then
        mv $changelog $changelogold
    fi
    ./generate_rpm_changelog_entry.sh $megacmd_VERSION $BASEPATH/examples/megacmd/megacmdversion.h > $changelog #TODO: read this from somewhere
    if [ -f $changelogold ]; then
        cat $changelogold >> $changelog
        rm $changelogold
    fi

    # add DEB ChangeLog entry
    changelog="megacmd/debian.changelog"
    changelogold="megacmd/debian.changelog.old"
    if [ -f $changelog ]; then
        mv $changelog $changelogold
    fi
    ./generate_deb_changelog_entry.sh $megacmd_VERSION $BASEPATH/examples/megacmd/megacmdversion.h > $changelog #TODO: read this from somewhere
    if [ -f $changelogold ]; then
        cat $changelogold >> $changelog
        rm $changelogold
    fi

    # update version file
    echo $megacmd_VERSION > $version_file
fi

# create archive
mkdir $megacmd_NAME
ln -s ../megacmd/megacmd.spec $megacmd_NAME/megacmd.spec
ln -s ../megacmd/debian.postinst $megacmd_NAME/debian.postinst
ln -s ../megacmd/debian.postrm $megacmd_NAME/debian.postrm
ln -s ../megacmd/debian.copyright $megacmd_NAME/debian.copyright

#for i in $BASEPATH/{aclocal.m4,autogen.sh,autom4te.cache,bindings,clean.sh,config.guess,config.log,config.status,config.sub,configure,configure.ac,contrib,depcomp,doc,examples,include,install-sh,libmega.pc,libmega.pc.in,libtool,LICENSE,logo.png,ltmain.sh,m4,Makefile,Makefile.am,Makefile.in,Makefile.win32,missing,py-compile,README.md,src,test-driver,tests,third_party}; do
for i in $BASEPATH/{autogen.sh,m4,configure.ac,include,Makefile.am,src,tests,bindings,libmega.pc.in,doc}; do
	ln -s $i $megacmd_NAME/
done
mkdir -p $megacmd_NAME/examples/megacmd
for i in $BASEPATH/examples/{include.am,megacli.*,megasimple*}; do 
    ln -s $i $megacmd_NAME/examples/
done    
for i in $BASEPATH/examples/megacmd/{*cpp,*.h,client,megacmdshell}; do
	ln -s $i $megacmd_NAME/examples/megacmd/
done


mkdir -p $megacmd_NAME/contrib/
ln -s $BASEPATH/contrib/build_sdk.sh $megacmd_NAME/contrib/

ln -s $archives $megacmd_NAME/archives
tar czfh $megacmd_NAME.tar.gz $megacmd_NAME
rm -rf $megacmd_NAME

# delete any previous archive
rm -fr megacmd/megacmd_*.tar.gz
# transform arch name, to satisfy Debian requirements
mv $megacmd_NAME.tar.gz megacmd/megacmd_$megacmd_VERSION.tar.gz

#get md5sum and replace in PKGBUILD
MD5SUM=`md5sum megacmd/megacmd_$megacmd_VERSION.tar.gz | awk '{print $1}'`
sed "s/MD5SUM/$MD5SUM/g"  -i megacmd/PKGBUILD

######
######
rm -fr $archives
