#!/bin/bash -x

##
 # @file src/build/create_tarball.sh
 # @brief Generates megacmd tarballs and compilation scripts
 #
 # (c) 2013-2014 by Mega Limited, Auckland, New Zealand
 #
 # This file is part of the MEGAcmd.
 #
 # MEGAcmd is distributed in the hope that it will be useful,
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
BASEPATH=$(pwd)/../

# get current version
megacmd_VERSION=$(cat $BASEPATH/src/megacmdversion.h  | grep define | grep _VERSION | grep -v CODE | head -n 3 | awk 'BEGIN{ORS=""; first=1}{if(first){first=0;}else{print ".";}print $3}')
export megacmd_NAME=megacmd-$megacmd_VERSION
rm -rf $megacmd_NAME.tar.gz
rm -rf $megacmd_NAME

echo "megacmd version: $megacmd_VERSION"

# delete previously generated files
rm -fr megacmd/megacmd*.dsc

# fix version number in template files and copy to appropriate directories
sed -e "s/megacmd_VERSION/$megacmd_VERSION/g" templates/megacmd/megacmd.spec | sed "s#^ *##g" > megacmd/megacmd.spec
sed -e "s/megacmd_VERSION/$megacmd_VERSION/g" templates/megacmd/megacmd.dsc > megacmd/megacmd.dsc
sed -e "s/megacmd_VERSION/$megacmd_VERSION/g" templates/megacmd/PKGBUILD > megacmd/PKGBUILD
for dscFile in `find templates/megacmd/ -name megacmd-xUbuntu_* -o -name megacmd-Debian_* -o -name megacmd-Raspbian_*`; do
    sed -e "s/megacmd_VERSION/$megacmd_VERSION/g" "${dscFile}" > megacmd/`basename ${dscFile}`
done

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
    ./generate_rpm_changelog_entry.sh $megacmd_VERSION $BASEPATH/src/megacmdversion.h > $changelog #TODO: read this from somewhere
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
    ./generate_deb_changelog_entry.sh $megacmd_VERSION $BASEPATH/src/megacmdversion.h > $changelog #TODO: read this from somewhere
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
ln -s ../megacmd/debian.prerm $megacmd_NAME/debian.prerm
ln -s ../megacmd/debian.postrm $megacmd_NAME/debian.postrm
ln -s ../megacmd/debian.copyright $megacmd_NAME/debian.copyright

for i in $BASEPATH/{src,sdk}; do
	ln -s $i $megacmd_NAME/
done

tar czfh $megacmd_NAME.tar.gz --exclude-vcs $megacmd_NAME
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
