### find
Find nodes matching a pattern

Usage: `find [remotepath] [-l] [--pattern=PATTERN] [--type=d|f] [--mtime=TIMECONSTRAIN] [--size=SIZECONSTRAIN] [--use-pcre] [--time-format=FORMAT] [--show-handles|--print-only-handles]`
<pre>
Options:
 --pattern=PATTERN	Pattern to match (Perl Compatible Regular Expressions with "--use-pcre"
   or wildcarded expressions with ? or * like f*00?.txt)
 --type=d|f           	Determines type. (d) for folder, f for files
 --mtime=TIMECONSTRAIN	Determines time constrains, in the form: [+-]TIMEVALUE
                      	  TIMEVALUE may include hours(h), days(d), minutes(M),
                      	   seconds(s), months(m) or years(y)
                      	  Examples:
                      	   "+1m12d3h" shows files modified before 1 month,
                      	    12 days and 3 hours the current moment
                      	   "-3h" shows files modified within the last 3 hours
                      	   "-3d+1h" shows files modified in the last 3 days prior to the last hour
 --size=SIZECONSTRAIN	Determines size constrains, in the form: [+-]TIMEVALUE
                      	  TIMEVALUE may include (B)ytes, (K)ilobytes, (M)egabytes, (G)igabytes & (T)erabytes
                      	  Examples:
                      	   "+1m12k3B" shows files bigger than 1 Mega, 12 Kbytes and 3Bytes
                      	   "-3M" shows files smaller than 3 Megabytes
                      	   "-4M+100K" shows files smaller than 4 Mbytes and bigger than 100 Kbytes
 --show-handles	Prints files/folders handles (H:XXXXXXXX). You can address a file/folder by its handle
 --print-only-handles	Prints only files/folders handles (H:XXXXXXXX). You can address a file/folder by its handle
 --use-pcre	use PCRE expressions
 -l	Prints file info
 --time-format=FORMAT	show time in available formats. Examples:
               RFC2822:  Example: Fri, 06 Apr 2018 13:05:37 +0200
               ISO6081:  Example: 2018-04-06
               ISO6081_WITH_TIME:  Example: 2018-04-06T13:05:37
               SHORT:  Example: 06Apr2018 13:05:37
               SHORT_UTC:  Example: 06Apr2018 13:05:37
               CUSTOM. e.g: --time-format="%Y %b":  Example: 2018 Apr
                 You can use any strftime compliant format: http://www.cplusplus.com/reference/ctime/strftime/
</pre>
