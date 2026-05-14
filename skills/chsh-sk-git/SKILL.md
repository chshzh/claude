---
name: chsh-sk-git
description: Plans and executes git commits and push. Extracts rationale from the current conversation and delegates execution to the chsh-ag-git subagent. Supports Conventional Commits (user app repos) and Zephyr style (NCS/Zephyr repos). Use when the user asks to commit, push, prepare commits, split commits, or wrap up work.
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

Omit `Push target` entirely — the subagent will always ask via `AskQuestion`.

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

## After push — NCS / firmware repos

The subagent does **not** watch CI or flash firmware. For those next steps:

| Need | Use |
|---|---|
| Watch CI, fix failures, loop | `chsh-sk-git-release` |
| Flash pre-built artifact + UART verify | `chsh-sk-ncs-debug` Mode G |
| Full release (tag → CI → artifact → flash) | `chsh-sk-git-release` |

> Rule: CI green is not enough for firmware repos — always flash the pre-built
> artifact and verify `uart:~$` on the correct VCOM port before marking done.

## Fallback: inline execution

Only skip the subagent if:
- User says "skip the subagent, commit inline"
- You are already inside a subagent (no nested subagent chains)
- `chsh-ag-git` is unavailable

For inline execution, follow the workflow in `~/.claude/agents/chsh-ag-git.md` directly — that file is the canonical reference for inspection, planning, approval gates, formats, and grouping rules.

The skill should stay concise and delegation-focused; commit trailer format details and fallback behavior are source-of-truth in `~/.claude/agents/chsh-ag-git.md`.

## Self-Update Policy

At the end of each conversation, check if any facts here or in `~/.claude/agents/chsh-ag-git.md` are new, corrected, or outdated. If so:
1. Collect proposed changes with rationale.
2. Ask for approval via `AskQuestion`.
3. Apply approved updates immediately.
