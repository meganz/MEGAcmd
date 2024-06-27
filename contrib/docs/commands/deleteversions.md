### deleteversions
Deletes previous versions.

Usage: `deleteversions [-f] (--all | remotepath1 remotepath2 ...)  [--use-pcre]`
<pre>
This will permanently delete all historical versions of a file.
The current version of the file will remain.
Note: any file version shared to you from a contact will need to be deleted by them.

Options:
 -f   	Force (no asking)
 --all	Delete versions of all nodes. This will delete the version histories of all files (not current files).
 --use-pcre	use PCRE expressions

To see versions of a file use "ls --versions".
To see space occupied by file versions use "du --versions".
</pre>
