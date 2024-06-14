### update
Updates MEGAcmd

Usage: `update [--auto=on|off|query]`
<pre>
Looks for updates and applies if available.
This command can also be used to enable/disable automatic updates.
Options:
 --auto=ON|OFF|query	Enables/disables/queries status of auto updates.

If auto updates are enabled it will be checked while MEGAcmd server is running.
 If there is an update available, it will be downloaded and applied.
 This will cause MEGAcmd to be restarted whenever the updates are applied.

Further info at https://github.com/meganz/megacmd#megacmd-updates
Note: this command is not available on Linux
</pre>
