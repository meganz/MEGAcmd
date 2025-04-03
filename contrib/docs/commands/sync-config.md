### sync-config
Controls sync configuration.

Usage: `sync-config [--delayed-uploads-wait-seconds | --delayed-uploads-max-attempts]`
<pre>
Displays current configuration.

New uploads for files that change frequently in syncs may be delayed until a wait time passes to avoid wastes of computational resources.
 Delay times and number of changes may change overtime
Options:
 --delayed-uploads-wait-seconds   Shows the seconds to be waited before a file that's being delayed is uploaded again.
 --delayed-uploads-max-attempts   Shows the max number of times a file can change in quick succession before it starts to get delayed.
</pre>
