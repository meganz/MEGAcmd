# Warning Templates — MEGAcmd Security Audit

## Template Types (3 levels)

### Level 1: Banner Warning (Moderado)
Used for: credential exposure, network exposure, data disclosure

Format:
```
> ⚠️ **Security Warning — [Risk Title]**
> [2-3 sentences describing the specific risk and its consequences]
> **Recommendation:** [1-2 sentences on how to mitigate]
```

Placement: Before the section header, as a standalone blockquote

### Level 2: Light Warning
Used for: find -delete in troubleshooting (user already frustrated)

Format:
```
> ⚠️ The following commands delete local files without confirmation. Verify paths before running.
```

Placement: Single line above the code block, not inside it

### Level 3: Generic Note
Used for: proxy credentials (command doesn't show inline creds but supports them)

Format:
```
> ⚠️ If using [feature with credentials], avoid passing them on the command line. Prefer environment variables or configuration files.
```

Placement: Near the relevant command, as a note

---

## Per-Finding Warning Assignment

| Finding | Template | Location | Exact Warning Text |
|---------|----------|----------|-------------------|
| F1 (Triggers EN) | N/A - frontmatter edit | SKILL.md L3-6 | Refine description text (not template-based) |
| F2 (General EN) | Banner | SKILL.md L33-43 | See Warning #12 (banner covers F2+F4) |
| F3 (Triggers PT) | N/A - frontmatter edit | SKILL.pt-BR.md L3-6 | Refine description text (not template-based) |
| F4 (General PT) | Banner | SKILL.pt-BR.md L33-43 | See Warning #12 (banner covers F2+F4) |
| F5 (Login EN) | Banner | SKILL.md before L100 | `> ⚠️ **Security Warning — Credential Exposure**\n> Running mega-login with inline password exposes credentials to shell history, process listings, audit logs, and agent telemetry. Prefer interactive login where possible, and always use 2FA for account protection.\n> **Recommendation:** Use `mega-login` without password argument in shared/automated environments, or ensure secure credential storage.` |
| F6 (Sharing EN) | Banner | SKILL.md before L139 + L147 | `> ⚠️ **Security Warning — Data Disclosure**\n> Creating public links or sharing folders exposes your cloud data to third parties. Links with --writable expose your account. Use passwords for sensitive files, set expiration dates, and grant minimum necessary permission levels.\n> **Recommendation:** Review shared links periodically. Use --password and --expire flags for sensitive content.` |
| F7 (WebDAV/FTP EN) | Banner | SKILL.md before L209 + L219 | `> ⚠️ **Security Warning — Network Exposure (HIGH)**\n> Starting WebDAV or FTP servers exposes your MEGA content over the network. Without TLS, traffic and credentials are sent in plain text. The --public flag makes the service accessible beyond localhost.\n> **Recommendation:** Use --tls with valid certificates. Avoid --public unless necessary. Stop services when not in use with mega-webdav -d / mega-ftp -d.` |
| F8 (Login PT) | Banner | SKILL.pt-BR.md before L100 | Same as F5 translated to PT-BR |
| F9 (find -delete) | Light | SKILL.md before L337 | `> ⚠️ The following commands delete local files without confirmation. Verify paths are correct before running.` |
| F10 (Session ID) | Banner | SKILL.md near L109 | `> ⚠️ **Security Warning — Session Token**\n> The session ID is a bearer credential. If exposed, anyone can impersonate your session until it expires or is revoked.\n> **Recommendation:** Do not share the session output in logs, screenshots, or automated reports.` |
| F11 (Master Key) | Banner | SKILL.md near L107 + strengthen L380 | `> ⚠️ **Security Warning — Recovery Key**\n> The master key (recovery key) is the only way to recover your data if you lose your password. Store it securely, encrypted, with restricted file permissions.\n> **Recommendation:** Save to encrypted storage (password manager, encrypted USB). Avoid cloud-syncing the plaintext file.` |
| F12 (Proxy) | Generic | SKILL.md near L268 | `> ⚠️ If using proxy authentication, avoid passing credentials on the command line where they may be captured by shell history or process listings. Use environment variables or a configuration file instead.` |
| F13 (FTP/WebDAV public) | Banner | SKILL.md before L209 + L219 | Covered by F7 warning above (same section) |
