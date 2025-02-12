# How to build MEGAcmd for Synology NAS drives.

We use docker to cross-compile MEGAcmd for Synology. To build for a specific platform, run:
```
docker build -t megacmd-dms-build-env -f $PWD/build/SynologyNAS/synology-cross-build.dockerfile $PWD --build-arg PLATFORM=<platform>
```
The `-t` argument tags the container with a name, whereas `-f` specifies the dockerfile. Note that the working directory has to be the MEGAcmd repository.

The possible platforms are:
```
alpine alpine4k apollolake armada37xx armada38x avoton broadwell broadwellnk broadwellnkv2 broadwellntbap bromolow braswell denverton epyc7002 geminilake grantley kvmx64 monaco purley r1000 rtd1296 rtd1619b v1000
```

After building successfully, we need to run the container so we can extract the generated package. To do so, run:
```
docker run -d --name megacmd-dms megacmd-dms-build-env
```
Then, copy the package folder to the current directory with:
```
docker cp megacmd-dms:/image/<platform> .
```
After copying, we can stop and remove the container if needed:
```
docker rm -f megacmd-dms
```

The script `build/SynologyNAS/generate_pkg.sh` automates all of these steps, creating the necessary docker container, extracting the package from it, and deleting it afterwards. In this case, the package will be extracted to `build/SynologyNAS/packages`.

To install the package on your NAS device, login using the web interface. Navigate to the Package Manager, and click Manual Install. This will open up a menu which lets you choose a local file. Select your generated .pkg file, and install.

To actually run MEGAcmd, you'll need to enable a telnet or SSH connection in the Synology NAS, and run a remote terminal on it. Executables are available at `/volume1/@appstore/megacmdpkg`.

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

