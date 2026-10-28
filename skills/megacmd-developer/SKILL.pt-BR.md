---
name: megacmd-developer
description: >-
  Build, debug, e contribuição ao projeto MEGAcmd (C++/CMake/vcpkg).
  Use quando o usuário precisar compilar, configurar ambiente de desenvolvimento,
  rodar testes, criar pacotes, ou entender a arquitetura interna do MEGAcmd.
  NÃO use para operações de usuário (upload, sync, backup) — esse é outro skill.
license: MIT-0
metadata:
  version: "1.0.0"
  category: development
  stack: [cpp, cmake, vcpkg]
  platform: [linux, macos, windows]
---

# MEGAcmd — Guia para Desenvolvedores

## O que este skill faz

Instruções para BUILDAR, DEBUGAR, TESTAR e CONTRIBUIR com o repositório MEGAcmd. Este skill é para **desenvolvedores** que precisam compilar o projeto, configurar ambiente, ou investigar o código fonte C++.

> ⚠️ **Não ative este skill** se o usuário quer apenas USAR o MEGAcmd (upload, sync, backup). Para isso existe o skill `megacmd`.

## Quando usar

- O usuário quer compilar/instalar o MEGAcmd a partir do código fonte
- O usuário quer configurar ambiente de desenvolvimento (CMake, vcpkg)
- O usuário quer rodar testes (unitários ou integração)
- O usuário quer debuggar problemas no servidor/cliente
- O usuário quer criar pacotes (Debian, RPM, Arch, Synology, Windows)
- O usuário quer contribuir com código ou PRs
- O usuário pergunta sobre a arquitetura interna do código

## Quando NÃO usar

- O usuário quer fazer upload, download, sync, backup, compartilhar — use `megacmd`
- O usuário quer usar a interface web do MEGA
- O usuário quer instalar o MEGAcmd via pacotes prontos (mega.nz/cmd)

---

## Estrutura do Projeto

```
MEGAcmd/
├── CMakeLists.txt              # Build system principal (488 linhas)
├── vcpkg.json                  # Dependências gerenciadas via vcpkg
├── src/
│   ├── megacmd_server_main.cpp # Entry point do servidor
│   ├── megacmd.cpp/h           # Core do MEGAcmd
│   ├── megacmdexecuter.cpp/h   # Executor de comandos
│   ├── megacmdutils.cpp/h      # Utilitários
│   ├── megacmdcommonutils.cpp/h # Utilitários comuns
│   ├── megacmdlogger.cpp/h     # Sistema de logging
│   ├── megacmd_fuse.cpp/h      # Suporte a FUSE
│   ├── megacmdshell/           # Shell interativo
│   ├── client/                 # Cliente (mega-exec + mega-* wrappers)
│   │   ├── megacmd_client_main.cpp
│   │   ├── megacmdclient.cpp/h
│   │   ├── mega-*              # Wrappers bash (Linux/macOS)
│   │   └── win/mega-*.bat      # Wrappers Windows
│   ├── comunicationsmanager.cpp/h  # IPC (File Sockets / Named Pipes)
│   ├── configurationmanager.cpp/h  # Configuração
│   ├── listeners.cpp/h         # Listeners do SDK
│   ├── sync_command.cpp/h      # Comando de sync
│   ├── sync_ignore.cpp/h       # Padrões de ignorar
│   └── sync_issues.cpp/h       # Problemas de sync
├── tests/
│   ├── unit/                   # Testes unitários (C++)
│   ├── integration/            # Testes de integração (C++)
│   └── *.sh, *.py              # Testes shell/Python
├── build/
│   ├── cmake/modules/          # Módulos CMake
│   ├── installer/              # Instaladores (NSIS, DMG, scripts)
│   ├── megacmd/                # Pacotes Debian
│   └── templates/megacmd/      # RPM spec, PKGBUILD, DSC
├── contrib/
│   ├── docs/                   # Documentação (76 comandos + guias)
│   ├── sanitizer/              # ASan, LSan, TSan suppressions
│   └── updater/                # Lista de arquivos para atualizador
├── sdk/                        # MEGA SDK (submódulo git)
└── jenkinsfile/                # CI/CD (Jenkins)
```

