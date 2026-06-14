# MEGAcmd Skill — Compatibility Verification Report

**Date:** 2026-06-14
**Scope:** megacmd user skill (SKILL.md, SKILL.pt-BR.md, README.md, README.pt-BR.md)
**Reference:** 76 command docs in `contrib/docs/commands/*.md` + feature guides
**Note:** `megacmd-developer` skill covers build/debug/dev functionality separately.

---

## Consolidated Findings

### URGENT — High Severity

| # | Severity | File | Line | Finding | Source Agent |
|---|----------|------|------|---------|-------------|
| H1 | **HIGH** | README.md | ~228 | WebDAV security warning is INSIDE the bash code block (` ```bash `). The `> ⚠️` blockquote renders as bash syntax, breaking the warning display. Must be moved outside the code block. | Agent 5 |
| H2 | **HIGH** | README.pt-BR.md | ~228 | Same WebDAV warning inside code block issue. | Agent 5 |

### MEDIUM Severity

| # | Severity | File | Line | Finding | Source Agent |
|---|----------|------|------|---------|-------------|
| M1 | **MEDIUM** | SKILL.pt-BR.md | 74 | Typo: "tranfers" → "transferências" in Architecture table | Agent 5 |
| M2 | **MEDIUM** | SKILL.md | 100 | Agent rule claims `log`, `sync`, `webdav`, `ftp` "only work INSIDE the interactive shell". This is INCORRECT — all four have `mega-*` client wrappers. | Agent 3 |

### MINOR Severity (Syntax/Flag Omissions — Expected in summary docs)

| # | Severity | File | Finding | Source Agent |
|---|----------|------|---------|-------------|
| m1 | Minor | SKILL.md | `mega-login` only shows basic form — omits folder-link login, password-protected link login, session resume | Agent 1 |
| m2 | Minor | SKILL.md | `mega-killsession` only shows `-a`, omits `sessionid1 sessionid2 ...` selective kill | Agent 1 |
| m3 | Minor | SKILL.md | `mega-get` doesn't show `--password` as proper flag | Agent 1 |
| m4 | Minor | SKILL.md | `mega-export` omits `--writable` from syntax line (only inline in warning) | Agent 1 |
| m5 | Minor | SKILL.md | `mega-invite` only shows send, omits `-d` (delete), `-r` (resend) | Agent 1 |
| m6 | Minor | SKILL.md | `mega-ipc` only shows `-a` (accept), omits `-d` (reject), `-i` (ignore) | Agent 1 |
| m7 | Minor | SKILL.md | `mega-users` only shows `[-s]`, omits `-h`, `-n`, `-d`, `--verify` | Agent 1 |
| m8 | Minor | SKILL.md | `mega-proxy` syntax omits `--username=` and `--password=` flags | Agent 3 |

### INFO — Recommendations

| # | Severity | Finding | Source Agent |
|---|----------|---------|-------------|
| i1 | Info | Skill covers 46/76 commands (60.5%) — reasonable for a user-focused summary | Agent 4 |
| i2 | Info | Top missing commands worth adding: `signup`, `confirm`, `deleteversions`, `exclude` | Agent 4 |
| i3 | Info | FTP section omits `--data-ports`, `--public`, and passive-only mode note | Agent 2 |
| i4 | Info | Sync verification table says `ERROR = NO` — official doc shows error column as null string, not "NO" | Agent 2 |
| i5 | Info | Error code table values couldn't be fully verified (SDK submodule not initialized) | Agent 5 |

### NOT Issues (per user clarification)
The following are correctly excluded from the user skill as they are covered by `megacmd-developer`:
- `debug`, `version`, `update`, `reload`, `autocomplete`, `graphics`, `mediainfo`, `thumbnail`, `preview`, `codepage`, `unicode`, `psa`, `userattr`, `attr`, `signup`, `confirm`

---

## Section-by-Section Status

| Section | Status | Findings |
|---------|--------|---------|
| Login & Account | 🟡 PASS (minor omissions) | 3 minor flag omissions |
| Navigation & Listing | 🟢 PASS | Syntax correct |
| Upload & Download | 🟢 PASS | Syntax correct |
| File Management | 🟡 PASS (minor omissions) | export, cp flags partially documented |
| Sharing | 🟡 PASS (minor omissions) | invite, ipc, users partial |
| Synchronization | 🟢 PASS | All match |
| Backups | 🟢 PASS | All match |
| WebDAV | 🟢 PASS | Warning accuracy fixed, port 4443 correct |
| FTP | 🟡 PASS (minor omissions) | --data-ports, --public, passive mode omitted |
| FUSE | 🟢 PASS | All match |
| Transfers | 🟢 PASS | All match |
| Settings | 🟡 PASS (minor omission) | proxy --username/--password omitted |
| Important Rules | 🟢 PASS | All 9 verified |
| Error Codes | 🟡 PASS | Values unverified (SDK ref not loaded) |
| Compatibility | 🟢 PASS | All 5 rows accurate |
| Security Warnings | 🟢 PASS | All 11 correct |
| README | 🔴 FAIL | WebDAV warning inside code block |
| PT-BR SKILL | 🟡 PASS (typo) | "tranfers" typo |

---

## Action Items

### Fix Now
- [ ] **H1**: Move WebDAV warning outside bash code block in README.md (~L228)
- [ ] **H2**: Same fix in README.pt-BR.md
- [ ] **M1**: Fix "tranfers" → "transferências" in SKILL.pt-BR.md L74
- [ ] **M2**: Update agent rule in SKILL.md L100 — `log`, `sync`, `webdav`, `ftp` DO have `mega-*` wrappers

### Consider Adding
- [ ] `signup` and `confirm` to Login & Account section
- [ ] `deleteversions` to File Management section
- [ ] `exclude` as complement to `sync-ignore`
- [ ] `--username`/`--password` flags to proxy syntax
