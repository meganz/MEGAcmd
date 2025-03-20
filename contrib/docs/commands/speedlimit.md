### speedlimit
Displays/modifies upload/download rate limits: either speed or max connections

Usage: `speedlimit [-u|-d|--upload-connections|--download-connections] [-h] [NEWLIMIT]`
<pre>
NEWLIMIT is the new limit to set. If no option is provided, NEWLIMIT will be 
  applied for both download/upload speed limits. 0, for speeds, means unlimited.
 NEWLIMIT may include (B)ytes, (K)ilobytes, (M)egabytes, (G)igabytes & (T)erabytes.
  Examples: "1m12k3B" "3M". If no units are given, bytes are assumed.

Options:
 -d                       Set/Read download speed limit, expressed in size per second.
 -u                       Set/Read dpload speed limit, expressed in size per second
 --upload-connections     Set/Read max number of connections for an upload transfer
 --download-connections   Set/Read max number of connections for a download transfer

Display options:
 -h                       Human readable

Notice: these limits will be saved for the next time you execute MEGAcmd server. They will be removed if you logout.
</pre>
