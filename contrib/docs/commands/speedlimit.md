### speedlimit
Displays/modifies upload/download rate limits

Usage: `speedlimit [-u|-d|--upload-connections|--download-connections] [-h] [NEWLIMIT]`
<pre>
NEWLIMIT establish the new limit in size per second (0 = no limit)
 NEWLIMIT may include (B)ytes, (K)ilobytes, (M)egabytes, (G)igabytes & (T)erabytes.
  Examples: "1m12k3B" "3M". If no units are given, bytes are assumed

Options:
 -d                       Download speed limit
 -u                       Upload speed limit
 --upload-connections     Max number of connections for an upload transfer
 --download-connections   Max number of connections for a download transfer
 -h                       Human readable

Notice: these limits will be saved for the next time you execute MEGAcmd server. They will be removed if you logout.
</pre>
