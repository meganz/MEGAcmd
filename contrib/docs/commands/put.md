### put
Uploads files/folders to a remote folder

Usage: `put  [-c] [-q] [--print-tag-at-start] localfile [localfile2 localfile3 ...] [dstremotepath]`
<pre>
Options:
 -c	Creates remote folder destination in case of not existing.
 -q	queue upload: execute in the background. Don't wait for it to end
 --print-tag-at-start	Prints start message including transfer TAG, even when using -q.

Notice that the dstremotepath can only be omitted when only one local path is provided.
 In such case, the current remote working dir will be the destination for the upload.
 Mind that using wildcards for local paths in non-interactive mode in a supportive console (e.g. bash),
 could result in multiple paths being passed to MEGAcmd.
</pre>
