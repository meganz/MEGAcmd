### fuse-config
Modifies the specified FUSE mount configuration.

Usage: `fuse-config [--name=name] [--enable-at-startup=yes|no] [--persistent=yes|no] [--read-only=yes|no] (ID|localPath|name)`
<pre>
Parameters:
ID|localPath|name   The identifier of the mount we want to configure. It can be one of the following:
                         ID: The unique identifier for the mount.
                         Local path: The local mount point in the filesystem.
                         Name: the user-friendly name of the mount, set when it was added or by fuse-config.

Options:
 --name=name                  Sets the name of the mount.
 --enable-at-startup=yes|no   Controls whether or not the mount should be enabled automatically on startup.
 --persistent=yes|no          Controls whether or not the mount is saved across restarts.
 --read-only=yes|no           Controls whether the mount is read-only or writable.

Note: FUSE commands are only available on Linux.
</pre>
