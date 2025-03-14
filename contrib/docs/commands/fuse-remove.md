### fuse-remove
Deletes a specified FUSE mount.

Usage: `fuse-remove (ID|localPath|name)`
<pre>
A mount must be disabled before it can be removed. See fuse-disable.

Parameters:
ID|localPath|name   The identifier of the mount we want to remove. It can be one of the following:
                         ID: The unique identifier for the mount.
                         Local path: The local mount point in the filesystem.
                         Name: the user-friendly name of the mount, specified when it was added or by fuse-config.

Note: FUSE commands are in early BETA. They're only available on Linux. If you experience any issues, please contact support@mega.nz.
</pre>
