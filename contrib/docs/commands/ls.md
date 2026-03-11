### ls
Lists files in a remote path

Usage: `ls [-halRr] [--show-handles] [--tree] [--versions] [remotepath] [--use-pcre] [--show-creation-time] [--time-format=FORMAT]`
<pre>
remotepath can be a pattern (Perl Compatible Regular Expressions with "--use-pcre"
   or wildcarded expressions with ? or * like f*00?.txt)
 Also, constructions like /PATTERN1/PATTERN2/PATTERN3 are allowed

Options:
 -R|-r	List folders recursively
 --tree	Prints tree-like exit (implies -r)
 --show-handles	Prints files/folders handles (H:XXXXXXXX). You can address a file/folder by its handle
 -l	Print summary (--tree has no effect)
   	 SUMMARY contents:
   	   FLAGS: Indicate type/status of an element:
   	     xxxx
   	     |||+---- Sharing status: (s)hared, (i)n share or not shared(-)
   	     ||+----- if exported, whether it is (p)ermanent or (t)temporal
   	     |+------ e/- whether node is (e)xported
   	     +-------- Type(d=folder,-=file,r=root,i=inbox,b=rubbish,x=unsupported)
   	   VERS: Number of versions in a file
   	   SIZE: Size of the file in bytes:
   	   DATE: Modification date for files and creation date for folders (in UTC time):
   	   NAME: name of the node
 -h	Show human readable sizes in summary
 -a	Include extra information
   	 If this flag is repeated (e.g: -aa) more info will appear
   	 (public links, expiration dates, ...)
 --versions	show historical versions
   	You can delete all versions of a file with "deleteversions"
 --show-creation-time	show creation time instead of modification time for files
 --time-format=FORMAT	show time in available formats. Examples:
               RFC2822:  Example: Fri, 06 Apr 2018 13:05:37 +0200
               ISO6081:  Example: 2018-04-06
               ISO6081_WITH_TIME:  Example: 2018-04-06T13:05:37
               SHORT:  Example: 06Apr2018 13:05:37
               SHORT_UTC:  Example: 06Apr2018 13:05:37
               CUSTOM. e.g: --time-format="%Y %b":  Example: 2018 Apr
                 You can use any strftime compliant format: http://www.cplusplus.com/reference/ctime/strftime/
 --use-pcre	use PCRE expressions
</pre>
