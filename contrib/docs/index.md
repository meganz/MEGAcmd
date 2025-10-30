# MEGAcmd Documentation

Welcome to the complete command-line reference for MEGAcmd, the powerful terminal interface for MEGA cloud storage.

## Command Categories

### 1. File Management
Manage files and folders on your MEGA account:

- **Browse & Navigate**: `cd`, `ls`, `pwd`, `find`, `mount`
- **File Operations**: `get`, `put`, `cp`, `mv`, `rm`, `mkdir`
- **Content Management**: `cat`, `preview`, `thumbnail`, `tree`
- **Metadata & Attributes**: `attr`, `du`, `df`, `graphics`, `mediainfo`
- **Import/Export**: `export`, `import`


### 2. Synchronization and Backup
Manage sync and backup operations:

- **Synchronization**: `sync`, `sync-config`, `sync-ignore`, `sync-issues`
- **Backup Management**: `backup`, `reload`, `exclude`, `deleteversions`
- **Sharing & Permissions**: `share`, `permissions`, `invite`
- **Contacts & Transfers**: `users`, `ipc`, `showpcr`, `transfers`


### 3. System and Debugging
Low-level system commands and troubleshooting:

- **Server Management**: `ftp`, `webdav`
- **FUSE Mounting**: `fuse-add`, `fuse-remove`, `fuse-enable`, `fuse-disable`, `fuse-config`, `fuse-show`
- **Debugging**: `debug`, `exec`


### 4. Account and Session
Authentication and user management:

- **Login/Logout**: `login`, `logout`, `whoami`
- **Session Management**: `session`, `killsession`
- **Account Operations**: `signup`, `confirm`, `confirmcancel`, `passwd`
- **User Settings**: `userattr`, `masterkey`


### 5. Utilities
General-purpose tools and configurations:

- **Help & Info**: `help`, `version`, `errorcode`
- **Configuration**: `speedlimit`, `https`, `proxy`
- **Navigation**: `lcd`, `lpwd`
- **System Utilities**: `log`, `quit`


## Feature Guides

- [FTP Server Configuration](FTP.md)
- [FUSE Mounting Guide](FUSE.md)
- [WebDAV Integration](WEBDAV.md)
- [Backup Strategies](BACKUPS.md)
- [Debugging Techniques](DEBUG.md)

## Quick Start

```bash
# Install MEGAcmd
sudo apt install megacmd

# Start interactive session
mega-cmd

# Basic commands
mega-login your@email.com
mega-ls /
mega-put local_file.txt /remote_folder/
