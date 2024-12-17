### fuse-set
Sets the specified FUSE mount configurations.

Usage: `fuse-set (--by-name=name | --by-path=path) [--disabled-at-startup | --enabled-at-startup] [--name=name] [--persistent | --transient] [--read-only | --writable]`
<pre>
Options:
 --by-name=name Specifies which mount by name.
 --by-path=path Specifies which mount by path.
 --disabled-at-startup Disables the mount on startup, meaning the mount will not be active on restart.
 --enabled-at-startup  Enables the mount on startup, meaning the mount will be active on restart.
 --name=name           Sets the name of the mount.
 --persistent          Sets the mount as persistent, meaning it will remain across on restarts.
 --transient           Sets the mount as transient, meaning it will be lost on restart.
 --read-only           Sets the mount as read only.
 --writable            Sets the mount as writable.
</pre>
