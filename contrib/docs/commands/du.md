### du
Prints size used by files/folders

Usage: `du [-h] [--versions] [remotepath remotepath2 remotepath3 ... ] [--use-pcre]`
<pre>
remotepath can be a pattern (Perl Compatible Regular Expressions with "--use-pcre"
   or wildcarded expressions with ? or * like f*00?.txt)

Options:
 -h	Human readable
 --versions	Calculate size including all versions.
   	You can remove all versions with "deleteversions" and list them with "ls --versions"
 --path-display-size=N	Use a fixed size of N characters for paths
 --use-pcre	use PCRE expressions
</pre>