---

## Build

### Pré-requisitos

```bash
# Git + submódulos
git clone https://github.com/meganz/MEGAcmd.git
cd MEGAcmd && git submodule update --init --recursive
```

### Compilar

```bash
# Debug
cmake -B build/build-cmake-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/build-cmake-Debug -j$(nproc)

# Release
cmake -B build/build-cmake-Release -DCMAKE_BUILD_TYPE=Release
cmake --build build/build-cmake-Release -j$(nproc)

# Com testes
cmake -B build/build-cmake-Debug -DCMAKE_BUILD_TYPE=Debug -DENABLE_MEGACMD_TESTS=ON
cmake --build build/build-cmake-Debug -j$(nproc)
```

### Instalar (Linux/macOS)

```bash
sudo cmake --install build/build-cmake-Release
```

### Opções de build importantes

| Flag | Descrição |
|---|---|
| `-DVCPKG_ROOT=/path` | Caminho para vcpkg (default: ../vcpkg) |
| `-DCMAKE_CXX_COMPILER_LAUNCHER=ccache` | Usar ccache |
| `-DENABLE_MEGACMD_TESTS=ON` | Compilar testes |
| `-DCMAKE_INSTALL_PREFIX=/path` | Diretório de instalação |

### Targets CMake

| Target | Tipo | Descrição |
|---|---|---|
| `mega-cmd-server` | Executável | Servidor |
| `mega-cmd` | Executável | Shell interativo |
| `mega-exec` | Executável | Cliente não-interativo |
| `mega-cmd-updater` | Executável | Atualizador |
| `mega-cmd-tests-unit` | Executável | Testes unitários |
| `mega-cmd-tests-integration` | Executável | Testes integração |
| `LMegaCmdCommonUtils` | Biblioteca estática | Utilitários comuns |
| `LMegacmdServer` | Biblioteca estática | Lógica do servidor |
| `LMegacmdClient` | Biblioteca estática | Lógica do cliente |

---

## Dependências (vcpkg)

### Obrigatórias
pcre, cryptopp, curl (com zstd), icu, libsodium, sqlite3

### Opcionais (features do vcpkg.json)
| Feature | Dependência | Ativa |
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

## Testes

```bash
# Unitários
./build/build-cmake-Debug/tests/mega-cmd-tests-unit

# Integração (requer servidor rodando)
./build/build-cmake-Debug/tests/mega-cmd-tests-integration

# Testes Python
python3 tests/megacmd_put_test.py
python3 tests/megacmd_get_test.py
python3 tests/megacmd_find_test.py
```

---

## Docker

```bash
# Build padrão
docker build -f build-with-docker/Dockerfile.cmake .

# Cross-compile Synology
docker build -f build/SynologyNAS/dockerfile/synology-cross-build.dockerfile .
```

---

## Debug

### Logging

```bash
# Iniciar servidor com debug
MEGAcmdServer --debug          # MEGAcmd=DEBUG, SDK=DEFAULT
MEGAcmdServer --debug-full     # MEGAcmd=DEBUG, SDK=DEBUG
MEGAcmdServer --verbose-full   # MEGAcmd=VERBOSE, SDK=VERBOSE

# Ou via env var
MEGACMD_LOGLEVEL=FULLVERBOSE MEGAcmdServer
MEGACMD_JSON_LOGS=1 MEGAcmdServer  # JSON das requisições HTTP
```

### Arquivos de log

- Linux/macOS: `$HOME/.megaCmd/megacmdserver.log`
- Windows: `%LOCALAPPDATA%\MEGAcmd\.megaCmd\megacmdserver.log`

### Logger Rotativo

Configurar via `megacmd.cfg` no diretório do log:

```
RotatingLogger:RotationType=Timestamp    # Timestamp | Numbered
RotatingLogger:CompressionType=Gzip      # Gzip | None
RotatingLogger:MaxFileMB=50
RotatingLogger:MaxFilesToKeep=20
RotatingLogger:MaxFileAgeSeconds=2592000 # 30 dias
RotatingLogger:MaxMessageBusMB=512
```

