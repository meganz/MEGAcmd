### exclude
Manages default exclusion rules in syncs.

Usage: `exclude [(-a|-d) pattern1 pattern2 pattern3]`
<pre>
These default rules will be used when creating new syncs. Existing syncs won't be affected. To modify the exclusion rules of existing syncs, use mega-sync-ignore.

Options:
 -a pattern1 pattern2 ...	adds pattern(s) to the exclusion list
                         	          (* and ? wildcards allowed)
 -d pattern1 pattern2 ...	deletes pattern(s) from the exclusion list

This command is DEPRECATED. Use sync-ignore instead.
</pre>
