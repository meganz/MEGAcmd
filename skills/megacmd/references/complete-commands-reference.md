# MEGAcmd — Complete Command Reference

> Reference documentation for ALL 76 MEGAcmd commands.
> Version: 2.5.2 | License: BSD 2-Clause

**Legend:**
- `[]` = optional
- `|` = either/or
- `--use-pcre` = use PCRE (Perl Compatible Regular Expressions)
- `--time-format=FORMAT` = formats: RFC2822, ISO6081, ISO6081_WITH_TIME, SHORT, SHORT_UTC, CUSTOM strftime
- `--path-display-size=N` = fixed display size for paths
- `--col-separator=X` = column separator
- `--output-cols=COL1,COL2,...` = select columns to display

---

## Table of Contents

1. [Account Management](#1-account-management)
2. [Contacts & Invitations](#2-contacts--invitations)
3. [Navigation](#3-navigation)
4. [Listing & Search](#4-listing--search)
5. [File Management](#5-file-management)
6. [Sharing](#6-sharing)
7. [Transfers](#7-transfers)
8. [Configuration](#8-configuration)
9. [Utilities](#9-utilities)
10. [Sync](#10-sync)
11. [Backups](#11-backups)
12. [FTP & WebDAV Servers](#12-ftp--webdav-servers)
13. [FUSE](#13-fuse)
14. [Common Flags](#14-common-flags)
15. [Error Codes](#15-error-codes)
16. [IPC](#16-ipc)
17. [Logging](#17-logging)

---

## 1. Account Management

### signup
Register a new user.
```
signup email password [--name="Your Name"]
```
- `--name` — Registration name
- Password must not contain `"` or `'`
- Use `confirm` after receiving the email link
- **Important:** Save your Master Key!

### confirm
Confirm an account using the link received by email.
```
confirm link email password
```

### cancel
Permanently cancel your MEGA account.
```
cancel
```
- Account permanently closed and data deleted
- Requires confirmation via link (see `confirmcancel`)

### confirmcancel
Confirm account cancellation.
```
confirmcancel link password
```

### login
Log into a MEGA account.
```
login [--auth-code=XXXX] email password
login exportedfolderurl#key [--auth-key=XXXX] [--resume]
login passwordprotectedlink [--password=PASSWORD]
login session
```
- `--auth-code=XXXX` — MFA token (two-factor authentication)
- `--password=PASSWORD` — Password for protected links
- `--auth-key=AUTHKEY` — For writable folder links
- `--resume` — Attempt to load from cache
- Only one entity can be logged in at a time

### logout
Log out.
```
logout [--keep-session]
```
- `--keep-session` — Keep current session (do not delete cache)

### session
Print the (secret) session ID.
```
session
```

### whoami
Print info about the logged-in user.
```
whoami [-l]
```
- `-l` — Extended info: total storage, storage per main folder, Pro level, balance, active sessions

### killsession
Kill a session of the current user.
```
killsession [-a | sessionid1 sessionid2 ...]
```
- `-a` — Kill all sessions except the current one

### passwd
Change user password.
```
passwd [-f] [--auth-code=XXXX] newpassword
```
- `-f` — Force (no asking)
- `--auth-code=XXXX` — Two-factor authentication code
- Changes password and closes all active sessions (except current)

### masterkey
Show your Master Key (Recovery Key).
```
masterkey pathtosave
```
- **Essential** for recovering access to data
- Without the Master Key, losing your password means losing all data

---

## 2. Contacts & Invitations

### invite
Invite a contact / delete an invitation.
```
invite [-d|-r] dstemail [--message="MESSAGE"]
```
- `-d` — Delete invitation
- `-r` — Resend invitation
- `--message` — Custom message

### showpcr
Show incoming and outgoing contact requests.
```
showpcr [--in | --out] [--time-format=FORMAT]
```
- `--in` — Incoming requests
- `--out` — Outgoing invitations

### ipc
Manage incoming contact invitations.
```
ipc email|handle -a|-d|-i
```
- `-a` — Accept invitation
- `-d` — Decline invitation
- `-i` — Ignore invitation (caution!)

### users
List contacts.
```
users [-s] [-h] [-n] [-d contact@email] [--time-format=FORMAT] [--verify|--unverify contact@email.com] [--help-verify [contact@email.com]]
```
- `-d email` — Delete contact
- `-s` — Show shared folders with contacts
- `-h` — Show all (hidden, blocked)
- `-n` — Show user names
- `--verify email` — Verify contact (manually check credentials first!)
- `--unverify email` — Mark as unverified

### userattr
List/update user attributes.
```
userattr [-s attribute value|attribute|--list] [--user=user@email]
```
- `--user=user@email` — Select user to query
- `-s attribute value` — Set attribute
- `--list` — List valid attributes

---

## 3. Navigation

### cd
Change the current remote folder.
```
cd [remotepath]
```
- No arguments: go back to root (/)

### pwd
Print the current remote folder.
```
pwd
```

### lcd
Change the current local folder (for the interactive console).
```
lcd [localpath]
```
- No arguments: go back to home
- In non-interactive mode, the local directory is the shell's working directory

### lpwd
Print the current local folder.
```
lpwd
```

### mount
List all root nodes.
```
mount
```
Displays: ROOT, INBOX, RUBBISH, INSHARE (folders shared by other users)

### tree
List files in a nested tree decorated output.
```
tree [remotepath]
```

---

## 4. Listing & Search

### ls
List files in a remote path.
```
ls [-halRr] [--show-handles] [--tree] [--versions] [remotepath] [--use-pcre] [--show-creation-time] [--time-format=FORMAT]
```
- `-R`/`-r` — List recursively
- `--tree` — Tree-like output (implies -r)
- `--show-handles` — Show handles (H:XXXXXXXX)
- `-l` — Detailed summary (FLAGS, VERS, SIZE, DATE, NAME)
  - Type flags: d=dir, f=file, r=root, i=inbox, b=rubbish, x=inShare
  - Exported: e/-
  - Sharing: s=shared, i=inShare, -=none
- `-h` — Human-readable sizes
- `-a` — Extra info (`-aa` shows public links and expiration)
- `--versions` — Show historical versions
- `--show-creation-time` — Show creation time instead of modification
- `remotepath` — May contain patterns like `/FOLDER1/PATTERN2/*.txt`

### find
Find nodes matching a pattern.
```
find [remotepath] [-l] [--pattern=PATTERN] [--type=d|f] [--mtime=TIMECONSTRAIN] [--size=SIZECONSTRAIN] [--use-pcre] [--time-format=FORMAT] [--show-handles|--print-only-handles]
```
- `--pattern=PATTERN` — Search pattern (PCRE or wildcards `?` and `*`)
- `--type=d|f` — Filter: `d` (folders), `f` (files)
- `--mtime=TIMECONSTRAIN` — Time constraint: `+1m12d3h` (older than), `-3h` (last 3h). Units: h, d, M, s, m, y
- `--size=SIZECONSTRAIN` — Size constraint: `+1M12k3B` (larger than), `-3M` (smaller than). Units: B, K, M, G, T
- `--show-handles` / `--print-only-handles` — Display handles only

### du
Print size used by files/folders.
```
du [-h] [--versions] [remotepath remotepath2 ...] [--use-pcre]
```
- `-h` — Human-readable
- `--versions` — Include versions in calculation

### df
Show storage info.
```
df [-h]
```

### mediainfo
Print media info of remote files.
```
mediainfo remotepath1 remotepath2 ...
```

---

## 5. File Management

### mkdir
Create a directory.
```
mkdir [-p] remotepath
```
- `-p` — Create full hierarchy (mkdir -p)

### cp
Copy files/folders (all remote).
```
cp [--use-pcre] srcremotepath [srcremotepath2 ...] dstremotepath|dstemail:
```
- If destination exists and is a folder: source is copied into it
- If destination does not exist and there is a single source: rename
- If `dstemail:`: sends to the user's inbox

### mv
Move/rename files/folders (all remote).
```
mv srcremotepath [--use-pcre] [srcremotepath2 ...] dstremotepath
```

### rm
Delete a remote file/folder.
```
rm [-r] [-f] [--use-pcre] remotepath
```
- `-r` — Recursive (for folders)
- `-f` — Force (no asking)

### put
Upload files/folders.
```
put [-c] [-q] [--print-tag-at-start] localfile [localfile2 ...] [dstremotepath]
```
- `-c` — Create remote destination folder if it does not exist
- `-q` — Queue: runs in background, does not wait to finish
- `--print-tag-at-start` — Print start message with transfer TAG (even with `-q`)
- `dstremotepath` can be omitted if only 1 localfile (uses current remote directory)

### get
Download a remote file/folder or public link.
```
get [-m] [-q] [--ignore-quota-warn] [--use-pcre] [--password=PASSWORD] exportedlink|remotepath [localpath]
```
- `-q` — Queue: background, does not wait
- `-m` — Merge: if folder already exists, merge contents (preserve existing)
- `--ignore-quota-warn` — Ignore quota exceeded warning
- `--password=PASSWORD` — Password for protected links (avoid `"` or `'`)
- For folders: downloads all contents
- If destination exists and is identical: nothing is done; if different, creates new with ` (NUM)`

### cat
Print the contents of remote files (text).
```
cat remotepath1 remotepath2 ...
```
On Windows, to preserve binary content, use non-interactive mode with `-o /path/to/file`.

### preview
Download/upload the preview of a file.
```
preview [-s] remotepath localpath
```
- `-s` — Upload (without `-s`, downloads)

### thumbnail
Download/upload the thumbnail of a file.
```
thumbnail [-s] remotepath localpath
```
- `-s` — Upload

---

## 6. Sharing

### export
Create/manage export links.
```
export [-d|-a [--writable] [--mega-hosted] [--password=PASSWORD] [--expire=TIMEDELAY] [-f]] [remotepath] [--use-pcre] [--time-format=FORMAT]
```
- `-a` — Add export (errors if already exists)
  - `--writable` — Editable link (generates auth-key for write access)
  - `--mega-hosted` — Share key with MEGA (for S4)
  - `--password=PASSWORD` — Password protect (PRO)
  - `--expire=TIMEDELAY` — Expiration (PRO). Format: `1m12d3h`
  - `-f` — Accept copyright terms implicitly
- `-d` — Remove export (file is not deleted)
- Without `-a` or `-d`: lists exports in the tree

### import
Import the contents of a remote link into the cloud.
```
import exportedlink [--password=PASSWORD] [remotepath]
```
- If no `remotepath`: uses current directory

### share
Manage folder shares.
```
share [-p] [-d|-a --with=user@email.com [--level=LEVEL]] [remotepath] [--use-pcre] [--time-format=FORMAT]
```
- `-p` — Show pending shares
- `--with=email` — Target user
- `-d` — Stop sharing
- `-a` — Add/modify share
- `--level=LEVEL` — Access level:
  - `0` = Read
  - `1` = Read+Write
  - `2` = Full access
  - `3` = Owner access
- Sharing with non-contacts is pending until the contact accepts

### permissions
Default permissions for created files/folders.
```
permissions [(--files|--folders) [-s XXX]]
```
- `-s XXX` — New octal value (min: 600 files, 700 folders)
- Not available on Windows
- Persists between executions, removed on logout

---

## 7. Transfers

### transfers
List or operate on transfers.
```
transfers [-c TAG|-a] | [-r TAG|-a] | [-p TAG|-a] [--only-downloads | --only-uploads] [SHOWOPTIONS]
```
- `-c` — Cancel transfer (or `-a` = all)
- `-p` — Pause
- `-r` — Resume
- `--only-uploads` / `--only-downloads` — Filter
- Display options:
  - `--summary` — Summary
  - `--show-syncs` — Show sync transfers
  - `--show-completed` / `--only-completed` — Completed
  - `--limit=N` — Limit rows
- Legend: `⇓` Download, `⇑` Upload, `⇵` Sync, `⏫` Backup

### speedlimit
Display/modify speed limits.
```
speedlimit [-u|-d|--upload-connections|--download-connections] [-h] [NEWLIMIT]
```
- `-d` — Download (size/second)
- `-u` — Upload
- `--upload-connections` / `--download-connections` — Max connections
- `NEWLIMIT=0` = unlimited
- Units: B, K, M, G, T

### deleteversions
Delete previous file versions.
```
deleteversions [-f] (--all | remotepath1 remotepath2 ...) [--use-pcre]
```
- `-f` — Force (no asking)
- `--all` — Delete versions of all nodes
- Current version is preserved

### reload
Force a reload of remote files.
```
reload
```
Also resumes synchronizations.

---

## 8. Configuration

### configure
Global configuration settings.
```
configure [key [value]]
```
Keys:
- `max_nodes_in_cache` — Max nodes in memory (controls SDK cache)
- `exported_folders_sdks` — Additional SDK instances for exported links. Default 5, Min 0, Max 20

### https
HTTPS for transfers.
```
https [on|off]
```
- Data is already end-to-end encrypted; HTTPS adds overhead
- Setting saved between sessions, removed on logout

### proxy
Configure proxy.
```
proxy [URL|--auto|--none] [--username=USERNAME --password=PASSWORD]
```

### errorcode
Translate error code to string.
```
errorcode number
```

### graphics
Enable/disable graphics features (thumbnails/previews).
```
graphics [on|off]
```

### codepage
Configure codepage (Windows).
```
codepage [N [M]]
```
- `N` — Main codepage (65001 = Unicode)
- `M` — Secondary codepage

### autocomplete
Autocomplete mode.
```
autocomplete [dos | unix]
```

### unicode
Toggle Unicode in interactive shell.
```
unicode
```
- Experimental — Windows only

---

## 9. Utilities

### help
Print command list.
```
help [-f|-ff|--non-interactive|--upgrade|--paths] [--show-all-options]
```
- `-f` — Brief command descriptions
- `-ff` — Full descriptions
- `--non-interactive` — Scripting usage info
- `--upgrade` — PRO plan info
- `--paths` — Path caveats

### version
Print version and info.
```
version [-l] [-c]
```
- `-c` — Changelog for current version
- `-l` — Extended info (SDK version, features)

### update
Update MEGAcmd.
```
update [--auto=on|off|query]
```
- Automatic updates for Windows and macOS (not Linux)
- After update: MEGAcmd restarts

### clear
Clear screen.
```
clear
```

### exit / quit
Exit MEGAcmd.
```
exit [--only-shell]
quit [--only-shell]
```
- `--only-shell` — Only exit the interactive shell (server keeps running)
- Session stays active, caches available

### debug
Enter debugging mode (HIGHLY VERBOSE).
```
debug
```

### psa
Show Public Service Announcement.
```
psa [--discard]
```
- `--discard` — Discard last PSA

---

## 10. Sync

### sync — Control Synchronizations
```
sync [localpath dstremotepath | [-dpe] [ID|localpath]]
```
**Create sync:** `sync /path/to/local/folder /folder/in/mega`

**List syncs:** `sync`
Columns: ID, LOCALPATH, REMOTEPATH, RUN_STATE (Pending/Loading/Running/Suspended/Disabled), STATUS (None/Synced/Pending/Syncing/Processing), ERROR, SIZE, FILE, DIRS

**Manage:**
- `sync -d ID|localpath` — Delete sync (does not delete files)
- `sync -p ID|localpath` — Pause
- `sync -e ID|localpath` — Resume

### sync-ignore — Ignore Filters
```
sync-ignore [--show|[--add|--add-exclusion|--remove|--remove-exclusion] filter1 filter2 ...] (ID|localpath|DEFAULT)
```
**Filter format:** `<CLASS><TARGET><TYPE><STRATEGY>:<PATTERN>`

| Part | Values | Meaning |
|---|---|---|
| CLASS | `-` (exclude) or `+` (include) | Exclude or include |
| TARGET | `d` (dir), `f` (file), `s` (symlink), `a` (all) | Entry type |
| TYPE | `N` (local name), `p` (path), `n` (subtree name) | Name scope |
| STRATEGY | `G`/`g` (glob), `R`/`r` (regexp) | Upper = case-sensitive |

**Examples:** `-f:*.txt`, `+fg:work*.txt`, `-N:*.avi`, `-nr:.*foo.*`, `-d:private`

### sync-config — Sync Configuration
```
sync-config [--delayed-uploads-wait-seconds | --delayed-uploads-max-attempts]
```
- `--delayed-uploads-wait-seconds` — Seconds before re-uploading a delayed file
- `--delayed-uploads-max-attempts` — Max times a file can change quickly before being delayed

### sync-issues — Sync Issues
```
sync-issues [[--detail (ID|--all)] [--limit=rowcount] [--disable-path-collapse]] | [--enable-warning|--disable-warning]
```
Columns: ISSUE_ID, PARENT_SYNC, REASON

### exclude (DEPRECATED — use sync-ignore)
```
exclude [(-a|-d) pattern1 pattern2 pattern3]
```

---

## 11. Backups

### backup — Control Backups
```
backup (localpath remotepath --period="PERIODSTRING" --num-backups=N | [-lhda] [TAG|localpath] [--period="PERIODSTRING"] [--num-backups=N]) [--time-format=FORMAT]
```
> BETA feature

**Create backup:**
`backup /path/mega/folder /remote/path --period="0 0 4 * * *" --num-backups=10`

- First backup runs immediately
- Stored as: `/remote/path/myfolder_bk_TIME1`, `/remote/path/myfolder_bk_TIME2`, ...
- `--period`: time in TIMEFORMAT (`1m12d3h`) or cron expression (`S M H D Mo DoW`)
- `--num-backups=N`: maximum backups stored

**List:**
- `backup` — List
- `backup -l` — Extended info (period, next schedule)
- `backup -h` — Backup history

**Manage:**
- `backup -d TAG|localpath` — Remove configuration
- `backup -a TAG|localpath` — Abort ongoing backup
- `backup 4 --period=2h` — Change period
- `backup /path/folder --num-backups=1` — Change max number

**Monitoring:** `watch mega-backup -lh`

---

## 12. FTP & WebDAV Servers

### ftp — FTP Server
```
ftp [-d (--all | remotepath)] [ remotepath [--port=PORT] [--data-ports=BEGIN-END] [--public] [--tls --certificate=/path/to/certificate.pem --key=/path/to/certificate.key]] [--use-pcre]
```
> BETA. Passive mode only.

**Serve folder:** `ftp /path/mega/folder`
**Streaming:** `ftp /path/to/myfile.mp4`
**List:** `ftp`
**Parameters:**
- `--port=PORT` — Port (default: 4990)
- `--data-ports=BEGIN-END` — Passive data port range (default: 1500-1600)
- `--public` — External access (default: localhost only)
- `--tls` — FTPS with TLS
- `--certificate` / `--key` — PEM certificate and key
**Stop:** `ftp -d /path/mega/folder` or `ftp -d --all`

### webdav — WebDAV Server
```
webdav [-d (--all | remotepath)] [ remotepath [--port=PORT] [--public] [--tls --certificate=/path/to/certificate.pem --key=/path/to/certificate.key]] [--use-pcre]
```
> BETA

**Serve folder:** `webdav /path/mega/folder`
**Streaming:** `webdav /path/to/myfile.mp4`
**List:** `webdav`
**Parameters:**
- `--port=PORT` — Port (default: 4443)
- `--public` — External access
- `--tls` — HTTPS with TLS
**Stop:** `webdav -d /path/mega/folder` or `webdav -d --all`

**Notes:** Only one server at a time. First location's configuration applies to all.

---

## 13. FUSE

> BETA. Linux only.

### fuse-add — Create mount
```
fuse-add [--name=name] [--disabled] [--transient] [--read-only] localPath remotePath
```
- `--name` — Friendly name
- `--read-only` — Read-only
- `--transient` — Lost on restart
- `--disabled` — Do not enable after adding

### fuse-show — List mounts
```
fuse-show [--only-enabled] [--disable-path-collapse] [[--limit=rowcount] | [name|localPath]]
```
Columns: NAME, LOCAL_PATH, REMOTE_PATH, PERSISTENT (YES/NO), ENABLED (YES/NO)

### fuse-enable / fuse-disable — Enable/Disable
```
fuse-enable [--temporarily] (name|localPath)
fuse-disable [--temporarily] (name|localPath)
```

### fuse-config — Configure
```
fuse-config [--name=name] [--enable-at-startup=yes|no] [--persistent=yes|no] [--read-only=yes|no] (name|localPath)
```

### fuse-remove — Remove
```
fuse-remove (name|localPath)
```
**Note:** Must be disabled before removing.

### FUSE Cache
- Located at `$HOME/.megaCmd/fuse-cache`
- Files downloaded completely before opening (streaming not supported)
- Cache is cleaned automatically

### Troubleshooting
"Transport endpoint is not connected":
```bash
fusermount -u /local/path/to/fuse/mountpoint
fusermount -u -z /local/path/to/fuse/mountpoint  # If it fails
```

---

## 14. Common Flags

### Cross-command Flags
| Flag | Description | Commands |
|---|---|---|
| `--use-pcre` | Use PCRE in paths | cp, mv, rm, ls, find, du, get, export, share, ftp, webdav, deleteversions |
| `--time-format=FORMAT` | Date/time format | ls, find, export, share, backup, whoami, users, sync-issues, showpcr |
| `--path-display-size=N` | Fixed path size | backup, du, sync, transfers, mediainfo |
| `--col-separator=X` | Column separator | sync, transfers, sync-issues, fuse-show |
| `--output-cols=COLS` | Columns to display | sync, transfers, sync-issues, fuse-show |

### Date/Time Formats
- `RFC2822` — `Thu, 26 Apr 2018 11:20:09 +1200`
- `ISO6081` — `2018-04-26T11:20:09`
- `ISO6081_WITH_TIME` — `2018-04-26T11:20:09+12:00`
- `SHORT` — `26Apr2018 11:20:09`
- `SHORT_UTC` — `26Apr2018 11:20:09 UTC`
- `CUSTOM strftime` — Custom format

### Time Expressions
Units: `s` (seconds), `m` (minutes), `h` (hours), `d` (days), `M` (months), `y` (years)
Example: `1m12d3h` = 1 month, 12 days, 3 hours

### Cron Expressions (backups)
Format: `S M H D Mo DoW`
- `0 0 4 * * *` = every day at 4:00 UTC
- `0 0 */2 * *` = every 2 hours
- `0 30 8 * * 1-5` = weekdays at 8:30

### Size Format
Units: `B`, `K`, `M`, `G`, `T`
Examples: `1M12k3B`, `-3G`

---

## 15. Error Codes

| Code | Name | Description |
|---|---|---|
| `0` | API_OK | Success |
| `-1` | API_ARGS | Invalid arguments |
| `-2` | API_EACCESS | Access denied / permission |
| `-3` | API_ECAPTCHA | Captcha required |
| `-4` | API_ETEMPUNAVAIL | Temporarily unavailable |
| `-5` | API_ERATELIMIT | Rate limit exceeded |
| `-6` | API_EFAILED | Operation failed |
| `-7` | API_ETOOMANY | Too many simultaneous requests |
| `-8` | API_ERANGE | Out of range |
| `-9` | API_EEXPIRED | Resource expired |
| `-10` | API_ENOENT | Not found |
| `-11` | API_ECIRCULAR | Circular operation |
| `-12` | API_ENOACCESS | No access to node |
| `-13` | API_EEXIST | Already exists |
| `-14` | API_EINCOMPLETE | Incomplete |
| `-15` | API_EKEY | Invalid/changed key |
| `-16` | API_ESID | Invalid session |
| `-17` | API_EBLOCKED | Blocked |
| `-18` | API_EOVERQUOTA | Quota exceeded |
| `-19` | API_ETEMPORARILYDISABLED | Temporarily disabled |
| `-20` | API_EBUSINESSPASTDUE | Business account past due |
| `-21` | API_EPAYWALL | Paywall |

Use `errorcode NUM` to translate.

---

## 16. IPC — Communication Protocol

### TCP Socket (mega-execports — Python)
Alternative client at `src/client/python/mega-execports`, port 12300:

1. Connect to `127.0.0.1:12300`
2. Send command as string (space-separated args)
3. Receive 2 bytes: `socketOutId` (unsigned short)
4. Connect to `127.0.0.1:12300 + socketOutId`
5. Receive 4 bytes: `outCode` (int) — exit code
6. Receive remaining data: `commandOutput` (string)
7. If `outCode < 0`: exit `-outCode`; otherwise: exit `outCode`

### File Sockets (Unix)
Used on Linux/macOS for communication between `mega-exec` and `mega-cmd-server`.

### Named Pipes (Windows)
Used on Windows.

---

## 17. Logging

### Log Levels
| Type | Label | Source |
|---|---|---|
| MEGAcmd | `cmd` | Command processing |
| SDK | `sdk` | Engine, requests, network |

Levels: FATAL → ERROR → WARNING → INFO → DEBUG → VERBOSE

### log command
```
log [-sc] level
```
- `-c` — MEGAcmd log level
- `-s` — SDK log level

### Per-command Verbosity
Any command accepts: `-v` (Warnings), `-vv` (Debug), `-vvv` (Verbose)

### Startup Verbosity
```
MEGAcmdServer --debug         # MEGAcmd=DEBUG, SDK=DEFAULT
MEGAcmdServer --debug-full    # MEGAcmd=DEBUG, SDK=DEBUG
MEGAcmdServer --verbose       # MEGAcmd=VERBOSE, SDK=DEFAULT
MEGAcmdServer --verbose-full  # MEGAcmd=VERBOSE, SDK=VERBOSE
```
Or via env: `MEGACMD_LOGLEVEL=FULLVERBOSE MEGAcmdServer`

### JSON Logs
When SDK log level = VERBOSE. Controlled via `MEGACMD_JSON_LOGS=0|1`.

### Rotating Logger
Configured via `megacmd.cfg`:
```
RotatingLogger:RotationType=Timestamp
RotatingLogger:CompressionType=Gzip
RotatingLogger:MaxFileMB=40.25
RotatingLogger:MaxFilesToKeep=20
RotatingLogger:MaxFileAgeSeconds=3600
RotatingLogger:MaxMessageBusMB=64.0
```

### Log Location
- Linux/macOS: `$HOME/.megaCmd/megacmdserver.log`
- Windows: `%LOCALAPPDATA%\MEGAcmd\.megaCmd\megacmdserver.log`

---

> See `command --help` for up-to-date details on each command.
