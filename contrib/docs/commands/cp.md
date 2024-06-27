### cp
Copies files/folders into a new location (all remotes)

Usage: `cp [--use-pcre] srcremotepath [srcremotepath2 srcremotepath3 ..] dstremotepath|dstemail:`
<pre>
If the location exists and is a folder, the source will be copied there
If the location doesn't exist, and only one source is provided,
 the file/folder will be copied and renamed to the destination name given.

If "dstemail:" provided, the file/folder will be sent to that user's inbox (//in)
 e.g: cp /path/to/file user@doma.in:
 Remember the trailing ":", otherwise a file with the name of that user ("user@doma.in") will be created
Options:
 --use-pcre	use PCRE expressions
</pre>
