---
name: chsh-sk-ncs-3.4-git-commit
description: Use when the user asks to commit, push, prepare commits, split commits, or wrap up work. Works for any git repo. Auto-detects commit style: Conventional Commits (app/general repos), Zephyr style (NCS/Zephyr workspace repos). Delegates to chsh-ag-git.
---

# Git Commit Skill

This skill has one job: **extract rationale from the current conversation, then hand off to `chsh-ag-git`**. The subagent owns inspection, grouping, approval gates, and execution.

## Step 1 — Extract rationale from conversation

Scan the conversation for these signals before delegating:

| Signal | How to use it |
|---|---|
| Stated goal ("add X", "fix the bug where Y") | Commit subject |
| Reasons / decisions ("doing X because Y") | Commit body |
| Rejected alternatives ("tried Z, didn't work because…") | Body, if non-trivial |
| Ticket / PR refs (`NCSDK-xxxxx`, `Upstream PR #xxxxxx`) | `Ref:` / `Upstream PR #:` |

Do not invent rationale. If the conversation has none, pass an empty context field and let the subagent write diff-derived messages.

## Step 2 — Delegate to `chsh-ag-git`

```
Use chsh-ag-git to commit [and push].

Context: <1–2 sentence summary of what was built/fixed and why — or omit if none>
References: <NCSDK-xxxxx / Upstream PR #xxxxxx / etc. — or omit>
```

Omit `Push target` entirely — the subagent will ask about pushing, either as part of the commit plan approval ("Approve and push") or as a separate gate after committing.

Real example:

```
Use chsh-ag-git to commit.

Context: Fixed Wi-Fi STA reconnection hang after AP disconnect — root
cause was missing WIFI_DISCONNECTED handler in wifi_module.c.
Tried net_mgmt callback first but it fired before STA state cleared.
References: NCSDK-12345
```

## Step 3 — Hand off and stop

Once delegated:
- Forward approval questions from the subagent to the user
- Forward the subagent's final report back
- Do not second-guess grouping or messages unless the user asks

## Doc-sync check (user app repos)

The subagent automatically classifies `src/` changes into two severity levels using diff-pattern matching, then checks if the corresponding version pins are already bumped in the batch:

**Phase 1 + 2** — user-visible behavior changed (new shell command, new Wi-Fi mode, banner change, timing constant, new public `.h` API):
- Shows: `docs/pm-prd/PRD.md` (current: `<timestamp>`) and `docs/dev-specs/` (current: `<timestamp>`)
- First option: **"Stop — update PRD + specs first"**

**Phase 2 only** — implementation boundary crossed, no user-visible change (new Zbus channel, new SMF state, new Kconfig, new module):
- Shows: `docs/dev-specs/overview.md` (current: `<timestamp>`)
- First option: **"Stop — update specs first"**

If pins are already bumped in the same batch → no warning shown.
This check does **not** run for Zephyr-style repos, or any repo where `docs/qa-test/` does not exist.

## After push — NCS / firmware repos only

Skip this section for non-firmware repos (scripts, skills, configs, etc.).

The subagent does **not** watch CI or flash firmware. For those next steps:

| Need | Use |
|---|---|
| Watch CI, fix failures, loop | `chsh-sk-ncs-3.5-release` |
| Flash pre-built artifact + UART verify | `chsh-sk-ncs-3.2-debug` Mode G |
| Full release (tag → CI → artifact) | `chsh-sk-ncs-3.5-release` |

> **Firmware repos only:** CI green is not enough — always flash the pre-built
> artifact and verify `uart:~$` on the correct VCOM port before marking done.

## Fallback: inline execution

Only skip the subagent if:
- User says "skip the subagent, commit inline"
- You are already inside a subagent (no nested subagent chains)
- `chsh-ag-git` is unavailable

For inline execution, follow the workflow in `~/.claude/agents/chsh-ag-git.md` directly — that file is the canonical reference for inspection, planning, approval gates, formats, and grouping rules.

The skill should stay concise and delegation-focused; commit trailer format details and fallback behavior are source-of-truth in `~/.claude/agents/chsh-ag-git.md`.

## Gotchas
- TODO: add one entry per real observed failure or routing false-positive

## Self-Update Policy

At the **end of each conversation**, check if any facts here or in `~/.claude/agents/chsh-ag-git.md` are new, corrected, or outdated.

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
