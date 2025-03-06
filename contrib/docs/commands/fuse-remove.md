### fuse-remove
Deletes a specified FUSE mount.

Usage: `fuse-remove (ID|localPath|name)`
<pre>
A mount must be disabled before it can be removed. See fuse-disable.

Parameters:
ID|localPath|name   The identifier of the mount we want to remove. It can be one of the following:
                         ID: The unique alphanumeric string that identifies the mount.
                         Local path: The local mount point in the filesystem.
                         Name: the user-friendly name of the mount, specified when it was added or by fuse-config.

Note: FUSE commands are only available on Linux.
</pre>
