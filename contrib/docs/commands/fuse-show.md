### fuse-show
Displays the list of FUSE mounts and their information. If a name or local path provided, displays information of that mount instead.

Usage: `fuse-show [--only-enabled] [--disable-path-collapse] [[--limit=rowcount] | [name|localPath]]`
<pre>
When all mounts are shown, the following columns are displayed:
   NAME: The user-friendly name of the mount, specified when it was added or by fuse-config.
   LOCAL_PATH: The local mount point in the filesystem.
   REMOTE_PATH: The cloud directory or share that is exposed locally.
   PERSISTENT: If the mount is saved across restarts, "YES". Otherwise, "NO".
   ENABLED: If the mount is currently enabled, "YES". Otherwise, "NO".

Parameters:
 name|localPath   The identifier of the mount we want to remove. It can be one of the following:
                   Name: the user-friendly name of the mount, specified when it was added or by fuse-config.
                   Local path: The local mount point in the filesystem.
                    If not provided, the list of mounts will be shown instead.

Options:
 --only-enabled           Only shows mounts that are enabled.
 --disable-path-collapse  Ensures all paths are fully shown. By default long paths are truncated for readability.
 --limit=rowcount         Limits the amount of rows displayed. Set to 0 to display unlimited rows. Default is unlimited.
 --col-separator=X	Uses the string "X" as column separator. Otherwise, spaces will be added between columns to align them.
 --output-cols=COLUMN_NAME_1,COLUMN_NAME2,...	Selects which columns to show and their order.

Note: FUSE commands are in early BETA. They are not available in macOS. If you experience any issues, please contact support@mega.nz.
</pre>