### Sanitizers

Arquivos em `contrib/sanitizer/`:
- `asan.suppressions` — AddressSanitizer
- `lsan.suppressions` — LeakSanitizer
- `tsan.suppressions` — ThreadSanitizer

---

## CI/CD

### Jenkins
Jenkinsfiles em `jenkinsfile/`:
- `Jenkinsfile_MR_linux`, `Jenkinsfile_MR_macos`, `Jenkinsfile_MR_windows`
- `Jenkinsfile_MR_linux_packages`
- `Branch_status/` — Pipelines para builds de release

### GitHub Issues
Template em `.github/ISSUE_TEMPLATE/bug_report.yml`

---

## Packaging

### Debian/APT
Arquivos em `build/megacmd/`: control, rules, postinst, prerm, changelog

### RPM (Fedora/SUSE)
`build/templates/megacmd/megacmd.spec`

### Arch Linux
`build/templates/megacmd/PKGBUILD`

### Windows (NSIS)
- `build/installer_win.nsi` — Script do instalador
- `build/installer/` — Ícones, banners, recursos

### macOS (DMG)
- `build/installer_mac.sh` — Script de instalação
- `build/installer/Info.plist.in` — Template Info.plist

### Synology NAS
- `build/SynologyNAS/generate_pkg.sh` — Gerar pacote SPK
- `build/SynologyNAS/toolkit/source/MEGAcmd/` — Scripts de instalação
- Docker cross-compile disponível

### Gerar changelog
```bash
./build/generate_deb_changelog_entry.sh
./build/generate_rpm_changelog_entry.sh
```

---

## Atualizador (Updater)

Arquivos em `src/updater/` e `contrib/updater/`:
- `MegaUpdater.cpp` — Lógica de atualização
- `fileswin.txt`, `fileswin64.txt`, `filesmacos.txt` — Lista de arquivos por plataforma

---

## IPC (Comunicação Cliente-Servidor)

### File Sockets (Unix/Linux/macOS)
Usado entre `mega-exec` e `mega-cmd-server`. Implementação em `comunicationsmanagerfilesockets.cpp`.

### Named Pipes (Windows)
Usado no Windows. Implementação em `comunicationsmanagernamedpipes.cpp` e `megacmdshellcommunicationsnamedpipes.cpp`.

### TCP Socket (alternativa Python)
`src/client/python/mega-execports` — conecta na porta 12300:

1. Conecta em `127.0.0.1:12300`
2. Envia comando como string
3. Recebe 2 bytes (socketOutId)
4. Conecta em `127.0.0.1:12300 + socketOutId`
5. Recebe 4 bytes (outCode) + output
6. Exit code = -outCode (se negativo) ou outCode

---

## Configurações Avançadas

### CMake Options

Opções em `build/cmake/modules/megacmd_options.cmake`:
- `USE_PCRE` — PCRE para expressões regulares (default: ON)
- `USE_MEDIAINFO` — MediaInfo (default: ON)
- `USE_FREEIMAGE` — FreeImage (default: ON)
- `USE_FFMPEG` — FFMPEG (default: ON)
- `USE_LIBUV` — libuv para WebDAV/FTP (default: ON)
- `USE_PDFIUM` — PDFium (default: ON)
- `USE_READLINE` — Readline (default: ON)
- `ENABLE_MEGACMD_TESTS` — Compilar testes (default: OFF)

### Version

Arquivo `build/version` contém a versão atual: **2.5.2**

---

## Dicas para Contribuidores

1. O código usa C++17
2. Namespace principal: `megacmd`
3. SDK MEGA via submódulo em `sdk/`
4. Estilo: 4 spaces, BSD 2-Clause header em todos os arquivos
5. Sempre rode testes unitários antes de submeter PR
6. Para adicionar novo comando: editar `megacmdexecuter.cpp` (método `executecommand`)
7. Para adicionar nova flag: atualizar `getUsageStr()` em `megacmd.cpp` e `HelpFlags`
8. Sanitizers ativados por default em build Debug
