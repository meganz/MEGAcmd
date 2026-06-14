---
name: megacmd
description: >-
  Agente CLI para MEGA.nz. Ative quando o usuário pedir para enviar arquivos ao
  MEGA, baixar do MEGA, sincronizar pastas locais com a nuvem MEGA, agendar
  backups para o MEGA, compartilhar arquivos via links públicos do MEGA, montar
  MEGA como pasta local (FUSE no Linux), servir arquivos via WebDAV/FTP do
  MEGA, ou gerenciar configurações da conta MEGA.
license: MIT-0
metadata:
  version: "1.0.0"
  category: cloud-storage
  platform: [linux, macos, windows]
---

# MEGAcmd — Guia para Agentes de IA

## O que este skill faz

Instruções para agentes usarem o **MEGAcmd**, a interface CLI oficial do MEGA.nz. Abrange operações com arquivos, sync bidirecional, backups, servidores WebDAV/FTP, montagem FUSE, compartilhamento e gerenciamento de conta.

> **Importante:** Este skill é para **USUÁRIOS** do MEGAcmd. Se o objetivo é BUILDAR, DEBUGAR ou CONTRIBUIR com o código fonte, não ative este skill.

## Quando usar

- O usuário pede upload/download de arquivos no MEGA.nz
- O usuário quer sincronizar pastas locais ↔ nuvem MEGA
- O usuário precisa de backups agendados com versões
- O usuário quer compartilhar arquivos via links públicos
- O usuário precisa servir arquivos via WebDAV ou FTP
- O usuário quer montar pasta MEGA como sistema de arquivos (Linux)
- O usuário precisa gerenciar conta, senha, sessões, ou contatos
- O usuário reporta que o sync não está funcionando

## Quando NÃO usar

- O usuário quer usar a interface web do MEGA (navegador)
- O usuário quer BUILDAR, DEBUGAR ou CONTRIBUIR com o código fonte do MEGAcmd — esse é outro skill
- O usuário quer usar a SDK MEGA para integração customizada
- O MEGAcmd não está instalado (a skill não instala — só instrui sobre instalação)
- O usuário quer acessar arquivos via MEGA Desktop App (não CLI)

---

> ⚠️ **Visão Geral de Segurança**
> Este skill pode realizar operações poderosas na sua conta MEGA, incluindo gerenciamento de arquivos, sync bidirecional, backups agendados, compartilhamento de links públicos e serviços de rede (WebDAV/FTP).
> **Cuidado para o agente:** Exija confirmação explícita do usuário antes de fazer login, excluir arquivos, sincronizar pastas, criar backups, exportar links, compartilhar pastas, alterar configurações da conta, montar FUSE ou iniciar WebDAV/FTP.
> **Segurança de credenciais:** Evite colocar senhas, IDs de sessão, credenciais de proxy ou chaves de recuperação diretamente em comandos ou logs visíveis no chat. Prefira login interativo ou mecanismos protegidos de secrets.

## Pré-requisitos

Antes de usar qualquer comando, SEMPRE verifique:

```bash
# 1. MEGAcmd está instalado?
which mega-exec 2>/dev/null && echo "INSTALADO" || echo "NÃO INSTALADO"

# 2. Servidor está rodando?
ps aux | grep -q "[m]ega-cmd-server" && echo "SERVIDOR OK" || echo "SERVIDOR PARADO"

# 3. Está logado?
mega-whoami >/dev/null 2>&1 && echo "LOGADO" || echo "NÃO LOGADO"
```

Se o servidor não estiver rodando: `mega-cmd-server &`
Se não estiver logado: `mega-login email password`

---

## Arquitetura

| Componente | Executável | Função |
|---|---|---|
| Servidor | `mega-cmd-server` | Roda em background, processa comandos, gerencia sync/backups/tranfers |
| Shell | `mega-cmd` | Modo interativo (comandos SEM prefixo `mega-`) |
| Client | `mega-exec` + `mega-*` | Modo scriptável (comandos COM prefixo `mega-`) |

