# MEGAcmd — Agent Skill

AI agent skill for **MEGAcmd**, the official command-line interface for [MEGA.nz](https://mega.nz) cloud storage.

> **Technical name:** `megacmd`  
> **Category:** cloud-storage  
> **Compatibility:** OpenCode, Cline, Claude Code, Continue.dev, and SKILL.md-compatible tools  

---

## Objectives

This skill enables AI agents to:

- **Manage files** on MEGA cloud (upload, download, copy, move, delete)
- **Synchronize** local folders with the cloud (bidirectional sync)
- **Schedule backups** with historical version retention
- **Share** files via public links (with or without passwords, with or without expiration)
- **Serve** files via WebDAV or FTP (with TLS support)
- **Mount** MEGA folders as local filesystem (FUSE, Linux only)
- **Manage** account (login, password, master key, sessions, 2FA)
- **Manage** contacts and folder sharing between users
- **Diagnose** and resolve synchronization issues

---

## System Requirements

### Operating System

| OS | Support | Notes |
|---|---|---|
| **Linux** | ✅ Full | FUSE, autocomplete, file permissions |
| **macOS** | ✅ Full | No FUSE support |
| **Windows** | ✅ Full | No bash autocomplete, no file permissions |

### Dependencies

MEGAcmd must be installed on the system. Dependencies are managed automatically by the official installer.

| Dependency | Required? | Purpose |
|---|---|---|
| MEGAcmd (>= 2.5.0) | ✅ Yes | CLI binaries |
| `bash` (Linux/macOS) | ✅ Yes | Execution of `mega-*` wrappers |
| `PATH` configured | ✅ Yes | Access to `mega-*` commands |
| `ps`, `grep`, `which` | ✅ Yes | Diagnostics and verification |
| Internet connection | ✅ Yes | Access to MEGA servers |

### Network Ports

| Service | Default Port | Purpose |
|---|---|---|
| WebDAV | 4443 | Serve files via HTTP/HTTPS |
| FTP | 4990 | Serve files via FTP (passive mode) |
| FTP data | 1500-1600 | FTP data channel |
| IPC (TCP) | 12300 | Client-server communication (Python alternative) |

---

## Account Requirements

### MEGA Account

- A [MEGA.nz](https://mega.nz) account is required for most operations
- Downloading public links does not require login
- Uploading to public folders (File Requests) does not require login
- A **Pro** plan is required for:
  - Password-protected links (`export --password`)
  - Expiring links (`export --expire`)
  - Higher storage and transfer limits

### Required Credentials

| Operation | Requires | How to get |
|---|---|---|
| Login | Email + password | Create at mega.nz |
| 2FA | `--auth-code` | Enable 2FA in account settings |
| Password-protected link | Link password | Set when exporting |
| Writable link | `--auth-key` | Generated when exporting with `--writable` |

### Master Key (Recovery Key)

**Essential.** Without the Master Key, losing your password means losing **all your data**. It must be saved immediately after creating your account:

```bash
mega-masterkey ./my-recovery-key.txt
```

---

## Installing MEGAcmd

### Via Official Package (Recommended)

Download the installer for your system at: [https://mega.nz/cmd](https://mega.nz/cmd)

**Linux (Ubuntu/Debian):**
```bash
# Add repository
wget -O /tmp/megacmd.deb https://mega.nz/linux/repo/xUbuntu_24.04/amd64/megacmd_2.5.2-1_amd64.deb
sudo dpkg -i /tmp/megacmd.deb
sudo apt install -f
```

**macOS:**
```bash
# Download DMG from https://mega.nz/cmd and drag to Applications
export PATH=/Applications/MEGAcmd.app/Contents/MacOS:$PATH
```

**Windows:**
```powershell
# Download and run installer from https://mega.nz/cmd
# Silent installation:
MEGAcmdSetup.exe /S

# Add to PATH:
$env:PATH += ";$env:LOCALAPPDATA\MEGAcmd"
```

### Via Manual Build

```bash
git clone https://github.com/meganz/MEGAcmd.git
cd MEGAcmd && git submodule update --init --recursive
cmake -B build/build-cmake-Release -DCMAKE_BUILD_TYPE=Release
cmake --build build/build-cmake-Release -j$(nproc)
sudo cmake --install build/build-cmake-Release
```

### Verify Installation

```bash
which mega-exec        # Should show the binary path
mega-exec version      # Should show the version
mega-help              # Should list commands
```

---

## How to Use This Skill

### Activation

The skill is **automatically** activated by the agent when the conversation context matches the `description`. To force activation, mention "MEGAcmd", "MEGA.nz", "upload to MEGA", "download from MEGA", "sync with MEGA", "backup to MEGA", or "MEGA link".

### File Structure

```
skills/megacmd/
├── SKILL.md                   # ⬅️ Main instructions (English)
├── SKILL.pt-BR.md             # ⬅️ Main instructions (Portuguese)
├── README.md                  # ⬅️ This file (skill documentation, English)
├── README.pt-BR.md            # ⬅️ Skill documentation (Portuguese)
└── references/
    ├── complete-commands-reference.md     # ⬅️ Full command reference (English)
    └── comandos-completos.pt-BR.md        # ⬅️ Full command reference (Portuguese)
```

### Agent Fallback

If the agent cannot find the skill, it can use:
- `AGENTS.md` (English) or `AGENTS.pt-BR.md` (Portuguese) at the project root
- `mega-command --help` for each command's help
- `mega-help` for a complete command list

---

## Interaction Modes

### Scriptable (recommended for agents)

Commands with the `mega-` prefix executed directly in the terminal:

```bash
mega-login user@email.com password
mega-put ~/document.pdf /Documents/
mega-get /remote/file.pdf ~/Downloads/
```

> ⚠️ **Security Warning — Credential Exposure**
> The login example above passes the password as a command-line argument, exposing it to shell history, process listings, and logs. In shared or automated environments, prefer `mega-login` without the password argument.
> **Recommendation:** Use `mega-login` interactively when possible. Always enable 2FA. Avoid hardcoding credentials in scripts.


### Interactive (MEGAcmd shell)

Commands without prefix inside the shell:

```bash
mega-cmd
MEGA CMD> login user@email.com
MEGA CMD> ls /
MEGA CMD> get file.pdf
```

> ⚠️ Agents in a bash terminal should **always** use the `mega-` prefix.

---

## Quick Examples

> ⚠️ **Security Warning — Credential Exposure**
> The `mega-login` example below passes your password as a command-line argument. This exposes credentials to shell history, process listings, audit logs, and agent telemetry.
> **Recommendation:** Use `mega-login` interactively (without the password) when possible. Always enable 2FA. Never commit credentials to scripts or version control.

```bash
# Login
mega-login me@email.com my-password

# Upload
mega-put ~/photo.jpg /Images/

# Download
mega-get /Documents/report.pdf ~/Downloads/

# Bidirectional sync
mega-sync ~/Documents /CloudDocs

# Public link
mega-export -a /Shared/Folder

# Daily backup
mega-backup ~/Photos /Backups --period="0 0 4 * * *" --num-backups=10

# WebDAV
mega-webdav /Videos --public --port=8080
> ⚠️ **Security Warning — Network Exposure**
> Starting WebDAV exposes your MEGA content over the network. Without TLS, traffic is unencrypted. The `--public` flag makes the service accessible beyond localhost.
> **Recommendation:** Use `--tls` with valid certificates. Avoid `--public` unless necessary. Stop services when not in use with `mega-webdav -d`.


# Check status
mega-whoami -l
mega-df -h
mega-sync
```

---

## License

This skill is distributed under the **MIT-0 (MIT No Attribution)** license.  

---

## Useful Links

- [MEGA.nz](https://mega.nz) — Official website
- [MEGAcmd Releases](https://mega.nz/cmd) — Downloads
- [MEGAcmd GitHub](https://github.com/meganz/MEGAcmd) — Source code
- [MEGA Help Center](https://mega.nz/help) — Help center
- [User Guide](https://github.com/meganz/MEGAcmd/blob/master/UserGuide.md) — User guide
