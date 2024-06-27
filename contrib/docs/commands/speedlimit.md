### speedlimit
Displays/modifies upload/download rate limits

Usage: `speedlimit [-u|-d] [-h] [NEWLIMIT]`
<pre>
NEWLIMIT establish the new limit in size per second (0 = no limit)
 NEWLIMIT may include (B)ytes, (K)ilobytes, (M)egabytes, (G)igabytes & (T)erabytes.
  Examples: "1m12k3B" "3M". If no units are given, bytes are assumed

Options:
 -d	Download speed limit
 -u	Upload speed limit
 -h	Human readable

Notice: this limit will be saved for the next time you execute MEGAcmd server. They will be removed if you logout.
</pre>
