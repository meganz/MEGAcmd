### sync-issues
Show all issues with current syncs

Usage: `sync-issues [[--detail (ID|--all)] [--limit=rowcount] [--disable-path-collapse]] | [--enable-warning|--disable-warning]`
<pre>
When MEGAcmd detects conflicts with the data it's synchronizing, a sync issue is triggered. Syncing is stopped, and no progress is made on the conflicting data.
A notification will appear when sync issues are detected. Because the sync engine might clear the internal issue list when processing, the notification can appear even if there were already issues before.
Note: the sync issue list might not contain the latest updated data. Some issues might still be being processing by the sync engine, and some might not have been removed yet.

Options:
 --detail (ID | --all) 	Provides additional information on a particular sync issue.
                       	The ID of the sync where this issue appeared is shown, alongside its local and cloud paths.
                       	All paths involved in the issue are shown. For each path, the following columns are displayed:
                       		PATH: The conflicting local or cloud path (cloud paths are prefixed with "<CLOUD>").
                       		PATH_ISSUE: A brief explanation of the problem this file or folder has (if any).
                       		LAST_MODIFIED: The most recent date when this file or directory was updated.
                       		UPLOADED: For cloud paths, the date of upload or creation. Empty for local paths.
                       		SIZE: The size of the file. Empty for directories.
                       		TYPE: The type of the path (file or directory). This column is hidden if the information it's not relevant for the particular sync issue.
                       	The "--all" argument can be used to show the details of all issues.
 --limit=rowcount 	Limits the amount of rows displayed. Can also be combined with "--detail".
 --disable-path-collapse 	Ensures all paths are fully shown. By default long paths are truncated for readability.
 --enable-warning 	Enables the notification that appears when issues are detected. This setting is stored locally for all users.
 --disable-warning 	Disables the notification that appears when issues are detected. This setting is stored locally for all users.
 --col-separator=X	Use the string "X" as column separator. Otherwise, spaces will be added between columns to align them.
 --output-cols=COLUMN_NAME_1,COLUMN_NAME2,...	Select which columns to show and their order.

DISPLAYED columns:
	ISSUE_ID: A unique identifier of the sync issue. The ID can be used alongside the "--detail" argument.
	PARENT_SYNC: The identifier of the sync that has this issue.
	REASON: A brief explanation on what the issue is. Use the "--detail" argument to get extended information on a particular sync.
</pre>
