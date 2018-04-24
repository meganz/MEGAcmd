# MEGAcmd User Guide

This document relates to MEGAcmd version 0.9.9.

### What is it
A command line tool to work with your MEGA account and files.  You can run it in interactive mode where it processes all commands directly, or you can use all the features from your favourite Linux or Mac shell such as bash, or you can even run it in a Windows command prompt.   You can also use its commands in scripts.

### Where can you get it  
Download it from the MEGA.nz website: https://mega.nz/cmd

### What can you do with it
The major features are
	* Move files around inside your MEGA account or between MEGA and your PC in a way similar to how you manage files in a Linux shell or Windows command prompt.
	* Use those same commands in scripts to manage your files.
	* Set up synchronization between a folder on your machine, and a folder on your MEGA account.   (use the 'sync' command)
	* set up a backup schedule, so that you can have regular snapshots made of your local folders.  (use the 'backup' command)
	
Security aspects
	* Where does it store temp files / cached account data (encrypted of course)
	* Keep your password safe
	* Be careful with sessionids

## Command Summary

These summaries use the usual conventions - `[]` indicates its content is optional,  `|` indicates you should choose either the item on the left or the one on the right (but not both)

### Account / Contacts
* [`signup`](#signup) `email [password] [--name="Your Name"]`  * Register as user with a given email
* [`confirm`](#confirm) `link email [password]`  Confirm an account using the link provided after the "signup" process.
* [`invite`](#invite)`[-d|-r] dstemail [--message="MESSAGE"]`  Invites a contact / deletes an invitation
* [`showpcr`](#showpcr) `[--in | --out]`  Shows incoming and outgoing contact requests.
* [`ipc`](#ipc) `email|handle -a|-d|-i`  Manages contact incoming invitations.
* [`passwd`](#passwd) `[oldpassword newpassword]`  Modifies user password
* [`masterkey`](#masterkey) `pathtosave`  Shows your master key.

### Login / Logout
* [`login`](#login) `[email [password]] | exportedfolderurl#key | session` Logs into MEGA
* [`logout`](#logout) `[--keep-session]` Logs out
* [`whoami`](#whoami) `[-l]` Print info of the user
* [`session`](#session) Prints (secret) session ID
* [`killsession`](#killsession) `[-a|sessionid]` Kills a session of current user.
* [`userattr`](#userattr) `[-s attribute value|attribute] [--user=user@email]` Lists/updates user attributes
* [`users`](#users) `[-s] [-h] [-n] [-d contact@email]` List contacts
	  
### Browse
* [`cd`](#cd) `[remotepath]` Changes the current remote folder
* [`lcd`](#lcd) `[localpath]` Changes the current local folder for the interactive console
* [`ls`](#ls) `[-lRr] [remotepath]` Lists files in a remote path
* [`pwd`](#pwd) Prints the current remote folder
* [`lpwd`](#lpwd) Prints the current local folder for the interactive console
* [`attr`](#attr) `remotepath [-s attribute value|-d attribute]`  Lists/updates node attributes
* [`du`](#du) `[-h] [remotepath remotepath2 remotepath3 ... ]` Prints size used by files/folders
* [`find`](#find) `[remotepath] [-l] [--pattern=PATTERN] [--mtime=TIMECONSTRAIN] [--size=SIZECONSTRAIN]` Find nodes matching a pattern
* [`mount`](#mount) Lists all the main nodes

### Moving/Copying Files
* [`mkdir`](#mkdir) `[-p] remotepath` Creates a directory or a directory hierarchy
* [`cp`](#cp) `srcremotepath dstremotepath|dstemail` Moves a file/folder into a new location (all remotes)
* [`put`](#put) `[-c] [-q] [--ignore-quota-warn] localfile [localfile2 localfile3 ...] [dstremotepath]` Uploads files/folders to a remote folder
* [`get`](#get) `[-m] [-q] [--ignore-quota-warn] exportedlink#key|remotepath [localpath]` Downloads a remote file/folder or a public link
* [`preview`](#preview) `[-s] remotepath localpath` To download/upload the preview of a file.
* [`thumbnail`](#thumbnail) `[-s] remotepath localpath` To download/upload the thumbnail of a file.
* [`mv`](#mv) `srcremotepath [srcremotepath2 srcremotepath3 ..] dstremotepath` Moves file(s)/folder(s) into a new location (all remotes)
* [`rm`](#rm) `[-r] [-f] remotepath` Deletes a remote file/folder
* [`transfers`](#transfers) `[-c TAG|-a] | [-r TAG|-a]  | [-p TAG|-a] [--only-downloads | --only-uploads] [SHOWOPTIONS]` List or operate with transfers
* [`speedlimit`](#speedlimit) `[-u|-d] [-h] [NEWLIMIT]` Displays/modifies upload/download rate limits
* [`sync`](#sync) `[localpath dstremotepath| [-dsr] [ID|localpath]` Controls synchronizations
* [`exclude`](#exclude) `[(-a|-d) pattern1 pattern2 pattern3 [--restart-syncs]]` Manages exclusions in syncs.
	  
### Sharing (your own files, of course)
* [`cp`](#cp) `srcremotepath dstremotepath|dstemail` Moves a file/folder into a new location (all remotes)
* [`export`](#export) `[-d|-a [--expire=TIMEDELAY]] [remotepath]` Prints/Modifies the status of current exports
* [`import`](#import) `exportedfilelink#key [remotepath]` Imports the contents of a remote link into your account
* [`share`](#share) `[-p] [-d|-a --with=user@email.com [--level=LEVEL]] [remotepath]` Prints/Modifies the status of current shares

### Misc
* [`version`](#version) `[-l][-c]` Prints MEGAcmd versioning and extra info
* [`unicode`](#unicode) Toggle unicode input enabled/disabled in interactive shell
* [`reload`](#reload) Forces a reload of the remote files of the user
* [`help`](#help) `[-f]` Prints list of commands
* [`https`](#https) `[on|off]` Shows if HTTPS is used for transfers. Use "https on" to enable it.
* [`clear`](#clear) Clear screen
* [`log`](#log) `[-sc] level` Prints/Modifies the current logs level
* [`debug`](#debug) Enters debugging mode (HIGHLY VERBOSE)
* [`exit`](#exit)|[`quit`] [--only-shell]` Quits MEGAcmd


## Command Detail

### attr
Usage: attr remotepath [-s attribute value|-d attribute]
Lists/updates node attributes

Options:
 -s     attribute value         sets an attribute to a value
 -d     attribute               removes the attribute
 
### signup

### confirm

### invite

### showpcr

### ipc

### passwd

### masterkey



