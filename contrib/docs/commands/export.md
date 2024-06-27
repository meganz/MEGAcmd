### export
Prints/Modifies the status of current exports

Usage: `export [-d|-a [--writable] [--mega-hosted] [--password=PASSWORD] [--expire=TIMEDELAY] [-f]] [remotepath] [--use-pcre] [--time-format=FORMAT]`
<pre>
Options:
 --use-pcre	The provided path will use Perl Compatible Regular Expressions (PCRE)
 -a	Adds an export.
   	Returns an error if the export already exists.
   	To modify an existing export (e.g. to change expiration time, password, etc.), it must be deleted and then re-added.
 --writable	Turn an export folder into a writable folder link. You can use writable folder links to share and receive files from anyone; including people who don’t have a MEGA account. 
           	This type of link is the same as a "file request" link that can be created through the webclient, except that writable folder links are not write-only. Writable folder links and file requests cannot be mixed, as they use different encryption schemes.
           	The auth-key shown has the following format <handle>#<key>:<auth-key>. The auth-key must be provided at login, otherwise you will log into this link with read-only privileges. See "mega-login --help" for more details about logging into links.
 --mega-hosted	The share key of this specific folder will be shared with MEGA.
              	This is intended to be used for folders accessible through MEGA's S4 service.
              	Encryption will occur nonetheless within MEGA's S4 service.
 --password=PASSWORD	Protects the export with a password. Passwords cannot contain " or '.
                    	A password-protected link will be printed only after exporting it.
                    	If "mega-export" is used to print it again, it will be shown unencrypted.
                    	Note: only PRO users can protect an export with a password.
 --expire=TIMEDELAY	Sets the expiration time of the export.
                   	The time format can contain hours(h), days(d), minutes(M), seconds(s), months(m) or years(y).
                   	E.g., "1m12d3h" will set an expiration time of 1 month, 12 days and 3 hours (relative to the current time).
                   	Note: only PRO users can set an expiration time for an export.
 -f	Implicitly accepts copyright terms (only shown the first time an export is made).
   	MEGA respects the copyrights of others and requires that users of the MEGA cloud service comply with the laws of copyright.
   	You are strictly prohibited from using the MEGA cloud service to infringe copyright.
   	You may not upload, download, store, share, display, stream, distribute, email, link to, transmit or otherwise make available any files, data or content that infringes any copyright or other proprietary rights of any person or entity.
 -d	Deletes an export.
   	The file/folder itself is not deleted, only the export link.
 --time-format=FORMAT	show time in available formats. Examples:
               RFC2822:  Example: Fri, 06 Apr 2018 13:05:37 +0200
               ISO6081:  Example: 2018-04-06
               ISO6081_WITH_TIME:  Example: 2018-04-06T13:05:37
               SHORT:  Example: 06Apr2018 13:05:37
               SHORT_UTC:  Example: 06Apr2018 13:05:37
               CUSTOM. e.g: --time-format="%Y %b":  Example: 2018 Apr
                 You can use any strftime compliant format: http://www.cplusplus.com/reference/ctime/strftime/

If a remote path is provided without the add/delete options, all existing exports within its tree will be displayed.
If no remote path is given, the current working directory will be used.
</pre>
