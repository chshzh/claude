---
name: chsh-sk-security-scan
description: Use when scanning files or a git repo for credentials, tokens, private keys, or sensitive data before committing, during a security audit, or when asked to check for secrets or leaks. Also called as a subroutine by chsh-ag-git, chsh-sk-skill-review, and chsh-sk-llm-wiki-review.
---

# Security Scan

Runs `scripts/scan.py` to detect sensitive data. Called standalone or as a subroutine from `chsh-ag-git`, `chsh-sk-skill-review`, and `chsh-sk-llm-wiki-review`.

## Modes

```bash
SKILL=~/.claude/skills/chsh-sk-security-scan

python3 $SKILL/scripts/scan.py staged              # staged files only (pre-commit default)
python3 $SKILL/scripts/scan.py tracked             # all tracked files in current repo
python3 $SKILL/scripts/scan.py dir <path>          # all files under a directory tree
python3 $SKILL/scripts/scan.py <filepath>          # single file
python3 $SKILL/scripts/scan.py sanitize <filepath> # redact BLOCK findings in-place
```

Run `staged` and `tracked` from inside the target git repo. `dir` and `sanitize` can be run from anywhere.

### Sanitize mode

Strips BLOCK-level credentials from a file in-place, preserving surrounding structure (JSON keys, quotes). Runs a post-sanitize scan to confirm CLEAN before exiting.

```bash
# Typical workflow: sanitize a config backup before committing it
python3 $SKILL/scripts/scan.py sanitize ~/.claude/.claude.json.backup
# Then commit the sanitized backup safely
```

Replacements preserve structure:
- `"AGENTMEMORY_SECRET": "c9259..."` → `"AGENTMEMORY_SECRET": "<REDACTED>"`
- `"Authorization": "Bearer github_pat_xxx"` → `"Authorization": "Bearer github_pat_<REDACTED>"`

## Exit codes and what to do

| Code | Verdict | Action |
|------|---------|--------|
| 0 | `CLEAN` | Proceed |
| 1 | `BLOCK` | **Stop.** Show findings. Do not commit until user resolves. |
| 2 | `WARN` | Show findings. User decides at next approval gate. |

## Known false positives (this repo)

- **Lab IPs in `skills/`, `wiki/`, `agents/`**: `192.168.75.30` (agentmemory VM), `192.168.92.1` (router), `192.168.75.1` (OpenWrt) are lab topology docs — auto-suppressed by the scanner for those paths.
- **Placeholder patterns**: `<token>`, `$ENV_VAR`, `{{template}}`, `your-password` — all auto-suppressed.
- **Skill credential-handling examples**: `password = "your-password-here"` inside code blocks is documentation, not a leak. Scanner suppresses by placeholder pattern, but read context if it fires anyway.

## Calling from chsh-ag-git

After gathering context (Step 1), before proposing the plan (Step 4):

```
python3 ~/.claude/skills/chsh-sk-security-scan/scripts/scan.py staged

BLOCK (exit 1) → stop, show findings, do not proceed with commit plan.
WARN  (exit 2) → add a ⚠ row to the commit plan table; user decides at approval gate.
CLEAN (exit 0) → proceed silently.
```

## Calling from skill-review / wiki-review

```bash
SKILL=~/.claude/skills/chsh-sk-security-scan

# Scan all skills
python3 $SKILL/scripts/scan.py dir ~/.claude/skills/

# Scan a specific wiki
python3 $SKILL/scripts/scan.py dir ~/.claude/wiki/
```

BLOCK findings → P0. WARN findings → P1 (review required, may be intentional lab docs).

## Gotchas

- **64-char hex strings**: The scanner flags `secret|token|password = <40+ hex chars>` — not bare hex. A git SHA in a comment will not fire.
- **Binary files**: Skipped silently (decode errors caught).
- **`git show :<path>` fails for deleted files**: Staged deletions return empty content — that's safe, skip them.

## Self-Update Policy

After each use, check if a new false positive or missed pattern was observed. If so, propose an update to this file and `scripts/scan.py` via `AskQuestion`.
