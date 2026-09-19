# MEGA-FUSE — Serve your files as a "Filesystem in Userspace" (FUSE) with MEGAcmd
This is a brief tutorial on how to configure and manage [FUSE](https://en.wikipedia.org/wiki/Filesystem_in_Userspace) with MEGAcmd.

Configuring a FUSE mount will let you access your MEGA files as if they were located in your computer. After enabling a FUSE mount, you can use your favourite tools to browse, play, and edit your MEGA files.

Note: this functionality is in beta, and not supported on macOS.

## Reading files
Files are downloaded as you read them. Opening a file does not download it first: a read returns as
soon as the bytes it asked for have arrived, and reading from one point keeps the download going
forward while you keep reading. Only the parts you read are kept on disk.

A file you modify is the exception. The rest of it is downloaded before the new version can be
uploaded.

### What a mount cannot check
When you download a file with [`get`](commands/get.md), MEGAcmd checks the whole file against the
checksum held in its key, and the download fails if they do not match. That check needs every byte,
so it cannot be done while reading through a mount, which fetches only the parts you use. Reads are
still encrypted in transit and decrypted with your key; what a mount cannot tell you is whether the
bytes are the ones that were uploaded.

The same is true of [webdav and ftp](WEBDAV.md) streaming. Before you run or install something,
fetch it with `get` and use that copy.

## Local cache
The content read or written through a mount is kept on disk, in the `file-service` folder of the
MEGAcmd configuration folder: `$HOME/.megaCmd/file-service` (`%LOCALAPPDATA%\MEGAcmd\.megaCmd\file-service`
in Windows), under a subfolder named after your session. [Webdav streaming](WEBDAV.md) uses the same
cache.

The cache is required for mounts to work. It is cleaned up on a timer, and the
`file_service_reclaim_*` keys of the [`configure`](commands/configure.md) command set the limits; see
`help --streaming`. To free space sooner, lower `file_service_reclaim_age_threshold` and
`file_service_reclaim_period`. A cleanup only drops content that is already in your cloud, so it
never loses a file you have just written.

A full `logout` also removes the cached content. `logout --keep-session` and restarting the MEGAcmd
Server keep it: the session is resumed and the same cache is used again.

If you used a FUSE mount with an earlier MEGAcmd, the `fuse-cache` folder beside it is left over
from that version. Nothing uses it any more and it can be deleted.

### Important caveats
Files and folders created locally may not be immediately available in your MEGA cloud: the mount
engine will transfer them transparently in the background. A file you write starts uploading at
least 30 seconds after your last write to it, and longer for large files. Closing the file does not
start the upload; `fsync` does.

The MEGAcmd Server needs to be running for these uploads to complete. Stopping it is safe, they
resume on the next start. Logging out is not: anything not uploaded yet is discarded.

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
