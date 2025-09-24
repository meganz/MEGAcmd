# MEGAcmd User Guide

This document relates to MEGAcmd version 2.3.0.  It contains introductory information and the [Command Summary](#command-summary), with links to detailed command descriptions.

### What is it
A command line tool to work with your MEGA account and files.  The intent is to offer all the MEGA account functionality via command line.  You can run it in [interactive](#interactive) mode where it processes all commands directly, or you can run its [scriptable](#scriptable) commands from your favourite Linux or Mac shell such as bash, or you can even run its commands in a Windows command prompt. And of course you can write scripts using those scriptable commands.

Here is an example of downloading a file using MEGAcmd.  In this case we are downloading a file specified by a public link, which does not require being logged in: <p>
```
mega-get https://mega.nz/#F!ABcD1E2F!gHiJ23k-LMno45PqrSTUvw /path/to/local/folder
```
Note:- If you get any error like this one
```
Event not found
```
In link put ```\``` in front of every ```!``` for example
```
mega-get https://mega.nz/#F\!ABcD1E2F\!gHiJ23k-LMno45PqrSTUvw /path/to/local/folder
```

And here is an example of uploading a file using MEGAcmd, and making a link available to share it, that will expire after 10 minutes.<p>
```
mega-put /path/to/my/temporary_resource /exportedstuff/
mega-export -a  /exportedstuff/temporary_resource --expire=10M | awk '{print $4}'
```

And here is an example of the power of using [scriptable](#scriptable) MEGAcmd commands in bash.  In this case we are going to share some promotional videos previously uploaded to MEGA: <p>
```
for i in $(mega-find /enterprise/video/promotional2015/may --pattern="*mpeg")
do
mega-export -a $i | awk '{print $4}';
done
```

In addition to running commands on request, MEGAcmd can also be configured to [synchronise](#synchronisation-configurations) folders between your local device and your MEGA account, or perform regular [backups](#backup-configurations) from your device to your MEGA account.

In order to enable synchronisation and backup features, and for efficiency running commands, MEGAcmd runs a server process in the background which the MEGAcmd shell or the script commands forward requests to.   The server keeps running in the background until it is told to close with the [`quit`](#quit) command.   If you want it to keep running when you quit the interactive shell (to keep sync and backup running for example), use `quit --only-shell`.

Working with your MEGA account requires signing in with your email and password using the [`login`](#login) command, though you can download public links or upload to public folders without logging in.  Logging in with your username and password starts a [Session](#session), and causes some of your account such as the folder structure to be downloaded to your [Local Cache](#local-cache).

### Where can you get it
For Linux, Mac, or Windows: Download it from the MEGA.nz website: https://mega.nz/cmd <p>
We are also building it for some NAS systems, please check your provider's App Store.

### What can you do with it
The major features are
* Move files around inside your MEGA account or between MEGA and your PC using command line tools.
* Use those same commands in scripts to manage your files.
* Set up synchronization or a backup schedule between a folder on your machine, and a folder on your MEGA account.   (use the [`sync`](#sync) or [`backup`](#backup) commands).
* Set up WebDAV access to files in your MEGA account (use the [`webdav`](#webdav) command).
* [Linux only] Set up a FUSE mount point to seamlessly access files in your MEGA account (use the [`fuse-add`](#fuse-add) command).

See our Help Centre pages for the basics of getting started, and friendly examples of common usages with plenty of pictures:  https://mega.nz/help

## Terminology and Descriptions

### Interactive
Interactive refers to running the MEGAcmd shell which only processes MEGA commands.  You invoke commands by typing and pressing Enter.  MEGAcmd shell provides a lot of feedback about what it's doing.  You can start the MEGAcmd shell with `mega-cmd` (or `MEGAcmd` on Windows).  You can then issue commands like `ls` directly: <p>
`ls /my/account/folder`<p>
or you can get a list of available commands with: <p>
`help`<p>
or you can get detailed information about any particular command by using the `--help` flag with that command:<p>
`ls --help`<p>
Autocompletion (pressing tab to fill in the remainder of a command) is available in interactive mode.

### Scriptable
Scriptable refers to running the MEGAcmd commands from a shell such as bash or the windows powershell.  If the PATH to the MEGAcmd commands are not yet on the PATH in that shell, you'll need to add it.  You can then issue commands like `ls` by prefixing them with the `mega-` prefix: <p>
`mega-ls /my/account/folder`<p>
or you can get a list of available commands with: <p>
`mega-help`<p>
or you can get detailed information about any particular command by using the `--help` flag with that command:<p>
`mega-ls --help`<p>
Scriptable commands can of course be used in scripts to achieve a lot in a short space of time, using loops or preparing all the desired commands ahead of time.
If you are using bash as your shell, the MEGAcmd commands support auto-completion.

### Contact
A contact is someone (identified by their email address) that also has a MEGA account, who you can share files or folders with, and can chat with on MEGAchat.

### Remote Path
This refers to a file or a folder stored in your MEGA account, or a publicly available file or folder in the MEGA cloud.  Remote paths always use the '/' character as the separator between folder and file elements.

Some MEGAcmd commands allow the use of regular expressions in remote paths.  You can check if the command supports those by using the `--help` flag with the command.  If you use these in the [scriptable](#scriptable) way, you need to escape characters that would otherwise be intercepted and interpreted by the shell.

Paths to folders shared to you from another person start with their email and a : character, see the example at ([example](#shared-folders-example))

### Local Path
This refers to a file or folder on the PC or device that MEGAcmd is running in.

### Session
When you log in with your email and MEGA account password, that creates a session.  The session exists until you log out of it or kill it from another client.  In MEGAcmd, use `whoami -l` to see all your open sessions across all devices, and use `killsession` to close them.   You can use other MEGA clients such as the phone app, or webclient to close these also.   Devices that were using a killed session will have their connection to MEGA closed immediately and will no longer have access to your account, until you log in on them again.   Syncs, backups, and webdavs are specific to a session, so logging out will cause them to be cancelled.

### Local Cache
Logging in with MEGAcmd creates your Local Cache, a subfolder of your home folder.  MEGAcmd downloads and stores some data in your Local Cache relating to your account, such as folder structure and contacts, for performance reasons.  The MEGAcmd background server keeps the local cache up to date when changes to your account occur from other clients.  The cache does contain a way for MEGAcmd to access your MEGA account when it starts up again if you have not specifically logged out.  The Local Cache also contains information from your Session, including sync, backup, and webdav configurations.  Logging out cleans the Local Cache, but also closes your session and the sync, backup, and webdav configurations are wiped.

### Synchronisation configurations
MEGAcmd can set up a synchronisation between a folder on your local machine and a folder in your MEGA account, using the [`sync`](#sync) command.   This is the same mechanism that MEGAsync uses.  The synchronisation is two-way: the folders you nominate to be synced will mirror any action!  Whatever you add or delete in your sync folder on your device gets added or deleted in your sync folder in your MEGA account.  And additions or deletions in your synced folder in your MEGA account will similarly be applied to your local synced folder.  Files that are removed from sync folders are moved to a hidden local folder (Rubbish/.debris inside your local sync folder, or SyncDebris folder in the Rubbish Bin of your MEGA account).

Here is a very simple example of setting up a synchronisation with MEGAcmd: <p>
```
sync /path/to/local/folder /folder/in/mega
```

You can set up more than one pair or folders to be synced, and you can also set a sync from another device to the same folder, to achieve folder synchronisations between different devices.   The changes are sent via your MEGA account rather than directly between the devices in that case.

Additional information about synchronising folders is available in our Help Centre:  https://mega.nz/help/client/megasync/syncing

### Backup configurations
MEGAcmd can set up a periodic copy of a local folder to your MEGA account using the [`backup`](#backup) command.  Here is a simple example that will back up a folder immediately and then at 4am each day, keeping the 10 most recent backups: <p>
```
backup /path/mega/folder /remote/path --period="0 0 4 * * *" --num-backups=10
```

For further information on backups, please see the [`backup`](#backup) command and the [tutorial](contrib/docs/BACKUPS.md).

### WebDAV configurations
MEGAcmd can set up access to folders or files in your MEGA account as if they were local folders and files on your device using the [`webdav`](#webdav) command.  For example making the folder appear like a local drive on your PC, or providing a hyperlink a browser can access, where the hyperlink is to your PC.

For further information on WebDAV, please see the [`webdav`](#webdav) command and the [tutorial](contrib/docs/WEBDAV.md

### FUSE mount point
If you use Linux, MEGAcmd can set up access to folders or files in your MEGA account as if they were local folders and files on your device using Filesystem in User Space via [`fuse-add`](#fuse-add) command.

For further information on FUSE, please see the [`fuse-add`](#fuse-add) command and the [tutorial](contrib/docs/FUSE.md).

### Linux
On Linux, MEGAcmd commands are installed at /usr/bin and so will already be on your PATH.  The interactive shell is `mega-cmd` and the background server is `mega-cmd-server`, which will be automatically started on demand.  The various scriptable commands are installed at the same location, and invoke `mega-exec` to send the command to `mega-cmd-server`.

If you are using the scriptable commands in bash (or using the interactive commands in mega-cmd), the commands will auto-complete.

### Macintosh
For MacOS, after installing the dmg, you can launch the server using **MEGAcmd** in Applications. If you wish to use the client commands from MacOS Terminal, open the Terminal and include the installation folder in the PATH.<p>
Typically:
```
export PATH=/Applications/MEGAcmd.app/Contents/MacOS:$PATH
```

And for bash completion, source `megacmd_completion.sh` :
```
source /Applications/MEGAcmd.app/Contents/MacOS/megacmd_completion.sh
```

### Windows
Once you have MEGAcmd installed, you can start the [interactive](#interactive) shell from the Start Menu or desktop icon.  On windows the interactive shell executable is called `MEGAcmdShell.exe` and the server is `MEGAcmdServer.exe`.

You can also start MEGAcmd Shell from inside PowerShell. To do so, start powershell from the Start Menu and then execute these commands to start it:
```
$env:PATH += ";$env:LOCALAPPDATA\MEGAcmd"
MEGAcmdShell
```

For [scriptable](#scriptable) usage, the MEGAcmd commands are provided via installed .bat files which pass the command to the MEGAcmdServer.exe.  Provided you have set the PATH as above, you can use these like normal command line tools in PowerShell:
```
$env:PATH += ";$env:LOCALAPPDATA\MEGAcmd"
mega-cd /my/favourite/folder
mega-ls
```

Or in Command Prompt:
```
set PATH=%LOCALAPPDATA%\MEGAcmd;%PATH%
mega-cd /my/favourite/folder
mega-ls
```


And of course those can be invoked in your own .bat or .cmd files.
Autocompletion is not available for the scriptable commands, but is in the interactive shell.

Unicode is supported though it currently in the interactive shell it needs to be switched on, and to have a suitable font selected; please execute `help --unicode` for the latest information.  There are plans to improve this.  Please report any issues experienced to our support team.

### NAS Support
We have released packages for QNAP and Synology, which you can download and install from the App Center in QNAP, and the Package Center in Synology.   In QNAP, please make sure to turn on "Enable home folder for all users" from the control panel, and set HOME=/share/homes/<username> before starting any MEGA commands, and in Synology, 'Enable user home service', so that the `mega-cmd-server` creates the `.megaCmd` local cache folder there (as the default HOME location may be erased on restart).

## Command Summary

These summaries use the usual conventions - `[]` indicates its content is optional,  `|` indicates you should choose either the item on the left or the one on the right (but not both)

Each command is described as it would be used in the [interactive](#interactive) MEGAcmd shell, and the corresponding [scriptable](#scriptable) command (which must be prefixed with `mega-`) works in the same way.

Commands referring to a [remote path](#remote-path) are talking about a file in your MEGA account online, whereas a [local path](#local-path) refers to a file or folder on your local device where MEGAcmd is running.

Verbosity: You can increase the amount of information given by any command by passing `-v` (`-vv`, `-vvv`, ...)

### Account / Contacts
* [`signup`](contrib/docs/commands/signup.md)`email password [--name="Your Name"]` Register as user with a given email
* [`confirm`](contrib/docs/commands/confirm.md)`link email password` Confirm an account using the link provided after the "signup" process.
* [`invite`](contrib/docs/commands/invite.md)`[-d|-r] dstemail [--message="MESSAGE"]` Invites a contact / deletes an invitation
* [`showpcr`](contrib/docs/commands/showpcr.md)`[--in | --out] [--time-format=FORMAT]` Shows incoming and outgoing contact requests.
* [`ipc`](contrib/docs/commands/ipc.md)`email|handle -a|-d|-i` Manages contact incoming invitations.
* [`users`](contrib/docs/commands/users.md)`[-s] [-h] [-n] [-d contact@email] [--time-format=FORMAT] [--verify|--unverify contact@email.com] [--help-verify [contact@email.com]]` List contacts
* [`userattr`](contrib/docs/commands/userattr.md)`[-s attribute value|attribute|--list] [--user=user@email]` Lists/updates user attributes
* [`passwd`](contrib/docs/commands/passwd.md)`[-f]  [--auth-code=XXXX] newpassword` Modifies user password
* [`masterkey`](contrib/docs/commands/masterkey.md)`pathtosave` Shows your master key.

### Login / Logout
* [`login`](contrib/docs/commands/login.md)`[--auth-code=XXXX] email password | exportedfolderurl#key [--auth-key=XXXX] [--resume] | passwordprotectedlink [--password=PASSWORD] | session` Logs into a MEGA account, folder link or a previous session. You can only log into one entity at a time.
* [`logout`](contrib/docs/commands/logout.md)`[--keep-session]` Logs out
* [`whoami`](contrib/docs/commands/whoami.md)`[-l]` Prints info of the user
* [`session`](contrib/docs/commands/session.md) Prints (secret) session ID
* [`killsession`](contrib/docs/commands/killsession.md)`[-a | sessionid1 sessionid2 ... ]` Kills a session of current user.

### Browse
* [`cd`](contrib/docs/commands/cd.md)`[remotepath]` Changes the current remote folder
* [`lcd`](contrib/docs/commands/lcd.md)`[localpath]` Changes the current local folder for the interactive console
* [`ls`](contrib/docs/commands/ls.md)`[-halRr] [--show-handles] [--tree] [--versions] [remotepath] [--use-pcre] [--show-creation-time] [--time-format=FORMAT]` Lists files in a remote path
* [`pwd`](contrib/docs/commands/pwd.md) Prints the current remote folder
* [`lpwd`](contrib/docs/commands/lpwd.md) Prints the current local folder for the interactive console
* [`attr`](contrib/docs/commands/attr.md)`remotepath [--force-non-officialficial] [-s attribute value|-d attribute [--print-only-value]` Lists/updates node attributes.
* [`du`](contrib/docs/commands/du.md)`[-h] [--versions] [remotepath remotepath2 remotepath3 ... ] [--use-pcre]` Prints size used by files/folders
* [`find`](contrib/docs/commands/find.md)`[remotepath] [-l] [--pattern=PATTERN] [--type=d|f] [--mtime=TIMECONSTRAIN] [--size=SIZECONSTRAIN] [--use-pcre] [--time-format=FORMAT] [--show-handles|--print-only-handles]` Find nodes matching a pattern
* [`mount`](contrib/docs/commands/mount.md) Lists all the root nodes

### Moving / Copying files
* [`mkdir`](contrib/docs/commands/mkdir.md)`[-p] remotepath` Creates a directory or a directories hierarchy
* [`cp`](contrib/docs/commands/cp.md)`[--use-pcre] srcremotepath [srcremotepath2 srcremotepath3 ..] dstremotepath|dstemail` : Copies files/folders into a new location (all remotes)
* [`put`](contrib/docs/commands/put.md)`[-c] [-q] [--ignore-quota-warn] localfile [localfile2 localfile3 ...] [dstremotepath]` Uploads files/folders to a remote folder
* [`get`](contrib/docs/commands/get.md)`[-m] [-q] [--ignore-quota-warn] [--use-pcre] [--password=PASSWORD] exportedlink|remotepath [localpath]` Downloads a remote file/folder or a public link
* [`preview`](contrib/docs/commands/preview.md)`[-s] remotepath localpath` To download/upload the preview of a file.
* [`thumbnail`](contrib/docs/commands/thumbnail.md)`[-s] remotepath localpath` To download/upload the thumbnail of a file.
* [`mv`](contrib/docs/commands/mv.md)`srcremotepath [--use-pcre] [srcremotepath2 srcremotepath3 ..] dstremotepath` Moves file(s)/folder(s) into a new location (all remotes)
* [`rm`](contrib/docs/commands/rm.md)`[-r] [-f] [--use-pcre] remotepath` Deletes a remote file/folder
* [`transfers`](contrib/docs/commands/transfers.md)`[-c TAG|-a] | [-r TAG|-a]  | [-p TAG|-a] [--only-downloads | --only-uploads] [SHOWOPTIONS]` List or operate with transfers
* [`speedlimit`](contrib/docs/commands/speedlimit.md)`[-u|-d|--upload-connections|--download-connections] [-h] [NEWLIMIT]` Displays/modifies upload/download rate limits: either speed or max connections
* [`sync`](contrib/docs/commands/sync.md)`[localpath dstremotepath| [-dpe] [ID|localpath]` Controls synchronizations.
* [`sync-issues`](contrib/docs/commands/sync-issues.md)`[[--detail (ID|--all)] [--limit=rowcount] [--disable-path-collapse]] | [--enable-warning|--disable-warning]` Show all issues with current syncs
* [`sync-ignore`](contrib/docs/commands/sync-ignore.md)`[--show|[--add|--add-exclusion|--remove|--remove-exclusion] filter1 filter2 ...] (ID|localpath|DEFAULT)` Manages ignore filters for syncs
* [`sync-config`](contrib/docs/commands/sync-config.md)`[--delayed-uploads-wait-seconds | --delayed-uploads-max-attempts]` Controls sync configuration.
* [`exclude`](contrib/docs/commands/exclude.md)`[(-a|-d) pattern1 pattern2 pattern3]` Manages default exclusion rules in syncs.
* [`backup`](contrib/docs/commands/backup.md)`(localpath remotepath --period="PERIODSTRING" --num-backups=N  | [-lhda] [TAG|localpath] [--period="PERIODSTRING"] [--num-backups=N]) [--time-format=FORMAT]` Controls backups

### Sharing (your own files, of course, without infringing any copyright)
* [`export`](contrib/docs/commands/export.md)`[-d|-a [--writable] [--mega-hosted] [--password=PASSWORD] [--expire=TIMEDELAY] [-f]] [remotepath] [--use-pcre] [--time-format=FORMAT]` Prints/Modifies the status of current exports
* [`import`](contrib/docs/commands/import.md)`exportedlink [--password=PASSWORD] [remotepath]` Imports the contents of a remote link into user's cloud
* [`share`](contrib/docs/commands/share.md)`[-p] [-d|-a --with=user@email.com [--level=LEVEL]] [remotepath] [--use-pcre] [--time-format=FORMAT]` Prints/Modifies the status of current shares
* [`webdav`](contrib/docs/commands/webdav.md)`[-d (--all | remotepath ) ] [ remotepath [--port=PORT] [--public] [--tls --certificate=/path/to/certificate.pem --key=/path/to/certificate.key]] [--use-pcre]` Configures a WEBDAV server to serve a location in MEGA

### FUSE (mount your cloud folder to the local system)
* [`fuse-add`](contrib/docs/commands/fuse-add.md)`[--name=name] [--disabled] [--transient] [--read-only] localPath remotePath` Creates a new FUSE mount.
* [`fuse-remove`](contrib/docs/commands/fuse-remove.md)`(name|localPath)` Deletes a specified FUSE mount.
* [`fuse-enable`](contrib/docs/commands/fuse-enable.md)`[--temporarily] (name|localPath)` Enables a specified FUSE mount.
* [`fuse-disable`](contrib/docs/commands/fuse-disable.md)`[--temporarily] (name|localPath)` Disables a specified FUSE mount.
* [`fuse-show`](contrib/docs/commands/fuse-show.md)`[--only-enabled] [--disable-path-collapse] [[--limit=rowcount] | [name|localPath]]` Displays the list of FUSE mounts and their information. If a name or local path provided, displays information of that mount instead.
* [`fuse-config`](contrib/docs/commands/fuse-config.md)`[--name=name] [--enable-at-startup=yes|no] [--persistent=yes|no] [--read-only=yes|no] (name|localPath)` Modifies the specified FUSE mount configuration.

### Misc.
* [`autocomplete`](contrib/docs/commands/autocomplete.md)`[dos | unix]` Modifes how tab completion operates.
* [`cancel`](contrib/docs/commands/cancel.md) Cancels your MEGA account
* [`cat`](contrib/docs/commands/cat.md)`remotepath1 remotepath2 ...` Prints the contents of remote files
* [`clear`](contrib/docs/commands/clear.md) Clear screen
* [`codepage`](contrib/docs/commands/codepage.md)`[N [M]]` Switches the codepage used to decide which characters show on-screen.
* [`configure`](contrib/docs/commands/configure.md)`[key [value]]` Shows and modifies global configurations.
* [`confirmcancel`](contrib/docs/commands/confirmcancel.md)`link password` Confirms the cancellation of your MEGA account
* [`debug`](contrib/docs/commands/debug.md) Enters debugging mode (HIGHLY VERBOSE)
* [`deleteversions`](contrib/docs/commands/deleteversions.md)`[-f] (--all | remotepath1 remotepath2 ...)  [--use-pcre]` Deletes previous versions.
* [`df`](contrib/docs/commands/df.md)`[-h]` Shows storage info
* [`errorcode`](contrib/docs/commands/errorcode.md)`number` Translate error code into string
* [`exit`](contrib/docs/commands/exit.md)`[--only-shell]` Quits MEGAcmd
* [`ftp`](contrib/docs/commands/ftp.md)`[-d ( --all | remotepath ) ] [ remotepath [--port=PORT] [--data-ports=BEGIN-END] [--public] [--tls --certificate=/path/to/certificate.pem --key=/path/to/certificate.key]] [--use-pcre]` Configures a FTP server to serve a location in MEGA
* [`graphics`](contrib/docs/commands/graphics.md)`[on|off]` Shows if special features related to images and videos are enabled.
* [`help`](contrib/docs/commands/help.md)`[-f|-ff|--non-interactive|--upgrade|--paths] [--show-all-options]` Prints list of commands
* [`https`](contrib/docs/commands/https.md)`[on|off]` Shows if HTTPS is used for transfers. Use "https on" to enable it.
* [`info`](contrib/docs/commands/info.md)`remotepath1 remotepath2 ...` Prints media info of remote files
* [`log`](contrib/docs/commands/log.md)`[-sc] level` Prints/Modifies the log level
* [`permissions`](contrib/docs/commands/permissions.md)`[(--files|--folders) [-s XXX]]` Shows/Establish default permissions for files and folders created by MEGAcmd.
* [`proxy`](contrib/docs/commands/proxy.md)`[URL|--auto|--none] [--username=USERNAME --password=PASSWORD]` Show or sets proxy configuration
* [`psa`](contrib/docs/commands/psa.md)`[--discard]` Shows the next available Public Service Announcement (PSA)
* [`quit`](contrib/docs/commands/quit.md)`[--only-shell]` Quits MEGAcmd
* [`reload`](contrib/docs/commands/reload.md) Forces a reload of the remote files of the user
* [`tree`](contrib/docs/commands/tree.md)`[remotepath]` Lists files in a remote path in a nested tree decorated output
* [`unicode`](contrib/docs/commands/unicode.md) Toggle unicode input enabled/disabled in interactive shell
* [`update`](contrib/docs/commands/update.md)`[--auto=on|off|query]` Updates MEGAcmd
* [`version`](contrib/docs/commands/version.md)`[-l][-c]` Prints MEGAcmd versioning and extra info

## Examples
Some examples of typical MEGAcmd workflows and commands.

**Note:** command output might differ. It could be slightly outdated, or it could've been manually re-formatted to better fit this markdown page.

### Account management
#### Creating new accounts
<pre>
MEGA CMD> <b>signup eg.email_1@example.co.nz --name="test1"</b>
New Password:
Retype New Password:
Account <eg.email_1@example.co.nz> created succesfully. You will receive a confirmation link. Use "confirm" with the provided link to confirm that account

MEGA CMD> <b>confirm https://mega.nz/#confirmQFSfjtUkExc5M2Us6q5d-klx60Rfx<REDACTED>Vbxjhk eg.email_1@example.co.nz</b>
Password:
Account eg.email_1@example.co.nz confirmed succesfully. You can login with it now

MEGA CMD> <b>signup eg.email_2@example.co.nz --name="test2"</b>
New Password:
Retype New Password:
Account <eg.email_2@example.co.nz> created succesfully. You will receive a confirmation link. Use "confirm" with the provided link to confirm that account

MEGA CMD> <b>confirm https://mega.nz/#confirmcz7Ss68ChhMKk8WEFTQCqLMHJg8es<REDACTED>AEEpQE eg.email_2@example.co.nz</b>
Password:
Account eg.email_2@example.co.nz confirmed succesfully. You can login with it now
</pre>

#### Logging-in and contacts
<pre>
MEGA CMD> <b>login eg.email_1@example.co.nz</b>
Password:
[SDK:info: 23:19:14] Fetching nodes ...
Fetching nodes ||########################################||(38/38 MB: 100.00 %)
[SDK:info: 23:19:17] Loading transfers from local cache
[SDK:info: 23:19:17] Login complete as eg.email_1@example.co.nz
</pre>

#### Adding a contact and viewing
<pre>
eg.email_1@example.co.nz:/$ <b>invite eg.email_2@example.co.nz</b>
Invitation to user: eg.email_2@example.co.nz sent

eg.email_1@example.co.nz:/$ <b>showpcr</b>
Outgoing PCRs:
 eg.email_2@example.co.nz  (id: 47Xhz6wvVTk, creation: Thu, 26 Apr 2018 11:20:09 +1200, modification: Thu, 26 Apr 2018 11:20:09 +1200)

eg.email_1@example.co.nz:/$ <b>logout</b>
Logging out...

MEGA CMD> <b>login eg.email_2@example.co.nz</b>
Password:
[SDK:info: 23:21:10] Fetching nodes ...
[SDK:info: 23:21:12] Loading transfers from local cache
[SDK:info: 23:21:12] Login complete as eg.email_2@example.co.nz

eg.email_2@example.co.nz:/$ <b>showpcr</b>
Incoming PCRs:
 eg.email_1@example.co.nz  (id: 47Xhz6wvVTk, creation: Thu, 26 Apr 2018 11:20:09 +1200, modification: Thu, 26 Apr 2018 11:20:09 +1200)

eg.email_2@example.co.nz:/$ <b>ipc 47Xhz6wvVTk -a</b>
Accepted invitation by eg.email_1@example.co.nz

eg.email_2@example.co.nz:/$ <b>users</b>
eg.email_1@example.co.nz, visible since Thu, 26 Apr 2018 11:22:02 +1200

eg.email_2@example.co.nz:/$ <b>userattr --user=eg.email_1@example.co.nz</b>
        firstname = test1
        ed25519 = 5Xl2-mUtsZkaATmSS88Ncepju5805uw66Hfdh_-SwpE
        cu25519 = ejoYtpaJIZvlpmPsYviIa6tNvPTdVjfkYf9G1k8PKgM
        rsa = AAAAAFrhDPPMS1AXAhJwScpJ_GKqFUJ42uIIcwxLp5RIalkWtsa5j87u2LFhoZlI_rHIzGXrdsbywgs7Msisw0CjodrtwtME
        cu255 = AAAAAFrhDPPWUOP2tNByV72zU4M3EKNoddyVCT13VkkouMldniR2UZtLrPjUjUeOZOLvOL7H1C0W0Q_b3QqYSvAKo775pUwD

eg.email_2@example.co.nz:/$ <b>logout</b>
Logging out...

MEGA CMD> <b>login eg.email_1@example.co.nz</b>
Password:
[SDK:info: 23:24:26] Fetching nodes ...
[SDK:info: 23:24:27] Loading transfers from local cache
[SDK:info: 23:24:27] Login complete as eg.email_1@example.co.nz

eg.email_1@example.co.nz:/$ <b>users</b>
eg.email_2@example.co.nz, visible

eg.email_1@example.co.nz:/$ <b>userattr --user=eg.email_2@example.co.nz</b>
        firstname = test2
        ed25519 = M7SLy2RajwUAvynxJQaVkhe6hxGpbwJmvve3dgl8B1o
        cu25519 = VaXluGS2c5xbo0xOHHJciqLRxwMaWZHVK8iuxtlCBTk
        rsa = AAAAAFrhDWemabQ4JAOtP7zcoy6m74PsFTFCbj04Zh4G8K_TZB5Sm9T5Xj9CXYzwWnpfRd1McPdDouKdsASQ6Er7i4Y4LpEA
        cu255 = AAAAAFrhDWcXE_7AHZmvxk5Hk0G7V65UnvFO42tb1gM9SYy3BpsMCas0X-pbqkYwf6_2eBG-ZLvkonGfXB3DWonWNvnVehIB
</pre>

### Node operations
#### Getting user info
<pre>
MEGA CMD> <b>login eg.email_1@example.co.nz</b>
Password:
[SDK:info: 23:43:14] Fetching nodes ...
[SDK:info: 23:43:14] Loading transfers from local cache
[SDK:info: 23:43:14] Login complete as eg.email_1@example.co.nz

eg.email_1@example.co.nz:/$ <b>whoami -l</b>
Account e-mail: eg.email_1@example.co.nz
    Available storage: 50.00 GBytes
        In ROOT:      146... KBytes in     1 file(s) and     0 folder(s)
        In INBOX:       0.00  Bytes in     0 file(s) and     0 folder(s)
        In RUBBISH:     0.00  Bytes in     0 file(s) and     0 folder(s)
        Total size taken up by file versions:      0.00  Bytes
    Pro level: 0
    Subscription type:
    Account balance:
Current Active Sessions:
    * Current Session
    Session ID: m3a8eluyPdo
    Session start: 4/26/2018 11:43:12 AM
    Most recent activity: 4/26/2018 11:43:13 AM
    IP: 122.56.56.232
    Country: NZ
    User-Agent: MEGAcmd/0.9.9.0 (Windows 10.0.16299) MegaClient/3.3.5
    -----
1 active sessions opened

eg.email_1@example.co.nz:/$ <b>mount</b>
ROOT on /
INBOX on //in
RUBBISH on //bin
</pre>

#### Downloading a file
<pre>
eg.email_1@example.co.nz:/$ <b>ls</b>
Welcome to MEGA.pdf

eg.email_1@example.co.nz:/$ <b>get "Welcome to MEGA.pdf"</b>
TRANSFERING ||################################################################################||(1/1 MB: 100.00 %)
Download finished: Welcome to MEGA.pdf
TRANSFERING ||################################################################################||(1/1 MB: 100.00 %)
</pre>

#### Uploading a file
<pre>
eg.email_1@example.co.nz:/$ <b>mkdir my-pictures</b>

eg.email_1@example.co.nz:/$ <b>cd my-pictures/</b>

eg.email_1@example.co.nz:/my-pictures$ <b>put C:\Users\MYWINDOWSUSER\Pictures</b>
TRANSFERING ||################################################################################||(1/1 MB: 100.00 %)
Upload finished: C:\Users\MYWINDOWUSER\Pictures
TRANSFERING ||################################################################################||(1/1 MB: 100.00 %)
</pre>

#### Creating and navigating directories
<pre>
eg.email_1@example.co.nz:/my-pictures$ <b>pwd</b>
/my-pictures

eg.email_1@example.co.nz:/my-pictures$ <b>ls</b>
Pictures

eg.email_1@example.co.nz:/my-pictures$ <b>cd Pictures/</b>

eg.email_1@example.co.nz:/my-pictures/my-pictures$ <b>ls</b>
Camera Roll
Feedback
Saved Pictures
megacmdpkg.gif
megacmdpkg_80.gif
megacmdpkg_gray.gif

eg.email_1@example.co.nz:/my-pictures/my-pictures$ <b>pwd</b>
/my-pictures/Pictures

eg.email_1@example.co.nz:/my-pictures/my-pictures$ <b>cd /</b>

eg.email_1@example.co.nz:/$ <b>du -h my-pictures/</b>
FILENAME                                        SIZE
my-pictures:                                 1.31 MB
----------------------------------------------------------------
Total storage used:                          1.31 MB
</pre>

#### Logging-out
<pre>
eg.email_1@example.co.nz:/$ <b>logout</b>
Logging out...
MEGA CMD>
</pre>

### Syncing
<pre>
email_1@example.co.nz:/$ <b>sync c:\Go go-backup/</b>
Added sync: //?\c:\Go to /go-backup

email_1@example.co.nz:/$ <b>sync</b>
ID          LOCALPATH                   REMOTEPATH RUN_STATE STATUS  ERROR SIZE      FILES DIRS
WOOmFwZfQwM \\?\c:\Go                   /go-backup Running   Syncing NO    119.13 KB 10    97

email_1@example.co.nz:/$ <b>sync</b>
ID          LOCALPATH                   REMOTEPATH RUN_STATE STATUS  ERROR SIZE     FILES DIRS
WOOmFwZfQwM \\?\c:\Go                   /go-backup Running   Syncing NO    61.22 MB 1252  463

email_1@example.co.nz:/$ <b>sync</b>
ID          LOCALPATH                   REMOTEPATH RUN_STATE STATUS  ERROR SIZE      FILES DIRS
WOOmFwZfQwM \\?\c:\Go                   /go-backup Running   Syncing NO    232.94 MB 4942  773

email_1@example.co.nz:/$ <b>sync</b>
ID          LOCALPATH                   REMOTEPATH RUN_STATE STATUS ERROR SIZE      FILES DIRS
WOOmFwZfQwM \\?\c:\Go                   /go-backup Running   Synced NO    285.91 MB 7710  1003
</pre>

Then, on a windows cmd prompt:
<pre>
C:\Users\ME><b>rmdir /s c:\go\blog</b>
c:\go\blog, Are you sure (Y/N)? <b>Y</b>
</pre>

Back in MEGAcmd (the update has been applied to MEGA already):
<pre>
email_1@example.co.nz:/$ <b>sync</b>
ID          LOCALPATH                   REMOTEPATH RUN_STATE STATUS ERROR SIZE      FILES DIRS
WOOmFwZfQwM \\?\c:\Go                   /go-backup Running   Synced NO    268.53 MB 7306  961
</pre>

### Backups
<pre>
eg.email@example.co.nz:/$ <b>backup c:/cmake /cmake-backup --period="0 0 4 * * *" --num-backups=3</b>
Backup established: c:/cmake into /cmake-backup period="0 0 4 * * *" Number-of-Backups=3

eg.email@example.co.nz:/$ <b>backup</b>
TAG   LOCALPATH                                               REMOTEPARENTPATH                                                STATUS
166   \\?\c:\cmake                                            /cmake-backup                                                  COMPLETE

eg.email@example.co.nz:/$ <b>backup -h</b>
TAG   LOCALPATH                                               REMOTEPARENTPATH                                                STATUS
166   \\?\c:\cmake                                            /cmake-backup                                                  COMPLETE
   -- HISTORY OF BACKUPS --
  NAME                                                    DATE                    STATUS   FILES FOLDERS
  cmake_bk_20180426133300                                 26Apr2018 13:33:00      COMPLETE     0      92
</pre>

### WebDAV
<pre>
eg.email@example.co.nz:/$ <b>webdav myfile.tif --port=1024</b>
Serving via webdav myfile.tif: http://127.0.0.1:1024/5mYHQT4B/myfile.tif

eg.email@example.co.nz:/$ <b>webdav</b>
WEBDAV SERVED LOCATIONS:
/myfile.tif: http://127.0.0.1:1024/5mYHQT4B/myfile.tif

eg.email@example.co.nz:/$ <b>webdav -d myfile.tif</b>
myfile.tif no longer served via webdav
</pre>

### Exporting and importing
<pre>
eg.email_1@example.co.nz:/$ <b>export -a Pictures/</b>
MEGA respects the copyrights of others and requires that users of the MEGA cloud service comply with the laws of copyright.
You are strictly prohibited from using the MEGA cloud service to infringe copyrights.
You may not upload, download, store, share, display, stream, distribute, email, link to, transmit or otherwise make available any files, data or content that infringes any copyright or other proprietary rights of any person or entity. Do you accept this terms? (Yes/No): Yes
Please enter [y]es/[n]o/[a]ll/none:yes
Exported /Pictures: https://mega.nz/#F!iaZlEBIL!mQD3rFuJhKov0sco-6s9xg

eg.email_1@example.co.nz:/$ <b>export</b>
Pictures (folder, shared as exported permanent folder link: https://mega.nz/#F!iaZlEBIL!mQD3rFuJhKov0sco-6s9xg)

eg.email_1@example.co.nz:/$ <b>logout --keep-session</b>
Logging out...
Session closed but not deleted. Warning: it will be restored the next time you execute the application. Execute "logout" to delete the session permanently.
You can also login with the session id: ARo7aiLAxK-jseOdVBYhj285Twb06ivWsFmT4XAnkTsiaDRRbm5oYS1zRm-V3I0FHHOvwj7P2RPvrSw_

MEGA CMD> <b>login eg.email_2@example.co.nz</b>
Password:
[SDK:info: 01:55:04] Fetching nodes ...
[SDK:info: 01:55:05] Loading transfers from local cache
[SDK:info: 01:55:05] Login complete as eg.email_2@example.co.nz

eg.email_2@example.co.nz:/$ <b>ls</b>
Welcome to MEGA.pdf

eg.email_2@example.co.nz:/$ <b>import https://mega.nz/#F!iaZlEBIL!mQD3rFuJhKov0sco-6s9xg</b>
Imported folder complete: /Pictures

eg.email_2@example.co.nz:/$ <b>ls</b>
Pictures
Welcome to MEGA.pdf

eg.email_2@example.co.nz:/$ <b>ls Pictures/</b>
Camera Roll
Feedback
Saved Pictures
megacmdpkg.gif
megacmdpkg_80.gif
megacmdpkg_gray.gif

eg.email_2@example.co.nz:/$ <b>logout</b>
Logging out...

MEGA CMD> <b>login ARo7aiLAxK-jseOdVBYhj285Twb06ivWsFmT4XAnkTsiaDRRbm5oYS1zRm-V3I0FHHOvwj7P2RPvrSw_</b>
eg.email_1@example.co.nz:/$ <b>export</b>
Pictures (folder, shared as exported permanent folder link: https://mega.nz/#F!iaZlEBIL!mQD3rFuJhKov0sco-6s9xg)

eg.email_1@example.co.nz:/$ <b>export -d Pictures/</b>
Disabled export: /Pictures

eg.email_1@example.co.nz:/$ <b>export</b>
Couldn't find anything exported below current folder. Use -a to export it
</pre>

### Transfers
<pre>
eg.email@example.co.nz:/tmp-test/Mega.dir$ <b>transfers</b>
DIR/SYNC TAG  SOURCEPATH                         DESTINYPATH                              PROGRESS           STATE
 U     17361 \\?\C:\Users\ME\...ebug\megaapi.obj /tmp-test/Mega.dir/Mega.dir/Debug    100.00% of 2016.62 KB  ACTIVE
 U     17362 \\?\C:\Users\ME\...megaapi_impl.obj /tmp-test/Mega.dir/Mega.dir/Debug     13.64% of   13.85 MB  ACTIVE
 U     17363 \\?\C:\Users\ME\...g\megaclient.obj /tmp-test/Mega.dir/Mega.dir/Debug      0.00% of   15.46 MB  QUEUED
 U     17364 \\?\C:\Users\ME\..._http_parser.obj /tmp-test/Mega.dir/Mega.dir/Debug      0.00% of   85.15 KB  QUEUED
 U     17365 \\?\C:\Users\ME\...ega_utf8proc.obj /tmp-test/Mega.dir/Mega.dir/Debug      0.00% of  312.44 KB  QUEUED
 U     17366 \\?\C:\Users\ME\...\mega_zxcvbn.obj /tmp-test/Mega.dir/Mega.dir/Debug      0.00% of  589.88 KB  QUEUED
 U     17367 \\?\C:\Users\ME\...ir\Debug\net.obj /tmp-test/Mega.dir/Mega.dir/Debug      0.00% of    3.20 MB  QUEUED
 U     17368 \\?\C:\Users\ME\...r\Debug\node.obj /tmp-test/Mega.dir/Mega.dir/Debug      0.00% of    3.73 MB  QUEUED
 U     17369 \\?\C:\Users\ME\...ntactrequest.obj /tmp-test/Mega.dir/Mega.dir/Debug      0.00% of  352.22 KB  QUEUED
 U     17370 \\?\C:\Users\ME\...\Debug\proxy.obj /tmp-test/Mega.dir/Mega.dir/Debug      0.00% of  203.57 KB  QUEUED
 ...  Showing first 10 transfers ...

eg.email@example.co.nz:/tmp-test/Mega.dir$ <b>transfers -p 17367</b>
Transfer 17367 paused successfully.

eg.email@example.co.nz:/tmp-test/Mega.dir$ <b>transfers -c 17370</b>
Transfer 17370 cancelled successfully.

eg.email@example.co.nz:/tmp-test/Mega.dir$ <b>transfers</b>
DIR/SYNC TAG  SOURCEPATH                         DESTINYPATH                              PROGRESS           STATE
 U     17362 \\?\C:\Users\ME\...megaapi_impl.obj /tmp-test/Mega.dir/Mega.dir/Debug     96.32% of   13.85 MB  ACTIVE
 U     17363 \\?\C:\Users\ME\...g\megaclient.obj /tmp-test/Mega.dir/Mega.dir/Debug      0.20% of   15.46 MB  ACTIVE
 U     17364 \\?\C:\Users\ME\..._http_parser.obj /tmp-test/Mega.dir/Mega.dir/Debug      0.00% of   85.15 KB  QUEUED
 U     17365 \\?\C:\Users\ME\...ega_utf8proc.obj /tmp-test/Mega.dir/Mega.dir/Debug      0.00% of  312.44 KB  QUEUED
 U     17366 \\?\C:\Users\ME\...\mega_zxcvbn.obj /tmp-test/Mega.dir/Mega.dir/Debug      0.00% of  589.88 KB  QUEUED
 U     17367 \\?\C:\Users\ME\...ir\Debug\net.obj /tmp-test/Mega.dir/Mega.dir/Debug      0.00% of    3.20 MB  PAUSED
 U     17368 \\?\C:\Users\ME\...r\Debug\node.obj /tmp-test/Mega.dir/Mega.dir/Debug      0.00% of    3.73 MB  QUEUED
 U     17369 \\?\C:\Users\ME\...ntactrequest.obj /tmp-test/Mega.dir/Mega.dir/Debug      0.00% of  352.22 KB  QUEUED
 U     17371 \\?\C:\Users\ME\...pubkeyaction.obj /tmp-test/Mega.dir/Mega.dir/Debug      0.00% of  355.75 KB  QUEUED
 U     17372 \\?\C:\Users\ME\...ebug\request.obj /tmp-test/Mega.dir/Mega.dir/Debug      0.00% of  933.14 KB  QUEUED
 ...  Showing first 10 transfers ...
</pre>

### Shared folders
<pre>
eg.email@example.co.nz:/$ mount
ROOT on /
INBOX on //in
RUBBISH on //bin
INSHARE on //from/family.member@example.co.nz:photos_Jan_1_2020 (read access)
INSHARE on //from/family.member@example.co.nz:other_folder (read access)

eg.email@example.co.nz:/$ ls family.member@example.co.nz:photos_Jan_1_2020
photo1.jpg
photo2.jpg

eg.email@example.co.nz:/$ get family.member@example.co.nz:photos_Jan_1_2020/photo1.jpg
TRANSFERRING ||###########################################################################################||(5/5 MB: 100.00 %)
Download finished: .\photo1.jpg

eg.email@example.co.nz:/$ share -a --with=family.member@example.co.nz --level=0  "/Camera Uploads/my_photos_from_that_day"
Shared /Camera Uploads/my_photos_from_that_day : family.member@example.co.nz accessLevel=0
</pre>
