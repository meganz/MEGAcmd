# How to build MEGAcmd for Synology NAS drives.

We follow the cross compiling system provided by Synology, described in their developer-guide.pdf (also at https://developer.synology.com/developer-guide/getting_started/index.html).
Tested with ubuntu 16.04 as the build machine - others may work but that one does for sure.

**All commands need to be run as 'sudo'**.  We are going to set up a root folder /toolkit, and work out of that.  The cross-compile system uses chroot to build inside a subfolder of that.

Copy this folder tree into /toolkit
`cp -r ...git.../MEGAcmd/build/SynologyNAS/toolkit /`
`cd /toolkit`

Copy the MEGACmd folder (and sdk subfolder) into it also.  The sdk should already be in the 'sdk' subfolder of MEGAcmd
`cp -r ...git.../MEGAcmd /toolkit/source/megacmdpkg/`

Get the Synology scripts
`git clone https://github.com/SynologyOpenSource/pkgscripts-ng`

Download and install the cross-compile build enviroment for one or many targets.  Using armada38x here for the platform, as that's the chip for a DS218j NAS which was tested first.  6.1 is the NAS OS version.
`./pkgscripts-ng/EnvDeploy -v 6.1 -p armada38x`

Adjust the package details, including version number.  Adjust OS version in 'depends' if building for a later OS than 6.1
`vi ./source/megacmdpkg/INFO.sh`
`vi ./source/megacmdpkg/SynoBuildConf/depends`

Run the build process
`./pkgscripts-ng/PkgCreate.py megacmdpkg`
Results can now be seen in /toolkit/build_env/ds.armada38x-6.1/source/megacmdpkg/MEGAcmd

Package up the executables and scripts, provided the build actually succeeded (though it may say it failed even though we force 'exit 0' from the build script)
For official releases we may sign with an RSA key at this point, but were we are skipping signing with -S
`./pkgscripts-ng/PkgCreate.py -i -S megacmdpkg`
Package is now available at /toolkit/build_env/ds.armada38x-6.1/image/packages/
Install on your NAS from its web browser interface, Package Manager, Manual Install button which lets you choose a local file

To actually run it, you'll need to enable telnet connections in the Synology NAS, then telnet to it.   PuTTY works nicely for this.
Executables are available at /volume1/@appstore/MEGAcmd/usr/local/bin
If you want to make a clean build after making changes inside /toolkit/source, then get rid of this (substitute the platform appropriately), and re-run EnvDeploy:
`rm -rf /toolkit/build_env/ds.armada38x-6.1`
