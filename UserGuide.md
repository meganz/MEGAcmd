# MEGAcmd User Guide

This document relates to MEGAcmd version 0.9.9.

### What is it
A command line tool to work with your MEGA account and files.  You can run it in interactive mode where it processes all commands directly, or you can use all the features from your favourite Linux or Mac shell such as bash, or you can even run it in a Windows command prompt.   You can also use its commands in scripts.

In addition to running commands on request, MEGAcmd can also be configured to synchronise folders between your local device to your MEGA account, or perform regular backups from your device to your MEGA account.

In order to enable synchronisation and backup features, and for efficiency running commands, MEGAcmd runs a server process in the background which the MEGAcmd shell or the script commands forward requests to.   The server keeps running in the background until it is told to close with the `exit` or `quit` commands.   If you want it to keep running (for sync and backup for example) when you exit MEGAcmd, use the `--only-shell` flag.

Working with your MEGA account requires signing in with your email and password using the `login` command, though you can download public links or upload to public folders without logging in.  Logging in with your username and password starts a [Session](#session), and causes some of your account such as the folder structure to be downloaded and cached locally for performance (encrypted, of course).  That cache is kept on disk, and will be reused the next time you run MEGAcmd.  While the background server is running, the cache is kept up to date.  Closing and reopening MEGAcmd will not require you to log in again, unless you logged out.   Logging out closes your session, and means that the next login will require downloading the cache again.  Logging out means your device cannot access your MEGA account again until you provide your password (or use a Session ID to log on), so it is more secure, however the sync and backup features will not be able to operate, and your local cache will not be kept up to date.

### Where can you get it  
For Linux, Mac, or Windows: Download it from the MEGA.nz website: https://mega.nz/cmd <p>
We are also building it for some NAS systems, please check your provider's App Store.

### What can you do with it
The major features are
* Move files around inside your MEGA account or between MEGA and your PC using command line tools.
* Use those same commands in scripts to manage your files.
* Set up synchronization or a backup schedule between a folder on your machine, and a folder on your MEGA account.   (use the [`sync`](#sync) or [`backup`](#backup) commands)
* Set up WebDAV access to files in your MEGA account (use the [`webdav`](#webdav) command)

## Terminology and Descriptions

### Contact
A contact is someone (identified by their email address) that also has a MEGA account, who you can share files or folders with, and can chat with on MEGAchat.

### Remote Path
This refers to a file or a folder stored in your MEGA account, or a publicly available file or folder in the MEGA cloud.  Remote paths always use the '/' character as the separator between folder and file elements.

### Local Path
This refers to a file or folder on the PC or device that MEGAcmd is running in.  

### Session
When you log in with your email and MEGA account password, that creates a session.  The session exists until you log out of it or kill it.  In MEGAcmd, use `whoami -l` to see all your open sessions across all devices, and use `killsession` to close them.   You can use other MEGA clients such as the phone app, or webclient to close these also.   Devices that were using a killed session will have their connection to MEGA closed immediately and will no longer have access to your account, until you log in on them again.   Syncs, backups, and webdavs are specific to a session, so logging out will cause them to be cancelled.

### Local Cache
MEGAcmd holds some encrypted data on your device relating to your account, such as folder structure and contacts, for performance reasons.  The MEGAcmd background server keeps the local cache up to date when changes to your account occur from other clients.  The cache does contain a way for MEGAcmd to log back into your account when it starts up again if you have not logged out fully, so if your device is not completely secure between sessions then you should do that.

## Command Summary

These summaries use the usual conventions - `[]` indicates its content is optional,  `|` indicates you should choose either the item on the left or the one on the right (but not both)

Each command is described as it would be used in the MEGAcmd shell, and the corresponding shell script (prefixed with `mega-`) works in the same way.

Commands referring to a `remote path` are talking about a file in your MEGA account online, whereas a `local path` refers to a file or folder on your local device where MEGAcmd is running.

Verbosity: You can increase the amount of information given by any command by passing `-v` (`-vv`, `-vvv`, ...)

### Account / Contacts
* [`signup`](#signup)`email [password] [--name="Your Name"]`  Register as user with a given email.
* [`confirm`](#confirm)`link email [password]`  Confirm an account using the link provided after the "signup" process.
* [`invite`](#invite)`[-d|-r] dstemail [--message="MESSAGE"]`  Invites a contact / deletes an invitation.
* [`showpcr`](#showpcr)`[--in | --out]`  Shows incoming and outgoing contact requests.
* [`ipc`](#ipc)`email|handle -a|-d|-i`  Manages contact incoming invitations.
* [`users`](#users)`[-s] [-h] [-n] [-d contact@email]` List contacts
* [`userattr`](#userattr)`[-s attribute value|attribute] [--user=user@email]` Lists/updates user attributes
* [`passwd`](#passwd)`[oldpassword newpassword]`  Modifies user password
* [`masterkey`](#masterkey)`pathtosave`  Shows your master key.

### Login / Logout
* [`login`](#login)`[email [password]] | exportedfolderurl#key | session` Logs into MEGA
* [`logout`](#logout)`[--keep-session]` Logs out
* [`whoami`](#whoami)`[-l]` Print info of the user
* [`session`](#session) Prints (secret) session ID
* [`killsession`](#killsession)`[-a|sessionid]` Kills a session of current user.
	  
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
* [`cp`](#cp)`srcremotepath dstremotepath|dstemail` Copies a file/folder into a new location (all remotes)
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
* [`deleteversions`](#deleteversions)` [-f] (--all | remotepath1 remotepath2 ...)` Delete prior versions of files to save space.
* [`unicode`](#unicode) Toggle unicode input enabled/disabled in interactive shell
* [`reload`](#reload) Forces a reload of the remote files of the user
* [`help`](#help)`[-f]` Prints list of commands
* [`https`](#https)`[on|off]` Shows if HTTPS is used for transfers. Use `https on` to enable it.
* [`clear`](#clear) Clear screen
* [`log`](#log)`[-sc] level` Prints/Modifies the current logs level
* [`debug`](#debug) Enters debugging mode (HIGHLY VERBOSE)
* [`exit`](#exit)`|`[`quit`](#quit)` [--only-shell]` Quits MEGAcmd


## Command Detail

### attr
Lists/updates node attributes

Usage: `attr remotepath [-s attribute value|-d attribute]`
<pre>
Options:
  -s attribute value    sets an attribute to a value
  -d attribute          removes the attribute
</pre>

### backup
Sets up or controls backups.  ([example](#backup-example))  ([tutorial](https://github.com/meganz/MEGAcmd/blob/master/contrib/docs/BACKUPS.md))

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

Syncs are associated with your Session, so logging out will cancel them.

Caveat: This functionality is in BETA state. If you experience any issue with this, please contact: support@mega.nz
</pre>

### cd
Changes the current remote folder  ([example](#login-logout-whoami-mkdir-cd-get-put-du-mount-example))

Usage: `cd [remotepath]`
<pre>
If no folder is provided, it will be changed to the root folder
</pre>

### clear
Clear screen

Usage: `clear`

### confirm
Confirm an account using the link provided after the "signup" process.  ([example](#signup-confirm-invite-showpcr-ipc-users-userattr-example))

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
For a finer control of log level see [`log`](#log)
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

To see versions of a file use `ls --versions`.
To see space occupied by file versions use `du --versions`.
</pre>

### du  
Prints size used by files/folders  ([example](#login-logout-whoami-mkdir-cd-get-put-du-mount-example))

Usage: `du [-h] [--versions] [remotepath remotepath2 remotepath3 ... ]`
<pre>
remotepath can be a pattern (it accepts wildcards: ? and *. e.g.: f*00?.txt)

Options:
 -h             Human readable
 --versions     Calculate size including all versions.

You can remove all versions with `deleteversions` and list them with `ls --versions <remotepath>`
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
To only exit current shell and keep server running, use `exit --only-shell`

Exiting the server does not cancel the session, and the encrypted local cache of your account is kept on your PC.
The session will be resumed when the service is restarted.
</pre>

### export
Prints/Modifies the status of current exports ([example](#export-import-example))

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

If a remote path is given it'll be used to add/delete or in case of no option selected, it will display all the exports existing in the tree of that path
</pre>

### find
Find nodes matching a pattern

Usage: `find [remotepath] [-l] [--pattern=PATTERN] [--mtime=TIMECONSTRAIN] [--size=SIZECONSTRAIN]`
<pre>
Options:
  -l                     Prints file info
  --pattern=PATTERN      Pattern to match (it accepts wildcards: ? and *. e.g.: f*00?.txt)
  --mtime=TIMECONSTRAIN  Determines time constrains, in the form: [+-]TIMEVALUE
                         TIMEVALUE may include hours(h), days(d), minutes(M), seconds(s), months(m) or years(y)
                         Examples:
                           "+1m12d3h" shows files modified before 1 month, 12 days and 3 hours the current moment
                           "-3h" shows files modified within the last 3 hours
                           "-3d+1h" shows files modified in the last 3 days prior to the last hour
  --size=SIZECONSTRAIN   Determines size constrains, in the form: [+-]TIMEVALUE
                         TIMEVALUE may include (B)ytes, (K)ilobytes, (M)egabytes, (G)igabytes & (T)erabytes
                         Examples:
                           "+1m12k3B" shows files bigger than 1 Mega, 12 Kbytes and 3Bytes
                           "-3M" shows files smaller than 3 Megabytes
                           "-4M+100K" shows files smaller than 4 Mbytes and bigger than 100 Kbytes
</pre>

### get
Downloads a remote file/folder or a public link  ([example](#login-logout-whoami-mkdir-cd-get-put-du-mount-example))

Usage: `get [-m] [-q] [--ignore-quota-warn] exportedlink#key|remotepath [localpath]`
<pre>
If the remotepath is a file, it will be downloaded to folder specified in localpath (or to the current folder if not specified).
If the localpath (destination) already exists and is the same (by content) then nothing will be done. If it differs, it will create a new file appending " (NUM)".
If the remotepath or exportedlink is a folder, the folder and its entire contents will be downloaded into the destination folder.

Options:
  -q                    queue download: execute in the background. 
  -m                    if the folder already exists, the contents will be merged with the downloaded one (preserving the existing files)
  --ignore-quota-warn   ignore quota surpassing warning. The download will be attempted anyway.
</pre>

### help
Prints list of commands

Usage: `help [-f]`
<pre>
Options:
  -f     Include a brief description of the commands
</pre>

### https
Shows if HTTPS is used for transfers. Use `https on` to enable it.  

Usage: `https [on|off]`
<pre>
HTTPS is not necesary since all data is already encrypted before being stored or transfered anyway.
Enabling it will increase CPU usage and add network overhead.

This setting is ephemeral: it will reset for the next time you open MEGAcmd
</pre>

### import
Imports the contents of a remote link into your MEGA account or to a local folder.  ([example](#export-import-example))

Usage: `import exportedfilelink#key [remotepath]`
<pre>
If no remote path is provided, the current local folder will be used
</pre>

### invite
Invites a contact / deletes an invitation  ([example](#signup-confirm-invite-showpcr-ipc-users-userattr-example))

Usage: invite [-d|-r] dstemail [--message="MESSAGE"]
<pre>
Options:
  -d                   Deletes invitation
  -r                   Resends the invitation
  --message="MESSAGE"  Sends the invitation, including your message.
</pre>

### ipc
Manages contact incoming invitations.   ([example](#signup-confirm-invite-showpcr-ipc-users-userattr-example))

Usage: `ipc email|handle -a|-d|-i`
<pre>
Options:
  -a     Accepts invitation
  -d     Rejects invitation
  -i     Ignores invitation [WARNING: do not use unless you know what you are doing]
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
Changes the current local folder for the interactive console

Usage: lcd [localpath]
<pre>
It will be used for uploads and downloads

If not using interactive console, the current local folder will be that of the shell executing mega comands
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

Regardless of the log level of the  interactive shell, you can increase the amount of information given by any command by passing `-v` (`-vv`, `-vvv`, ...)
</pre>

### login
Log into your MEGA account ([example](#login-logout-whoami-mkdir-cd-get-put-du-mount-example))

Usage: `login [email [password]] | exportedfolderurl#key | session`
<pre>
You can log in either with email and password, with session ID, or into a folder (an exported/public folder).
If logging into a folder indicate url#key
</pre>

### logout
Closes your session for security or to allow subsequently logging into a different account. ([example](#login-logout-whoami-mkdir-cd-get-put-du-mount-example))

Usage: logout [--keep-session] 
<pre>
Options:
  --keep-session    The current session is not closed, allowing logging back into it later using the session ID rather than email/password.
  
MEGAcmd will still log back into your account automatically on restart if you specify --keep-session, similar to exiting it without logging out.
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
Creates a directory or a directories hierarchy  ([example](#login-logout-whoami-mkdir-cd-get-put-du-mount-example))

Usage: `mkdir [-p] remotepath`
<pre>
Options:
  -p     Allow recursive
</pre>

### mount
Lists all the main nodes  ([example](#login-logout-whoami-mkdir-cd-get-put-du-mount-example))

Usage: `mount`

### mv
Copies files/folders to a new location in your MEGA account

Usage: `mv srcremotepath [srcremotepath2 srcremotepath3 ..] dstremotepath`
<pre>
If the destination remote path exists and is a folder, the source will be copied there.
If the destination remote path doesn't exist, the source will be renamed to the given dstremotepath leaf name.
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
Uploads files/folders to a remote folder  ([example](#login-logout-whoami-mkdir-cd-get-put-du-mount-example))

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
Forces re-downloading the Local Cache information from MEGA.

Usage: `reload`
<pre>
It will also resume synchronizations.
</pre>

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

If a remote path is given it'll be used to add/delete or in case of no option selected, it will display all the shares existing in the tree of that path

When sharing a folder with a user that is not a contact (see "users --help") the share will be in a pending state. You can list pending shares with `share -p`. Your contact will need to accept your invitation (see [`ipc`](#ipc))

If someone has shared something with you, it will be listed as a root folder.
Use [`mount`](#mount) to list folders shared with you
</pre>

### showpcr
Shows incoming and outgoing contact requests.  ([example](#signup-confirm-invite-showpcr-ipc-users-userattr-example))

Usage: `showpcr [--in | --out]`
<pre>
Options:
  --in   Shows incoming requests
  --out  Shows outgoing invitations
</pre>

### signup
Register as user with a given email ([example](#signup-confirm-invite-showpcr-ipc-users-userattr-example))

Usage: `signup email [password] [--name="Your Name"]`
<pre>
Options:
  --name="Your Name"     Name to register. e.g. "John Smith"

You will receive an email to confirm your account.
Once you have received the email, please proceed to confirm the link included in that email with "confirm".
</pre>

### speedlimit
Displays/modifies upload/download rate limits

Usage: `speedlimit [-u|-d] [-h] [NEWLIMIT]`
<pre>
NEWLIMIT establish the new limit in size per second (0 = no limit)
NEWLIMIT may include (B)ytes, (K)ilobytes, (M)egabytes, (G)igabytes & (T)erabytes.
Examples: "1m12k3B" "3M". If no units are given, bytes are assumed.

Options:
  -d     Download speed limit
  -u     Upload speed limit
  -h     Human readable

Notice: these limits are saved for the next time you execute MEGAcmd server.  They will be removed if you logout.
</pre>

### sync
Sets up synchronisation between a local folder and one in your MEGA account.  ([example](#sync-example))

Usage: `sync [localpath dstremotepath| [-dsr] [ID|localpath]`
<pre>
If no argument is provided, it lists current configured synchronizations

If provided local and remote paths, it will start synchronizing a local folder into a remote folder

If an ID/local path is provided, it will list such synchronization unless an option is specified.

Options:
  -d ID|localpath deletes a synchronization
  -s ID|localpath stops(pauses) a synchronization
  -r ID|localpath resumes a synchronization
  --path-display-size=N  Use a fixed size of N characters for paths

Syncs are associated with your Session, so logging out will cancel them.
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
List or operate with queued transfers ([example](#transfers-example))

Usage: `transfers [-c TAG|-a] | [-r TAG|-a]  | [-p TAG|-a] [--only-downloads | --only-uploads] [SHOWOPTIONS]`
<pre>
If executed without option it will list the first 10 tranfers
Options:
  -c (TAG|-a)            Cancel transfer with TAG (or all with -a)
  -p (TAG|-a)            Pause transfer with TAG (or all with -a)
  -r (TAG|-a)            Resume transfer with TAG (or all with -a)
  -only-uploads          Show/Operate only upload transfers
  -only-downloads        Show/Operate only download transfers

Show options:
  -show-syncs            Show synchronization transfers
  -show-completed        Show completed transfers
  -only-completed        Show only completed download
  --limit=N              Show only first N transfers
  --path-display-size=N  Use a fixed size of N characters for paths
</pre>

### unicode
Toggle unicode input enabled/disabled in interactive shell

Usage: `unicode`
<pre>
Unicode mode is experimental, you might experience some issues interacting with the console (e.g. history navigation fails).
Type "help --unicode" for further info.
</pre>

### userattr
Lists/updates user attributes  ([example](#signup-confirm-invite-showpcr-ipc-users-userattr-example))

Usage: `userattr [-s attribute value|attribute] [--user=user@email]`
<pre>
Options:
  -s attribute value     sets an attribute to a value
  --user=user@email      select the user to query
</pre>

### users
List contacts  ([example](#signup-confirm-invite-showpcr-ipc-users-userattr-example))

Usage: `users [-s] [-h] [-n] [-d contact@email]`
<pre>
Options:
  -s     Show shared folders with listed contacts
  -h     Show all contacts (hidden, blocked, ...)
  -n     Show users names
  -d     contact@email Deletes the specified contact
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
Configures a WEBDAV server to serve a location in MEGA  ([example](#webdav-example))

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

Webdav setup is associated with your Session, so logging out will cancel them.

Caveat: This functionality is in BETA state. If you experience any issue with this, please contact: support@mega.nz
</pre>

### whoami
Print account information  ([example](#login-logout-whoami-mkdir-cd-get-put-du-mount-example))

Usage: `whoami [-l]`
<pre>
Options:
 -l     Show extended info: total storage used, storage per main folder
        (see mount), pro level, account balance, and also the active sessions
</pre>


## Examples

### signup confirm invite showpcr ipc users userattr example
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
MEGA CMD> <b>login eg.email_1@example.co.nz</b>
Password:
[API:info: 23:19:14] Fetching nodes ...
[API:info: 23:19:17] Loading transfers from local cache
[API:info: 23:19:17] Login complete as eg.email_1@example.co.nz
MEGA CMD>
eg.email_1@example.co.nz:/$ <b>invite eg.email_2@example.co.nz</b>
Invitation to user: eg.email_2@example.co.nz sent
eg.email_1@example.co.nz:/$ <b>showpcr</b>
Outgoing PCRs:
 eg.email_2@example.co.nz  (id: 47Xhz6wvVTk, creation: Thu, 26 Apr 2018 11:20:09 +1200, modification: Thu, 26 Apr 2018 11:20:09 +1200)
eg.email_1@example.co.nz:/$ <b>logout</b>
Logging out...
eg.email_1@example.co.nz:/$
MEGA CMD> <b>login eg.email_2@example.co.nz</b>
Password:
[API:info: 23:21:10] Fetching nodes ...
[API:info: 23:21:12] Loading transfers from local cache
[API:info: 23:21:12] Login complete as eg.email_2@example.co.nz
MEGA CMD>
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
eg.email_2@example.co.nz:/$ <b>showpcr</b>
eg.email_2@example.co.nz:/$ <b>logout</b>
Logging out...
MEGA CMD> <b>login eg.email_1@example.co.nz</b>
Password:
[API:info: 23:24:26] Fetching nodes ...
[API:info: 23:24:27] Loading transfers from local cache
[API:info: 23:24:27] Login complete as eg.email_1@example.co.nz
MEGA CMD>
eg.email_1@example.co.nz:/$ <b>showpcr</b>
eg.email_1@example.co.nz:/$ <b>users</b>
eg.email_2@example.co.nz, visible
eg.email_1@example.co.nz:/$ <b>userattr --user=eg.email_2@example.co.nz</b>
        firstname = test2
        ed25519 = M7SLy2RajwUAvynxJQaVkhe6hxGpbwJmvve3dgl8B1o
        cu25519 = VaXluGS2c5xbo0xOHHJciqLRxwMaWZHVK8iuxtlCBTk
        rsa = AAAAAFrhDWemabQ4JAOtP7zcoy6m74PsFTFCbj04Zh4G8K_TZB5Sm9T5Xj9CXYzwWnpfRd1McPdDouKdsASQ6Er7i4Y4LpEA
        cu255 = AAAAAFrhDWcXE_7AHZmvxk5Hk0G7V65UnvFO42tb1gM9SYy3BpsMCas0X-pbqkYwf6_2eBG-ZLvkonGfXB3DWonWNvnVehIB
eg.email_1@example.co.nz:/$
</pre>

### login logout whoami mkdir cd get put du mount example
<pre>
MEGA CMD> <b>login eg.email_1@example.co.nz</b>
Password:
[API:info: 23:43:14] Fetching nodes ...
[API:info: 23:43:14] Loading transfers from local cache
[API:info: 23:43:14] Login complete as eg.email_1@example.co.nz
MEGA CMD>
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
eg.email_1@example.co.nz:/$ <b>ls</b>
Welcome to MEGA.pdf
eg.email_1@example.co.nz:/$ <b>get "Welcome to MEGA.pdf"</b>
TRANSFERING ||################################################################################||(1/1 MB: 100.00 %)
eg.email_1@example.co.nz:/$ <b>mkdir my-pictures</b>
eg.email_1@example.co.nz:/$ <b>cd my-pictures/</b>
eg.email_1@example.co.nz:/my-pictures$ <b>put C:\Users\MYWINDOWSUSER\Pictures</b>
TRANSFERING ||################################################################################||(1/1 MB: 100.00 %)
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
eg.email_1@example.co.nz:/$ <b>du my-pictures/</b>
FILENAME                                        SIZE
my-pictures:                                 1376143
----------------------------------------------------------------
Total storage used:                          1376143
eg.email_1@example.co.nz:/$ <b>logout</b>
Logging out...
MEGA CMD>
</pre>

### sync example
<pre>
email_1@example.co.nz:/$ <b>sync c:\Go go-backup/</b>
email_1@example.co.nz:/$ <b>sync</b>
ID LOCALPATH                                  REMOTEPATH                                 ActState   SyncState     SIZE  FILES   DIRS
 0 \\?\c:\Go                                  /go-backup                                 InitScan   Syncing   119.13 KB     10     97 
email_1@example.co.nz:/$ <b>sync</b>
ID LOCALPATH                                  REMOTEPATH                                 ActState   SyncState     SIZE  FILES   DIRS
 0 \\?\c:\Go                                  /go-backup                                 InitScan   Syncing   61.22 MB   1252    463
email_1@example.co.nz:/$ <b>sync</b>
ID LOCALPATH                                  REMOTEPATH                                 ActState   SyncState     SIZE  FILES   DIRS
 0 \\?\c:\Go                                  /go-backup                                 InitScan   Syncing   232.94 MB   4942    773 
email_1@example.co.nz:/$ <b>sync</b>
ID LOCALPATH                                  REMOTEPATH                                 ActState   SyncState     SIZE  FILES   DIRS
 0 \\?\c:\Go                                  /go-backup                                 Active     Synced    285.91 MB   7710   1003 
 
[then on a windows cmd prompt] 
C:\Users\ME><b>rmdir /s c:\go\blog</b>
c:\go\blog, Are you sure (Y/N)? <b>Y</b>

[back in MEGAcmd- update has been applied to MEGA already] 
email_1@example.co.nz:/$ <b>sync</b>
ID LOCALPATH                                  REMOTEPATH                                 ActState   SyncState     SIZE  FILES   DIRS
 0 \\?\c:\Go                                  /go-backup                                 Active     Synced    268.53 MB   7306    961 
</pre>

### backup example
<pre>
eg.email@example.co.nz:/$ <b>backup c:/cmake /cmake-backup --period="0 0 4 * * *" --num-backups=3</b>
Backup established: c:/cmake into /cmake-backup period="0 0 4 * * *" Number-of-Backups=3
eg.email@example.co.nz:/$ <b>backup</b>
TAG   LOCALPATH                                               REMOTEPARENTPATH                                                STATUS
166   \\?\c:\cmake                                            /cmake-backup                                                  ONGOING
eg.email@example.co.nz:/$ <b>backup -h</b>
TAG   LOCALPATH                                               REMOTEPARENTPATH                                                STATUS
166   \\?\c:\cmake                                            /cmake-backup                                                  ONGOING
   -- SAVED BACKUPS --
  NAME                                                    DATE                    STATUS  FILES FOLDERS
  cmake_bk_20180426133300                                 26Apr2018 13:33:00     ONGOING      0      92
eg.email@example.co.nz:/$
</pre>

### webdav example
<pre>
eg.email@example.co.nz:/$ <b>webdav myfile.tif --port=1024</b>
Serving via webdav myfile.tif: http://127.0.0.1:1024/5mYHQT4B/myfile.tif
eg.email@example.co.nz:/$ <b>webdav</b>
WEBDAV SERVED LOCATIONS:
/myfile.tif: http://127.0.0.1:1024/5mYHQT4B/myfile.tif
eg.email@example.co.nz:/$ <b>webdav -d myfile.tif</b>
myfile.tif no longer served via webdav
eg.email@example.co.nz:/$
</pre>

### export import example
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
[API:info: 01:55:04] Fetching nodes ...
[API:info: 01:55:05] Loading transfers from local cache
[API:info: 01:55:05] Login complete as eg.email_2@example.co.nz
MEGA CMD>
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
eg.email_2@example.co.nz:/$
MEGA CMD> <b>login ARo7aiLAxK-jseOdVBYhj285Twb06ivWsFmT4XAnkTsiaDRRbm5oYS1zRm-V3I0FHHOvwj7P2RPvrSw_</b>
eg.email_1@example.co.nz:/$ <b>export</b>
Pictures (folder, shared as exported permanent folder link: https://mega.nz/#F!iaZlEBIL!mQD3rFuJhKov0sco-6s9xg)
eg.email_1@example.co.nz:/$ <b>export -d Pictures/</b>
Disabled export: /Pictures
eg.email_1@example.co.nz:/$ <b>export</b>
Couldn't find anything exported below current folder. Use -a to export it
eg.email_1@example.co.nz:/$
</pre>

### transfers example
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
eg.email@example.co.nz:/tmp-test/Mega.dir$
</pre>





