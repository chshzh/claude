---
name: chsh-agentmemory-inject-session
description: Use when bulk-importing Cursor, Claude, or GitHub Copilot (VS Code) chat history into AgentMemory, injecting past sessions, or backfilling a remote memory server from local transcripts.
---

# AgentMemory: Inject Session History

Bulk-import Cursor, Claude, and GitHub Copilot chat history into a remote AgentMemory server.
Safe to re-run — sessions already on the server are fetched first and skipped.

## Quick run

```bash
SKILL=~/.claude/skills/chsh-agentmemory-inject-session
AGENTMEMORY_URL=http://<server-ip>:3111 \
AGENTMEMORY_SECRET=<secret> \
node $SKILL/scripts/import-history.mjs
```

If `AGENTMEMORY_URL` / `AGENTMEMORY_SECRET` are not set, the script reads them
from `~/.cursor/mcp.json` (strips `//` comments before JSON parse).

## What gets imported

| Step | Source | Session ID | Period |
|------|--------|------------|--------|
| 1 | `~/.claude/projects/**/*.jsonl` — Claude Code transcripts | from `sessionId` field | ongoing |
| 2 | `~/.cursor/projects/**/*.jsonl` — Cursor agent transcripts | from `sessionId` field | ongoing |
| 3 | `~/Library/…/state.vscdb` cursorDiskKV `bubbleId:*` — Cursor sidebar chats | composer UUID | ongoing |
| 4a | `~/Library/…/workspaceStorage/*/GitHub.copilot-chat/transcripts/*.jsonl` — Copilot JSONL | `session.start` → `data.sessionId` | Apr 2026+ |
| 4b | `~/Library/…/workspaceStorage/*/chatSessions/*.json` — Copilot legacy JSON | `sessionId` field | Oct 2025 – Mar 2026 |

`~/Library/…` expands to `~/Library/Application Support/Code/User`.

## Deduplication

On every run, server session IDs are fetched first.
Local sessions whose ID already exists on the server are **never uploaded** —
they are skipped before scanning, not after.

The import strategy sent to the server is `skip`, which provides a second
safety net for any IDs that slip through (e.g., IDs generated with random
fallbacks).

## Re-run behaviour

```
Fetching server session list for dedup… 558 sessions already on server (will be skipped)
Step 1 (Claude JSONL):   14 files scanned → 0 new sessions
Step 2 (Cursor JSONL):  249 files scanned → 0 new sessions
Step 3 (Cursor sidebar): 0 new sessions (0 observations)
Step 4 (Copilot):        27 transcript files, 102 chatSession files scanned → 0 new transcript sessions + 0 new chatSession sessions (0 observations)

Nothing new to import — all sessions already on server.
```

## Config auto-detection

The script reads `~/.cursor/mcp.json` using a comment-stripping regex before
`JSON.parse`, so the file's inline `//` comments do not break parsing.

## Gotchas

- **state.vscdb is 1.6 GB** — Step 3 streams in batches of 400 rows via hex;
  expect ~30 s for 49 k bubbles. Do not increase `batchSize` above 500 or
  `maxBuffer` may overflow.
- **Sidebar `type` field**: `1` = user bubble, `2` = assistant bubble.
  Bubbles with no extractable text (tool-only turns) are silently skipped;
  this is expected — only ~92 of 1 354 composer threads had text.
- **Copilot transcript format**: Only `user.message` and `assistant.response`
  event types produce observations. The `session.start` event provides the
  sessionId and optional `cwd`. Events with empty `content` are silently skipped.
- **Copilot chatSessions format**: Only user messages are stored (assistant
  responses are not in the legacy JSON). Sessions with zero user messages are
  skipped.
- **workspaceStorage has ~100+ workspace hashes** on a typical macOS install.
  Step 4 scans them all but is fast (JSON file I/O only, no SQLite).
- **VERSION constant** must match the AgentMemory server's accepted import
  versions list. If import fails with "Unsupported export version", update
  `VERSION` in the script to the installed server version (`agentmemory status`).
- **mcp.json has inline comments** — raw `JSON.parse` fails; the script strips
  `//…` lines first. If `AGENTMEMORY_SECRET` still isn't found, pass it via env.
- **Re-run token cost**: Step 3 still reads all 49 k bubbles from SQLite even
  when nothing is new. This takes ~30 s but costs nothing in network/API calls.
- **Lab server address**: The personal AgentMemory API runs at `192.168.75.30:3111`
  (dashboard at `:3113`). With mcp.json auto-detection, you don't need to set
  `AGENTMEMORY_URL` at all — the script reads it from `~/.cursor/mcp.json`.

## Related Skills

| Task | Skill |
|------|-------|
| Commit or push after updating the script | `chsh-sk-git-commit` |

## Self-Update Policy

At the **end of each conversation**, check whether any new import sources,
format changes, or dedup edge cases were discovered. Update this file if:
- A new VS Code session format is found
- The agentmemory server import API version changes
- A new gotcha is observed during an actual run
