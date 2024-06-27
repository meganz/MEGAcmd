### login
Logs into a MEGA account

Usage: `login [--auth-code=XXXX] email password | exportedfolderurl#key [--auth-key=XXXX] | passwordprotectedlink [--password=PASSWORD] | session`
<pre>
You can log in either with email and password, with session ID,
 or into a folder (an exported/public folder)
 If logging into a folder indicate url#key (or pasword protected link)
   Pass --auth-key=XXXX	with the authentication key (last part of the Auth Token)
   to be able to write into the accessed folder

 Please, avoid using passwords containing " or '.

Options:
 --auth-code=XXXX	Two-factor Authentication code. More info: https://mega.nz/blog_48
 --password=XXXX 	Password to decrypt password protected links (See "mega-export --help")
</pre>
