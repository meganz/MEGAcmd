### exclude
Manages exclusions in syncs.

Usage: `exclude [(-a|-d) pattern1 pattern2 pattern3]`
<pre>
Options:
 -a pattern1 pattern2 ...	adds pattern(s) to the exclusion list
                         	          (* and ? wildcards allowed)
 -d pattern1 pattern2 ...	deletes pattern(s) from the exclusion list

Caveat: removal of patterns may not trigger sync transfers right away.
Consider:
 a) disable/reenable synchronizations manually
 b) restart MEGAcmd server
</pre>
