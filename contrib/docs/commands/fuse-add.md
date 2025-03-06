### fuse-add
Creates a new FUSE mount.

Usage: `fuse-add [--name=name] [--disabled] [--transient] [--read-only] localPath remotePath`
<pre>
Mounts are automatically enabled after being added, making the chosen MEGA folder accessible within the local filesystem.
When a mount is disabled, its configuration will be saved, but the cloud folder will not be mounted locally (see fuse-disable).
Mounts are persisted after restarts and writable by default. You may change these and other options of a FUSE mount with fuse-config.
Use fuse-show to display the list of mounts.

Parameters:
 localPath    Specifies where the files contained by remotePath should be visible on the local filesystem.
 remotePath   Specifies what directory (or share) should be exposed on the local filesystem.

Options:
 --name=name  A user friendly name which the mount can be identified by. If not provided, the display name
              of the entity specified by remotePath will be used. If remotePath specifies the entire cloud
              drive, the mount's name will be "MEGA". If remotePath specifies the rubbish bin, the mount's
              name will be "MEGA Rubbish".
 --read-only  Specifies that the mount should be read-only. Otherwise, the mount is writable.
 --transient  Specifies that the mount should be transient, meaning it will be lost on restart.
              Otherwise, the mount is persistent, meaning it will remain across on restarts.
 --disabled   Specifies that the mount should not enabled after being added, and must be enabled manually. See fuse-enable.
              If this option is passed, the mount will not be automatically enabled at startup.

Note: FUSE commands are only available on Linux.
</pre>