**Dados:** `$HOME/.megaCmd/` (Linux) | `%LOCALAPPDATA%\MEGAcmd\.megaCmd\` (Windows)

**Sessão:** Login salva cache local. `logout` limpa. `logout --keep-session` mantém cache.

---

## Modos de Uso — ATENÇÃO AOS PREFIXOS

### Modo Scriptável (agente usa este → SEMPRE usar `mega-`)
```bash
mega-login email senha
mega-put ~/arquivo.pdf /Destino/
mega-get /remoto/arquivo.pdf ~/Downloads/
exit code 0 = sucesso, != 0 = falha
```

### Modo Interativo (shell do MEGAcmd → comandos SEM `mega-`)
```bash
mega-cmd
MEGA CMD> login email senha
MEGA CMD> put ~/arquivo.pdf /Destino/
```

> ⚠️ **Regra para o agente:** Você está em um terminal bash. **SEMPRE** use `mega-` como prefixo. Comandos sem `mega-` (`sync`, `webdav`, `ftp`, `log`) só funcionam DENTRO do shell interativo (`mega-cmd`).

---

> ⚠️ **Aviso de Segurança — Exposição de Credenciais**
> Executar mega-login com senha inline expõe credenciais ao histórico do shell, listas de processos, logs de auditoria e telemetria do agente. Em ambientes compartilhados ou automatizados, prefira métodos de login que não passem a senha como argumento da linha de comando.
> **Recomendação:** Use `mega-login` interativamente (sem o argumento da senha) quando possível. Sempre habilite 2FA para proteção da conta. Evite incluir credenciais em scripts ou saída de automação.

## Comandos Essenciais

### Login e Conta
| Comando | Descrição |
|---|---|
| `mega-login email password [--auth-code=XXXX]` | Login (2FA opcional) |
| `mega-logout [--keep-session]` | Sair (ou manter cache) |
| `mega-whoami [-l]` | Info da conta |
| `mega-df [-h]` | Espaço de armazenamento |
| `mega-masterkey ./arquivo.txt` | Salvar chave de recuperação |
| `mega-passwd [-f] [--auth-code=XXXX] nova-senha` | Alterar senha |
| `mega-session` | Mostrar session ID |
| `mega-killsession -a` | Matar todas outras sessões |


> ⚠️ **Aviso de Segurança — Chave de Recuperação**
> A chave mestre (chave de recuperação) é essencial para a recuperação de dados. Se perdida, você não pode recuperar seus dados sem a senha. Se exposta, um invasor ganha acesso durável à sua conta.
> **Recomendação:** Salve a chave mestre em armazenamento criptografado (gerenciador de senhas, USB criptografado). Armazene com permissões de arquivo restritas (ex.: `chmod 600`). Não sincronize o arquivo de texto puro para a nuvem.

> ⚠️ **Aviso de Segurança — Token de Sessão**
> O session ID é uma credencial do tipo bearer. Se exposto, qualquer pessoa pode se passar pela sua sessão até que ela expire ou seja revogada.
> **Recomendação:** Não compartilhe a saída do comando `mega-session` em logs, capturas de tela, scripts ou relatórios automatizados.

### Navegação e Listagem
| Comando | Descrição |
|---|---|
| `mega-ls [-lhR] [--versions] [caminho]` | Listar arquivos |
| `mega-ls -l` | Listar com detalhes (tipo, tamanho, data) |
| `mega-find [caminho] --pattern="*.pdf" [--type=f\|d]` | Buscar arquivos |
| `mega-find / --pattern="*.tmp" --mtime="-7d"` | Buscar com data |
| `mega-du [-h] [--versions] [caminho]` | Espaço usado por pasta |
| `mega-cd [caminho]` | Mudar diretório remoto |
| `mega-pwd` | Diretório remoto atual |
| `mega-mount` | Listar nós raiz |

### Upload e Download
| Comando | Descrição |
|---|---|
| `mega-put [-c] [-q] local [destino]` | Upload (`-c`=cria pasta, `-q`=background) |
| `mega-get [-q] origem [local]` | Download (`--password` p/ links protegidos) |
| `mega-get "link#key" ./dir` | Download de link público |
| `mega-cat caminho` | Exibir conteúdo de arquivo texto |

> ⚠️ **Aviso de Segurança — Exposição de Dados**
> Criar links públicos expõe seus dados na nuvem para qualquer pessoa com o link. Links com `--writable` concedem acesso de escrita à pasta compartilhada.
> **Recomendação:** Use `--password` para proteger arquivos sensíveis. Defina `--expire` para acesso com prazo limitado. Revise e remova periodicamente links de exportação não utilizados.
### Gerenciamento de Arquivos
| Comando | Descrição |
|---|---|
| `mega-mkdir [-p] caminho` | Criar diretório |
| `mega-cp [--use-pcre] origem destino` | Copiar (tudo remoto) |
| `mega-mv [--use-pcre] origem destino` | Mover/renomear |
| `mega-rm [-r] [-f] caminho` | Deletar (recursivo/forçado) |
| `mega-export -a caminho` | Criar link público |
| `mega-export -d caminho` | Remover link |
| `mega-export -a caminho --password="x" --expire="30d"` | Link com senha (PRO) |
| `mega-import link [destino]` | Importar link para nuvem |

> ⚠️ **Aviso de Segurança — Compartilhamento de Pastas**
> Compartilhar pastas dá acesso a usuários externos ao seu conteúdo do MEGA. Os níveis de permissão variam de Leitura (0) a Proprietário (3). Compartilhar em excesso ou conceder permissões excessivas pode expor dados sensíveis.
> **Recomendação:** Conceda o nível mínimo de permissão necessário. Remova o compartilhamento quando não for mais necessário. Revise pastas compartilhadas periodicamente.

### Compartilhamento
| Comando | Descrição |
|---|---|
| `mega-share -a --with="email" --level=N /pasta` | Níveis: 0=Read, 1=R+W, 2=Full, 3=Owner |
| `mega-share -d --with="email" /pasta` | Parar compartilhamento |
| `mega-invite email [--message="..."]` | Convidar contato |
| `mega-ipc email -a` | Aceitar convite |
| `mega-users [-s]` | Listar contatos |

---

## Sincronização (Sync)

> Sync é bidirecional. Arquivos removidos vão para `SyncDebris` no Rubbish Bin.

```bash
mega-sync ~/Documentos /MEGA/Documentos    # Criar sync
mega-sync                                    # Listar syncs
mega-sync -p ID                              # Pausar
mega-sync -e ID                              # Retomar
mega-sync -d ID                              # Remover (não deleta arquivos)
```

**Ignorar padrões:**
```bash
mega-sync-ignore --add "-f:*.tmp" ID
mega-sync-ignore --add "-f:node_modules" ID
mega-sync-ignore --show ID
```

Formato do filtro: `<CLASS><TARGET><TYPE><STRATEGY>:<PATTERN>`
- CLASS: `-` (exclude) / `+` (include)
- TARGET: `d`(dir), `f`(file), `s`(symlink), `a`(all)
- TYPE: `N`(local name), `p`(path), `n`(subtree name)
- STRATEGY: `G`/`g`(glob), `R`/`r`(regexp). Upper=case-sensitive

**Verificar:** `mega-sync` mostra STATUS = `Synced` quando tudo OK.

---

## Backups

> Funcionalidade BETA. Backups são unidirecionais (local → nuvem).

```bash
mega-backup ~/Fotos /Backups/Fotos --period="0 0 4 * * *" --num-backups=10
mega-backup ~/Projetos /Backups --period="2h" --num-backups=24

