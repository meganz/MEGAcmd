# How to build MEGAcmd for QNAP NAS drives.  

We follow the cross compiling system provided by QNAP, described at (https://www.qnap.com/event/dev/en/p_qdk.php#qdk_btn1_show) 

Tested with ubuntu 16.04 as the build machine - others may work but that one does for sure.  

There are two main steps.   First, cross-compiling the executables, which can be done on Ubuntu 16.04 for example.   Second, preparing a QNAP package including those binaries, which must be done on a QNAP NAS device.  


# Cross-compiling 

This example is for ARM, x86 and x86_64 will be similar.

Cross compiling machine requirements:
  - **Most or all commands need to be run as 'sudo'**.  
  - dos2unix
  - cross compiler from QNAP: install by following QNAP instructions to unzip cross-project-arm-20110901.tar.gz in your `/opt/cross-project` folder.

Then copy or git clone the MEGAcmd project to /opt/cross-project/arm/marvell/arm-none-linux-gnueabi/libc/marvell-f/src/MEGAcmd.  The sdk must be copied or cloned to the 'sdk' subfolder of that.

From the MEGAcmd folder, execute:
`./build/QNAP/build`
which will result in the binaries and scripts ending up in the MEGAcmd/install folder.


# Building a QNAP package

This must be done on a QNAP NAS as it requires their QDK package.  Download QDK_2.2.16.qpkg and use the QNAP package manager to manually install that.

Next, use `scp` or a similar tool to copy the MEGAcmd/build/QNAP_NAS/megacmdpkg folder to the NAS, followed by the contents of the MEGAcmd/install folder:
`scp -r build/QNAP_NAS/megacmdpkg admin@10.12.0.248:/share/CACHEDEV1_DATA/.qpkg/QDK/`
`scp -r install/* admin@10.12.0.248:/share/CACHEDEV1_DATA/.qpkg/QDK/megacmdpkg/arm-x41`

On the NAS, using ssh or similar, edit the qpkg.cfg to adjust version numbers etc and then run the `qbuild` command from the megacmdpkg folder.   The package will be built and put in the 'build' subfolder.


