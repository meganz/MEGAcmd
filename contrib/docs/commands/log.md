### log
Prints/Modifies the current logs level

Usage: `log [-sc] level`
<pre>
Options:
 -c	CMD log level (higher level messages).
   	 Messages captured by MEGAcmd server.
 -s	SDK log level (lower level messages).
   	 Messages captured by the engine and libs

Regardless of the log level of the
 interactive shell, you can increase the amount of information given
   by any command by passing "-v" ("-vv", "-vvv", ...)
</pre>
