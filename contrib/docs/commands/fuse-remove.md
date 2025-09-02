### fuse-remove
Deletes a specified FUSE mount.

Usage: `fuse-remove (name|localPath)`
<pre>
Parameters:
 name|localPath   The identifier of the mount we want to remove. It can be one of the following:
                   Name: the user-friendly name of the mount, specified when it was added or by fuse-config.
                   Local path: The local mount point in the filesystem.

Note: FUSE commands are in early BETA. They are not available in macOS. If you experience any issues, please contact support@mega.nz.
</pre>
