#!/bin/bash
# This scripts performs the build for all supported Synology platforms
set -ex
thisdir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

builddir=/toolkit
rm -rf $builddir

# Clone MEGAcmd into the build dir
mkdir -p $builddir/source/megacmdpkg
cd $builddir/source/megacmdpkg
git clone --recursive https://github.com/meganz/MEGAcmd.git

# Copy toolkit files into place and clone Synology scripts
cd $builddir
cp -r $builddir/source/megacmdpkg/MEGAcmd/build/SynologyNAS/toolkit/* .
git clone https://github.com/SynologyOpenSource/pkgscripts-ng

# Create a link to the toolkit tarballs
ln -s /toolkit_tarballs toolkit_tarballs

# Perform the actual build
./build_all_synology_packages || true

# Collect results
resultdir=synology_build_results

rm -rf $thisdir/images
mkdir $thisdir/images
cp -r $resultdir/image-* $thisdir/images

rm -f $thisdir/result
touch $thisdir/result
echo `ls -1 $resultdir/*.failed` >> $thisdir/result
echo `ls -1 $resultdir/*.built` >> $thisdir/result
