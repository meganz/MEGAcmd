# Learnings - MEGAcmd Security Audit Remediation

## Skill Structure
- SKILL.md (418 lines) and SKILL.pt-BR.md (418 lines) are direct translations
- README.md (240 lines) and README.pt-BR.md (240 lines) are direct translations
- references/ contains pure command syntax - no findings there
- All 13 security findings are about documentation gaps, not code vulnerabilities

## Decision Log
- Warning level: Moderado (:warning: banner + contexto)
- Frontmatter: Restringir triggers
- PT-BR: Mirror EN + verificar
- find -delete: Warning leve
- Proxy: Warning generico

## Verificacao de Compatibilidade (14-Jun-2026)
3 inacuracies found and fixed:
1. CRITICAL: --writable 'expose entire account' -> 'grant write access to shared folder'
2. MEDIUM: 'links persist after deletion' -> removed (no official doc support)
3. LOW: FTP 'accessible beyond localhost depending on network' -> 'local-only by default; use --public'

All other warnings verified against official MEGAcmd docs (WEBDAV.md, FTP.md,
session.md, masterkey.md, export.md, proxy.md, share.md) - no other issues found.

## Final Status
All 16 executable tasks (1-12 + F1-F4) completed.
Boulder status: completed.
No remaining work.
The 57 unchecked boxes are nested acceptance criteria sub-checkboxes
within task descriptions - not executable tasks.

## Actions Taken
- FTP warning banner added to SKILL.md (EN) and SKILL.pt-BR.md (PT-BR) before `### FTP` header
- Addresses finding #13 (Medium, 94% confidence)
- Warning covers: plain-text exposure, port accessibility beyond localhost, TLS recommendation, service shutdown[2026-06-14] Orchestration Complete. All 12 implementation tasks + 4 verification tasks done. 13/13 findings resolved.
