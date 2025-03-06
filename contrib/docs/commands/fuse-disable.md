### fuse-disable
Disables a specified FUSE mount.

Usage: `fuse-disable [--temporarily] (ID|localPath|name)`
<pre>
After a mount has been disabled, its cloud entities will no longer be accessible via the mount's local path.

Parameters:
ID|localPath|name   The identifier of the mount we want to disable. It can be one of the following:
                         ID: A unique alphanumeric string that identifies the mount.
                         Local path: The local mount point in the filesystem.
                         Name: the user-friendly name of the mount, set when it was added or by fuse-config.
Options:
 --temporarily   Specifies whether the mount should be disabled only until the server is restarted.
                 Has no effect on transient mounts, since any action on them is always temporary.

Note: FUSE commands are only available on Linux.
</pre>
