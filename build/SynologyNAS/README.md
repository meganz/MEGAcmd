# How to build MEGAcmd for Synology NAS drives.  

We follow the cross compiling system provided by Synology, described in their [developer-guide.pdf](https://global.download.synology.com/download/Document/DeveloperGuide/DSM_Developer_Guide.pdf) (also at https://originhelp.synology.com/developer-guide/getting_started/index.html).  
Tested with ubuntu 16.04 as the build machine - others may work but that one does for sure.  
  
**All commands need to be run as 'sudo'**.  We are going to set up a root folder /toolkit, and work out of that.  The cross-compile system uses chroot to build inside a subfolder of that.  

Copy this folder tree into /toolkit  
`cp -r ...git.../MEGAcmd/build/SynologyNAS/toolkit /`  
`cd /toolkit`  
  
Copy the MEGACmd folder (and sdk subfolder) into it also.  The sdk should already be in the 'sdk' subfolder of MEGAcmd  
`cp -r ...git.../MEGAcmd /toolkit/source/MEGAcmd/`  
  
Get the Synology scripts  
`git clone https://github.com/SynologyOpenSource/pkgscripts-ng`  

Download and install the cross-compile build enviroment for one or many targets.  Using armada38x here for the platform, as that's the chip for a DS218j NAS which was tested first.  6.1 is the NAS OS version.  
`./pkgscripts-ng/EnvDeploy -v 6.1 -p armada38x`  
  
Adjust the package details, including version number.  Adjust OS version in 'depends' if building for a later OS than 6.1  
`vi ./source/MEGAcmd/INFO.sh`  
`vi ./source/MEGAcmd/SynoBuildConf/depends`  
  
Run the build process  
`./pkgscripts-ng/PkgCreate.py MEGAcmd`  
Results can now be seen in /toolkit/build_env/ds.armada38x-6.1/source/MEGAcmd/MEGAcmd  
  
Package up the executables and scripts, provided the build actually succeeded (though it may say it failed even though we force 'exit 0' from the build script)  
For official releases we may sign with an RSA key at this point, but were we are skipping signing with -S  
`./pkgscripts-ng/PkgCreate.py -i -S MEGAcmd`  
Package is now available at /toolkit/build_env/ds.armada38x-6.1/image/packages/  
Install on your NAS from its web browser interface, Package Manager, Manual Install button which lets you choose a local file  
  
To actually run it, you'll need to enable telnet connections in the Synology NAS, then telnet to it.   PuTTY works nicely for this.  
Executables are available at /volume1/@appstore/MEGAcmd/usr/local/bin   
If you want to make a clean build after making changes inside /toolkit/source, then get rid of this (substitute the platform appropriately), and re-run EnvDeploy:  
`rm -rf /toolkit/build_env/ds.armada38x-6.1`  
  
# Notes

**Writable Home Directory**

MEGAcmd requires a user's home directory to be writable.

If you intend to use MEGAcmd as a non-root user on your Synology device,
make sure you've correctly set up a home directory for your user.

Generally this will require formatting a volume and configuring Synology's
"User Home Service."

You can learn more here:
[Synology DSM - Advanced User Settings](https://www.synology.com/en-global/knowledgebase/DSM/help/DSM/AdminCenter/file_user_advanced)

**Limit on Inotify Watches**

MEGAcmd makes use of the Linux kernel's inotify functionality to monitor the
filesystem for changes. Unfortunately, inotify watches are relatively
expensive constructs and so the Linux kernel imposes a configurable limit to
how many can be established.

How large this limit is depends on the machine running Linux.
Synology NAS, being embedded devices, have a somewhat small limit.

If you find yourself having trouble synchronizing large directory trees, you
can try increasing this limit using Linux's sysctl command.

To view the current limit, you can issue the following command:
`sysctl fs.inotify.max_user_watches`

To temporarily alter the limit, you can issue this command as root:
`sysctl fs.inotify.max_user_watches=131072`

To permanently alter the limit, you will need to edit /etc/sysctl.conf.

You can learn about sysctl.conf's format [here](https://man7.org/linux/man-pages/man5/sysctl.conf.5.html)

