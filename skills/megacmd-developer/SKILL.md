---
name: megacmd-developer
description: >-
  Build, debug, and contribute to the MEGAcmd project (C++/CMake/vcpkg).
  Use when the user needs to compile, set up a development environment,
  run tests, create packages, or understand MEGAcmd's internal architecture.
  Do NOT use for user operations (upload, sync, backup) — that is a different skill.
license: MIT-0
metadata:
  version: "1.0.0"
  category: development
  stack: [cpp, cmake, vcpkg]
  platform: [linux, macos, windows]
---

# MEGAcmd — Developer Guide

## What this skill does

Instructions for BUILDING, DEBUGGING, TESTING, and CONTRIBUTING to the MEGAcmd repository. This skill is for **developers** who need to compile the project, set up the environment, or investigate the C++ source code.

> ⚠️ **Do not activate this skill** if the user just wants to USE MEGAcmd (upload, sync, backup). That is what the `megacmd` skill is for.

## When to use

- The user wants to compile/install MEGAcmd from source code
- The user wants to set up a development environment (CMake, vcpkg)
- The user wants to run tests (unit or integration)
- The user wants to debug server/client issues
- The user wants to create packages (Debian, RPM, Arch, Synology, Windows)
- The user wants to contribute code or pull requests
- The user asks about the internal code architecture

## When NOT to use

- The user wants to upload, download, sync, backup, or share — use `megacmd`
- The user wants to use the MEGA web interface
- The user wants to install MEGAcmd via ready-made packages (mega.nz/cmd)

---

## Project Structure

```
MEGAcmd/
├── CMakeLists.txt              # Main build system (488 lines)
├── vcpkg.json                  # vcpkg-managed dependencies
├── src/
│   ├── megacmd_server_main.cpp # Server entry point
│   ├── megacmd.cpp/h           # MEGAcmd core
│   ├── megacmdexecuter.cpp/h   # Command executor
│   ├── megacmdutils.cpp/h      # Utilities
│   ├── megacmdcommonutils.cpp/h # Common utilities
│   ├── megacmdlogger.cpp/h     # Logging system
│   ├── megacmd_fuse.cpp/h      # FUSE support
│   ├── megacmdshell/           # Interactive shell
│   ├── client/                 # Client (mega-exec + mega-* wrappers)
│   │   ├── megacmd_client_main.cpp
│   │   ├── megacmdclient.cpp/h
│   │   ├── mega-*              # Bash wrappers (Linux/macOS)
│   │   └── win/mega-*.bat      # Windows wrappers
│   ├── comunicationsmanager.cpp/h  # IPC (File Sockets / Named Pipes)
│   ├── configurationmanager.cpp/h  # Configuration
│   ├── listeners.cpp/h         # SDK listeners
│   ├── sync_command.cpp/h      # Sync command
│   ├── sync_ignore.cpp/h       # Ignore patterns
│   └── sync_issues.cpp/h       # Sync issues
├── tests/
│   ├── unit/                   # Unit tests (C++)
│   ├── integration/            # Integration tests (C++)
│   └── *.sh, *.py              # Shell/Python tests
├── build/
│   ├── cmake/modules/          # CMake modules
│   ├── installer/              # Installers (NSIS, DMG, scripts)
│   ├── megacmd/                # Debian packages
│   └── templates/megacmd/      # RPM spec, PKGBUILD, DSC
├── contrib/
│   ├── docs/                   # Documentation (76 commands + guides)
│   ├── sanitizer/              # ASan, LSan, TSan suppressions
│   └── updater/                # Updater file lists
├── sdk/                        # MEGA SDK (git submodule)
└── jenkinsfile/                # CI/CD (Jenkins)
```

---

## Build

### Prerequisites

```bash
# Git + submodules
git clone https://github.com/meganz/MEGAcmd.git
cd MEGAcmd && git submodule update --init --recursive
```

### Compile