# Gerenciar
mega-backup                    # Listar
mega-backup -lh                # Com histórico
mega-backup -a TAG             # Abortar
mega-backup -d ~/Fotos         # Remover configuração
```

Padrão de armazenamento: `/remoto/pasta_bk_TIMESTAMP`

**Verificar:** `mega-backup -lh` mostra STATUS `COMPLETE` e histórico.

---

## Servidores WebDAV e FTP

> BETA. Apenas um servidor por vez. Configuração da primeira localização vale para todas.

> ⚠️ **Aviso de Segurança — Exposição de Rede**
> Iniciar o WebDAV expõe seu conteúdo do MEGA pela rede. Sem TLS, o tráfego não é criptografado. A opção `--public` torna o serviço acessível além de localhost.
> **Recomendação:** Use `--tls` com certificados válidos. Evite `--public` a menos que necessário. Pare os serviços quando não estiver em uso com `mega-webdav -d`.

### WebDAV
```bash
mega-webdav /Videos                            # Servir pasta (porta 4443)
mega-webdav /filme.mp4                         # Streaming
mega-webdav /Docs --tls --certificate=cert.pem --key=key.pem  # HTTPS
mega-webdav /Publico --public --port=8080      # Público
mega-webdav -d /Videos                         # Parar
mega-webdav -d --all                           # Parar tudo
```

> ⚠️ **Aviso de Segurança — Exposição de Rede**
> Iniciar o FTP expõe seu conteúdo da MEGA via FTP não criptografado. Sem `--tls`, credenciais e dados são enviados em texto puro. Por padrão, o servidor é local; use `--public` para permitir acesso remoto.
> **Recomendação:** Use `--tls` com certificados válidos para FTP seguro. Evite servir conteúdo sensível via FTP simples. Pare os serviços quando não estiver em uso com `mega-ftp -d`.

### FTP
```bash
mega-ftp /Publico                              # Servir pasta (porta 4990)
mega-ftp /Docs --tls --certificate=cert.pem --key=key.pem  # FTPs
mega-ftp -d /Publico                           # Parar
```

**Verificar:** `mega-webdav` ou `mega-ftp` lista as URLs ativas.

---

## FUSE (Linux apenas)

> BETA. Streaming não suportado — arquivos baixados completamente. Cache em `$HOME/.megaCmd/fuse-cache`.

```bash
mega-fuse-add --name=meus-docs /mnt/mega /Documentos
mega-fuse-show                                 # Listar
mega-fuse-enable meus-docs                     # Ativar
mega-fuse-disable meus-docs                    # Desativar
mega-fuse-remove meus-docs                     # Remover (precisa estar desativado)
```

**Problema:** "Transport endpoint is not connected"
```bash
fusermount -u /mnt/mega
```

---

## Transferências

```bash
mega-transfers                            # Listar ativas
mega-transfers --summary                  # Resumo
mega-transfers -c TAG                     # Cancelar
mega-transfers -p TAG                     # Pausar
mega-transfers -r TAG                     # Retomar
mega-transfers -c -a                      # Cancelar todas
mega-speedlimit -d 2M                     # Limitar download
mega-speedlimit -u 1M                     # Limitar upload
```

---

## Configurações

```bash
mega-https on|off
mega-proxy URL|--auto|--none
mega-configure
mega-configure max_nodes_in_cache N
mega-permissions --files -s 600          # Unix apenas
mega-log -c DEBUG                        # Ajustar nível de log MEGAcmd
mega-log -s INFO                         # Ajustar nível de log SDK
```
> ⚠️ Ao usar autenticação de proxy, evite passar credenciais na linha de comando, pois elas podem ser capturadas pelo histórico do shell ou por listagens de processos. Prefira usar variáveis de ambiente ou um arquivo de configuração.

---

## Diagnóstico Rápido — Sync Não Funciona

Quando o sync estiver travado ou falhando:

### Passo 1: Estado geral
```bash
# Servidor está rodando?
ps aux | grep -c "[m]ega-cmd-server"
# Deve retornar 1 ou mais

