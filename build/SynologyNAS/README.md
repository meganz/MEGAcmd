# How to build MEGAcmd for Synology NAS drives.

We follow the cross compiling system provided by Synology, described in their developer-guide.pdf (also at https://developer.synology.com/developer-guide/getting_started/index.html).
Use ubuntu 16.04 as the build machine - others may work but that one does for sure.

**All commands need to be run as 'sudo'**.  We are going to set up a root folder /toolkit, and work out of that.  The cross-compile system uses chroot to build inside a subfolder of that.

#copy this folder tree into /toolkit
mkdir /toolkit
cp -r ...git.../MEGAcmd/build/SynologyNAS/toolkit/* /toolkit/
cd /toolkit

#copy the MEGACmd and sdk into it also.  The sdk should already be in the 'sdk' subfolder of MEGAcmd
cp -r ...git.../MEGAcmd /toolkit/source/megacmdpkg/

#get the Synology scripts
git clone https://github.com/SynologyOpenSource/pkgscripts-ng

#download and install the cross-compile build enviroment for one or many targets.  Using armada38x here as that's the chip for a DS218j NAS.  6.1 is their OS version.
./pkgscripts-ng/EnvDeploy -v 6.1 -p armada38x

#adjust the package details, including version number.  Adjust OS version in 'depends' if building for a later OS than 6.1
vi ./source/megacmdpkg/INFO.sh
vi ./source/megacmdpkg/SynoBuildConf/depends

#run the build process
./pkgscripts-ng/PkgCreate.py megacmdpkg
#results can now be seen in /toolkit/build_env/ds.armada38x-6.1/source/megacmdpkg/MEGAcmd

#package up the executables and scripts, provided the build actually succeeded (though it may say it failed even though we force 'exit 0' from the build script)
#For official releases we may sign with an RSA key at this point, but were we are skipping signing with -S
./pkgscripts-ng/PkgCreate.py -i -S megacmdpkg
#package is now available at /toolkit/build_env/ds.armada38x-6.1/image/packages/
#install on your NAS from its web browser interface, Package Manager, Manual Install button which lets you choose a local file

#To actually run it, you'll need to enable telnet connections in the Synology NAS, then telnet to it.   PuTTY works nicely for this.
#Executables are available at /volume1/@appstore/MEGAcmd/usr/local/bin
#If you want to make a clean build after making changes inside /toolkit/source, then get rid of this (substitute for platform), and re-run EnvDeploy:
rm -rf /toolkit/build_env/ds.armada38x-6.1
