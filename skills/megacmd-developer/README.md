# MEGAcmd — Developer Skill

AI agent skill for BUILDING, DEBUGGING, TESTING, and CONTRIBUTING to **MEGAcmd** — the official CLI client for [MEGA.nz](https://mega.nz) cloud storage.

> **Technical name:** `megacmd-developer`  
> **Category:** development  
> **Stack:** C++, CMake, vcpkg  
> **Compatibility:** OpenCode, Cline, Claude Code, Continue.dev, and SKILL.md-compatible tools  

---

## Objectives

This skill enables AI agents to:

- **Compile** MEGAcmd from source code using CMake + vcpkg
- **Configure** the development environment (build options, dependencies, sanitizers)
- **Run** unit tests, integration tests, and test scripts
- **Debug** issues in the server, client, or synchronization mechanisms
- **Analyze** logs, configure the rotating logger, and adjust verbosity levels
- **Package** for distribution (Debian, RPM, Arch Linux, Windows NSIS, macOS DMG, Synology NAS)
- **Understand** the internal architecture: IPC, listeners, command execution, FUSE, sync
- **Publish** changes following repository practices

---

## System Requirements

### Operating System

| OS | Build | Tests | Packaging |
|---|---|---|---|
| **Linux** | ✅ Full | ✅ Full | ✅ Debian, RPM, Arch, Synology |
| **macOS** | ✅ Full | ✅ Partial (no FUSE) | ✅ DMG |
| **Windows** | ✅ Full | ✅ Partial | ✅ NSIS installer |

### Build Dependencies

#### Essentials

| Tool | Minimum Version | Installation (Linux) |
|---|---|---|
| **Git** | 2.x | `apt install git` |
| **CMake** | 3.16 | `apt install cmake` |
| **C++ Compiler** | C++17 | `apt install g++` or `clang` |
| **vcpkg** | managed by build | Cloned automatically |

#### vcpkg Dependencies (automatic)

| Library | Required? | Purpose |
|---|---|---|
| **pcre** | ✅ Yes | PCRE regular expressions |
| **cryptopp** | ✅ Yes | Cryptography |
| **curl** (with zstd) | ✅ Yes | HTTP requests |
| **icu** | ✅ Yes | Unicode support |
| **libsodium** | ✅ Yes | Cryptography |
| **sqlite3** | ✅ Yes | Cache and local storage |

#### Optional Dependencies (features)

| Feature | Library | CMake Activation |
|---|---|---|
| OpenSSL | openssl | `USE_OPENSSL=ON` |
| MediaInfo | libmediainfo | `USE_MEDIAINFO=ON` |
| FreeImage | freeimage + jasper | `USE_FREEIMAGE=ON` |
| FFMPEG | ffmpeg (avcodec, avformat, swresample, swscale) | `USE_FFMPEG=ON` |
| libuv | libuv | `USE_LIBUV=ON` (WebDAV/FTP) |
| PDFium | pdfium | `USE_PDFIUM=ON` |
| Readline | readline | `USE_READLINE=ON` |
| Tests | gtest | `ENABLE_MEGACMD_TESTS=ON` |

### Hardware Requirements

| Requirement | Minimum | Recommended |
|---|---|---|
| **RAM** | 4 GB | 8 GB+ |
| **Disk** | 2 GB free | 5 GB+ (with build cache) |
| **CPU** | 2 cores | 4+ cores |

---

## Account Requirements

For **building, debugging, and testing**, **no MEGA account is required**. However:

| Activity | Account Needed? | Notes |
|---|---|---|
| Compile | ❌ No | Build is 100% offline |
| Unit tests | ❌ No | Network-independent |
| Integration tests | ✅ Yes (optional) | Some tests require login |
| Sync debugging | ✅ Yes | Requires real MEGA folders |
| Packaging | ❌ No | Generates artifacts locally |
| CI/CD | ✅ Yes (GitHub) | Repository access |

A test account on [MEGA.nz](https://mega.nz) is recommended for integration testing.

---

## How to Use This Skill

### Activation

The `megacmd-developer` skill is **automatically** activated when the context involves MEGAcmd development. To force activation, mention "compile MEGAcmd", "build MEGAcmd", "debug MEGAcmd", "MEGAcmd tests", or "contribute to MEGAcmd".

### File Structure

```
skills/megacmd-developer/
├── SKILL.md           # ⬅️ Main instructions (English)
├── SKILL.pt-BR.md     # ⬅️ Main instructions (Portuguese)
├── README.md          # ⬅️ This file (skill documentation, English)
└── README.pt-BR.md    # ⬅️ Skill documentation (Portuguese)
```

### Related Skill

The **`megacmd`** skill (in `skills/megacmd/`) covers **using** MEGAcmd (upload, sync, backup). The two skills are complementary:

- Use `megacmd` when the user needs to **use** MEGAcmd
- Use `megacmd-developer` when the user needs to **develop/build** MEGAcmd

---

## Setting Up the Development Environment

### Linux (Ubuntu/Debian)

```bash
# Install system dependencies
sudo apt update
sudo apt install -y git cmake g++ pkg-config curl zip unzip tar

# Clone the repository
git clone https://github.com/meganz/MEGAcmd.git
cd MEGAcmd

# Initialize submodules (MEGA SDK)
git submodule update --init --recursive

# Configure with CMake (vcpkg is downloaded automatically)
cmake -B build/build-cmake-Debug -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build/build-cmake-Debug -j$(nproc)
```

### macOS

```bash
# Install dependencies
brew install git cmake pkg-config

# Clone and build (same procedure as Linux)
git clone https://github.com/meganz/MEGAcmd.git
cd MEGAcmd && git submodule update --init --recursive
cmake -B build/build-cmake-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/build-cmake-Debug -j$(sysctl -n hw.ncpu)
```

### Windows

```powershell
# Install Git, CMake, Visual Studio 2022 with "Desktop development with C++"
# Open "Developer Command Prompt for VS 2022"

git clone https://github.com/meganz/MEGAcmd.git
cd MEGAcmd
git submodule update --init --recursive

# Configure
cmake -B build\build-cmake-Debug -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build\build-cmake-Debug --config Debug
```

### Speed Up Builds

```bash
# Use ccache for faster recompilation
sudo apt install ccache

cmake -B build/build-cmake-Debug \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache

cmake --build build/build-cmake-Debug -j$(nproc)
```

### Use Existing vcpkg

```bash
# If you already have vcpkg elsewhere:
cmake -B build/build-cmake-Debug \
  -DCMAKE_BUILD_TYPE=Debug \
  -DVCPKG_ROOT=/path/to/vcpkg
```

---

## Verify Environment

```bash
# Git
git --version

# CMake
cmake --version

# C++ Compiler
g++ --version || clang++ --version

# vcpkg (if already installed)
/path/to/vcpkg/vcpkg version

# After successful build
ls build/build-cmake-Debug/src/mega-cmd-server
ls build/build-cmake-Debug/src/mega-cmd
ls build/build-cmake-Debug/src/mega-exec
```

---

## Source Code Directory Structure

```
src/
├── megacmd_server_main.cpp   # Server entry point
├── megacmd.cpp / megacmd.h   # MEGAcmd core
├── megacmdexecuter.cpp/.h    # Command executor (main logic)
├── megacmdutils.cpp/.h       # Parsing and formatting utilities
├── megacmdcommonutils.cpp/.h # Common utilities (path, string)
├── megacmdlogger.cpp/.h      # Logging system
├── megacmd_fuse.cpp/.h       # FUSE mount support
├── megacmdshell/             # Interactive shell
├── client/                   # mega-exec + mega-* wrapper scripts
├── sync_command.cpp/.h       # Sync logic
├── sync_ignore.cpp/.h        # Exclusion patterns
├── sync_issues.cpp/.h        # Conflict management
├── comunicationsmanager.*    # IPC (File Sockets / Named Pipes)
├── configurationmanager.*    # Configuration persistence
├── listeners.cpp/.h          # MEGA SDK listeners
└── updater/                  # Auto-update system
```

---

## License

This skill is distributed under the **MIT-0 (MIT No Attribution)** license.  

---

## Useful Links

- [MEGAcmd GitHub](https://github.com/meganz/MEGAcmd) — Official repository
- [MEGA SDK](https://github.com/meganz/sdk) — MEGA SDK (submodule)
- [vcpkg](https://github.com/microsoft/vcpkg) — Dependency manager
- [CMake](https://cmake.org) — Build system
- [MEGA.nz](https://mega.nz) — Official website
- [MEGAcmd Releases](https://mega.nz/cmd) — Pre-built package downloads
