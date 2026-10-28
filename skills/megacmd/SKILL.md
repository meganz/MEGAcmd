---
name: megacmd
description: >-
  CLI agent for MEGA.nz. Activate when the user asks to upload files to MEGA,
  download from MEGA, sync local folders with MEGA cloud, schedule backups to
  MEGA, share files via MEGA public links, mount MEGA as a local folder (FUSE on
  Linux), serve files via MEGA WebDAV/FTP, or manage MEGA account settings.
license: MIT-0
metadata:
  version: "1.0.0"
  category: cloud-storage
  platform: [linux, macos, windows]
---

# MEGAcmd — AI Agent Guide

## What this skill does

Instructions for agents to use **MEGAcmd**, the official CLI interface for MEGA.nz. Covers file operations, bidirectional sync, scheduled backups, WebDAV/FTP servers, FUSE mount, sharing, and account management.

> **Important:** This skill is for MEGAcmd **USERS**. If the goal is to BUILD, DEBUG, or CONTRIBUTE to the source code, do not activate this skill.

## When to use

- The user asks to upload/download files on MEGA.nz
- The user wants to sync local folders ↔ MEGA cloud
- The user needs scheduled backups with version retention
- The user wants to share files via public links
- The user needs to serve files via WebDAV or FTP
- The user wants to mount a MEGA folder as a local filesystem (Linux)
- The user needs to manage account, password, sessions, or contacts
- The user reports that sync is not working

## When NOT to use

- The user wants to use the MEGA web interface (browser)
- The user wants to BUILD, DEBUG, or CONTRIBUTE to MEGAcmd source code — that is a different skill
- The user wants to use the MEGA SDK for custom integration
- MEGAcmd is not installed (this skill does not install it — only provides installation instructions)
- The user wants to access files via the MEGA Desktop App (not CLI)

---

> ⚠️ **Security Overview**
> This skill can perform powerful operations on your MEGA account, including file management, bidirectional sync, scheduled backups, public link sharing, and network services (WebDAV/FTP).
> **Agent caution:** Require explicit user confirmation before login, deleting files, syncing folders, creating backups, exporting links, sharing folders, changing account settings, mounting FUSE, or starting WebDAV/FTP.
> **Credential safety:** Avoid putting passwords, session IDs, proxy credentials, or recovery keys directly in commands or chat-visible logs. Prefer interactive login or protected secrets mechanisms.

## Prerequisites

Before using any command, ALWAYS check:

```bash
# 1. Is MEGAcmd installed?
which mega-exec 2>/dev/null && echo "INSTALLED" || echo "NOT INSTALLED"

# 2. Is the server running?
ps aux | grep -q "[m]ega-cmd-server" && echo "SERVER OK" || echo "SERVER STOPPED"

# 3. Are you logged in?
mega-whoami >/dev/null 2>&1 && echo "LOGGED IN" || echo "NOT LOGGED IN"
```

If the server is not running: `mega-cmd-server &`
If not logged in: `mega-login email password`

---

## Architecture

| Component | Executable | Function |
|---|---|---|
| Server | `mega-cmd-server` | Runs in background, processes commands, manages sync/backups/transfers |
| Shell | `mega-cmd` | Interactive mode (commands WITHOUT `mega-` prefix) |
| Client | `mega-exec` + `mega-*` | Scriptable mode (commands WITH `mega-` prefix) |

