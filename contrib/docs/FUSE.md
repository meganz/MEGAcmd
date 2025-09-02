# MEGA-FUSE — Serve your files as a "Filesystem in Userspace" (FUSE) with MEGAcmd
This is a brief tutorial on how to configure and manage [FUSE](https://en.wikipedia.org/wiki/Filesystem_in_Userspace) with MEGAcmd.

Configuring a FUSE mount will let you access your MEGA files as if they were located in your computer. After enabling a FUSE mount, you can use your favourite tools to browse, play, and edit your MEGA files.

Note: this functionality is in beta, and not supported on macOS.

## Local cache
MEGAcmd will use a cache to place files while working with mount points. This cache will be used to store both files downloaded from MEGA, and files being uploaded to MEGA. It will be created automatically in `$HOME/.megaCmd/fuse-cache` (`%LOCALAPPDATA%\MEGAcmd\.megaCmd\fuse-cache` in Windows).

Bear in mind that this cache is fundamental to be able to work with FUSE mounts. Cache files are removed automatically. Restarting the MEGAcmd Server may help reduce the space used by the cache.

### Important caveats
Files and folders created locally may not be immediately available in your MEGA cloud: the mount engine will transfer them transparently in the background. Note that the MEGAcmd Server needs to be running in order for these actions to complete.

## Streaming
Currently, streaming is not directly supported. In order to open files in a FUSE mount point, they need to be downloaded completely to the local cache folder. You will need space in your hard drive to accomodate for these files.

## Usage
### Creating a new mount
You can create a new FUSE mount with:
```
$ fuse-add /local/path/to/fuse/mountpoint /cloud/dir
```

After creating the mount, MEGAcmd will try to enable it; if it fails, the mount will remain disabled. See [`fuse-add`](commands/fuse-add.md) for all the possible options and arguments.

The mount will have an associated name that we can use to refer to it when managing it. The name must be unique. A custom name can be chosen on creation with the `--name=custom_name` argument. We might also refer to a mount by its local path, but this is not necessarily unique: if multiple mounts share the same local path, we *must* use the name so we can distinguish between them.

### Displaying mounts
The list of existing mounts can be displayed with:
```
$ fuse-show
NAME LOCAL_PATH                     REMOTE_PATH PERSISTENT ENABLED
dir /local/path/to/fuse/mountpoint  /cloud/dir  YES        YES
```

Use `fuse-show <NAME|LOCAL_PATH>` to get further details on a specific mount. See [`fuse-show`](commands/fuse-show.md) for the list of all possible options and arguments.

### Enabling/disabling mounts
To make your cloud files available in your local filesystem, the mount must be enabled. We can do so with:
```
$ fuse-enable <NAME|LOCAL_PATH>
```

Note: mounts are enabled when created by default (unless `--disabled` is used).

Similarly, we can stop exposing our cloud files locally with:
```
$ fuse-disable <NAME|LOCAL_PATH>
```

Note: disabled mounts still exist and are shown in `fuse-show`. See [`fuse-enable`](commands/fuse-enable.md) and [`fuse-disable`](commands/fuse-disable.md) for more information on these commands.

### Adjusting configuration
As we've mentioned before, the full details of a mount can be displayed with:
```
$ fuse-show dir
Showing details of mount "dir"
  Local path:         /local/path/to/fuse/mountpoint
  Remote path:        /cloud/dir
  Name:               dir
  Persistent:         YES
  Enabled:            YES
  Enable at startup:  YES
  Read-only:          NO
```

To configure these flags, we can use the [`fuse-config`](commands/fuse-config.md) command. For example, to make the mount read-only, we would do:
```
$ fuse-config --read-only=yes dir
Mount "dir" now has the following flags
  Name:               dir
  Enable at startup:  YES
  Persistent:         YES
  Read-only:          YES
```

### Removing mounts
A mount can be removed with:
```
$ fuse-remove <NAME|LOCAL_PATH>
```
Note: mounts must be disabled before they can be removed.

## Issues
Occasionally, you may encounter issues in FUSE mounts that cannot be directly resolved within MEGAcmd (for example, if the MEGAcmd server was closed abruptly). The most common one is: "Error: cannot access '/local/path/to/fuse/mountpoint': Transport endpoint is not connected". To fix them, in Linux, you might need to use the `fusermount` command like so:
```
fusermount -u /local/path/to/fuse/mountpoint
```

Adding `-z` may help if the above fails. See the [fusermount man page](https://www.man7.org/linux/man-pages/man1/fusermount3.1.html).