# Está logado?
mega-whoami
# Deve mostrar o email da conta
```

### Passo 2: Verificar sync
```bash
mega-sync
# Colunas: RUN_STATE (Running/Pending/Disabled), STATUS (Synced/Pending/Syncing), ERROR
```

### Passo 3: Verificar conflitos
```bash
mega-sync-issues
# Se houver issues, investigue:
mega-sync-issues --detail ID_DO_ISSUE
```

### Passo 4: Verificar transferências
```bash
mega-transfers --summary
# Uploads ou downloads ativos? Progresso?
```

### Passo 5: Verificar logs
```bash
tail -50 ~/.megaCmd/megacmdserver.log
# Procure por: ERR, WARN, "sync issues", "quota", "rate limit"
```

### Passo 6: Verificar armazenamento
```bash
mega-df -h
# Cota excedida? (USED STORAGE perto de 100%)
```

### Causas comuns e soluções

| Sintoma | Provável causa | Solução |
|---|---|---|
| RUN_STATE = Disabled | Sync pausado | `mega-sync -e ID` |
| STATUS = Pending (nunca muda) | Varredura inicial de muitos arquivos | Aguardar (pode levar horas com 100k+ arquivos) |
| Sync Issues > 0 | Conflitos local × nuvem | `mega-sync-issues --detail ID`, remover/mover arquivos problemáticos |
| ERROR = "Sync Issues (N)" | Arquivos com problema | Executar passo 3 |
| Nenhuma transferência aparece | Scan ainda em andamento | Aguardar |
| "rate limit" no log | Muitas requisições em curto período | Aguardar alguns minutos |
| "quota" no log | Cota de armazenamento excedida | `mega-df -h`, liberar espaço |
| LOG cheio de "Can't find" | Arquivos deletados/movidos | Normalmente resolvido sozinho após rescaneamento |

> ⚠️ Os comandos a seguir excluem arquivos locais sem confirmação. Verifique se os caminhos estão corretos antes de executar.

### Plano de ação quando sync está travado

```bash
# 1. Pausar
mega-sync -p BK0pIuFWODQ    # use o ID real do seu sync

