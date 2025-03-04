### fuse-enable
Enables a specified FUSE mount.

Usage: `fuse-enable [--remember] (ID|localPath|name)`
<pre>
After a mount has been enabled, its cloud entities will be accessible via the mount's local path.

Parameters:
ID|localPath|name   The identifier of the mount we want to enable. It can be one of the following:
                         ID: A unique alphanumeric string that identifies the mount.
                         Local path: The local mount point in the filesystem.
                         Name: the user-friendly name of the mount, set when it was added or by fuse-config.

Options:
 --remember   Specifies whether to remember that this mount is enabled. Note that if this option is specified
              on a transient mount, that mount will become persistent.

Note: FUSE commands are only available on Linux.
</pre>
