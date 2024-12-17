### fuse-disable
Disables a specified fuse mount.

Usage: `fuse-disable (--by-name=name | --by-path=path) [--remember]`
<pre>
After a mount has been disabled, its cloud entities will no longer be accessible via the mount's local path.

Options:
 --by-name=name Specifies which mount by name.
 --by-path=path Specifies which mount by path.
 --remember     Specifies whether to remember that this mount is disabled. Note that if this option is specified
                on a transient mount, that mount will become persistent.
</pre>