# 2. Resolver issues (se houver)
# Remover arquivos problemáticos (Zone.Identifier, .lnk, .megaignore)
find ~/pasta-do-sync -name "*:Zone.Identifier" -delete 2>/dev/null
find ~/pasta-do-sync -name "*.lnk" -type f -delete 2>/dev/null
find ~/pasta-do-sync -name ".megaignore" -delete 2>/dev/null

# 3. Retomar
mega-sync -e BK0pIuFWODQ

# 4. Monitorar
sleep 10 && mega-sync && mega-transfers --summary
```

---

## Verificação — Como Confirmar Que Funcionou

| Operação | Como verificar |
|---|---|
| Login | `mega-whoami` mostra o email da conta |
| Listagem | `mega-ls /caminho` lista arquivos (ou erro se não existe) |
| Upload | `mega-ls /destino` mostra o arquivo enviado |
| Download | Arquivo existe no caminho local especificado |
| Sync ativo | `mega-sync` mostra STATUS = `Synced`, ERROR = `NO` |
| Sync em progresso | `mega-transfers` mostra transferências ativas |
| Backup criado | `mega-backup -lh` mostra histórico com STATUS = `COMPLETE` |
| Link público | `mega-export /caminho` mostra a URL |
| WebDAV ativo | `mega-webdav` lista URLs servindo |
| FTP ativo | `mega-ftp` lista URLs servindo |
| Sessão fechada | `mega-whoami` retorna erro de não logado |

---

## Regras Importantes

1. **Sempre verifique exit code**: `mega-comando || echo "FALHOU ($?)"`
2. **Escape `!` em links**: `mega-get https://mega.nz/#F\!ABcD\!Key ./dir`
3. **Master Key é ESSENCIAL**: `mega-masterkey ./recuperacao.txt` — sem ela, perder senha = perder tudo. Armazene em um local criptografado e com acesso restrito.
4. **Faça logout** em máquinas compartilhadas
5. **`logout --keep-session`** em máquina pessoal (mantém cache, retoma sessão)
6. **Links `--writable` expõem a pasta compartilhada** — qualquer pessoa com o link pode enviar, modificar ou excluir arquivos nessa pasta. Compartilhe links graváveis apenas com pessoas de confiança.
7. **Use `-q` (queue)** para operações grandes em background
8. **Em syncs, exclua `node_modules`, `.git`, `*.tmp`** com `mega-sync-ignore`
9. **Primeiro sync é mais lento** — 100k+ arquivos podem levar horas para escanear

## Códigos de Erro Comuns

| Código | Nome | Significado |
|---|---|---|
| `0` | API_OK | Sucesso |
| `-2` | API_EACCESS | Acesso negado / permissão |
| `-5` | API_ERATELIMIT | Muitas requisições — aguarde |
| `-10` | API_ENOENT | Arquivo/pasta não encontrado |
| `-13` | API_EEXIST | Já existe |
| `-16` | API_ESID | Sessão inválida — faça login novamente |
| `-18` | API_EOVERQUOTA | Cota de armazenamento excedida |

Use `mega-errorcode NUM` para traduzir qualquer código.

## Compatibilidade

| Funcionalidade | Linux | macOS | Windows |
|---|---|---|---|
| FUSE mounts | ✅ | ❌ | ❌ |
| Autocomplete (bash) | ✅ | ✅ | ❌ |
| Unicode no shell | ✅ | ✅ | Experimental |
| Auto-update | ❌ (pkg manager) | ✅ | ✅ |
| File permissions | ✅ | ✅ | ❌ |

---

## Referência Completa

Para a documentação detalhada de TODOS os 76 comandos (sintaxe completa, todas as flags, exemplos), consulte:

- **Sempre disponível:** `mega-comando --help` para ajuda de cada comando
