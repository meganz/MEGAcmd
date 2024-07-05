### get
Downloads a remote file/folder or a public link

Usage: `get [-m] [-q] [--ignore-quota-warn] [--use-pcre] [--password=PASSWORD] exportedlink|remotepath [localpath]`
<pre>
In case it is a file, the file will be downloaded at the specified folder
                             (or at the current folder if none specified).
  If the localpath (destination) already exists and is the same (same contents)
  nothing will be done. If differs, it will create a new file appending " (NUM)"

For folders, the entire contents (and the root folder itself) will be
                    by default downloaded into the destination folder

Exported links: Exported links are usually formed as publiclink#key.
 Alternativelly you can provide a password-protected link and
 provide the password with --password. Please, avoid using passwords containing " or '


Options:
 -q	queue download: execute in the background. Don't wait for it to end
 -m	if the folder already exists, the contents will be merged with the
                     downloaded one (preserving the existing files)
 --ignore-quota-warn	ignore quota surpassing warning.
                    	  The download will be attempted anyway.
 --password=PASSWORD	Password to decrypt the password-protected link. Please, avoid using passwords containing " or '
 --use-pcre	use PCRE expressions
</pre>