**Data location:** `$HOME/.megaCmd/` (Linux) | `%LOCALAPPDATA%\MEGAcmd\.megaCmd\` (Windows)

**Session:** Login saves local cache. `logout` clears it. `logout --keep-session` preserves cache.

---

## Usage Modes — PAY ATTENTION TO PREFIXES

### Scriptable Mode (agent uses this → ALWAYS use `mega-`)
```bash
mega-login email password
mega-put ~/file.pdf /Destination/
mega-get /remote/file.pdf ~/Downloads/
exit code 0 = success, != 0 = failure
```

### Interactive Mode (MEGAcmd shell → commands WITHOUT `mega-`)
```bash
mega-cmd
MEGA CMD> login email password
MEGA CMD> put ~/file.pdf /Destination/
```

> ⚠️ **Agent rule:** You are in a bash terminal. **ALWAYS** use `mega-` as prefix. Commands without `mega-` (`sync`, `webdav`, `ftp`, `log`) only work INSIDE the interactive shell (`mega-cmd`).

---

## Essential Commands

> ⚠️ **Security Warning — Credential Exposure**
> Running mega-login with an inline password exposes credentials to shell history, process listings, audit logs, and agent telemetry. In shared or automated environments, prefer login methods that do not pass the password as a command-line argument.
> **Recommendation:** Use `mega-login` interactively (without the password argument) when possible. Always enable 2FA for account protection. Avoid including credentials in scripts or automation output.

### Login & Account
| Command | Description |
|---|---|
| `mega-login email password [--auth-code=XXXX]` | Login (optional 2FA) |
| `mega-logout [--keep-session]` | Logout (or keep cache) |
| `mega-whoami [-l]` | Account info |
| `mega-df [-h]` | Storage space |
| `mega-masterkey ./file.txt` | Save recovery key |
| `mega-passwd [-f] [--auth-code=XXXX] new-password` | Change password |
| `mega-session` | Show session ID |
| `mega-killsession -a` | Kill all other sessions |


> ⚠️ **Security Warning — Recovery Key**
> The master key (recovery key) is essential for data recovery. If lost, you cannot recover your data without the password. If exposed, an attacker gains durable access to your account.
> **Recommendation:** Save the master key to encrypted storage (password manager, encrypted USB). Store it with restricted file permissions (e.g., `chmod 600`). Do NOT sync the plaintext file to cloud storage.

> ⚠️ **Security Warning — Session Token**
> The session ID is a bearer credential. If exposed, anyone can impersonate your session until it expires or is revoked.
> **Recommendation:** Do not share the session output in logs, screenshots, scripts, or automated reports.

### Navigation & Listing
| Command | Description |
|---|---|
| `mega-ls [-lhR] [--versions] [path]` | List files |
| `mega-ls -l` | Detailed list (type, size, date) |
| `mega-find [path] --pattern="*.pdf" [--type=f\|d]` | Search files |
| `mega-find / --pattern="*.tmp" --mtime="-7d"` | Search by date |
| `mega-du [-h] [--versions] [path]` | Folder disk usage |
| `mega-cd [path]` | Change remote directory |
| `mega-pwd` | Current remote directory |
| `mega-mount` | List root nodes |

### Upload & Download
| Command | Description |
|---|---|
| `mega-put [-c] [-q] local [destination]` | Upload (`-c`=create folder, `-q`=background) |
| `mega-get [-q] source [local]` | Download (`--password` for protected links) |
| `mega-get "link#key" ./dir` | Download from public link |
| `mega-cat path` | Display text file contents |

> ⚠️ **Security Warning — Data Disclosure**
> Creating public links exposes your cloud data to anyone with the link. Links with `--writable` grant write access to the shared folder.
> **Recommendation:** Use `--password` to protect sensitive files. Set `--expire` dates for time-limited access. Periodically review and remove unused export links.
### File Management
| Command | Description |
|---|---|
| `mega-mkdir [-p] path` | Create directory |
| `mega-cp [--use-pcre] source destination` | Copy (all remote) |
| `mega-mv [--use-pcre] source destination` | Move/rename |
| `mega-rm [-r] [-f] path` | Delete (recursive/forced) |
| `mega-export -a path` | Create public link |
| `mega-export -d path` | Remove link |
| `mega-export -a path --password="x" --expire="30d"` | Password-protected link (PRO) |
| `mega-import link [destination]` | Import link to cloud |

> ⚠️ **Security Warning — Folder Sharing**
> Sharing folders gives external users access to your MEGA content. Permission levels range from Read (0) to Owner (3). Over-sharing or granting excessive permissions can expose sensitive data.
> **Recommendation:** Grant the minimum permission level needed. Remove sharing when no longer required. Review shared folders periodically.

### Sharing
| Command | Description |
|---|---|
| `mega-share -a --with="email" --level=N /folder` | Levels: 0=Read, 1=R+W, 2=Full, 3=Owner |
| `mega-share -d --with="email" /folder` | Stop sharing |
| `mega-invite email [--message="..."]` | Invite contact |
| `mega-ipc email -a` | Accept invitation |
| `mega-users [-s]` | List contacts |

---

## Synchronization (Sync)

> Sync is bidirectional. Removed files go to `SyncDebris` in the Rubbish Bin.

```bash
mega-sync ~/Documents /MEGA/Documents     # Create sync
mega-sync                                    # List syncs
mega-sync -p ID                              # Pause
mega-sync -e ID                              # Resume
mega-sync -d ID                              # Remove (does not delete files)
```

**Ignore patterns:**
```bash
mega-sync-ignore --add "-f:*.tmp" ID
mega-sync-ignore --add "-f:node_modules" ID
mega-sync-ignore --show ID
```

Filter format: `<CLASS><TARGET><TYPE><STRATEGY>:<PATTERN>`
- CLASS: `-` (exclude) / `+` (include)
- TARGET: `d`(dir), `f`(file), `s`(symlink), `a`(all)
- TYPE: `N`(local name), `p`(path), `n`(subtree name)
- STRATEGY: `G`/`g`(glob), `R`/`r`(regexp). Upper=case-sensitive

**Verify:** `mega-sync` shows STATUS = `Synced` when everything is OK.

---

## Backups

> BETA feature. Backups are unidirectional (local → cloud).

```bash
mega-backup ~/Photos /Backups/Photos --period="0 0 4 * * *" --num-backups=10
mega-backup ~/Projects /Backups --period="2h" --num-backups=24

