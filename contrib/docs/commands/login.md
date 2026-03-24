### login
Logs into a MEGA account, folder link or a previous session. You can only log into one entity at a time.

Usage: `login [--auth-code=XXXX] email password | exportedfolderurl#key [--auth-key=XXXX] [--resume] | passwordprotectedlink [--password=PASSWORD] | session`
<pre>
Logging into a MEGA account:
	You can log into a MEGA account by providing either a session ID or a username and password. A session ID simply identifies a session that you have previously logged in with using a username and password; logging in with a session ID simply resumes that session. If this is your first time logging in, you will need to do so with a username and password.
Options:
	--auth-code=XXXXXX: If you're logging in using a username and password, and this account has multifactor authentication (MFA) enabled, then this option allows you to pass the MFA token in directly rather than being prompted for it later on. For more information on this topic, please visit https://mega.nz/blog_48.

Logging into a MEGA folder link (an exported/public folder):
	MEGA folder links have the form URL#KEY. To log into one, simply execute the login command with the link.
Options:
	--password=PASSWORD: If the link is a password protected link, then this option can be used to pass in the password for that link.
	--auth-key=AUTHKEY: If the link is a writable folder link, then this option allows you to log in with write privileges. Without this option, you will log into the link with read access only.
	--resume: A convenience option to try to resume from cache. When login into a folder, contrary to what occurs with login into a user account, MEGAcmd will not try to load anything from cache: loading everything from scratch. This option changes that. Note, login using a session string, will of course, try to load from cache. This option may be convenient, for instance, if you previously logged out using --keep-session.

For more information about MEGA folder links, see "mega-export --help".
</pre>
