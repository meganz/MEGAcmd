# MEGAcmd Skill — Security Audit Remediation Plan

## TL;DR

> **Quick Summary**: Analyze and remediate all 13 security findings from the ClawHub SkillSpector audit against the 6 MEGAcmd skill files, adding proportionate warnings and restricting vague activation triggers.
>
> **Deliverables**:
> - All 13 findings mapped to exact file:line locations
> - Warning blocks (⚠️ banner + contexto) added to SKILL.md, SKILL.pt-BR.md, README.md, README.pt-BR.md
> - Frontmatter `description` refined for more specific triggers
> - Verification that each change addresses the finding
>
> **Estimated Effort**: Medium
> **Parallel Execution**: YES — 4 waves + 1 final verification wave
> **Critical Path**: Task 1 → Task 2 → Task 3-13 → Wave FINAL

---

## Context

### Original Request
"Analisa a skill megacmd e observa no site https://clawhub.ai/alefsanderribeiro/megacmd/security-audit que ela foi feito uma verificação de segurança dessa skill. Faça o planejamento para poder verificar todos os apontamentos da análise de segurança para verificar a possibilidade de ajustar na skill."

### Interview Summary
**Key Discussions**:
- Skill location: `/home/alefsander/.config/opencode/skills/megacmd/` — 6 files total
- Files affected: SKILL.md (418 lines), SKILL.pt-BR.md (418 lines), README.md (240 lines), README.pt-BR.md (240 lines), references/* (clean — no changes needed)
- 13 findings from ClawHub SkillSpector audit (12 Medium, 1 High — WebDAV/FTP)
- VirusTotal: 64/64 clean — no malware concerns
- Warning level: **Moderado** (⚠️ banner + contexto) — caixas de aviso em destaque com explicação do risco
- Frontmatter description: **Sim, restringir** — gatilhos mais específicos
- PT-BR findings: **Mirror EN + verificar** — aplicar mesmas mudanças, verificar independentemente
- find -delete: **Warning leve** — aviso único acima do bloco de troubleshooting
- Proxy credentials: **Warning genérico** — nota sobre evitar credenciais inline

**Research Findings**:
- Security audit URL: https://clawhub.ai/alefsanderribeiro/megacmd/security-audit
- SkillSpector by NVIDIA — static analysis of SKILL.md documentation patterns
- All findings are about missing safety guidance, not code vulnerabilities
- Existing warnings in SKILL.md "Important Rules" section (lines 376-386) partially address some concerns but are buried

### Metis Review
**Identified Gaps** (addressed):
- Question 1 (warning level): Resolved — Moderado (⚠️ banner + contexto)
- Question 2 (frontmatter): Resolved — restrict triggers
- Question 3 (PT-BR): Resolved — mirror EN + verify
- Question 4 (find -delete in troubleshooting): Resolved — light warning
- Question 5 (proxy credentials): Resolved — generic warning (no inline credentials currently shown)
- Question 6 (test strategy): Resolved — none (markdown) + grep verification + diff review

---

## Work Objectives

### Core Objective
Analyze all 13 ClawHub security findings against the 6 MEGAcmd skill files and produce a guardrailed remediation plan with executable tasks for adding proportionate safety warnings.

### Concrete Deliverables
- `.omo/plans/megacmd-security-audit-review.md` — this plan file
- Warning blocks added to SKILL.md, SKILL.pt-BR.md, README.md, README.pt-BR.md
- Frontmatter `description` refined in both SKILL.md and SKILL.pt-BR.md
- Resolution table mapping each of the 13 findings to specific changes

### Definition of Done
- [ ] All 13 findings mapped to file:line ranges and resolution actions defined
- [ ] Each warning block can be verified via `grep` at its expected location
- [ ] EN/PT-BR consistency confirmed via cross-file comparison
- [ ] `git diff --stat` shows only the 4 main files changed
- [ ] No changes to `references/` directory

### Must Have
- All 13 findings must have a documented resolution approach before any editing begins
- Warnings must be additive (preserve all existing command syntax and examples)
- EN and PT-BR files must be modified in lockstep
- Frontmatter `description` must be refined for more specific triggers

### Must NOT Have (Guardrails)
- Do NOT modify MEGAcmd source code, `megacmd-developer` skill, or AGENTS.md
- Do NOT restructure, reformat, or reorganize the skill files
- Do NOT remove or alter any legitimate command syntax examples
- Do NOT create new files (SECURITY.md, SAFETY.md, etc.)
- Do NOT modify `references/` directory
- Do NOT add separate security sections — warnings should be contextual near each command

---

## Verification Strategy (MANDATORY)

> **ZERO HUMAN INTERVENTION** — ALL verification is agent-executed.

### Test Decision
- **Infrastructure**: N/A (markdown documentation — no code to test)
- **Automated tests**: None
- **Agent QA**: `grep` for each added warning at expected location + `diff` for EN/PT-BR consistency

### QA Policy
Every task MUST include agent-executed QA verification (see TODO template below).
Evidence saved to `.omo/evidence/task-{N}-{scenario-slug}.{ext}`.

- **Warning presence**: `grep "expected text" file.md` to confirm warning exists
- **EN/PT-BR consistency**: `diff <(sed...)` to confirm structural parity
- **No unintended changes**: `git diff --stat` to confirm only targeted files
- **Frontmatter integrity**: `grep "^description:"` to verify frontmatter change

---

## Execution Strategy

### Parallel Execution Waves

```
Wave 1 (Foundation):
├── Task 1: Map findings to exact file:line ranges [quick]
└── Task 2: Define warning templates + resolution table [quick]

Wave 2 (P0 — WebDAV/FTP High severity, MAX PARALLEL):
├── Task 3: WebDAV network exposure warnings (EN + PT-BR) [quick]
├── Task 4: FTP network exposure warnings (EN + PT-BR) [quick]

Wave 3 (P1 — Credential exposure + Triggers, MAX PARALLEL):
├── Task 5: mega-login credential warnings (EN + PT-BR) [quick]
├── Task 6: Session ID + Master Key warnings (EN + PT-BR) [quick]
├── Task 7: Proxy credential generic warning (EN + PT-BR) [quick]
├── Task 8: Frontmatter description restriction (EN + PT-BR) [quick]

Wave 4 (P2+P3+P4 — Remaining findings, MAX PARALLEL):
├── Task 9: README Activation section trigger refinement (EN + PT-BR) [quick]
├── Task 10: find -delete light warning in troubleshooting (EN + PT-BR) [quick]
├── Task 11: Sharing/Export data disclosure warnings (EN + PT-BR) [quick]
└── Task 12: Security overview banner at top of SKILL.md (EN + PT-BR) [quick]

Wave FINAL (Verification — 4 parallel reviews):
├── Task F1: Resolution table audit (oracle)
├── Task F2: grep verification of all warnings
├── Task F3: EN/PT-BR consistency check
└── Task F4: Final report — resolved vs pending findings
-> Present results -> Get explicit user okay

Critical Path: Task 1 → Task 2 → Tasks 3-12 → Wave FINAL
Parallel Speedup: ~60% faster than sequential
Max Concurrent: 4 (Waves 2-4)
```

### Dependency Matrix

- **1**: None → 2-12
- **2**: 1 → 3-12
- **3-12**: 2 → FINAL
- **F1-F4**: 3-12 → user okay

### Agent Dispatch Summary

- **Wave 1**: 2 tasks — `quick` (mapping + templating)
- **Wave 2**: 2 tasks — `quick` (WebDAV/FTP warnings)
- **Wave 3**: 4 tasks — `quick` (credential + trigger changes)
- **Wave 4**: 4 tasks — `quick` (remaining warnings + banners)
- **FINAL**: 4 tasks — `oracle` + `unspecified-low` (verification)

---

## TODOs
---

### Wave FINAL - Verification (after ALL implementation tasks)

---

- [ ] F1. **Resolution Table Audit** - `oracle`
  Read the resolution table from working notes. For each finding: verify the associated warning was actually added to the file (read file, grep for expected text). Check all 13 findings are addressed. Verify no warning was added for a different finding than intended.
  Output: `Findings [N/N addressed] | All warnings matching expected locations [PASS/FAIL] | VERDICT: APPROVE/REJECT`

- [ ] F2. **grep Verification of All Warnings** - `unspecified-low`
  For each of the 13 findings, execute `grep` to confirm the warning text exists at the expected location in the file. Log each grep command and its output. Flag any missing warnings.
  Output: `Warnings [N/N found] | Missing [N] | VERDICT`

- [ ] F3. **EN/PT-BR Consistency Check** - `unspecified-low`
  For each section modified, compare EN and PT-BR files to confirm structural parity: same sections have warnings, warning levels match (banner to banner, light to light), no warning exists in EN but missing in PT-BR, translation is accurate in meaning.
  Output: `Sections [N/N consistent] | Missing in PT-BR [N] | Missing in EN [N] | VERDICT`

- [ ] F4. **Final Report** - `quick`
  Compile a report with: resolution table with each of 13 findings and its status (Resolved/Partial/Unresolved), files modified and summary of changes, evidence paths for each verification, any remaining gaps or recommendations.
  Output: Markdown report saved to `.omo/evidence/final-report.md`

---

## Commit Strategy

- **Commit 1** (Tasks 1-2): `docs(skill): map 13 security findings and define warning templates`
- **Commit 2** (Tasks 3-4): `docs(skill): add network exposure warnings to WebDAV/FTP sections`
- **Commit 3** (Tasks 5-8): `docs(skill): add credential exposure warnings for login, session, master key, proxy`
- **Commit 4** (Tasks 9-12): `docs(skill): refine triggers, add warnings for delete/sharing/security banner`

---

## Success Criteria

### Verification Commands
```bash
# 1. Verify all warnings present
grep -c ":warning:" SKILL.md  # Should be >= 8
grep -c ":warning:" README.md  # Should be >= 2

# 2. Verify frontmatter change
grep "^description:" SKILL.md  # Should show new, more specific text

# 3. Verify no "or similar" remains
grep -n "or similar\|ou similar" README.md README.pt-BR.md || echo "CLEAN"

# 4. Verify EN/PT-BR structural parity
for section in "WebDAV" "FTP" "mega-login" "mega-export" "mega-share" "find.*-delete"; do
  echo "Section: $section"
  grep -c ":warning:" SKILL.md
  grep -c ":warning:" SKILL.pt-BR.md
done
```

### Final Checklist
- [ ] All 13 findings addressed with warnings
- [ ] All frontmatter descriptions refined
- [ ] "or similar" removed from activation text
- [ ] No changes to references/ directory
- [ ] No command syntax altered or removed
- [ ] EN and PT-BR consistent
- [ ] git diff shows only targeted changes
- [ ] Final report generated with resolution table
---

### Wave 4 - P2+P3+P4: Remaining Findings

---

- [ ] 9. Refine README Activation section triggers

  **What to do**:
  - Update the Activation section in README.md (lines ~143-145) and README.pt-BR.md
  - Remove "or similar" catch-all - replace with specific, exhaustive examples
  - List concrete trigger phrases: MEGAcmd, MEGA.nz, upload to MEGA, download from MEGA, sync with MEGA, backup to MEGA, MEGA link
  - Keep enough breadth but remove the open-ended "or similar"

  **Must NOT do**:
  - Do NOT remove activation instructions entirely (skill still needs to be discoverable)

  **Recommended Agent Profile**:
  - **Category**: `quick` - Targeted text edit, ~5 lines

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 10, 11, 12)
  - **Blocks**: Final verification
  - **Blocked By**: Task 8 (should be consistent with frontmatter change)

  **References**:
  - README.md lines ~143-145 (Activation section)
  - README.pt-BR.md lines ~143-145
  - Security audit findings #1 (Medium, 88%) and #3 (Medium, 94%)

  **Acceptance Criteria**:
  - [ ] "or similar" / "ou similar" removed from both README files
  - [ ] Specific trigger phrases listed
  - [ ] Activation still possible for all legitimate uses

  **QA Scenarios**:
  ```
  Scenario: Verify trigger refinement
    Tool: Bash
    Steps:
      1. grep "or similar" README.md - no matches (exit 1 expected)
      2. grep "force activation" -A5 README.md - specific phrases present
    Expected Result: No broad catch-alls, specific triggers only
    Evidence: .omo/evidence/task-9-triggers.txt
  ```

  **Evidence to Capture**:
  - [ ] grep results

  **Commit**: YES (groups with Tasks 10, 11, 12)
  - Message: `docs(skill): refine triggers, add warnings for delete/sharing/security banner`
  - Files: `README.md`, `README.pt-BR.md`, `SKILL.md`, `SKILL.pt-BR.md`

- [ ] 10. Add light warning for find -delete in troubleshooting

  **What to do**:
  - Add a single :warning: warning line above the Action Plan code block in the Quick Diagnostics section
  - Location: SKILL.md before line ~339 (action plan block), SKILL.pt-BR.md similarly
  - Warning text: `> :warning: The following commands delete local files without confirmation. Verify paths are correct before running.`
  - Light touch because user is troubleshooting a stuck sync and already frustrated

  **Must NOT do**:
  - Do NOT add per-line warnings inside the action plan code block
  - Do NOT add friction comments to a troubleshooting guide

  **Recommended Agent Profile**:
  - **Category**: `quick` - Single line addition at known location

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 9, 11, 12)
  - **Blocks**: Final verification
  - **Blocked By**: Tasks 1, 2

  **References**:
  - SKILL.md lines ~337-354 (Action plan for stuck sync with find -delete)
  - SKILL.pt-BR.md lines ~337-354
  - Security audit finding #9 (Medium, 91%)

  **Acceptance Criteria**:
  - [ ] Warning line added before action plan block in SKILL.md
  - [ ] Same in SKILL.pt-BR.md

  **QA Scenarios**:
  ```
  Scenario: Verify find -delete warning
    Tool: Bash
    Steps:
      1. grep -B2 "find.*-delete" SKILL.md - check for warning
    Expected Result: Warning present before find -delete commands
    Evidence: .omo/evidence/task-10-find-delete-warning.txt
  ```

  **Evidence to Capture**:
  - [ ] grep output

  **Commit**: NO (groups with Task 9)

- [ ] 11. Add data disclosure warnings to sharing/export sections

  **What to do**:
  - Add :warning: banner warnings to two locations in SKILL.md and SKILL.pt-BR.md:
    - **Export section** (before line ~139): Warning about public link creation exposing data to third parties, best practice to use passwords and set expiration
    - **Sharing section** (before line ~147): Warning about folder sharing giving external access, recommend minimum necessary permission level
  - Strengthen existing Important Rules #6 (`--writable` links expose account) if insufficient

  **Must NOT do**:
  - Do NOT remove any sharing or export commands

  **Recommended Agent Profile**:
  - **Category**: `quick` - Two targeted warning blocks

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 9, 10, 12)
  - **Blocks**: Final verification
  - **Blocked By**: Tasks 1, 2

  **References**:
  - SKILL.md lines ~139-142 (export commands)
  - SKILL.md lines ~144-151 (sharing commands)
  - SKILL.md line ~383 (Important Rules #6 about --writable)
  - Security audit finding #6 (Medium, 83%)

  **Acceptance Criteria**:
  - [ ] Warning added before export section in both files
  - [ ] Warning added before sharing section in both files
  - [ ] Important Rules #6 verified as sufficient or strengthened

  **QA Scenarios**:
  ```
  Scenario: Verify sharing/export warnings
    Tool: Bash
    Steps:
      1. grep -B5 "mega-export" SKILL.md - check for warning
      2. grep -B5 "mega-share" SKILL.md - check for warning
    Expected Result: Warnings present before both sections
    Evidence: .omo/evidence/task-11-sharing-warnings.txt
  ```

  **Evidence to Capture**:
  - [ ] grep output for both sections

  **Commit**: NO (groups with Task 9)

- [ ] 12. Add security overview banner to SKILL.md top section

  **What to do**:
  - Add a :warning: security overview banner early in SKILL.md (after "When NOT to use" section ~line 40, before Prerequisites) and SKILL.pt-BR.md
  - The banner should summarize: this skill can perform powerful operations, agents should require explicit user confirmation before login/deleting/syncing/sharing/network services, credential safety
  - 4-6 lines maximum - not a full security section, just a contextual reminder
  - This addresses findings #2 (Medium, 84%) and #4 (Medium, 91%)

  **Must NOT do**:
  - Do NOT add a separate Security section
  - Do NOT make warnings so aggressive the skill becomes unusable

  **Recommended Agent Profile**:
  - **Category**: `quick` - Single banner addition, standardized text

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 9, 10, 11)
  - **Blocks**: Final verification
  - **Blocked By**: Tasks 1, 2

  **References**:
  - SKILL.md lines ~40-44 (between "When NOT to use" and "Prerequisites")
  - Security audit findings #2 (Medium, 84%) and #4 (Medium, 91%)

  **Acceptance Criteria**:
  - [ ] Banner added between "When NOT to use" and "Prerequisites" in SKILL.md
  - [ ] Same in SKILL.pt-BR.md
  - [ ] Banner covers: powerful operations warning, confirmation requirement, credential safety

  **QA Scenarios**:
  ```
  Scenario: Verify security overview banner
    Tool: Bash
    Steps:
      1. grep -A10 "Prerequisites" SKILL.md - check for warning before section
    Expected Result: Banner present before Prerequisites section
    Evidence: .omo/evidence/task-12-banner.txt
  ```

  **Evidence to Capture**:
  - [ ] grep output with banner text

  **Commit**: NO (groups with Task 9)
---

### Wave 3 - P1: Credential Exposure + Triggers

---

- [ ] 5. Add credential warnings to mega-login section

  **What to do**:
  - Add :warning: banner warning before the Login and Account command table in SKILL.md (before line ~100) and SKILL.pt-BR.md
  - Warnings must cover: risks of passing credentials on the command line (shell history, process listings, agent telemetry, audit logs), recommendation to use 2FA, alternative to login interactively without password argument, note that mega-login syntax is shown for reference

  **Must NOT do**:
  - Do NOT remove or alter the mega-login command syntax
  - Do NOT remove examples showing the syntax

  **Recommended Agent Profile**:
  - **Category**: `quick` - Standardized warning block, no complex logic

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 6, 7, 8)
  - **Blocks**: Final verification
  - **Blocked By**: Tasks 1, 2

  **References**:
  - SKILL.md lines ~100-110 (Login and Account table, includes mega-login at line 103)
  - SKILL.pt-BR.md lines ~100-110
  - Security audit findings #5 (Medium, 97%) and #8 (Medium, 95%)
  - README.md lines ~175-179 (interaction modes with login example)

  **Acceptance Criteria**:
  - [ ] Banner warning before Login section in SKILL.md + SKILL.pt-BR.md
  - [ ] Same or contextual warning in README.md + README.pt-BR.md where login examples appear

  **QA Scenarios**:
  ```
  Scenario: Verify login credential warnings
    Tool: Bash
    Steps:
      1. grep -B5 "mega-login" SKILL.md - check for warning text
      2. Repeat for PT-BR
    Expected Result: Warning text present before mega-login
    Evidence: .omo/evidence/task-5-login-warnings.txt
  ```

  **Evidence to Capture**:
  - [ ] Warning presence in all 4 files

  **Commit**: YES (groups with Tasks 6, 7, 8)
  - Message: `docs(skill): add credential exposure warnings for login, session, master key, proxy`
  - Files: `SKILL.md`, `SKILL.pt-BR.md`, `README.md`, `README.pt-BR.md`

- [ ] 6. Add session ID and master key warnings

  **What to do**:
  - Add :warning: banner warnings to two locations:
    - **Session ID**: Before mega-session command (SKILL.md line ~109, SKILL.pt-BR.md line ~109) - warn that session tokens are bearer credentials, exposure enables account takeover, recommend not sharing session output
    - **Master Key**: Before mega-masterkey section (SKILL.md line ~107, SKILL.pt-BR.md line ~107) - warn that master key is a recovery secret, recommend storing encrypted with restricted file permissions
  - Strengthen existing Important Rules #3 (Master Key) if current text is insufficient

  **Must NOT do**:
  - Do NOT remove the mega-session or mega-masterkey commands

  **Recommended Agent Profile**:
  - **Category**: `quick` - Two targeted warning blocks, pre-defined template

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 5, 7, 8)
  - **Blocks**: Final verification
  - **Blocked By**: Tasks 1, 2

  **References**:
  - SKILL.md lines ~107-110 (mega-masterkey, mega-session)
  - SKILL.md lines ~380-381 (Important Rules #3 about master key)
  - Security audit findings #10 (Medium, 92% - session ID) and #11 (Medium, 95% - master key)

  **Acceptance Criteria**:
  - [ ] Session ID warning in both SKILL.md and SKILL.pt-BR.md
  - [ ] Master key warning in both SKILL.md and SKILL.pt-BR.md
  - [ ] Important Rules #3 strengthened if current text is insufficient

  **QA Scenarios**:
  ```
  Scenario: Verify session and master key warnings
    Tool: Bash
    Steps:
      1. grep -A10 "mega-session" SKILL.md - check for warning
      2. grep -A10 "mega-masterkey" SKILL.md - check for warning
    Expected Result: Both commands have preceding warnings
    Evidence: .omo/evidence/task-6-session-masterkey.txt
  ```

  **Evidence to Capture**:
  - [ ] grep output for both warnings

  **Commit**: NO (groups with Task 5)

- [ ] 7. Add proxy credential generic warning

  **What to do**:
  - Add a generic :warning: warning note in the Settings section (SKILL.md lines ~266-268, SKILL.pt-BR.md)
  - Warning: If using proxy credentials, avoid passing them inline on the command line. Prefer environment variables or configuration files that are not captured by shell history or process listings.
  - Current example shows `mega-proxy URL|--auto|--none` with no inline credentials - warning is about inherent capability, not documented pattern

  **Must NOT do**:
  - Do NOT add credentials to the existing proxy example

  **Recommended Agent Profile**:
  - **Category**: `quick` - Single generic warning note

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 5, 6, 8)
  - **Blocks**: Final verification
  - **Blocked By**: Tasks 1, 2

  **References**:
  - SKILL.md line ~268 (mega-proxy command)
  - Security audit finding #12 (Medium, 90% - proxy credentials)

  **Acceptance Criteria**:
  - [ ] Warning added near mega-proxy in SKILL.md
  - [ ] Same in SKILL.pt-BR.md

  **QA Scenarios**:
  ```
  Scenario: Verify proxy warning exists
    Tool: Bash
    Steps:
      1. grep -A5 "mega-proxy" SKILL.md - check for warning
    Expected Result: Warning present near proxy command
    Evidence: .omo/evidence/task-7-proxy-warning.txt
  ```

  **Evidence to Capture**:
  - [ ] grep output

  **Commit**: NO (groups with Task 5)

- [ ] 8. Restrict frontmatter description for specific triggers

  **What to do**:
  - Update the `description:` field in both SKILL.md (lines ~3-6) and SKILL.pt-BR.md (lines ~3-6)
  - Goal: Make more specific without breaking legitimate activation
  - Strategy: Remove vague catch-alls, use action-oriented phrases that describe what the agent WILL DO, not what is possible
  - Keep all listed capabilities but phrase them more concretely

  **Must NOT do**:
  - Do NOT make the description so narrow that legitimate MEGAcmd operations fail to activate
  - Do NOT remove any functionality from the description

  **Recommended Agent Profile**:
  - **Category**: `quick` - Brief frontmatter edit

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Tasks 5, 6, 7)
  - **Blocks**: Task 9 (README activation section should be consistent)
  - **Blocked By**: Tasks 1, 2

  **References**:
  - SKILL.md lines ~2-6 (frontmatter description)
  - SKILL.pt-BR.md lines ~2-6
  - Security audit findings #1 (Medium, 88%) and #3 (Medium, 94%)

  **Acceptance Criteria**:
  - [ ] New description is more specific (remove vague patterns, use concrete actions)
  - [ ] Both EN and PT-BR updated consistently
  - [ ] All existing capabilities preserved in description

  **QA Scenarios**:
  ```
  Scenario: Verify frontmatter change
    Tool: Bash
    Steps:
      1. grep "^description:" SKILL.md - show new text
      2. Confirm no vague patterns remain
    Expected Result: Description is specific and action-oriented
    Evidence: .omo/evidence/task-8-frontmatter.txt
  ```

  **Evidence to Capture**:
  - [ ] grep output showing new description

  **Commit**: NO (groups with Task 5)
---

### Wave 2 - P0: WebDAV/FTP Network Exposure (HIGH, 95% confidence)

---

- [ ] 3. Add network exposure warnings to WebDAV section

  **What to do**:
  - Add prominent :warning: banner warnings to the WebDAV section in SKILL.md (lines ~209-216) and SKILL.pt-BR.md (lines ~209-216)
  - Warnings must cover: risk of exposing MEGA content over network, distinction between localhost-only and `--public` flag, warning about unattended services and port exposure, recommendation to use `--tls` with proper certificates
  - Also update README.md Quick Examples section where `webdav --public --port=8080` is shown

  **Must NOT do**:
  - Do NOT remove or alter any command syntax examples
  - Do NOT restructure the WebDAV section

  **Recommended Agent Profile**:
  - **Category**: `quick` - Targeted text additions at specific line ranges

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Task 4)
  - **Blocks**: Final verification
  - **Blocked By**: Tasks 1, 2

  **References**:
  - SKILL.md lines ~209-217 (WebDAV section)
  - SKILL.pt-BR.md lines ~209-217 (WebDAV section)
  - README.md Quick Examples
  - Security audit finding #7 (High, 95%)

  **Acceptance Criteria**:
  - [ ] SKILL.md: Banner warning before first WebDAV command
  - [ ] SKILL.pt-BR.md: Same banner translated
  - [ ] README.md: Warning near `webdav --public --port=8080` example
  - [ ] README.pt-BR.md: Same warning translated

  **QA Scenarios**:
  ```
  Scenario: Verify WebDAV warnings exist
    Tool: Bash
    Steps:
      1. grep "WebDAV" SKILL.md - confirm banner near result
      2. grep "WebDAV" SKILL.pt-BR.md - confirm same
    Expected Result: At least 3 warning lines added per file
    Evidence: .omo/evidence/task-3-webdav-warnings.txt
  ```

  **Evidence to Capture**:
  - [ ] grep output confirming warning presence in all 4 files

  **Commit**: YES (groups with Task 4)
  - Message: `docs(skill): add network exposure warnings to WebDAV/FTP sections`
  - Files: `SKILL.md`, `SKILL.pt-BR.md`, `README.md`, `README.pt-BR.md`

- [ ] 4. Add network exposure warnings to FTP section

  **What to do**:
  - Add :warning: banner warnings to the FTP section in SKILL.md (lines ~220-224) and SKILL.pt-BR.md (lines ~220-224)
  - Warnings must cover: risk of exposing MEGA content via FTP, plain FTP insecurity when TLS is not enabled, remote attack surface and unauthorized access risks, recommendation to use `--tls`

  **Must NOT do**:
  - Do NOT remove or alter any command syntax

  **Recommended Agent Profile**:
  - **Category**: `quick` - Same pattern as Task 3

  **Parallelization**:
  - **Can Run In Parallel**: YES (with Task 3)
  - **Blocks**: Final verification
  - **Blocked By**: Tasks 1, 2

  **References**:
  - SKILL.md lines ~220-224 (FTP section)
  - SKILL.pt-BR.md lines ~220-224 (FTP section)
  - Security audit findings #13 (Medium, 94%) + #7 (High, 95%)

  **Acceptance Criteria**:
  - [ ] SKILL.md: Banner warning before first FTP command
  - [ ] SKILL.pt-BR.md: Same banner translated

  **QA Scenarios**:
  ```
  Scenario: Verify FTP warnings exist
    Tool: Bash
    Steps:
      1. grep "FTP" SKILL.md - confirm warning near result
      2. Repeat for PT-BR
    Expected Result: Warning text present before FTP commands
    Evidence: .omo/evidence/task-4-ftp-warnings.txt
  ```

  **Evidence to Capture**:
  - [ ] grep output for FTP warnings

  **Commit**: NO (groups with Task 3)
- [x] 1. Map all 13 findings to exact file:line ranges

  **What to do**:
  - Read each of the 13 ClawHub findings and map each to: Finding ID (F1-F13), Severity + confidence, Exact affected file + line numbers, Current text at that location, Proposed warning text
  - Create a resolution table in the working notes
  - Files to inspect: SKILL.md, SKILL.pt-BR.md, README.md, README.pt-BR.md

  **Must NOT do**:
  - Do NOT edit files yet - this is pure analysis
  - Do NOT guess line numbers - use grep to find exact locations

  **Recommended Agent Profile**:
  - **Category**: `quick` - Systematic read-and-record task, no domain expertise needed

  **Parallelization**:
  - **Can Run In Parallel**: NO (foundation task - all other tasks depend on mapping)
  - **Blocks**: All other tasks
  - **Blocked By**: None

  **References**:
  - `~/.config/opencode/skills/megacmd/SKILL.md` (418 lines)
  - `~/.config/opencode/skills/megacmd/SKILL.pt-BR.md` (418 lines)
  - `~/.config/opencode/skills/megacmd/README.md` (240 lines)
  - `~/.config/opencode/skills/megacmd/README.pt-BR.md` (240 lines)
  - Security audit: `https://clawhub.ai/alefsanderribeiro/megacmd/security-audit`

  **Acceptance Criteria**:
  - [ ] Resolution table with columns: Finding ID, Severity, File, Lines, Current Text, Proposed Change
  - [ ] All 13 findings accounted for

  **QA Scenarios**:
  ```
  Scenario: Verify all 13 findings are mapped
    Tool: Bash (grep + file read)
    Preconditions: Resolution table exists as working document
    Steps:
      1. Count mapped findings - confirm 13
      2. Spot-check 2-3 findings against actual file content
    Expected Result: All 13 findings linked to real file content
    Evidence: .omo/evidence/task-1-mapping-table.json
  ```

  **Evidence to Capture**:
  - [ ] Resolution table (JSON or Markdown)

  **Commit**: NO (part of group with Task 2)

- [x] 2. Define warning templates + finalize resolution table

  **What to do**:
  - Create warning template patterns based on user decisions:
    - **Banner** (Moderado): `> :warning: **Security Warning**: [risk description]` with context and mitigation
    - **Light** (find -delete): `> :warning: [brief warning]` single line
    - **Generic** (proxy): `> :warning: [general caution]` avoiding inline credentials
  - Expand resolution table from Task 1 with template type and exact warning text

  **Must NOT do**:
  - Do NOT edit files yet

  **Recommended Agent Profile**:
  - **Category**: `quick` - Template definition, no creative writing needed

  **Parallelization**:
  - **Can Run In Parallel**: NO (depends on mapping from Task 1)
  - **Blocks**: All task-specific execution (3-12)
  - **Blocked By**: Task 1

  **Acceptance Criteria**:
  - [ ] Warning templates defined for each prominence level
  - [ ] Resolution table expanded with template type and exact text for each finding

  **QA Scenarios**:
  ```
  Scenario: Verify templates are consistent
    Tool: Bash
    Steps:
      1. Read template definitions
      2. Confirm each resolves to a specific finding
    Expected Result: Templates cover all 3 levels (banner, light, generic)
    Evidence: .omo/evidence/task-2-templates.md
  ```

  **Evidence to Capture**:
  - [ ] Template definitions
  - [ ] Final resolution table

  **Commit**: YES
  - Message: `docs(skill): map 13 security findings and define warning templates`
  - Files: `.omo/plans/megacmd-security-audit-review.md`, `.omo/drafts/megacmd-security-audit-review.md`
