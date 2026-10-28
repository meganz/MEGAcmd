# MEGAcmd — Developer Skill

Skill para agentes de IA BUILDAREM, DEBUGAREM, TESTAREM e CONTRIBUIREM com o **MEGAcmd** — o CLI client oficial do [MEGA.nz](https://mega.nz).

> **Nome técnico:** `megacmd-developer`  
> **Categoria:** development  
> **Stack:** C++, CMake, vcpkg  
> **Compatibilidade:** OpenCode, Cline, Claude Code, Continue.dev, e ferramentas compatíveis com SKILL.md  

---

## Objetivos

Esta skill capacita agentes de IA a:

- **Compilar** o MEGAcmd a partir do código fonte com CMake + vcpkg
- **Configurar** ambiente de desenvolvimento (opções de build, dependências, sanitizers)
- **Executar** testes unitários, de integração e scripts de teste
- **Depurar** problemas no servidor, cliente ou mecanismos de sincronização
- **Analisar** logs, configurar logger rotativo e níveis de verbosidade
- **Empacotar** para distribuição (Debian, RPM, Arch Linux, Windows NSIS, macOS DMG, Synology NAS)
- **Compreender** a arquitetura interna: IPC, listeners, execução de comandos, FUSE, sync
- **Publicar** alterações seguindo as práticas do repositório

---

## Requisitos de Sistema

### Sistema Operacional

| SO | Build | Testes | Empacotamento |
|---|---|---|---|
| **Linux** | ✅ Completo | ✅ Completo | ✅ Debian, RPM, Arch, Synology |
| **macOS** | ✅ Completo | ✅ Parcial (sem FUSE) | ✅ DMG |
| **Windows** | ✅ Completo | ✅ Parcial | ✅ NSIS installer |

### Dependências de Build

#### Essenciais

| Ferramenta | Versão Mínima | Instalação (Linux) |
|---|---|---|
| **Git** | 2.x | `apt install git` |
| **CMake** | 3.16 | `apt install cmake` |
| **Compilador C++** | C++17 | `apt install g++` ou `clang` |
| **vcpkg** | gerenciado pelo build | Clonado automaticamente |

#### Dependências via vcpkg (gerenciadas automaticamente)

| Biblioteca | Obrigatória? | Finalidade |
|---|---|---|
| **pcre** | ✅ Sim | Expressões regulares PCRE |
| **cryptopp** | ✅ Sim | Criptografia |
| **curl** (com zstd) | ✅ Sim | Requisições HTTP |
| **icu** | ✅ Sim | Suporte a Unicode |
| **libsodium** | ✅ Sim | Criptografia |
| **sqlite3** | ✅ Sim | Cache e armazenamento local |

#### Dependências Opcionais (features)

| Feature | Biblioteca | Ativa via CMake |
|---|---|---|
| OpenSSL | openssl | `USE_OPENSSL=ON` |
| MediaInfo | libmediainfo | `USE_MEDIAINFO=ON` |
| FreeImage | freeimage + jasper | `USE_FREEIMAGE=ON` |
| FFMPEG | ffmpeg (avcodec, avformat, swresample, swscale) | `USE_FFMPEG=ON` |
| libuv | libuv | `USE_LIBUV=ON` (WebDAV/FTP) |
| PDFium | pdfium | `USE_PDFIUM=ON` |
| Readline | readline | `USE_READLINE=ON` |
| Testes | gtest | `ENABLE_MEGACMD_TESTS=ON` |

### Recursos de Hardware

| Requisito | Mínimo | Recomendado |
|---|---|---|
| **RAM** | 4 GB | 8 GB+ |
| **Disco** | 2 GB livres | 5 GB+ (com cache de build) |
| **CPU** | 2 cores | 4+ cores |

---

## Requisitos de Conta

Para **build, debug e testes**, **nenhuma conta MEGA é necessária**. No entanto:

| Atividade | Conta Necessária? | Observações |
|---|---|---|
| Compilar | ❌ Não | Build é 100% offline |
| Testes unitários | ❌ Não | Independentes de rede |
| Testes de integração | ✅ Sim (opcional) | Alguns testes exigem login |
| Debug de sync | ✅ Sim | Requer pastas MEGA reais |
| Empacotamento | ❌ Não | Gera artefatos localmente |
| CI/CD | ✅ Sim (GitHub) | Acesso ao repositório |

Para testes de integração, uma conta de teste no [MEGA.nz](https://mega.nz) é recomendada.

---

## Como Usar a Skill

### Ativação

A skill `megacmd-developer` é ativada **automaticamente** quando o contexto envolver desenvolvimento do MEGAcmd. Para forçar a ativação, mencione "compilar MEGAcmd", "buildar MEGAcmd", "debuggar MEGAcmd", "testes do MEGAcmd" ou "contribuir com MEGAcmd".

### Estrutura de Arquivos

```
skills/megacmd-developer/
├── SKILL.md       # ⬅️ Instruções principais de build, debug, testes, packaging
└── README.md      # ⬅️ Este arquivo (documentação da skill)
```

### Skill Relacionada

A skill **`megacmd`** (em `skills/megacmd/`) cobre o **uso** do MEGAcmd (upload, sync, backup). As duas skills são complementares:

- Use `megacmd` quando o usuário precisar **usar** o MEGAcmd
- Use `megacmd-developer` quando o usuário precisar **desenvolver/buildar** o MEGAcmd

---

## Instalação do Ambiente de Desenvolvimento

### Linux (Ubuntu/Debian)

```bash
# Instalar dependências de sistema
sudo apt update
sudo apt install -y git cmake g++ pkg-config curl zip unzip tar

# Clonar o repositório
git clone https://github.com/meganz/MEGAcmd.git
cd MEGAcmd

# Inicializar submódulos (SDK MEGA)
git submodule update --init --recursive

# Configurar com CMake (vcpkg é baixado automaticamente)
cmake -B build/build-cmake-Debug -DCMAKE_BUILD_TYPE=Debug

# Compilar
cmake --build build/build-cmake-Debug -j$(nproc)

# (Opcional) Compilar com testes
cmake -B build/build-cmake-Test \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_MEGACMD_TESTS=ON
cmake --build build/build-cmake-Test -j$(nproc)
```

### macOS

```bash
# Instalar dependências
brew install git cmake pkg-config

# Clonar e compilar (mesmo procedimento do Linux)
git clone https://github.com/meganz/MEGAcmd.git
cd MEGAcmd && git submodule update --init --recursive
cmake -B build/build-cmake-Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/build-cmake-Debug -j$(sysctl -n hw.ncpu)
```

### Windows

```powershell
# Instalar Git, CMake, Visual Studio 2022 com "Desktop development with C++"
# Abrir "Developer Command Prompt for VS 2022"

git clone https://github.com/meganz/MEGAcmd.git
cd MEGAcmd
git submodule update --init --recursive

# Configurar
cmake -B build\build-cmake-Debug -DCMAKE_BUILD_TYPE=Debug

# Compilar
cmake --build build\build-cmake-Debug --config Debug
```

### Acelerar Builds

```bash
# Usar ccache para acelerar recompilações
sudo apt install ccache

cmake -B build/build-cmake-Debug \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache

cmake --build build/build-cmake-Debug -j$(nproc)
```

### Usar vcpkg Existente

```bash
# Se você já tem vcpkg em outro local:
cmake -B build/build-cmake-Debug \
  -DCMAKE_BUILD_TYPE=Debug \
  -DVCPKG_ROOT=/caminho/para/vcpkg
```

---

## Verificar Ambiente

```bash
# Git
git --version

# CMake
cmake --version

# Compilador C++
g++ --version || clang++ --version

# vcpkg (se já instalado)
/path/to/vcpkg/vcpkg version

# Após build bem-sucedido
ls build/build-cmake-Debug/src/mega-cmd-server
ls build/build-cmake-Debug/src/mega-cmd
ls build/build-cmake-Debug/src/mega-exec
```

---

## Estrutura de Diretórios do Código Fonte

```
src/
├── megacmd_server_main.cpp   # Entry point do servidor
├── megacmd.cpp / megacmd.h   # Core do MEGAcmd
├── megacmdexecuter.cpp/.h    # Executor de comandos (lógica principal)
├── megacmdutils.cpp/.h       # Utilitários de parsing e formatação
├── megacmdcommonutils.cpp/.h # Utilitários comuns (path, string)
├── megacmdlogger.cpp/.h      # Sistema de logging
├── megacmd_fuse.cpp/.h       # Suporte a montagem FUSE
├── megacmdshell/             # Shell interativo
├── client/                   # Cliente mega-exec + mega-* wrappers
├── sync_command.cpp/.h       # Lógica de sincronização
├── sync_ignore.cpp/.h        # Padrões de exclusão
├── sync_issues.cpp/.h        # Gerenciamento de conflitos
├── comunicationsmanager.*    # IPC (File Sockets / Named Pipes)
├── configurationmanager.*    # Persistência de configuração
├── listeners.cpp/.h          # Listeners do SDK MEGA
└── updater/                  # Sistema de atualização automática
```

---

## Licença

Esta skill é distribuída sob licença **MIT-0 (MIT No Attribution)**.  

---

## Links Úteis

- [MEGAcmd GitHub](https://github.com/meganz/MEGAcmd) — Repositório oficial
- [MEGA SDK](https://github.com/meganz/sdk) — SDK MEGA (submódulo)
- [vcpkg](https://github.com/microsoft/vcpkg) — Gerenciador de dependências
- [CMake](https://cmake.org) — Sistema de build
- [MEGA.nz](https://mega.nz) — Site oficial
- [MEGAcmd Releases](https://mega.nz/cmd) — Downloads de pacotes prontos
