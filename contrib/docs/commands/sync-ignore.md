### sync-ignore
Manages ignore filters for syncs

Usage: `sync-ignore [--show|[--add|--add-exclusion|--remove|--remove-exclusion] filter1 filter2 ...] (ID|localpath|DEFAULT)`
<pre>
To modify the default filters, use "DEFAULT" instead of local path or ID.
Note: when modifying the default filters, existing syncs won't be affected. Only newly created ones.

If no action is provided, filters will be shown for the selected sync.
Only the filters at the root of the selected sync will be accessed. Filters beloging to sub-folders must be modified manually.

Options:
--show	Show the existing filters of the selected sync
--add	Add the specified filters to the selected sync
--add-exclusion	Same as "--add", but the <CLASS> is 'exclude'
               	Note: the `-` must be omitted from the filter (using '--' is not necessary)
--remove	Remove the specified filters from the selected sync
--remove-exclusion	Same as "--remove", but the <CLASS> is 'exclude'
                  	Note: the `-` must be omitted from the filter (using '--' is not necessary)

Filters must have the following format: <CLASS><TARGET><TYPE><STRATEGY>:<PATTERN>
	<CLASS> Must be either exclude, or include
		exclude (`-`): This filter contains files or directories that *should not* be synchronized
		               Note: exclude filters must be preceded by '--', or they won't be recognized
		include (`+`): This filter contains files or directories that *should* be synchronized
	<TARGET> May be one of the following: directory, file, symlink, or all
		directory (`d`): This filter applies only to directories
		file (`f`): This filter applies only to files
		symlink (`s`): This filter applies only to symbolic links
		all (`a`): This filter applies to all of the above
	Default (when omitted) is `a`
	<TYPE> May be one of the following: local name, path, or subtree name
		local name (`N`): This filter has an effect only in the root directory of the sync
		path (`p`): This filter matches against the path relative to the rooth directory of the sync
		            Note: the path separator is always '/', even on Windows
		subtree name (`n`): This filter has an effect in all directories below the root directory of the sync, itself included
	Default (when omitted) is `n`
	<STRATEGY> May be one of the following: glob, or regexp
		glob (`G` or `g`): This filter matches against a name or path using a wildcard pattern
		regexp (`R` or `r`): This filter matches against a name or path using a pattern expressed as a POSIX-Extended Regular Expression
	Note: uppercase `G` or `R` specifies that the matching should be case-sensitive
	Default (when omitted) is `G`
	<PATTERN> Must be a file or directory pattern
Some examples:
	`-f:*.txt`  Filter will exclude all *.txt files in and beneath the sync directory
	`+fg:work*.txt`  Filter will include all work*.txt files excluded by the filter above
	`-N:*.avi`  Filter will exclude all *.avi files contained directly in the sync directory
	`-nr:.*foo.*`  Filter will exclude files whose name contains 'foo'
	`-d:private`  Filter will exclude all directories with the name 'private'

See: https://help.mega.io/installs-apps/desktop/megaignore more info.
</pre>
