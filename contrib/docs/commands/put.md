### put
Uploads files/folders to a remote folder

Usage: `put  [-c] [-q] [--ignore-quota-warn] localfile [localfile2 localfile3 ...] [dstremotepath]`
<pre>
Options:
 -c	Creates remote folder destination in case of not existing.
 -q	queue upload: execute in the background. Don't wait for it to end
 --ignore-quota-warn	ignore quota surpassing warning.
                    	  The upload will be attempted anyway.

Notice that the dstremotepath can only be omitted when only one local path is provided.
 In such case, the current remote working dir will be the destination for the upload.
 Mind that using wildcards for local paths in non-interactive mode in a supportive console (e.g. bash),
 could result in multiple paths being passed to MEGAcmd.
</pre>
