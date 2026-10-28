# MEGAcmd Skill — Compatibility Verification Plan

## TL;DR

> **Quick Summary**: Cross-reference every command and feature described in the `megacmd` skill (SKILL.md) against the 76 official command docs + 5 feature guides from `contrib/docs/`, verifying accuracy, completeness, and consistency.
>
> **Deliverables**:
> - Verified command coverage map (skill commands vs project docs)
> - Gap analysis (commands missing from skill, skill errors found)
> - Warning accuracy check (no contradictions with project docs)
> - Final report with all issues found
>
> **Estimated Effort**: Large (76 command docs to cross-reference)
> **Parallel Execution**: YES — 5 verification agents + 1 synthesis wave
> **Critical Path**: All agents run in parallel → Synthesis → Report

---

## Context

### Original Request
"Faça uma verificação na skill em si e em todo o projeto do MEGAcmd para verificar se pode ter alguma funcionalidade que possa estar quebrada ou com problemas. ... Faça o planejamento para ser múltiplos agents fazendo as verificações pra constatar que tudo está de acordo com a skill e o projeto do MEGAcmd."

### Scope
- **Skill file**: `~/.config/opencode/skills/megacmd/SKILL.md` (456 lines, 6 sections of commands)
- **Project reference**: `contrib/docs/commands/*.md` (76 command docs) + `contrib/docs/*.md` (5 feature guides: WEBDAV.md, FTP.md, FUSE.md, BACKUPS.md, DEBUG.md)
- **Also check**: SKILL.pt-BR.md, README.md, README.pt-BR.md for consistency

### Prior Work
- Security audit warnings already verified against docs (3 fixes applied)
- Full list of 76 commands confirmed via glob

---

## Work Objectives

### Core Objective
Verify that every command and feature described in the `megacmd` skill accurately reflects the official MEGAcmd documentation, identifying any inaccuracies, missing commands, or contradictions.

### Verification Dimensions
- **Accuracy**: Does the skill describe command syntax correctly?
- **Completeness**: Are important commands or flags missing?
- **Consistency**: Do EN and PT-BR files agree?
- **Warning integrity**: Do added security warnings contradict official docs?

### Definition of Done
- [ ] All 5 verification agents complete their analysis
- [ ] Issues categorized (Critical/Major/Minor/Info)
- [ ] Final report saved to `.omo/evidence/compatibility-report.md`
- [ ] Any critical inaccuracies flagged for immediate fix

---

## Verification Strategy

### Test Decision
- **Infrastructure**: None (documentation cross-reference)
- **Automated tests**: None
- **Agent QA**: Each agent validates specific claims against official docs
- **Verification**: Synthesis agent cross-checks for conflicts between agent findings

---

## Execution Strategy

### Parallel Verification Agents (5 agents, all run simultaneously)

```
Run ALL in parallel (no dependencies between agents):

Agent A (Essential Commands — 35 commands):
├── Login & Account (10): login, logout, whoami, df, masterkey, passwd, session, 
│   killsession, signup, confirm
├── Navigation & Listing (8): ls, find, du, cd, pwd, mount, tree, lcd
├── Upload & Download (4): put, get, cat, preview
├── File Management (7): mkdir, cp, mv, rm, export, import, attr
└── Sharing (5): share, invite, ipc, users, showpcr

Agent B (Feature Modules — 20 docs):
├── Sync (4): sync, sync-ignore, sync-issues, sync-config
├── Backups (1): backup + BACKUPS.md
├── WebDAV (2): webdav + WEBDAV.md
├── FTP (2): ftp + FTP.md
├── FUSE (7): fuse-add, fuse-config, fuse-enable, fuse-disable, fuse-remove, 
│   fuse-show, FUSE.md
├── Transfers (2): transfers, speedlimit
└── logs/debug (2): log, debug + DEBUG.md

Agent C (Settings & Config — 12 commands):
├── Settings (5): https, proxy, configure, permissions, codepage
├── Account/Session (4): session, killsession, logout, login (overlap with A)
├── Diagnostics (2): errorcode, whoami (overlap)
└── Misc (3): version, help, unicode, graphics, mediainfo, thumbnail, reload

Agent D (Gap Analysis — cross-reference all 76 docs):
├── List ALL commands from contrib/docs/commands/*.md
├── Tag each as: SKILL_MENTIONED / SKILL_MISSING / SKILL_PARTIAL
├── Flag important missing commands (e.g., signup, confirm, reload, tree)
└── Count: skill describes ~30 of 76 = 40% coverage — is this acceptable?

Agent E (Warning & PT-BR Accuracy):
├── Verify ALL security warnings added don't contradict official docs
├── Check SKILL.pt-BR.md is accurate (not just literal translation)
├── Check README.md and README.pt-BR.md accuracy
└── Verify Important Rules are correct per docs

Synthesis Wave (after all 5 agents complete):
├── Collect all findings from A-E
├── Deduplicate and prioritize
├── Generate final compatibility report
└── Flag critical issues for immediate fix
```

