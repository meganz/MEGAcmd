### fuse-enable
Enables a specified FUSE mount.

Usage: `fuse-enable [--temporarily] (name|localPath)`
<pre>
After a mount has been enabled, its cloud entities will be accessible via the mount's local path.

Parameters:
 name|localPath   The identifier of the mount we want to remove. It can be one of the following:
                   Name: the user-friendly name of the mount, specified when it was added or by fuse-config.
                   Local path: The local mount point in the filesystem.

Options:
 --temporarily   Specifies whether the mount should be enabled only until the server is restarted.
                 Has no effect on transient mounts, since any action on them is always temporary.

Note: FUSE commands are in early BETA. They are not available in macOS. If you experience any issues, please contact support@mega.nz.
</pre>
