# MEGAcmd — Referência Completa de Comandos

> Documentação de referência para TODOS os 76 comandos do MEGAcmd.
> Versão: 2.5.2 | Licença: BSD 2-Clause

**Legenda:**
- `[]` = opcional
- `|` = ou um ou outro
- `--use-pcre` = usar Expressões Regulares PCRE
- `--time-format=FORMAT` = formatos: RFC2822, ISO6081, ISO6081_WITH_TIME, SHORT, SHORT_UTC, CUSTOM strftime
- `--path-display-size=N` = tamanho fixo para exibição de caminhos
- `--col-separator=X` = separador de colunas
- `--output-cols=COL1,COL2,...` = selecionar colunas

---

## Sumário

1. [Gerenciamento de Conta](#1-gerenciamento-de-conta)
2. [Contatos e Convites](#2-contatos-e-convites)
3. [Navegação](#3-navegação)
4. [Listagem e Busca](#4-listagem-e-busca)
5. [Gerenciamento de Arquivos](#5-gerenciamento-de-arquivos)
6. [Compartilhamento](#6-compartilhamento)
7. [Transferências](#7-transferências)
8. [Configuração](#8-configuração)
9. [Utilitários](#9-utilitários)
10. [Sync](#10-sync)
11. [Backups](#11-backups)
12. [Servidores FTP e WebDAV](#12-servidores-ftp-e-webdav)
13. [FUSE](#13-fuse)
14. [Flags Comuns](#14-flags-comuns)
15. [Códigos de Erro](#15-códigos-de-erro)
16. [IPC](#16-ipc)
17. [Logging](#17-logging)

---

## 1. Gerenciamento de Conta

### signup
Registrar novo usuário.
```
signup email password [--name="Your Name"]
```
- `--name` — Nome para registro
- A senha não deve conter `"` ou `'`
- Use `confirm` após receber o link por email
- **Importante:** Guarde a Master Key!

### confirm
Confirmar conta usando o link recebido por email.
```
confirm link email password
```

### cancel
Cancelar permanentemente a conta MEGA.
```
cancel
```
- Conta permanentemente fechada e dados deletados
- Requer confirmação via link (veja `confirmcancel`)

### confirmcancel
Confirmar o cancelamento da conta.
```
confirmcancel link password
```

### login
Entrar na conta MEGA.
```
login [--auth-code=XXXX] email password
login exportedfolderurl#key [--auth-key=XXXX] [--resume]
login passwordprotectedlink [--password=PASSWORD]
login session
```
- `--auth-code=XXXX` — Token MFA (autenticação de dois fatores)
- `--password=PASSWORD` — Senha para links protegidos
- `--auth-key=AUTHKEY` — Para writable folder links
- `--resume` — Tentar carregar do cache
- Só é possível logar em uma entidade por vez

### logout
Sair da conta.
```
logout [--keep-session]
```
- `--keep-session` — Mantém a sessão atual (não deleta o cache)

### session
Imprime o ID da sessão (secreto).
```
session
```

### whoami
Informações do usuário logado.
```
whoami [-l]
```
- `-l` — Informações estendidas: armazenamento total, espaço por pasta principal, nível Pro, saldo, sessões ativas

### killsession
Matar sessões do usuário atual.
```
killsession [-a | sessionid1 sessionid2 ...]
```
- `-a` — Mata todas as sessões exceto a atual

### passwd
Alterar senha.
```
passwd [-f] [--auth-code=XXXX] newpassword
```
- `-f` — Forçar (sem perguntar)
- `--auth-code=XXXX` — Código de autenticação de dois fatores
- Altera a senha e fecha todas as sessões ativas (exceto a atual)

### masterkey
Exibe a Master Key (Chave de Recuperação).
```
masterkey pathtosave
```
- **Essencial** para recuperar acesso aos dados
- Sem a Master Key, perder a senha = perder todos os dados

---

## 2. Contatos e Convites

### invite
Convidar um contato / deletar um convite.
```
invite [-d|-r] dstemail [--message="MESSAGE"]
```
- `-d` — Deletar convite
- `-r` — Reenviar convite
- `--message` — Mensagem personalizada

### showpcr
Mostrar solicitações de contato (incoming/outgoing).
```
showpcr [--in | --out] [--time-format=FORMAT]
```
- `--in` — Solicitações recebidas
- `--out` — Convites enviados

### ipc
Gerenciar convites de contato recebidos.
```
ipc email|handle -a|-d|-i
```
- `-a` — Aceitar convite
- `-d` — Rejeitar convite
- `-i` — Ignorar convite (cuidado!)

### users
Listar contatos.
```
users [-s] [-h] [-n] [-d contact@email] [--time-format=FORMAT] [--verify|--unverify contact@email.com] [--help-verify [contact@email.com]]
```
- `-d email` — Deletar contato
- `-s` — Mostrar pastas compartilhadas com cada contato
- `-h` — Mostrar todos (ocultos, bloqueados)
- `-n` — Mostrar nomes dos usuários
- `--verify email` — Verificar contato (verifique manualmente as credenciais primeiro!)
- `--unverify email` — Marcar como não verificado

### userattr
Listar/atualizar atributos de usuário.
```
userattr [-s attribute value|attribute|--list] [--user=user@email]
```
- `--user=user@email` — Selecionar usuário para consultar
- `-s attribute value` — Definir atributo
- `--list` — Listar atributos válidos

---

## 3. Navegação

### cd
Mudar diretório remoto atual.
```
cd [remotepath]
```
- Sem argumentos: volta para a raiz (/)

### pwd
Imprime o diretório remoto atual.
```
pwd
```

### lcd
Mudar diretório local (para uploads/downloads no shell interativo).
```
lcd [localpath]
```
- Sem argumentos: volta para home
- Em modo não-interativo, o diretório local é o do shell que executa

### lpwd
Imprime o diretório local atual.
```
lpwd
```

### mount
Lista todos os nós raiz.
```
mount
```
Exibe: ROOT, INBOX, RUBBISH, INSHARE (pastas compartilhadas por outros)

### tree
Lista arquivos em formato de árvore.
```
tree [remotepath]
```

---

## 4. Listagem e Busca

### ls
Lista arquivos em um diretório remoto.
```
ls [-halRr] [--show-handles] [--tree] [--versions] [remotepath] [--use-pcre] [--show-creation-time] [--time-format=FORMAT]
```
- `-R`/`-r` — Listar recursivamente
- `--tree` — Formato de árvore (implica -r)
- `--show-handles` — Mostrar handles (H:XXXXXXXX)
- `-l` — Resumo detalhado (FLAGS, VERS, SIZE, DATE, NAME)
  - Flags tipo: d=dir, f=file, r=root, i=inbox, b=rubbish, x=inShare
  - Exportado: e/-
  - Compartilhamento: s=shared, i=inShare, -=none
- `-h` — Tamanhos legíveis
- `-a` — Info extra (`-aa` mostra links públicos e expiração)
- `--versions` — Mostrar versões históricas
- `--show-creation-time` — Mostrar data de criação em vez de modificação
- `remotepath` — Pode conter padrões como `/PASTA1/PADRAO2/*.txt`

### find
Encontrar nós por padrão.
```
find [remotepath] [-l] [--pattern=PATTERN] [--type=d|f] [--mtime=TIMECONSTRAIN] [--size=SIZECONSTRAIN] [--use-pcre] [--time-format=FORMAT] [--show-handles|--print-only-handles]
```
- `--pattern=PATTERN` — Padrão de busca (PCRE ou wildcards `?` e `*`)
- `--type=d|f` — Filtrar: `d` (pastas), `f` (arquivos)
- `--mtime=TIMECONSTRAIN` — Restrição de tempo: `+1m12d3h` (mais antigo que), `-3h` (últimas 3h). Unidades: h, d, M, s, m, y
- `--size=SIZECONSTRAIN` — Restrição de tamanho: `+1M12k3B` (maior que), `-3M` (menor que). Unidades: B, K, M, G, T
- `--show-handles` / `--print-only-handles` — Exibir apenas handles

### du
Espaço usado por arquivos/pastas.
```
du [-h] [--versions] [remotepath remotepath2 ...] [--use-pcre]
```
- `-h` — Tamanhos legíveis
- `--versions` — Incluir versões no cálculo

### df
Informações de armazenamento.
```
df [-h]
```

### mediainfo
Informações de mídia de arquivos remotos.
```
mediainfo remotepath1 remotepath2 ...
```

---

## 5. Gerenciamento de Arquivos

### mkdir
Criar diretório.
```
mkdir [-p] remotepath
```
- `-p` — Criar hierarquia completa (mkdir -p)

### cp
Copiar arquivos/pastas (tudo remoto).
```
cp [--use-pcre] srcremotepath [srcremotepath2 ...] dstremotepath|dstemail:
```
- Se destino existe e é pasta: copia para dentro
- Se destino não existe e há só uma origem: renomeia
- Se `dstemail:`: envia para a inbox do usuário

### mv
Mover/renomear arquivos/pastas (tudo remoto).
```
mv srcremotepath [--use-pcre] [srcremotepath2 ...] dstremotepath
```

### rm
Deletar arquivo/pasta remoto.
```
rm [-r] [-f] [--use-pcre] remotepath
```
- `-r` — Recursivo (para pastas)
- `-f` — Forçar (sem perguntar)

### put
Upload de arquivos/pastas.
```
put [-c] [-q] [--print-tag-at-start] localfile [localfile2 ...] [dstremotepath]
```
- `-c` — Criar pasta remota de destino se não existir
- `-q` — Queue: executa em background, não espera terminar
- `--print-tag-at-start` — Mostra mensagem inicial com TAG da transferência (mesmo com `-q`)
- Pode omitir `dstremotepath` se apenas 1 localfile (usa diretório remoto atual)

### get
Download de arquivo/pasta ou link público.
```
get [-m] [-q] [--ignore-quota-warn] [--use-pcre] [--password=PASSWORD] exportedlink|remotepath [localpath]
```
- `-q` — Queue: background, não espera
- `-m` — Merge: se pasta já existe, mescla conteúdo (preserva existentes)
- `--ignore-quota-warn` — Ignorar aviso de cota excedida
- `--password=PASSWORD` — Senha para links protegidos (evitar `"` ou `'`)
- Para pastas: baixa todo o conteúdo
- Se destino já existe e é idêntico: nada é feito; se difere, cria novo com ` (NUM)`

### cat
Exibir conteúdo de arquivo remoto (texto).
```
cat remotepath1 remotepath2 ...
```
No Windows, para preservar conteúdo binário, use modo não-interativo com `-o /path/to/file`.

### preview
Download/upload de preview de arquivo.
```
preview [-s] remotepath localpath
```
- `-s` — Upload (sem `-s`, faz download)

### thumbnail
Download/upload de thumbnail de arquivo.
```
thumbnail [-s] remotepath localpath
```
- `-s` — Upload

---

## 6. Compartilhamento

### export
Criar/gerenciar links de exportação.
```
export [-d|-a [--writable] [--mega-hosted] [--password=PASSWORD] [--expire=TIMEDELAY] [-f]] [remotepath] [--use-pcre] [--time-format=FORMAT]
```
- `-a` — Adicionar export (erro se já existir)
  - `--writable` — Link editável (gera auth-key para acesso write)
  - `--mega-hosted` — Compartilha chave com MEGA (para S4)
  - `--password=PASSWORD` — Proteger com senha (PRO)
  - `--expire=TIMEDELAY` — Expiração (PRO). Formato: `1m12d3h`
  - `-f` — Aceitar termos de copyright implicitamente
- `-d` — Remover export (arquivo não é deletado)
- Sem `-a` nem `-d`: lista exports na árvore

### import
Importar conteúdo de link remoto para a nuvem.
```
import exportedlink [--password=PASSWORD] [remotepath]
```
- Se nenhum `remotepath`: usa diretório atual

### share
Gerenciar compartilhamentos de pastas.
```
share [-p] [-d|-a --with=user@email.com [--level=LEVEL]] [remotepath] [--use-pcre] [--time-format=FORMAT]
```
- `-p` — Mostrar compartilhamentos pendentes
- `--with=email` — Usuário alvo
- `-d` — Parar de compartilhar
- `-a` — Adicionar/modificar compartilhamento
- `--level=LEVEL` — Nível de acesso:
  - `0` = Read
  - `1` = Read+Write
  - `2` = Full access
  - `3` = Owner access
- Compartilhar com não-contatos fica pendente até o contato aceitar

### permissions
Permissões padrão para arquivos/pastas criados.
```
permissions [(--files|--folders) [-s XXX]]
```
- `-s XXX` — Novo valor octal (mín: 600 arquivos, 700 pastas)
- Não disponível no Windows
- Persiste entre execuções, removido no logout

---

## 7. Transferências

### transfers
Listar/operar transferências.
```
transfers [-c TAG|-a] | [-r TAG|-a] | [-p TAG|-a] [--only-downloads | --only-uploads] [SHOWOPTIONS]
```
- `-c` — Cancelar transferência (ou `-a` = todas)
- `-p` — Pausar
- `-r` — Retomar
- `--only-uploads` / `--only-downloads` — Filtrar
- Opções de exibição:
  - `--summary` — Resumo
  - `--show-syncs` — Mostrar transfers de sync
  - `--show-completed` / `--only-completed` — Completadas
  - `--limit=N` — Limitar linhas
- Legenda: `⇓` Download, `⇑` Upload, `⇵` Sync, `⏫` Backup

### speedlimit
Limites de velocidade.
```
speedlimit [-u|-d|--upload-connections|--download-connections] [-h] [NEWLIMIT]
```
- `-d` — Download (tamanho/segundo)
- `-u` — Upload
- `--upload-connections` / `--download-connections` — Máx. conexões
- `NEWLIMIT=0` = ilimitado
- Unidades: B, K, M, G, T

### deleteversions
Deletar versões anteriores de arquivos.
```
deleteversions [-f] (--all | remotepath1 remotepath2 ...) [--use-pcre]
```
- `-f` — Forçar (sem perguntar)
- `--all` — Deletar versões de todos os nós
- A versão atual permanece

### reload
Forçar recarregamento dos arquivos remotos.
```
reload
```
Também retoma sincronizações.

---

## 8. Configuração

### configure
Configurações globais.
```
configure [key [value]]
```
Chaves:
- `max_nodes_in_cache` — Máx. nós na memória (controla cache do SDK)
- `exported_folders_sdks` — Instâncias SDK adicionais para links exportados. Padrão 5, Min 0, Max 20

### https
HTTPS para transferências.
```
https [on|off]
```
- Dados já são criptografados ponta-a-ponta; HTTPS adiciona overhead
- Configuração salva entre sessões, removida no logout

### proxy
Configurar proxy.
```
proxy [URL|--auto|--none] [--username=USERNAME --password=PASSWORD]
```

### errorcode
Traduzir código de erro em string.
```
errorcode number
```

### graphics
Ativar/desativar recursos gráficos (thumbnails/previews).
```
graphics [on|off]
```

### codepage
Configurar codepage (Windows).
```
codepage [N [M]]
```
- `N` — Codepage principal (65001 = Unicode)
- `M` — Codepage secundária

### autocomplete
Modo de autocomplete.
```
autocomplete [dos | unix]
```

### unicode
Alternar Unicode no shell interativo.
```
unicode
```
- Experimental — Windows apenas

---

## 9. Utilitários

### help
Lista de comandos.
```
help [-f|-ff|--non-interactive|--upgrade|--paths] [--show-all-options]
```
- `-f` — Descrição breve dos comandos
- `-ff` — Descrição completa
- `--non-interactive` — Info sobre uso em scripts
- `--upgrade` — Info sobre planos PRO
- `--paths` — Cuidados com paths locais e remotos

### version
Versão e informações.
```
version [-l] [-c]
```
- `-c` — Changelog da versão atual
- `-l` — Info estendida (SDK version, features)

### update
Atualizar MEGAcmd.
```
update [--auto=on|off|query]
```
- Atualizações automáticas para Windows e macOS (não para Linux)
- Após atualizar: MEGAcmd reinicia

### clear
Limpar tela.
```
clear
```

### exit / quit
Sair.
```
exit [--only-shell]
quit [--only-shell]
```
- `--only-shell` — Apenas sai do shell (servidor continua rodando)
- Sessão continua ativa, caches disponíveis

### debug
Modo de depuração (ALTAMENTE VERBOSO).
```
debug
```

### psa
Mostrar Public Service Announcement.
```
psa [--discard]
```
- `--discard` — Descartar último PSA

---

## 10. Sync

### sync — Controlar Sincronizações
```
sync [localpath dstremotepath | [-dpe] [ID|localpath]]
```
**Criar sync:** `sync /path/to/local/folder /folder/in/mega`

**Listar syncs:** `sync`
Colunas: ID, LOCALPATH, REMOTEPATH, RUN_STATE (Pending/Loading/Running/Suspended/Disabled), STATUS (None/Synced/Pending/Syncing/Processing), ERROR, SIZE, FILE, DIRS

**Gerenciar:**
- `sync -d ID|localpath` — Deletar sync (não deleta arquivos)
- `sync -p ID|localpath` — Pausar
- `sync -e ID|localpath` — Retomar

### sync-ignore — Filtros de Ignição
```
sync-ignore [--show|[--add|--add-exclusion|--remove|--remove-exclusion] filter1 filter2 ...] (ID|localpath|DEFAULT)
```
**Formato do filtro:** `<CLASS><TARGET><TYPE><STRATEGY>:<PATTERN>`

| Parte | Valores | Significado |
|---|---|---|
| CLASS | `-` (exclude) ou `+` (include) | Excluir ou incluir |
| TARGET | `d` (dir), `f` (file), `s` (symlink), `a` (all) | Tipo de entrada |
| TYPE | `N` (local name), `p` (path), `n` (subtree name) | Escopo do nome |
| STRATEGY | `G`/`g` (glob), `R`/`r` (regexp) | Upper = case-sensitive |

**Exemplos:** `-f:*.txt`, `+fg:work*.txt`, `-N:*.avi`, `-nr:.*foo.*`, `-d:private`

### sync-config — Configuração de Sync
```
sync-config [--delayed-uploads-wait-seconds | --delayed-uploads-max-attempts]
```
- `--delayed-uploads-wait-seconds` — Segundos antes de re-upload de arquivo atrasado
- `--delayed-uploads-max-attempts` — Máx. vezes que um arquivo pode mudar rapidamente antes de atrasar

### sync-issues — Problemas de Sync
```
sync-issues [[--detail (ID|--all)] [--limit=rowcount] [--disable-path-collapse]] | [--enable-warning|--disable-warning]
```
Colunas: ISSUE_ID, PARENT_SYNC, REASON

### exclude (DEPRECATED — use sync-ignore)
```
exclude [(-a|-d) pattern1 pattern2 pattern3]
```

---

## 11. Backups

### backup — Controlar Backups
```
backup (localpath remotepath --period="PERIODSTRING" --num-backups=N | [-lhda] [TAG|localpath] [--period="PERIODSTRING"] [--num-backups=N]) [--time-format=FORMAT]
```
> Funcionalidade em BETA

**Criar backup:**
`backup /path/mega/folder /remote/path --period="0 0 4 * * *" --num-backups=10`

- Primeiro backup executado imediatamente
- Armazenados como: `/remote/path/myfolder_bk_TIME1`, `/remote/path/myfolder_bk_TIME2`, ...
- `--period`: tempo em TIMEFORMAT (`1m12d3h`) ou expressão cron (`S M H D Mo DoW`)
- `--num-backups=N`: máximo de backups armazenados

**Listar:**
- `backup` — Lista
- `backup -l` — Info estendida (período, próximo agendado)
- `backup -h` — Histórico de backups

**Gerenciar:**
- `backup -d TAG|localpath` — Remover configuração
- `backup -a TAG|localpath` — Abortar backup em andamento
- `backup 4 --period=2h` — Alterar período
- `backup /path/folder --num-backups=1` — Alterar número máximo

**Monitoramento:** `watch mega-backup -lh`

---

## 12. Servidores FTP e WebDAV

### ftp — Servidor FTP
```
ftp [-d (--all | remotepath)] [ remotepath [--port=PORT] [--data-ports=BEGIN-END] [--public] [--tls --certificate=/path/to/certificate.pem --key=/path/to/certificate.key]] [--use-pcre]
```
> BETA. Apenas passive mode.

**Servir pasta:** `ftp /path/mega/folder`
**Streaming:** `ftp /path/to/myfile.mp4`
**Listar:** `ftp`
**Parâmetros:**
- `--port=PORT` — Porta (padrão: 4990)
- `--data-ports=BEGIN-END` — Range para dados passivos (padrão: 1500-1600)
- `--public` — Acesso externo (padrão: apenas localhost)
- `--tls` — FTPS com TLS
- `--certificate` / `--key` — Certificado e chave PEM
**Parar:** `ftp -d /path/mega/folder` ou `ftp -d --all`

### webdav — Servidor WebDAV
```
webdav [-d (--all | remotepath)] [ remotepath [--port=PORT] [--public] [--tls --certificate=/path/to/certificate.pem --key=/path/to/certificate.key]] [--use-pcre]
```
> BETA

**Servir pasta:** `webdav /path/mega/folder`
**Streaming:** `webdav /path/to/myfile.mp4`
**Listar:** `webdav`
**Parâmetros:**
- `--port=PORT` — Porta (padrão: 4443)
- `--public` — Acesso externo
- `--tls` — HTTPS com TLS
**Parar:** `webdav -d /path/mega/folder` ou `webdav -d --all`

**Notas:** Apenas um servidor por vez. Configuração da primeira localização serve para todas.

---

## 13. FUSE

> BETA. Apenas Linux.

### fuse-add — Criar montagem
```
fuse-add [--name=name] [--disabled] [--transient] [--read-only] localPath remotePath
```
- `--name` — Nome amigável
- `--read-only` — Apenas leitura
- `--transient` — Perdido no restart
- `--disabled` — Não ativar após adicionar

### fuse-show — Listar montagens
```
fuse-show [--only-enabled] [--disable-path-collapse] [[--limit=rowcount] | [name|localPath]]
```
Colunas: NAME, LOCAL_PATH, REMOTE_PATH, PERSISTENT (YES/NO), ENABLED (YES/NO)

### fuse-enable / fuse-disable — Ativar/Desativar
```
fuse-enable [--temporarily] (name|localPath)
fuse-disable [--temporarily] (name|localPath)
```

### fuse-config — Configurar
```
fuse-config [--name=name] [--enable-at-startup=yes|no] [--persistent=yes|no] [--read-only=yes|no] (name|localPath)
```

### fuse-remove — Remover
```
fuse-remove (name|localPath)
```
**Nota:** Deve estar desabilitado antes de remover.

### Cache FUSE
- Localizado em `$HOME/.megaCmd/fuse-cache`
- Arquivos baixados completamente antes de abrir (streaming não suportado)
- Cache limpo automaticamente

### Resolução de Problemas
"Transport endpoint is not connected":
```bash
fusermount -u /local/path/to/fuse/mountpoint
fusermount -u -z /local/path/to/fuse/mountpoint  # Se falhar
```

---

## 14. Flags Comuns

### Flags Transversais
| Flag | Descrição | Comandos |
|---|---|---|
| `--use-pcre` | Usar PCRE em paths | cp, mv, rm, ls, find, du, get, export, share, ftp, webdav, deleteversions |
| `--time-format=FORMAT` | Formato de data/hora | ls, find, export, share, backup, whoami, users, sync-issues, showpcr |
| `--path-display-size=N` | Tamanho fixo para paths | backup, du, sync, transfers, mediainfo |
| `--col-separator=X` | Separador de colunas | sync, transfers, sync-issues, fuse-show |
| `--output-cols=COLS` | Colunas a exibir | sync, transfers, sync-issues, fuse-show |

### Formatos de Data/Hora
- `RFC2822` — `Thu, 26 Apr 2018 11:20:09 +1200`
- `ISO6081` — `2018-04-26T11:20:09`
- `ISO6081_WITH_TIME` — `2018-04-26T11:20:09+12:00`
- `SHORT` — `26Apr2018 11:20:09`
- `SHORT_UTC` — `26Apr2018 11:20:09 UTC`
- `CUSTOM strftime` — Formato personalizado

### Expressões de Tempo
Unidades: `s` (seconds), `m` (minutes), `h` (hours), `d` (days), `M` (months), `y` (years)
Exemplo: `1m12d3h` = 1 mês, 12 dias e 3 horas

### Expressões Cron (backups)
Formato: `S M H D Mo DoW`
- `0 0 4 * * *` = todos os dias às 4:00 UTC
- `0 0 */2 * *` = a cada 2 horas
- `0 30 8 * * 1-5` = dias de semana às 8:30

### Formato de Tamanho
Unidades: `B`, `K`, `M`, `G`, `T`
Exemplos: `1M12k3B`, `-3G`

---

## 15. Códigos de Erro

| Código | Nome | Descrição |
|---|---|---|
| `0` | API_OK | Sucesso |
| `-1` | API_ARGS | Argumentos inválidos |
| `-2` | API_EACCESS | Acesso negado / permissão |
| `-3` | API_ECAPTCHA | Captcha requerido |
| `-4` | API_ETEMPUNAVAIL | Temporariamente indisponível |
| `-5` | API_ERATELIMIT | Rate limit excedido |
| `-6` | API_EFAILED | Operação falhou |
| `-7` | API_ETOOMANY | Muitas requisições simultâneas |
| `-8` | API_ERANGE | Fora dos limites |
| `-9` | API_EEXPIRED | Recurso expirado |
| `-10` | API_ENOENT | Não encontrado |
| `-11` | API_ECIRCULAR | Operação circular |
| `-12` | API_ENOACCESS | Sem acesso ao nó |
| `-13` | API_EEXIST | Já existe |
| `-14` | API_EINCOMPLETE | Incompleto |
| `-15` | API_EKEY | Chave inválida/alterada |
| `-16` | API_ESID | Sessão inválida |
| `-17` | API_EBLOCKED | Bloqueado |
| `-18` | API_EOVERQUOTA | Cota excedida |
| `-19` | API_ETEMPORARILYDISABLED | Temporariamente desativado |
| `-20` | API_EBUSINESSPASTDUE | Conta business vencida |
| `-21` | API_EPAYWALL | Paywall |

Use `errorcode NUM` para traduzir.

---

## 16. IPC — Protocolo de Comunicação

### TCP Socket (mega-execports — Python)
Cliente alternativo em `src/client/python/mega-execports`, porta 12300:

1. Conecta em `127.0.0.1:12300`
2. Envia comando como string (args separados por espaço)
3. Recebe 2 bytes: `socketOutId` (unsigned short)
4. Conecta em `127.0.0.1:12300 + socketOutId`
5. Recebe 4 bytes: `outCode` (int) — código de saída
6. Recebe o restante: `commandOutput` (string)
7. Se `outCode < 0`: exit `-outCode`; senão: exit `outCode`

### File Sockets (Unix)
Usado no Linux/macOS para comunicação entre `mega-exec` e `mega-cmd-server`.

### Named Pipes (Windows)
Usado no Windows.

---

## 17. Logging

### Níveis de Log
| Tipo | Label | Origem |
|---|---|---|
| MEGAcmd | `cmd` | Processamento de comandos |
| SDK | `sdk` | Engine, requisições, rede |

Níveis: FATAL → ERROR → WARNING → INFO → DEBUG → VERBOSE

### Comando log
```
log [-sc] level
```
- `-c` — Nível de log MEGAcmd
- `-s` — Nível de log SDK

### Verbosidade por Comando
Qualquer comando aceita: `-v` (Warnings), `-vv` (Debug), `-vvv` (Verbose)

### Verbosidade na Inicialização
```
MEGAcmdServer --debug         # MEGAcmd=DEBUG, SDK=DEFAULT
MEGAcmdServer --debug-full    # MEGAcmd=DEBUG, SDK=DEBUG
MEGAcmdServer --verbose       # MEGAcmd=VERBOSE, SDK=DEFAULT
MEGAcmdServer --verbose-full  # MEGAcmd=VERBOSE, SDK=VERBOSE
```
Ou via env: `MEGACMD_LOGLEVEL=FULLVERBOSE MEGAcmdServer`

### Logs JSON
Quando SDK log level = VERBOSE. Controlável com `MEGACMD_JSON_LOGS=0|1`.

### Logger Rotativo
Configurado via `megacmd.cfg`:
```
RotatingLogger:RotationType=Timestamp
RotatingLogger:CompressionType=Gzip
RotatingLogger:MaxFileMB=40.25
RotatingLogger:MaxFilesToKeep=20
RotatingLogger:MaxFileAgeSeconds=3600
RotatingLogger:MaxMessageBusMB=64.0
```

### Localização dos Logs
- Linux/macOS: `$HOME/.megaCmd/megacmdserver.log`
- Windows: `%LOCALAPPDATA%\MEGAcmd\.megaCmd\megacmdserver.log`

---

> Consulte `comando --help` para detalhes atualizados de cada comando.
