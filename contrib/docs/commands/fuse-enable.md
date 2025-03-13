### fuse-enable
Enables a specified FUSE mount.

Usage: `fuse-enable [--temporarily] (ID|localPath|name)`
<pre>
After a mount has been enabled, its cloud entities will be accessible via the mount's local path.

Parameters:
ID|localPath|name   The identifier of the mount we want to enable. It can be one of the following:
                         ID: The unique identifier for the mount.
                         Local path: The local mount point in the filesystem.
                         Name: the user-friendly name of the mount, set when it was added or by fuse-config.

Options:
 --temporarily   Specifies whether the mount should be enabled only until the server is restarted.
                 Has no effect on transient mounts, since any action on them is always temporary.

Note: FUSE commands are in early BETA. They're only available on Linux. If you experience any issues, please contact support@mega.nz.
</pre>
