### permissions
Shows/Establish default permissions for files and folders created by MEGAcmd.

Usage: `permissions [(--files|--folders) [-s XXX]]`
<pre>
Permissions are unix-like permissions, with 3 numbers: one for owner, one for group and one for others
Options:
 --files	To show/set files default permissions.
 --folders	To show/set folders default permissions.
 --s XXX	To set new permissions for newly created files/folder.
        	 Notice that for files minimum permissions is 600,
        	 for folders minimum permissions is 700.
        	 Further restrictions to owner are not allowed (to avoid missfunctioning).
        	 Notice that permissions of already existing files/folders will not change.
        	 Notice that permissions of already existing files/folders will not change.

Notice: this permissions will be saved for the next time you execute MEGAcmd server. They will be removed if you logout.
</pre>
