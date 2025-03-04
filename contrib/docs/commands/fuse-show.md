### fuse-show
Displays the list of FUSE mounts and their information. If an ID, local path, or name is provided, displays information of that mount instead.

Usage: `fuse-show [--only-enabled] [--disable-path-collapse] [[--limit=rowcount] | [ID|localPath|name]]`
<pre>
When all mounts are shown, the following columns are displayed:
   MOUNT_ID: A unique alphanumeric string that identifies the mount.
   LOCAL_PATH: The local mount point in the filesystem.
   REMOTE_PATH: The cloud directory or share that is exposed locally.
   NAME: The user-friendly name of the mount, specified when it was added or by fuse-config.
   ENABLED: If the mount is currently enabled, "YES". Otherwise, "NO".

Parameters:
ID|localPath|name   The identifier of the mount we want to show. It can be one of the following:
                         ID: A unique alphanumeric string that identifies the mount.
                         Local path: The local mount point in the filesystem.
                         Name: the user-friendly name of the mount, set when it was added or by fuse-config.
                    If not provided, the list of mounts will be shown instead.

Options:
 --only-enabled           Only shows mounts that are enabled.
 --disable-path-collapse  Ensures all paths are fully shown. By default long paths are truncated for readability.
 --limit=rowcount         Limits the amount of rows displayed. Set to 0 to display unlimited rows. Default is unlimited.
 --col-separator=X	Uses the string "X" as column separator. Otherwise, spaces will be added between columns to align them.
 --output-cols=COLUMN_NAME_1,COLUMN_NAME2,...	Selects which columns to show and their order.

Note: FUSE commands are only available on Linux.
</pre>