```bash
# Debug
cmake -B build/build-cmake-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/build-cmake-Debug -j$(nproc)

# Release
cmake -B build/build-cmake-Release -DCMAKE_BUILD_TYPE=Release
cmake --build build/build-cmake-Release -j$(nproc)

# With tests
cmake -B build/build-cmake-Debug -DCMAKE_BUILD_TYPE=Debug -DENABLE_MEGACMD_TESTS=ON
cmake --build build/build-cmake-Debug -j$(nproc)
```

### Install (Linux/macOS)

```bash
sudo cmake --install build/build-cmake-Release
```

### Important Build Options

| Flag | Description |
|---|---|
| `-DVCPKG_ROOT=/path` | Path to vcpkg (default: ../vcpkg) |
| `-DCMAKE_CXX_COMPILER_LAUNCHER=ccache` | Use ccache |
| `-DENABLE_MEGACMD_TESTS=ON` | Build tests |
| `-DCMAKE_INSTALL_PREFIX=/path` | Installation directory |

### CMake Targets

| Target | Type | Description |
|---|---|---|
| `mega-cmd-server` | Executable | Server |
| `mega-cmd` | Executable | Interactive shell |
| `mega-exec` | Executable | Non-interactive client |
| `mega-cmd-updater` | Executable | Updater |
| `mega-cmd-tests-unit` | Executable | Unit tests |
| `mega-cmd-tests-integration` | Executable | Integration tests |
| `LMegaCmdCommonUtils` | Static library | Common utilities |
| `LMegacmdServer` | Static library | Server logic |
| `LMegacmdClient` | Static library | Client logic |

---

## Dependencies (vcpkg)

### Required
pcre, cryptopp, curl (with zstd), icu, libsodium, sqlite3

### Optional (vcpkg.json features)
| Feature | Dependency | Activation |
|---|---|---|
| `use-openssl` | openssl | CMake |
| `use-mediainfo` | libmediainfo | CMake |
| `use-freeimage` | freeimage + jasper | CMake |
| `use-ffmpeg` | ffmpeg (avcodec, avformat, swresample, swscale) | CMake |
| `use-libuv` | libuv | CMake (WebDAV/FTP) |
| `use-pdfium` | pdfium | CMake |
| `use-readline` | readline | CMake |
| `megacmd-enable-tests` | gtest | `-DENABLE_MEGACMD_TESTS=ON` |

---

## Tests

```bash
# Unit tests
./build/build-cmake-Debug/tests/mega-cmd-tests-unit

# Integration tests (requires server running)
./build/build-cmake-Debug/tests/mega-cmd-tests-integration

# Python tests
python3 tests/megacmd_put_test.py
python3 tests/megacmd_get_test.py
python3 tests/megacmd_find_test.py
```

---

## Docker

```bash
# Standard build
docker build -f build-with-docker/Dockerfile.cmake .

# Synology cross-compile
docker build -f build/SynologyNAS/dockerfile/synology-cross-build.dockerfile .
```

---

## Debug

### Logging

```bash
# Start server with debug
MEGAcmdServer --debug          # MEGAcmd=DEBUG, SDK=DEFAULT
MEGAcmdServer --debug-full     # MEGAcmd=DEBUG, SDK=DEBUG
MEGAcmdServer --verbose-full   # MEGAcmd=VERBOSE, SDK=VERBOSE

# Or via env var
MEGACMD_LOGLEVEL=FULLVERBOSE MEGAcmdServer
MEGACMD_JSON_LOGS=1 MEGAcmdServer  # JSON HTTP request logging
```

### Log Files

- Linux/macOS: `$HOME/.megaCmd/megacmdserver.log`
- Windows: `%LOCALAPPDATA%\MEGAcmd\.megaCmd\megacmdserver.log`

### Rotating Logger

Configure via `megacmd.cfg` in the log directory:

