# MEGAcmd User Guide

This document relates to MEGAcmd version 0.9.9.

### What is it
A command line tool to work with your MEGA account and files.  You can run it in interactive mode where it processes all commands directly, or you can use all the features from your favourite Linux or Mac shell such as bash, or you can even run it in a Windows command prompt.   You can also use its commands in scripts.

In order to enable running commands from scripts or OS shells without supplying a password every time, MEGAcmd runs a command server in the background, which the MEGAcmd shell or the script commands will forward requests to.   The server will exit when the MEGAcmd shell is closed with `exit` or `quit`, or the `mega-quit` shell script is used.  Using any of the `mega-*` shell scripts will also start the server.   

Working with your MEGA account requires signing in with your email and password using the `login` command, though you can download public links or upload to public folders without logging in.   Logging in causes some of your account such as the folder structure to be downloaded and cached locally for performance (encrypted, of course).  That cache is kept on disk, and will be reused the next time you run MEGAcmd under the same account.  Once you are logged in, the running server will maintain your access to your account.   If you exit it (with `quit`, `exit`, etc, you will need to log in again when you next start it.  For convenience you can use the `session` command to get the ID of your current session that can be used with the `login` command in future to log back into the same session.  That session string is valid until the session is closed with `logout`, or cancelled with `killsession`.   You must keep the id secure as it provides access to your account without a password.  You can see all the active session IDs with `whoami -l`.


### Where can you get it  
Download it from the MEGA.nz website: https://mega.nz/cmd

### What can you do with it
The major features are
	* Move files around inside your MEGA account or between MEGA and your PC in a way similar to how you manage files in a Linux shell or Windows command prompt.
	* Use those same commands in scripts to manage your files.
	* Set up synchronization or a backup schedule between a folder on your machine, and a folder on your MEGA account.   (use the `sync` or `backup` commands)

Security aspects
	* Where does it store temp files / cached account data (encrypted of course)
	* Keep your password safe
	* Be careful with sessionids

## Terminology and description

Contact

Remote Path

Local Path

Session ID

Local Cache



## Command Summary

These summaries use the usual conventions - `[]` indicates its content is optional,  `|` indicates you should choose either the item on the left or the one on the right (but not both)

Each command is described as it would be used in the MEGAcmd shell, and the corresponding shell script (prefixed with "mega-") works in the same way.

Commands referring to a `remote path` are talking about a file in your MEGA account online, whereas a `local path` refers to a file or folder on your local device where MEGAcmd is running.

Verbosity: You can increase the amount of information given by any command by passing `-v` (`-vv`, `-vvv`, ...)

### Account / Contacts
* [`signup`](#signup)`email [password] [--name="Your Name"]`  * Register as user with a given email
* [`confirm`](#confirm)`link email [password]`  Confirm an account using the link provided after the "signup" process.
* [`invite`](#invite)`[-d|-r] dstemail [--message="MESSAGE"]`  Invites a contact / deletes an invitation
* [`showpcr`](#showpcr)`[--in | --out]`  Shows incoming and outgoing contact requests.
* [`ipc`](#ipc)`email|handle -a|-d|-i`  Manages contact incoming invitations.
* [`passwd`](#passwd)`[oldpassword newpassword]`  Modifies user password
* [`masterkey`](#masterkey)`pathtosave`  Shows your master key.

### Login / Logout
* [`login`](#login)`[email [password]] | exportedfolderurl#key | session` Logs into MEGA
* [`logout`](#logout)`[--keep-session]` Logs out
* [`whoami`](#whoami)`[-l]` Print info of the user
* [`session`](#session) Prints (secret) session ID
* [`killsession`](#killsession)`[-a|sessionid]` Kills a session of current user.
* [`userattr`](#userattr)`[-s attribute value|attribute] [--user=user@email]` Lists/updates user attributes
* [`users`](#users)`[-s] [-h] [-n] [-d contact@email]` List contacts
	  
### Browse
* [`cd`](#cd)`[remotepath]` Changes the current remote folder
* [`lcd`](#lcd)`[localpath]` Changes the current local folder for the interactive console
* [`ls`](#ls)`[-lRr] [remotepath]` Lists files in a remote path
* [`pwd`](#pwd) Prints the current remote folder
* [`lpwd`](#lpwd) Prints the current local folder for the interactive console
* [`attr`](#attr)`remotepath [-s attribute value|-d attribute]`  Lists/updates node attributes
* [`du`](#du)`[-h] [remotepath remotepath2 remotepath3 ... ]` Prints size used by files/folders
* [`find`](#find)`[remotepath] [-l] [--pattern=PATTERN] [--mtime=TIMECONSTRAIN] [--size=SIZECONSTRAIN]` Find nodes matching a pattern
* [`mount`](#mount) Lists all the main nodes

### Moving/Copying Files
* [`mkdir`](#mkdir)`[-p] remotepath` Creates a directory or a directory hierarchy
* [`cp`](#cp)`srcremotepath dstremotepath|dstemail` Moves a file/folder into a new location (all remotes)
* [`put`](#put)`[-c] [-q] [--ignore-quota-warn] localfile [localfile2 localfile3 ...] [dstremotepath]` Uploads files/folders to a remote folder
* [`get`](#get)`[-m] [-q] [--ignore-quota-warn] exportedlink#key|remotepath [localpath]` Downloads a remote file/folder or a public link
* [`preview`](#preview)`[-s] remotepath localpath` To download/upload the preview of a file.
* [`thumbnail`](#thumbnail)`[-s] remotepath localpath` To download/upload the thumbnail of a file.
* [`mv`](#mv)`srcremotepath [srcremotepath2 srcremotepath3 ..] dstremotepath` Moves file(s)/folder(s) into a new location (all remotes)
* [`rm`](#rm)`[-r] [-f] remotepath` Deletes a remote file/folder
* [`transfers`](#transfers)`[-c TAG|-a] | [-r TAG|-a]  | [-p TAG|-a] [--only-downloads | --only-uploads] [SHOWOPTIONS]` List or operate with transfers
* [`speedlimit`](#speedlimit)`[-u|-d] [-h] [NEWLIMIT]` Displays/modifies upload/download rate limits
* [`sync`](#sync)`[localpath dstremotepath| [-dsr] [ID|localpath]` Controls synchronizations
* [`exclude`](#exclude)`[(-a|-d) pattern1 pattern2 pattern3 [--restart-syncs]]` Manages exclusions in syncs.
* [`backup`](#backup)`localpath remotepath --period="PERIODSTRING" --num-backups=N`  Set up a new backup folder and/or schedule
* [`backup`](#backup)`[-lhda] [TAG|localpath] [--period="PERIODSTRING"] [--num-backups=N])`  View/Modify an existing backup schedule 
	  
### Sharing (your own files, of course, without infringing any copyright)
* [`cp`](#cp)`srcremotepath dstremotepath|dstemail` Moves a file/folder into a new location (all remotes)
* [`export`](#export)`[-d|-a [--expire=TIMEDELAY]] [remotepath]` Prints/Modifies the status of current exports
* [`import`](#import)`exportedfilelink#key [remotepath]` Imports the contents of a remote link into your account
* [`share`](#share)`[-p] [-d|-a --with=user@email.com [--level=LEVEL]] [remotepath]` Prints/Modifies the status of current shares
* [`webdav`](#webdav)`[ [-d] remotepath [--port=PORT] [--public] [--tls --certificate=/path/to/certificate.pem --key=/path/to/certificate.key]]`  Sets up the ability to download a file from your MEGA account via your PC/device.

### Misc
* [`version`](#version)`[-l][-c]` Prints MEGAcmd versioning and extra info
* [`unicode`](#unicode) Toggle unicode input enabled/disabled in interactive shell
* [`reload`](#reload) Forces a reload of the remote files of the user
* [`help`](#help)`[-f]` Prints list of commands
* [`https`](#https)`[on|off]` Shows if HTTPS is used for transfers. Use "https on" to enable it.
* [`clear`](#clear) Clear screen
* [`log`](#log)`[-sc] level` Prints/Modifies the current logs level
* [`debug`](#debug) Enters debugging mode (HIGHLY VERBOSE)
* [`exit`](#exit)|[`quit`] [--only-shell]` Quits MEGAcmd


## Command Detail

### attr
Usage: attr remotepath [-s attribute value|-d attribute]
Lists/updates node attributes

Options:
* `-s attribute-value` \t sets an attribute to a value
* `-d attribute` \t removes the attribute

### backup
Controls backups
Usage: `backup (localpath remotepath --period="PERIODSTRING" --num-backups=N  | [-lhda] [TAG|localpath] [--period="PERIODSTRING"] [--num-backups=N])`

<pre>
This command can be used to configure and control backups.
A tutorial can be found here: https://github.com/meganz/MEGAcmd/blob/master/contrib/docs/BACKUPS.md

If no argument is given it will list the configured backups.
To get extra info on backups use -l or -h (see Options below)

When a backup of a folder (localfolder) is established in a remote folder (remotepath)
 MEGAcmd will create subfolder within the remote path with names like: "localfoldername_bk_TIME"
 which shall contain a backup of the local folder at that specific time
In order to configure a backup you need to specify the local and remote paths,
the period and max number of backups to store (see Configuration Options below).
Once configured, you can see extended info asociated to the backup (See Display Options)
Notice that MEGAcmd server need to be running for backups to be created.

Display Options:
-l      Show extended info: period, max number, next scheduled backup
         or the status of current/last backup
-h      Show history of created backups
        Backup states:
        While a backup is being performed, the backup will be considered and labeled as ONGOING
        If a transfer is cancelled or fails, the backup will be considered INCOMPLETE
        If a backup is aborted (see -a), all the transfers will be canceled and the backup be ABORTED
        If MEGAcmd server stops during a transfer, it will be considered MISCARRIED
          Notice that currently when MEGAcmd server is restarted, ongoing and scheduled transfers
          will be carried out nevertheless.
        If MEGAcmd server is not running when a backup is scheduled and the time for the next one has already arrived, an empty BACKUP will be created with state SKIPPED
        If a backup(1) is ONGOING and the time for the next backup(2) arrives, it won't start untill the previous one(1)
         is completed, and if by the time the first one(1) ends the time for the next one(3) has already arrived,
         an empty BACKUP(2) will be created with state SKIPPED
 --path-display-size=N  Use a fixed size of N characters for paths

Configuration Options:
--period="PERIODSTRING" Period: either time in TIMEFORMAT (see below) or a cron like expresisions
                         Cron like period is formatted as follows
                          - - - - - -
                          | | | | | |
                          | | | | | |
                          | | | | | +---- Day of the Week   (range: 1-7, 1 standing for Monday)
                          | | | | +------ Month of the Year (range: 1-12)
                          | | | +-------- Day of the Month  (range: 1-31)
                          | | +---------- Hour              (range: 0-23)
                          | +------------ Minute            (range: 0-59)
                          +-------------- Second            (range: 0-59)
                         examples:
                          - daily at 04:00:00 (UTC): "0 0 4 * * *"
                          - every 15th day at 00:00:00 (UTC) "0 0 0 15 * *"
                          - mondays at 04.30.00 (UTC): "0 30 4 * * 1"
                         TIMEFORMAT can be expressed in hours(h), days(d),
                           minutes(M), seconds(s), months(m) or years(y)
                           e.g. "1m12d3h" indicates 1 month, 12 days and 3 hours
                          Notice that this is an uncertain measure since not all months
                          last the same and Daylight saving time changes are not considered
                          If possible use a cron like expresion
                        Notice: regardless of the period expresion, the first time you establish a backup,
                         it will be created inmediately
--num-backups=N Maximum number of backups to store
                         After creating the backup (N+1) the oldest one will be deleted
                          That might not be true in case there are incomplete backups:
                           in order not to lose data, at least one COMPLETE backup will be kept
Use backup TAG|localpath --option=VALUE to modify existing backups

Management Options:
-d TAG|localpath        Removes a backup by its TAG or local path
                         Folders created by backup won't be deleted
-a TAG|localpath        Aborts ongoing backup

Caveat: This functionality is in BETA state. If you experience any issue with this, please contact: support@mega.nz
</pre>

### cd
Changes the current remote folder

Usage: `cd [remotepath]`
<pre>
If no folder is provided, it will be changed to the root folder
<pre>

### clear
Clear screen

Usage: `clear`

### confirm
Confirm an account using the link provided after the "signup" process.
Usage: `confirm link email [password]`
It requires the email and the password used to obtain the link.

### cp
Moves a file/folder into a new location (all remotes)

Usage: `cp srcremotepath dstremotepath|dstemail:`
<pre>
If the location exists and is a folder, the source will be copied there.
If the location doesn't exist, the file/folder will be renamed to the destination name given.

If "dstemail:" provided, the file/folder will be sent to that user's inbox (//in)
 e.g: cp /path/to/file user@doma.in:
 Remember the trailing ":", otherwise a file with the name of that user ("user@doma.in") will be created
</pre>

### debug
Enters debugging mode (HIGHLY VERBOSE)
Usage: `debug`
<pre>
For a finer control of log level see "log --help"
</pre>

### deleteversions
Deletes previous versions of files, keeping the current version.
Usage: `deleteversions [-f] (--all | remotepath1 remotepath2 ...)`
<pre>
This will permanently delete all historical versions of a file.
The current version of the file will remain.
Note: any file version shared to you from a contact will need to be deleted by them.

Options:
 -f     Force (no asking)
 --all  Delete versions of all nodes. This will delete the version histories of all files (not current files).

To see versions of a file use "ls --versions".
To see space occupied by file versions use "du" with "--versions".
</pre>

### du
Prints size used by files/folders
Usage: `du [-h] [--versions] [remotepath remotepath2 remotepath3 ... ]`
<pre>
remotepath can be a pattern (it accepts wildcards: ? and *. e.g.: f*00?.txt)

Options:
 -h     Human readable
 --versions     Calculate size including all versions.
        You can remove all versions with "deleteversions" and list them with "ls --versions"
</pre>

### exclude
Manages exclusions in syncs.
Usage: `exclude [(-a|-d) pattern1 pattern2 pattern3 [--restart-syncs]]`
<pre>
Options:
 -a pattern1 pattern2 ...       adds pattern(s) to the exclusion list
                                          (* and ? wildcards allowed)
 -d pattern1 pattern2 ...       deletes pattern(s) from the exclusion list
 --restart-syncs        Try to restart synchronizations.

Changes will not be applied inmediately to actions being performed in active syncs.
After adding/deleting patterns, you might want to:
 a) disable/reenable synchronizations manually
 b) restart MEGAcmd server
 c) use --restart-syncs flag. Caveats:
  This will cause active transfers to be restarted
  In certain cases --restart-syncs might be unable to re-enable a synchronization.
  In such case, you will need to manually resume it or restart MEGAcmd server.
</pre>

### exit
Quits MEGAcmd
Usage: `exit [--only-shell]`
<pre>
By default this command will exit both the interactive shell and the command server.
To only exit current shell and keep server running, use "exit --only-shell"

Exiting the server does not cancel the session, and the encrypted local cache of your account is kept on your PC.
The session will be resumed when the service is restarted.
</pre>

### export
Prints/Modifies the status of current exports

Usage: `export [-d|-a [--expire=TIMEDELAY] [-f]] [remotepath]`
<pre>
Options:
 -a     Adds an export (or modifies it if existing)
 --expire=TIMEDELAY     Determines the expiration time of a node.
                           It indicates the delay in hours(h), days(d),
                           minutes(M), seconds(s), months(m) or years(y)
                           e.g. "1m12d3h" establish an expiration time 1 month,
                           12 days and 3 hours after the current moment
 -f     Implicitly accept copyright terms (only shown the first time an export is made)
        MEGA respects the copyrights of others and requires that users of the MEGA cloud service
        comply with the laws of copyright.
        You are strictly prohibited from using the MEGA cloud service to infringe copyrights.
        You may not upload, download, store, share, display, stream, distribute, email, link to,
        transmit or otherwise make available any files, data or content that infringes any copyright
        or other proprietary rights of any person or entity.
 -d     Deletes an export

If a remote path is given it'll be used to add/delete or in case of no option selected,
 it will display all the exports existing in the tree of that path
</pre>

### find
Find nodes matching a pattern

Usage: `find [remotepath] [-l] [--pattern=PATTERN] [--mtime=TIMECONSTRAIN] [--size=SIZECONSTRAIN]`
<pre>
Options:
 --pattern=PATTERN      Pattern to match (it accepts wildcards: ? and *. e.g.: f*00?.txt)
 --mtime=TIMECONSTRAIN  Determines time constrains, in the form: [+-]TIMEVALUE
                          TIMEVALUE may include hours(h), days(d), minutes(M),
                           seconds(s), months(m) or years(y)
                          Examples:
                           "+1m12d3h" shows files modified before 1 month,
                            12 days and 3 hours the current moment
                           "-3h" shows files modified within the last 3 hours
                           "-3d+1h" shows files modified in the last 3 days prior to the last hour
 --size=SIZECONSTRAIN   Determines size constrains, in the form: [+-]TIMEVALUE
                          TIMEVALUE may include (B)ytes, (K)ilobytes, (M)egabytes, (G)igabytes & (T)erabytes
                          Examples:
                           "+1m12k3B" shows files bigger than 1 Mega, 12 Kbytes and 3Bytes
                           "-3M" shows files smaller than 3 Megabytes
                           "-4M+100K" shows files smaller than 4 Mbytes and bigger than 100 Kbytes
 -l     Prints file info
</pre>

### get
Downloads a remote file/folder or a public link

Usage: `get [-m] [-q] [--ignore-quota-warn] exportedlink#key|remotepath [localpath]`
<pre>
In case it is a file, the file will be downloaded at the specified folder
                             (or at the current folder if none specified).
  If the localpath(destiny) already exists and is the same (same contents)
  nothing will be done. If differs, it will create a new file appending " (NUM)"
  if the localpath(destiny) is a folder with a file with the same name on it,
         it will preserve the, it will create a new file appending " (NUM)"

For folders, the entire contents (and the root folder itself) will be
                    by default downloaded into the destination folder
Options:
 -q     queue download: execute in the background. Don't wait for it to end'
 -m     if the folder already exists, the contents will be merged with the
                     downloaded one (preserving the existing files)
 --ignore-quota-warn    ignore quota surpassing warning.
                          The download will be attempted anyway.
</pre>

### help
Prints list of commands

Usage: `help [-f]`
<pre>
Options:
 -f     Include a brief description of the commands
</pre>

### https
Shows if HTTPS is used for transfers. Use "https on" to enable it.  

Usage: `https [on|off]`
<pre>
HTTPS is not necesary since all data is already encrypted before being stored or transfered anyway.
Enabling it will increase CPU usage and add network overhead.

This setting is ephemeral: it will reset for the next time you open MEGAcmd
</pre>

### import
Imports the contents of a remote link into your MEGA account or to a local folder.

Usage: `import exportedfilelink#key [remotepath]`
<pre>
If no remote path is provided, the current local folder will be used
</pre>

### invite
Invites a contact / deletes an invitation
Usage: invite [-d|-r] dstemail [--message="MESSAGE"]
<pre>
Options:
 -d     Deletes invitation
 -r     Sends the invitation again
 --message="MESSAGE"    Sends inviting message

Use "showpcr" to browse invitations
Use "ipc" to manage invitations received
Use "users" to see contacts
</pre>

### ipc
Manages contact incoming invitations.

Usage: `ipc email|handle -a|-d|-i`
<pre>
Options:
 -a     Accepts invitation
 -d     Rejects invitation
 -i     Ignores invitation [WARNING: do not use unless you know what you are doing]

Use "invite" to send/remove invitations to other users
Use "showpcr" to browse incoming/outgoing invitations
Use "users" to see contacts
</pre>

### killsession
Kills a session of current user.

Usage: killsession [-a|sessionid]
<pre>
Options:
 -a     kills all sessions except the current one

To see all sessions use "whoami -l"
</pre>

### lcd
Usage: lcd [localpath]
<pre>
Changes the current local folder for the interactive console

It will be used for uploads and downloads

If not using interactive console, the current local folder will be
 that of the shell executing mega comands
</pre>

### log
Prints/Modifies the setting for how detailed log output is.

Usage: log [-sc] level
<pre>
Options:
 -c     CMD log level (higher level messages).
         Messages captured by MEGAcmd server.
 -s     SDK log level (lower level messages).
         Messages captured by the engine and libs

Regardless of the log level of the
 interactive shell, you can increase the amount of information given
   by any command by passing "-v" ("-vv", "-vvv", ...)
</pre>

### login
Logs into a MEGA account

Usage: `login [email [password]] | exportedfolderurl#key | session`
<pre>
 You can log in either with email and password, with session ID,
 or into a folder (an exported/public folder)
 If logging into a folder indicate url#key
</pre>

### logout
Logs out

Usage: logout [--keep-session]
<pre>
Options:
 --keep-session Keeps the current session.
</pre>

### lpwd
Prints the current local folder for the interactive console

Usage: `lpwd`
<pre>
It will be used for uploads and downloads

If not using interactive console, the current local folder will be
 that of the shell executing mega comands
</pre>

### ls
Usage: `ls [-halRr] [--versions] [remotepath]`
Lists files in a remote path

<pre>
 remotepath can be a pattern (it accepts wildcards: ? and *. e.g.: f*00?.txt)
 Also, constructions like /PATTERN1/PATTERN2/PATTERN3 are allowed

Options:
 -R|-r  list folders recursively
 -l     print summary
         SUMMARY contents:
           FLAGS: Indicate type/status of an element:
             xxxx
             |||+---- Sharing status: (s)hared, (i)n share or not shared(-)
             ||+----- if exported, whether it is (p)ermanent or (t)temporal
             |+------ e/- wheter node is (e)xported
             +-------- Type(d=folder,-=file,r=root,i=inbox,b=rubbish,x=unsupported)
           VERS: Number of versions in a file
           SIZE: Size of the file in bytes:
           DATE: Modification date for files and creation date for folders:
           NAME: name of the node
 -h     Show human readable sizes in summary
 -a     include extra information
 --versions     show historical versions
        You can delete all versions of a file with "deleteversions"
</pre>

### masterkey
Shows your master key.

Usage: `masterkey pathtosave`
<pre>
Getting the master key and keeping it in a secure location enables you to set a new password without data loss.
Always keep physical control of your master key (e.g. on a client device, external storage, or print)
</pre>

### mkdir
Creates a directory or a directories hierarchy

Usage: `mkdir [-p] remotepath`
<pre>
Options:
 -p     Allow recursive
</pre>

### mount
Lists all the main nodes

Usage: `mount`


### mv
Moves file(s)/folder(s) into a new location (all remotes)

Usage: `mv srcremotepath [srcremotepath2 srcremotepath3 ..] dstremotepath`
<pre>
If the destination remote path exists and is a folder, the source will be moved there.
If the destination remote path doesn't exist, the source will be renamed to the name given.
</pre>

### passwd
Modifies user password

Usage: `passwd [oldpassword newpassword]`

### preview
To download/upload the preview of a file.

Usage: preview [-s] remotepath localpath
<pre>
If no -s is inidicated, it will download the preview.

Options:
 -s     Sets the preview to the specified file
</pre>

### put
Uploads files/folders to a remote folder

Usage: `put  [-c] [-q] [--ignore-quota-warn] localfile [localfile2 localfile3 ...] [dstremotepath]`
<pre>
Options:
 -c     Creates remote folder destination in case of not existing.
 -q     queue upload: execute in the background. Don't wait for it to end'
 --ignore-quota-warn    ignore quota surpassing warning.
                          The upload will be attempted anyway.

Notice that the dstremotepath can only be omitted when only one local path is provided.
 In such case, the current remote working dir will be the destination for the upload.
 Mind that using wildcards for local paths will result in multiple paths.
</pre>

### pwd
Prints the current remote folder

Usage: `pwd`

### quit

Usage: `quit [--only-shell]`
<pre>
Quits MEGAcmd

Notice that the session will still be active, and local caches available
The session will be resumed when the service is restarted

Be aware that this will exit both the interactive shell and the server.
To only exit current shell and keep server running, use "exit --only-shell"
</pre>

### reload
Forces a reload of the remote files of the user

Usage: `reload`
<pre>
It will also resume synchronizations.

### rm
Deletes a remote file/folder

Usage: `rm [-r] [-f] remotepath`
<pre>
Options:
 -r     Delete recursively (for folders)
 -f     Force (no asking)
</pre>

### session
Prints (secret) session ID

Usage: `session`

### share
Prints/Modifies the status of current shares

Usage: `share [-p] [-d|-a --with=user@email.com [--level=LEVEL]] [remotepath]`
<pre>
Options:
 -p     Show pending shares
 --with=email   Determines the email of the user to [no longer] share with
 -d     Stop sharing with the selected user
 -a     Adds a share (or modifies it if existing)
 --level=LEVEL  Level of acces given to the user
                0: Read access
                1: Read and write
                2: Full access
                3: Owner access

If a remote path is given it'll be used to add/delete or in case
 of no option selected, it will display all the shares existing
 in the tree of that path

When sharing a folder with a user that is not a contact (see "users --help")
  the share will be in a pending state. You can list pending shares with
 "share -p". He would need to accept your invitation (see "ipc")

If someone has shared something with you, it will be listed as a root folder.
Use "mount" to list folders shared with you
</pre>

### showpcr
Shows incoming and outgoing contact requests.

Usage: `showpcr [--in | --out]`
<pre>
Options:
 --in   Shows incoming requests
 --out  Shows outgoing invitations

Use "ipc" to manage invitations received
Use "users" to see contacts
</pre>

### signup
Register as user with a given email

Usage: `signup email [password] [--name="Your Name"]`
<pre>
Options:
 --name="Your Name"     Name to register. e.g. "John Smith"

 You will receive an email to confirm your account.
 Once you have received the email, please proceed to confirm the link
 included in that email with "confirm".
</pre>

### speedlimit
Displays/modifies upload/download rate limits

Usage: `speedlimit [-u|-d] [-h] [NEWLIMIT]`
<pre>
 NEWLIMIT establish the new limit in size per second (0 = no limit)
 NEWLIMIT may include (B)ytes, (K)ilobytes, (M)egabytes, (G)igabytes & (T)erabytes.
  Examples: "1m12k3B" "3M". If no unit given, it'll use Bytes

Options:
 -d     Download speed limit
 -u     Upload speed limit
 -h     Human readable

Notice: this limit will be saved for the next time you execute MEGAcmd server. They will be removed if you logout.
</pre>

### sync
Controls synchronizations

Usage: `sync [localpath dstremotepath| [-dsr] [ID|localpath]`
<pre>
If no argument is provided, it lists current configured synchronizations

If provided local and remote paths, it will start synchronizing
 a local folder into a remote folder

If an ID/local path is provided, it will list such synchronization
 unless an option is specified.

Options:
-d ID|localpath deletes a synchronization
-s ID|localpath stops(pauses) a synchronization
-r ID|localpath resumes a synchronization
--path-display-size=N  Use a fixed size of N characters for paths
</pre>

### thumbnail
To download/upload the thumbnail of a file.

Usage: thumbnail [-s] remotepath localpath
<pre>
If no -s is inidicated, it will download the thumbnail.

Options:
 -s     Sets the thumbnail to the specified file
</pre>

### transfers
List or operate with queued transfers

Usage: `transfers [-c TAG|-a] | [-r TAG|-a]  | [-p TAG|-a] [--only-downloads | --only-uploads] [SHOWOPTIONS]`
<pre>
If executed without option it will list the first 10 tranfers
Options:
 -c (TAG|-a)    Cancel transfer with TAG (or all with -a)
 -p (TAG|-a)    Pause transfer with TAG (or all with -a)
 -r (TAG|-a)    Resume transfer with TAG (or all with -a)
 -only-uploads  Show/Operate only upload transfers
 -only-downloads        Show/Operate only download transfers

Show options:
 -show-syncs            Show synchronization transfers
 -show-completed        Show completed transfers
 -only-completed        Show only completed download
 --limit=N      Show only first N transfers
 --path-display-size=N  Use a fixed size of N characters for paths
</pre>

### unicode
Toggle unicode input enabled/disabled in interactive shell

Usage: `unicode`
<pre>
 Unicode mode is experimental, you might experience
 some issues interacting with the console
 (e.g. history navigation fails).
Type "help --unicode" for further info
</pre>

### userattr
Lists/updates user attributes

Usage: `userattr [-s attribute value|attribute] [--user=user@email]`
<pre>
Options:
 -s     attribute value         sets an attribute to a value
 --user=user@email      select the user to query
</pre>

### users
List contacts

Usage: `users [-s] [-h] [-n] [-d contact@email]`
<pre>
Options:
 -s     Show shared folders with listed contacts
 -h     Show all contacts (hidden, blocked, ...)
 -n     Show users names
 -d     contact@email Deletes the specified contact

Use "invite" to send/remove invitations to other users
Use "showpcr" to browse incoming/outgoing invitations
Use "ipc" to manage invitations received
Use "users" to see contacts
</pre>

### version
Prints MEGAcmd versioning and extra info

Usage: `version [-l][-c]`
<pre>
Options:
 -c     Shows changelog for the current version
 -l     Show extended info: MEGA SDK version and features enabled
</pre>

### webdav
Configures a WEBDAV server to serve a location in MEGA

Usage: `webdav [ [-d] remotepath [--port=PORT] [--public] [--tls --certificate=/path/to/certificate.pem --key=/path/to/certificate.key]]`
<pre>
This can also be used for streaming files. The server will be running as long as MEGAcmd Server is.
If no argument is given, it will list the webdav enabled locations.

Options:
 --d            Stops serving that location
 --public       *Allow access from outside localhost
 --port=PORT    *Port to serve. DEFAULT= 4443
 --tls          *Serve with TLS (HTTPS)
 --certificate=/path/to/certificate.pem *Path to PEM formated certificate
 --key=/path/to/certificate.key *Path to PEM formated key

*If you serve more than one location, these parameters will be ignored and use those of the first location served.

Caveat: This functionality is in BETA state. If you experience any issue with this, please contact: support@mega.nz
</pre>

### whoami
Print account information
Usage: `whoami [-l]`
<pre>
Options:
 -l     Show extended info: total storage used, storage per main folder
        (see mount), pro level, account balance, and also the active sessions
</pre>