### Agent Dispatch Summary

| Agent | Tasks | Profile | Estimated Time |
|-------|-------|---------|----------------|
| A | 35 essential command comparisons | `quick` | 5-8 min |
| B | 20 feature module comparisons | `quick` | 5-8 min |
| C | 12 setting commands + overlap check | `quick` | 3-5 min |
| D | Gap analysis (all 76 docs) | `unspecified-low` | 5-8 min |
| E | Warning + PT-BR accuracy | `unspecified-low` | 3-5 min |
| Synthesis | Merge, deduplicate, report | `quick` | 3-5 min |

---

## TODOs

- [x] 1. **Verify Essential Commands** — Check all 35 commands from the skill's Essential Commands section against their official docs at `contrib/docs/commands/`. Verify syntax, flags, descriptions.

  **What to do**:
  - Read the Essential Commands sections from SKILL.md (Login, Navigation, Upload/Download, File Management, Sharing)
  - For each command listed, read the corresponding `contrib/docs/commands/{cmd}.md`
  - Check: command name matches, syntax matches, options/flags match, description matches
  - Flag any discrepancies

  **References**: `contrib/docs/commands/{login,logout,whoami,df,masterkey,passwd,session,killsession,signup,confirm,ls,find,du,cd,pwd,mount,tree,lcd,put,get,cat,preview,mkdir,cp,mv,rm,export,import,attr,share,invite,ipc,users,showpcr}.md`

- [x] 2. **Verify Feature Modules** — Check all feature sections (Sync, Backups, WebDAV, FTP, FUSE, Transfers, Diagnostics) against their official docs.

  **What to do**:
  - Read the feature sections from SKILL.md
  - Compare against `sync.md`, `sync-ignore.md`, `sync-issues.md`, `sync-config.md`, `backup.md`, `BACKUPS.md`, `webdav.md`, `WEBDAV.md`, `ftp.md`, `FTP.md`, `fuse-*.md`, `FUSE.md`, `transfers.md`, `speedlimit.md`, `log.md`, `debug.md`, `DEBUG.md`
  - Check: command syntax matches, feature descriptions accurate, beta labels match, port numbers correct, limit values match

  **References**: `contrib/docs/commands/sync.md`, `sync-ignore.md`, `sync-issues.md`, `sync-config.md`, `backup.md`, `webdav.md`, `ftp.md`, `fuse-add.md`, `fuse-config.md`, `fuse-enable.md`, `fuse-disable.md`, `fuse-remove.md`, `fuse-show.md`, `transfers.md`, `speedlimit.md`, `log.md`, `debug.md`, `contrib/docs/{BACKUPS.md,WEBDAV.md,FTP.md,FUSE.md,DEBUG.md}`