```
RotatingLogger:RotationType=Timestamp    # Timestamp | Numbered
RotatingLogger:CompressionType=Gzip      # Gzip | None
RotatingLogger:MaxFileMB=50
RotatingLogger:MaxFilesToKeep=20
RotatingLogger:MaxFileAgeSeconds=2592000 # 30 days
RotatingLogger:MaxMessageBusMB=512
```

### Sanitizers

Files in `contrib/sanitizer/`:
- `asan.suppressions` — AddressSanitizer
- `lsan.suppressions` — LeakSanitizer
- `tsan.suppressions` — ThreadSanitizer

---

## CI/CD

### Jenkins
Jenkinsfiles in `jenkinsfile/`:
- `Jenkinsfile_MR_linux`, `Jenkinsfile_MR_macos`, `Jenkinsfile_MR_windows`
- `Jenkinsfile_MR_linux_packages`
- `Branch_status/` — Release build pipelines

### GitHub Issues
Template at `.github/ISSUE_TEMPLATE/bug_report.yml`

---

## Packaging

### Debian/APT
Files in `build/megacmd/`: control, rules, postinst, prerm, changelog

### RPM (Fedora/SUSE)
`build/templates/megacmd/megacmd.spec`

### Arch Linux
`build/templates/megacmd/PKGBUILD`

### Windows (NSIS)
- `build/installer_win.nsi` — Installer script
- `build/installer/` — Icons, banners, resources

### macOS (DMG)
- `build/installer_mac.sh` — Installation script
- `build/installer/Info.plist.in` — Info.plist template

### Synology NAS
- `build/SynologyNAS/generate_pkg.sh` — Generate SPK package
- `build/SynologyNAS/toolkit/source/MEGAcmd/` — Installation scripts
- Docker cross-compile available

### Generate Changelog
```bash
./build/generate_deb_changelog_entry.sh
./build/generate_rpm_changelog_entry.sh
```

---

## Updater

Files in `src/updater/` and `contrib/updater/`:
- `MegaUpdater.cpp` — Update logic
- `fileswin.txt`, `fileswin64.txt`, `filesmacos.txt` — File lists per platform

---

## IPC (Client-Server Communication)

### File Sockets (Unix/Linux/macOS)
Used between `mega-exec` and `mega-cmd-server`. Implementation in `comunicationsmanagerfilesockets.cpp`.

### Named Pipes (Windows)
Used on Windows. Implementation in `comunicationsmanagernamedpipes.cpp` and `megacmdshellcommunicationsnamedpipes.cpp`.

### TCP Socket (Python alternative)
`src/client/python/mega-execports` — connects on port 12300:

1. Connect to `127.0.0.1:12300`
2. Send command as string
3. Receive 2 bytes (socketOutId)
4. Connect to `127.0.0.1:12300 + socketOutId`
5. Receive 4 bytes (outCode) + output
6. Exit code = -outCode (if negative) or outCode

---

## Advanced Settings

### CMake Options

Options in `build/cmake/modules/megacmd_options.cmake`:
- `USE_PCRE` — PCRE for regular expressions (default: ON)
- `USE_MEDIAINFO` — MediaInfo (default: ON)
- `USE_FREEIMAGE` — FreeImage (default: ON)
- `USE_FFMPEG` — FFMPEG (default: ON)
- `USE_LIBUV` — libuv for WebDAV/FTP (default: ON)
- `USE_PDFIUM` — PDFium (default: ON)
- `USE_READLINE` — Readline (default: ON)
- `ENABLE_MEGACMD_TESTS` — Build tests (default: OFF)

### Version

File `build/version` contains the current version: **2.5.2**

---

## Tips for Contributors

1. The codebase uses C++17
2. Main namespace: `megacmd`
3. MEGA SDK via submodule in `sdk/`
4. Style: 4 spaces, BSD 2-Clause header on all files
5. Always run unit tests before submitting a PR
6. To add a new command: edit `megacmdexecuter.cpp` (`executecommand` method)
7. To add a new flag: update `getUsageStr()` in `megacmd.cpp` and `HelpFlags`
8. Sanitizers are enabled by default in Debug builds
