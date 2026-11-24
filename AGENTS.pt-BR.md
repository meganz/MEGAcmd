# MEGAcmd

CLI client for MEGA.nz cloud storage. Built with C++ / CMake. Uses vcpkg for dependencies.

## Quick Start (Build)

```bash
cmake -B build && cmake --build build
./build/src/mega-cmd-server
./build/src/mega-cmd
```

## Para Agentes de IA

### Skill Oficial (OpenCode) — Recomendado

`skills/megacmd/SKILL.md` contém instruções completas para usar todos os 76 comandos, sync, backup, WebDAV/FTP, FUSE, diagnóstico e códigos de erro.

### Fallback (qualquer ferramenta)

Se a skill não estiver acessível, use a referência inline:
- `mega-comando --help` para ajuda de cada comando (sempre disponível)
- `mega-help` para lista completa de comandos
- `mega-help --non-interactive` para guia de scripting
- `mega-errorcode NUM` para traduzir código de erro

## Key Commands

- `mega-login email password` — Login
- `mega-put /local/file /remote/` — Upload
- `mega-get /remote/file /local/` — Download
- `mega-sync ~/local /remote/` — Bidirectional sync
- `mega-ls /path` — List files
- `mega-export -a /path` — Create public link

## Estrutura do Projeto

- `src/` — C++ source files
- `contrib/docs/` — Feature guides + 76 command docs
- `tests/` — Unit/integration tests
- `build/` — CMake, installers, packaging
- `sdk/` — MEGA SDK submodule

## Project Config

- Version: 2.5.2
- Dependencies: vcpkg (pcre, cryptopp, curl, icu, libsodium, sqlite3)
- License: BSD 2-Clause