# Manage
mega-backup                    # List
mega-backup -lh                # With history
mega-backup -a TAG             # Abort
mega-backup -d ~/Photos        # Remove configuration
```

Storage pattern: `/remote/folder_bk_TIMESTAMP`

**Verify:** `mega-backup -lh` shows STATUS `COMPLETE` and history.

---

## WebDAV & FTP Servers

> BETA. Only one server at a time. First location's configuration applies to all.

> ⚠️ **Security Warning — Network Exposure**
> Starting WebDAV exposes your MEGA content over the network. Without TLS, traffic is unencrypted. The `--public` flag makes the service accessible beyond localhost.
> **Recommendation:** Use `--tls` with valid certificates. Avoid `--public` unless necessary. Stop services when not in use with `mega-webdav -d`.

### WebDAV
```bash
mega-webdav /Videos                            # Serve folder (port 4443)
mega-webdav /movie.mp4                         # Streaming
mega-webdav /Docs --tls --certificate=cert.pem --key=key.pem  # HTTPS
mega-webdav /Public --public --port=8080       # Public
mega-webdav -d /Videos                         # Stop
mega-webdav -d --all                           # Stop all
```

> ⚠️ **Security Warning — Network Exposure**
> Starting FTP exposes your MEGA content via unencrypted FTP. Without `--tls`, credentials and data are sent in plain text. By default the server is local-only; use `--public` to allow remote access.
> **Recommendation:** Use `--tls` with valid certificates for secure FTP. Avoid serving sensitive content over plain FTP. Stop services when not in use with `mega-ftp -d`.

### FTP
```bash
mega-ftp /Public                               # Serve folder (port 4990)
mega-ftp /Docs --tls --certificate=cert.pem --key=key.pem  # FTPs
mega-ftp -d /Public                            # Stop
```

**Verify:** `mega-webdav` or `mega-ftp` lists active URLs.

---

## FUSE (Linux only)

> BETA. Streaming not supported — files are fully downloaded. Cache at `$HOME/.megaCmd/fuse-cache`.

```bash
mega-fuse-add --name=my-docs /mnt/mega /Documents
mega-fuse-show                                 # List
mega-fuse-enable my-docs                       # Enable
mega-fuse-disable my-docs                      # Disable
mega-fuse-remove my-docs                       # Remove (must be disabled first)
```

**Issue:** "Transport endpoint is not connected"
```bash
fusermount -u /mnt/mega
```

---

## Transfers

```bash
mega-transfers                            # List active
mega-transfers --summary                  # Summary
mega-transfers -c TAG                     # Cancel
mega-transfers -p TAG                     # Pause
mega-transfers -r TAG                     # Resume
mega-transfers -c -a                      # Cancel all
mega-speedlimit -d 2M                     # Limit download
mega-speedlimit -u 1M                     # Limit upload
```

---

## Settings

```bash
mega-https on|off
mega-proxy URL|--auto|--none
mega-configure
mega-configure max_nodes_in_cache N
mega-permissions --files -s 600          # Unix only
mega-log -c DEBUG                        # Adjust MEGAcmd log level
mega-log -s INFO                         # Adjust SDK log level
```
> ⚠️ If using proxy authentication, avoid passing credentials on the command line where they may be captured by shell history or process listings. Prefer environment variables or a configuration file instead.

---

## Quick Diagnostics — Sync Not Working

When sync is stuck or failing:

### Step 1: General state
```bash
# Is the server running?
ps aux | grep -c "[m]ega-cmd-server"
# Should return 1 or more