- [x] 3. **Verify Settings & Config Commands** — Check Settings section and remaining utility commands.

  **What to do**:
  - Read Settings section from SKILL.md (https, proxy, configure, permissions, log)
  - Compare against `https.md`, `proxy.md`, `configure.md`, `permissions.md`, `log.md`, `codepage.md`, `errorcode.md`, `version.md`, `help.md`, `unicode.md`, `graphics.md`, `mediainfo.md`, `reload.md`
  - Check: syntax accuracy, flag descriptions, configuration behavior

  **References**: `contrib/docs/commands/{https,proxy,configure,permissions,codepage,errorcode,version,help,unicode,graphics,mediainfo,reload}.md`

- [x] 4. **Gap Analysis — Command Coverage** — Cross-reference all 76 project command docs against the skill to find missing commands.

  **What to do**:
  - List ALL 76 files from `contrib/docs/commands/*.md`
  - For each, check if the command appears in SKILL.md
  - Tag: MENTIONED / MISSING / PARTIAL
  - Evaluate: which missing commands are important enough to add?
  - Commands to flag: `signup`, `confirm`, `reload`, `tree`, `graphics`, `preview`, `thumbnail`, `mediainfo`, `showpcr`, `attr`, `cancel`, `deleteversions`, `userattr`, `psa`, `autocomplete`, `lcd`, `lpwd`, `unicode`, `codepage`

  **References**: All `contrib/docs/commands/*.md` files, SKILL.md content

- [x] 5. **Warning & PT-BR Accuracy** — Verify security warnings against official docs and check PT-BR/skill consistency.

  **What to do**:
  - Read all 9 ⚠️ warning blocks in SKILL.md
  - For each, verify no technical claim contradicts official docs
  - Read SKILL.pt-BR.md and confirm translations are accurate
  - Read README.md and README.pt-BR.md for accuracy
  - Check Important Rules (9 rules) against docs
  - Verify Error Codes table against `errorcode.md`

  **References**: `contrib/docs/commands/errorcode.md`, SKILL.md, SKILL.pt-BR.md, README.md, README.pt-BR.md

- [x] 6. **Synthesis & Report** — Collect all findings, deduplicate, prioritize, and final report.

  **What to do**:
  - Collect output from agents A through E
  - Deduplicate overlapping findings
  - Classify by severity: Critical / Major / Minor / Info
  - For each issue, note: what's wrong, where (file + line), what the correct value should be
  - Generate final report to `.omo/evidence/compatibility-report.md`
  - Flag Critical issues for immediate fix

---

## Final Verification Wave

- [x] F1. **Report Review** — `oracle`
  Read the compatibility report. Verify all issues are correctly classified. Check for any contradictions between agent findings. Confirm all 76 command docs were referenced.
  Output: `Commands [N/N verified] | Findings [N] | Critical [N] | VERDICT: APPROVE/REJECT`

---

## Success Criteria

### Verification Commands
```bash
# Count commands in skill
grep -c '^\| `mega-' SKILL.md  # Commands table entries

# Count command docs
ls contrib/docs/commands/*.md | wc -l  # Should be 76

# Verify warnings dont reference non-existent flags
grep -r '`--' SKILL.md | sort -u  # All flags should exist in --help
```

### Final Checklist
- [ ] All 76 command docs cross-referenced against skill
- [ ] Every skill command entry verified for accuracy
- [ ] Gap analysis complete (missing commands documented)
- [ ] Security warnings verified against project docs
- [ ] EN/PT-BR consistency confirmed
- [ ] Critical issues flagged for immediate fix
- [ ] Final report saved to `.omo/evidence/compatibility-report.md`

---

## Classificação de Issues

| Severidade | Definição |
|------------|-----------|
| **Critical** | Skill descreve flag/sintaxe que não existe no projeto, ou erra comportamento fundamental |
| **Major** | Skill omite flag importante, descrição enganosa, versão PT-BR com erro de tradução |
| **Minor** | Flag descrita mas sem detalhes, sintaxe parcial, exemplo simplificado demais |
| **Info** | Sugestão de melhoria, comando não documentado na skill mas útil, nota de rodapé |
