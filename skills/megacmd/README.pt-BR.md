# MEGAcmd — Agent Skill

Skill para agentes de IA usarem o **MEGAcmd**, a interface de linha de comando oficial do [MEGA.nz](https://mega.nz).

> **Nome técnico:** `megacmd`  
> **Categoria:** cloud-storage  
> **Compatibilidade:** OpenCode, Cline, Claude Code, Continue.dev, e ferramentas compatíveis com SKILL.md  

---

## Objetivos

Esta skill capacita agentes de IA a:

- **Gerenciar arquivos** na nuvem MEGA (upload, download, cópia, movimentação, exclusão)
- **Sincronizar** pastas locais com a nuvem (sync bidirecional)
- **Agendar backups** com retenção de versões históricas
- **Compartilhar** arquivos via links públicos (com ou sem senha, com ou sem expiração)
- **Servir** arquivos via WebDAV ou FTP (com suporte a TLS)
- **Montar** pastas MEGA como sistema de arquivos local (FUSE, apenas Linux)
- **Gerenciar** conta (login, senha, master key, sessões, 2FA)
- **Gerenciar** contatos e compartilhamentos entre usuários
- **Diagnosticar** e resolver problemas de sincronização

---

## Requisitos de Sistema

### Sistema Operacional

| SO | Suporte | Observações |
|---|---|---|
| **Linux** | ✅ Completo | FUSE, autocomplete, permissões de arquivo |
| **macOS** | ✅ Completo | Sem suporte a FUSE |
| **Windows** | ✅ Completo | Sem autocomplete bash, sem permissões de arquivo |

### Dependências

O MEGAcmd precisa ser instalado no sistema. As dependências são gerenciadas automaticamente pelo instalador oficial.

| Dependência | Obrigatória? | Finalidade |
|---|---|---|
| MEGAcmd (>= 2.5.0) | ✅ Sim | Binários do CLI |
| `bash` (Linux/macOS) | ✅ Sim | Execução dos wrappers `mega-*` |
| `PATH` configurado | ✅ Sim | Acesso aos comandos `mega-*` |
| `ps`, `grep`, `which` | ✅ Sim | Diagnóstico e verificação |
| Conexão com internet | ✅ Sim | Acesso aos servidores MEGA |

### Portas de Rede

| Serviço | Porta Padrão | Finalidade |
|---|---|---|
| WebDAV | 4443 | Servir arquivos via HTTP/HTTPS |
| FTP | 4990 | Servir arquivos via FTP (passive mode) |
| FTP data | 1500-1600 | Canal de dados FTP |
| IPC (TCP) | 12300 | Comunicação cliente-servidor (alternativa Python) |

---

## Requisitos de Conta

### Conta MEGA

- Uma conta no [MEGA.nz](https://mega.nz) é necessária para a maioria das operações
- Download de links públicos não requer login
- Upload para pastas públicas (File Requests) não requer login
- O plano **Pro** é necessário para:
  - Links com senha (`export --password`)
  - Links com expiração (`export --expire`)
  - Maiores limites de armazenamento e transferência

### Credenciais Necessárias

| Operação | Requer | Como obter |
|---|---|---|
| Login | Email + senha | Criar em mega.nz |
| 2FA | `--auth-code` | Configurar 2FA na conta |
| Link público com senha | Senha do link | Definida ao exportar |
| Link editável | `--auth-key` | Gerada ao exportar com `--writable` |

### Master Key (Chave de Recuperação)

**Essencial.** Sem a Master Key, perder a senha significa perder **todos os dados**. Deve ser salva imediatamente após criar a conta:

```bash
mega-masterkey ./minha-chave-recuperacao.txt
```

---

## Instalação do MEGAcmd

### Via Pacote Oficial (Recomendado)

Baixe o instalador para seu sistema em: [https://mega.nz/cmd](https://mega.nz/cmd)

**Linux (Ubuntu/Debian):**
```bash
# Adicionar repositório
wget -O /tmp/megacmd.deb https://mega.nz/linux/repo/xUbuntu_24.04/amd64/megacmd_2.5.2-1_amd64.deb
sudo dpkg -i /tmp/megacmd.deb
sudo apt install -f
```

**macOS:**
```bash
# Baixar DMG de https://mega.nz/cmd e arrastar para Applications
export PATH=/Applications/MEGAcmd.app/Contents/MacOS:$PATH
```

**Windows:**
```powershell
# Baixar e executar instalador de https://mega.nz/cmd
# Instalação silenciosa:
MEGAcmdSetup.exe /S

# Adicionar ao PATH:
$env:PATH += ";$env:LOCALAPPDATA\MEGAcmd"
```

### Via Build Manual

```bash
git clone https://github.com/meganz/MEGAcmd.git
cd MEGAcmd && git submodule update --init --recursive
cmake -B build/build-cmake-Release -DCMAKE_BUILD_TYPE=Release
cmake --build build/build-cmake-Release -j$(nproc)
sudo cmake --install build/build-cmake-Release
```

### Verificar Instalação

```bash
which mega-exec        # Deve mostrar o caminho do binário
mega-exec version      # Deve mostrar a versão
mega-help              # Deve listar os comandos
```

---

## Como Usar a Skill

### Ativação

A skill é ativada **automaticamente** pelo agente quando o contexto da conversa corresponder ao `description`. Para forçar a ativação, mencione "MEGAcmd", "MEGA.nz", "upload para MEGA", "download do MEGA", "sync com MEGA", "backup para MEGA", ou "link do MEGA".

### Estrutura de Arquivos

```
skills/megacmd/
├── SKILL.md                  # ⬅️ Instruções principais (Inglês)
├── SKILL.pt-BR.md             # ⬅️ Instruções principais (Português)
├── README.md                 # ⬅️ Documentação da skill (Inglês)
├── README.pt-BR.md            # ⬅️ Este arquivo (Português)
└── references/
    ├── complete-commands-reference.md    # ⬅️ Referência completa (Inglês)
    └── comandos-completos.pt-BR.md       # ⬅️ Referência completa (Português)
```

### Fallback para o Agente

Se o agente não encontrar a skill, ele pode usar:
- `AGENTS.md` na raiz do projeto MEGAcmd
- `mega-comando --help` para ajuda de cada comando
- `mega-help` para lista completa de comandos

---

## Modos de Interação

### Scriptável (recomendado para agentes)

Comandos com prefixo `mega-` executados diretamente no terminal:

```bash
mega-login usuario@email.com senha
mega-put ~/documento.pdf /Documentos/
mega-get /remoto/arquivo.pdf ~/Downloads/
```

> ⚠️ **Aviso de Segurança — Exposição de Credenciais**
> O exemplo de login acima passa a senha como argumento da linha de comando, expondo-a ao histórico do shell, listas de processos e logs. Em ambientes compartilhados ou automatizados, prefira `mega-login` sem o argumento da senha.
> **Recomendação:** Use `mega-login` interativamente quando possível. Sempre habilite 2FA. Evite hardcodar credenciais em scripts.


### Interativo (shell do MEGAcmd)

Comandos sem prefixo dentro do shell:

```bash
mega-cmd
MEGA CMD> login usuario@email.com
MEGA CMD> ls /
MEGA CMD> get arquivo.pdf
```

> ⚠️ Agentes em terminal bash devem **sempre** usar o prefixo `mega-`.

---

## Exemplos Rápidos

> ⚠️ **Aviso de Segurança — Exposição de Credenciais**
> O exemplo `mega-login` abaixo passa sua senha como argumento da linha de comando. Isso expõe credenciais ao histórico do shell, listas de processos, logs de auditoria e telemetria do agente.
> **Recomendação:** Use `mega-login` interativamente (sem a senha) quando possível. Sempre habilite 2FA. Nunca inclua credenciais em scripts ou versionamento.

```bash
# Login
mega-login meu@email.com minha-senha

# Upload
mega-put ~/foto.jpg /Imagens/

# Download
mega-get /Documentos/relatorio.pdf ~/Downloads/

# Sync bidirecional
mega-sync ~/Documentos /CloudDocs

# Link público
mega-export -a /Pasta/Compartilhada

# Backup diário
mega-backup ~/Fotos /Backups --period="0 0 4 * * *" --num-backups=10

# WebDAV
mega-webdav /Videos --public --port=8080
> ⚠️ **Aviso de Segurança — Exposição de Rede**
> Iniciar o WebDAV expõe seu conteúdo do MEGA pela rede. Sem TLS, o tráfego não é criptografado. A opção `--public` torna o serviço acessível além de localhost.
> **Recomendação:** Use `--tls` com certificados válidos. Evite `--public` a menos que necessário. Pare os serviços quando não estiver em uso com `mega-webdav -d`.


# Verificar status
mega-whoami -l
mega-df -h
mega-sync
```

---

## Licença

Esta skill é distribuída sob licença **MIT-0 (MIT No Attribution)**.  

---

## Links Úteis

- [MEGA.nz](https://mega.nz) — Site oficial
- [MEGAcmd Releases](https://mega.nz/cmd) — Downloads
- [MEGAcmd GitHub](https://github.com/meganz/MEGAcmd) — Código fonte
- [MEGA Help Center](https://mega.nz/help) — Central de ajuda
- [User Guide](https://github.com/meganz/MEGAcmd/blob/master/UserGuide.md) — Guia do usuário
