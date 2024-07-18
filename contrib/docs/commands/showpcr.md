### showpcr
Shows incoming and outgoing contact requests.

Usage: `showpcr [--in | --out] [--time-format=FORMAT]`
<pre>
Options:
 --in	Shows incoming requests
 --out	Shows outgoing invitations
 --time-format=FORMAT	show time in available formats. Examples:
               RFC2822:  Example: Fri, 06 Apr 2018 13:05:37 +0200
               ISO6081:  Example: 2018-04-06
               ISO6081_WITH_TIME:  Example: 2018-04-06T13:05:37
               SHORT:  Example: 06Apr2018 13:05:37
               SHORT_UTC:  Example: 06Apr2018 13:05:37
               CUSTOM. e.g: --time-format="%Y %b":  Example: 2018 Apr
                 You can use any strftime compliant format: http://www.cplusplus.com/reference/ctime/strftime/

Use "mega-ipc" to manage invitations received
Use "mega-users" to see contacts
</pre>
