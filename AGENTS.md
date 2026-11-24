# MEGAcmd

CLI client for MEGA.nz cloud storage. Built with C++ / CMake. Uses vcpkg for dependencies.

## Quick Start (Build)

```bash
cmake -B build && cmake --build build
./build/src/mega-cmd-server
./build/src/mega-cmd
```

## For AI Agents

### Official Skill (OpenCode) — Recommended

`skills/megacmd/SKILL.md` contains complete instructions for using all 76 commands, sync, backup, WebDAV/FTP, FUSE, diagnostics, and error codes.

### Fallback (any tool)

If the skill file is not accessible, use the inline reference:
- `mega-command --help` for help on each command (always available)
- `mega-help` for a complete command list
- `mega-help --non-interactive` for scripting guide
- `mega-errorcode NUM` to translate an error code

## Key Commands

- `mega-login email password` — Login
- `mega-put /local/file /remote/` — Upload
- `mega-get /remote/file /local/` — Download
- `mega-sync ~/local /remote/` — Bidirectional sync
- `mega-ls /path` — List files
- `mega-export -a /path` — Create public link

## Project Structure

- `src/` — C++ source files
- `contrib/docs/` — Feature guides + 76 command docs
- `tests/` — Unit/integration tests
- `build/` — CMake, installers, packaging
- `sdk/` — MEGA SDK submodule

## Project Config

- Version: 2.5.2
- Dependencies: vcpkg (pcre, cryptopp, curl, icu, libsodium, sqlite3)
- License: MIT-0
