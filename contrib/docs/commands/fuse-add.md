### fuse-add
Adds a new FUSE mount to the database.

Usage: `fuse-add [--name=name] [--transient] [--read-only] localPath remotePath`
<pre>
Mounts must be added to the database before they can be enabled. See fuse-enable.
Mounts are persistent and writable by default.

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
</pre>