# Are you logged in?
mega-whoami
# Should show the account email
```

### Step 2: Check sync
```bash
mega-sync
# Columns: RUN_STATE (Running/Pending/Disabled), STATUS (Synced/Pending/Syncing), ERROR
```

### Step 3: Check conflicts
```bash
mega-sync-issues
# If there are issues, investigate:
mega-sync-issues --detail ISSUE_ID
```

### Step 4: Check transfers
```bash
mega-transfers --summary
# Active uploads or downloads? Progress?
```

### Step 5: Check logs
```bash
tail -50 ~/.megaCmd/megacmdserver.log
# Look for: ERR, WARN, "sync issues", "quota", "rate limit"
```

### Step 6: Check storage
```bash
mega-df -h
# Quota exceeded? (USED STORAGE near 100%)
```

### Common causes and solutions

| Symptom | Likely cause | Solution |
|---|---|---|
| RUN_STATE = Disabled | Sync paused | `mega-sync -e ID` |
| STATUS = Pending (never changes) | Initial scan of many files | Wait (can take hours with 100k+ files) |
| Sync Issues > 0 | Local × cloud conflicts | `mega-sync-issues --detail ID`, remove/move problematic files |
| ERROR = "Sync Issues (N)" | Problem files | Run step 3 |
| No transfers appear | Scan still in progress | Wait |
| "rate limit" in log | Too many requests in short period | Wait a few minutes |
| "quota" in log | Storage quota exceeded | `mega-df -h`, free up space |
| LOG full of "Can't find" | Deleted/moved files | Usually resolves itself after rescan |

> ⚠️ The following commands delete local files without confirmation. Verify paths are correct before running.

### Action plan for stuck sync

```bash
# 1. Pause
mega-sync -p BK0pIuFWODQ    # use your actual sync ID

# 2. Resolve issues (if any)
# Remove problematic files (Zone.Identifier, .lnk, .megaignore)
find ~/sync-folder -name "*:Zone.Identifier" -delete 2>/dev/null
find ~/sync-folder -name "*.lnk" -type f -delete 2>/dev/null
find ~/sync-folder -name ".megaignore" -delete 2>/dev/null

# 3. Resume
mega-sync -e BK0pIuFWODQ

# 4. Monitor
sleep 10 && mega-sync && mega-transfers --summary
```

---

## Verification — How to Confirm It Worked

| Operation | How to verify |
|---|---|
| Login | `mega-whoami` shows the account email |
| List | `mega-ls /path` lists files (or error if not found) |
| Upload | `mega-ls /destination` shows the uploaded file |
| Download | File exists at the specified local path |
| Sync active | `mega-sync` shows STATUS = `Synced`, ERROR = `NO` |
| Sync in progress | `mega-transfers` shows active transfers |
| Backup created | `mega-backup -lh` shows history with STATUS = `COMPLETE` |
| Public link | `mega-export /path` shows the URL |
| WebDAV active | `mega-webdav` lists serving URLs |
| FTP active | `mega-ftp` lists serving URLs |
| Session closed | `mega-whoami` returns a not-logged-in error |

---

## Important Rules

1. **Always check exit code**: `mega-command || echo "FAILED ($?)"`
2. **Escape `!` in links**: `mega-get https://mega.nz/#F\!ABcD\!Key ./dir`
3. **Master Key is ESSENTIAL**: `mega-masterkey ./recovery.txt` — without it, losing your password means losing everything. Store it in an encrypted, access-restricted location.
4. **Logout** on shared machines
5. **`logout --keep-session`** on personal machines (preserves cache, resumes session)
6. **`--writable` links expose the shared folder** — anyone with the link can upload, modify, or delete files in that folder. Only share writable links with trusted parties.
7. **Use `-q` (queue)** for large operations in background
8. **In syncs, exclude `node_modules`, `.git`, `*.tmp`** with `mega-sync-ignore`
9. **First sync is slower** — 100k+ files can take hours to scan

## Common Error Codes

| Code | Name | Meaning |
|---|---|---|
| `0` | API_OK | Success |
| `-2` | API_EACCESS | Access denied / permission |
| `-5` | API_ERATELIMIT | Too many requests — wait |
| `-10` | API_ENOENT | File/folder not found |
| `-13` | API_EEXIST | Already exists |
| `-16` | API_ESID | Invalid session — login again |
| `-18` | API_EOVERQUOTA | Storage quota exceeded |

Use `mega-errorcode NUM` to translate any code.

## Compatibility

| Feature | Linux | macOS | Windows |
|---|---|---|---|
| FUSE mounts | ✅ | ❌ | ❌ |
| Autocomplete (bash) | ✅ | ✅ | ❌ |
| Unicode in shell | ✅ | ✅ | Experimental |
| Auto-update | ❌ (pkg manager) | ✅ | ✅ |
| File permissions | ✅ | ✅ | ❌ |

---

## Full Reference

For detailed documentation of ALL 76 commands (full syntax, all flags, examples), see:

- **Always available:** `mega-command --help` for each command's help
