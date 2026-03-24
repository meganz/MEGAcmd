### fuse-config
Modifies the specified FUSE mount configuration.

Usage: `fuse-config [--name=name] [--enable-at-startup=yes|no] [--persistent=yes|no] [--read-only=yes|no] (name|localPath)`
<pre>
Parameters:
 name|localPath   The identifier of the mount we want to remove. It can be one of the following:
                   Name: the user-friendly name of the mount, specified when it was added or by fuse-config.
                   Local path: The local mount point in the filesystem.

Options:
 --name=name                  Sets the friendly name used to uniquely identify the mount.
 --enable-at-startup=yes|no   Controls whether or not the mount should be enabled automatically on startup.
 --persistent=yes|no          Controls whether or not the mount is saved across restarts.
 --read-only=yes|no           Controls whether the mount is read-only or writable.

Note: FUSE commands are in early BETA. They are not available in macOS. If you experience any issues, please contact support@mega.nz.
</pre>
