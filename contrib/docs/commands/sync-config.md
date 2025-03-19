### sync-config
Shows and modifies global sync configuration.

Usage: `sync-config [--delayed-uploads-wait-seconds=waitsecs | --delayed-uploads-max-attempts=attempts]`
<pre>
Displays current configuration if no options are provided. Configuration values are persisted across restarts.

New uploads for files that change frequently in syncs will be delayed until a wait time passes.
Options:
 --delayed-uploads-wait-seconds   Sets the seconds to be waited before a file that's being delayed is uploaded again. Default is 30 minutes (1800 seconds).
                                  Note: wait seconds must be between 60 and 86399 (inclusive).
 --delayed-uploads-max-attempts   Sets the max number of times a file can change in quick succession before it starts to get delayed. Default is 2.
                                  Note: max attempts must be between 2 and 5 (inclusive).
</pre>
