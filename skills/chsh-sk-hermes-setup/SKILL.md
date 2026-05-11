---
name: chsh-sk-hermes-setup
description: Complete reference for Charlie's Hermes Agent + Hermes WebUI installation on Ubuntu 24.04 VM (192.168.75.30). Covers code paths, venv, config locations, env vars, systemd services, startup chains, and the NFS-based config centralization scheme. Use when working on the Hermes stack, troubleshooting startup, or referencing installation layout.
---

# Hermes Agent + WebUI Installation Layout

This skill documents the complete installation layout for Charlie's self-hosted Hermes stack on a UGREEN NAS + Ubuntu VM setup.

## Overview

```
                            UGREEN DX4600 NAS (192.168.75.10)
                            └── NFS share /volume1/CharlieII → /mnt/CharlieII (VM mount)

                            Ubuntu 24.04 VM (192.168.75.30)
                            ├── Hermes Agent — core AI agent
                            ├── Hermes WebUI — browser UI
                            └── ttyd (port 8080)
```

## Hermes Agent

### Paths

| Item | Path |
|------|------|
| **Code repo** | `/home/charlie/hermes-agent/` |
| **Python venv** | `/home/charlie/hermes-agent/.venv/` |
| **HERMES_HOME** (config, sessions, skills) | `/mnt/CharlieII/hermes/` |
| **Agent config** | `/mnt/CharlieII/hermes/config.yaml` + `/mnt/CharlieII/hermes/.env` |
| **Session DB** | `/mnt/CharlieII/hermes/state.db` |
| **Skills** | `/mnt/CharlieII/hermes/skills/` |
| **Logs** | `~/.hermes/logs/` |
| **Systemd service** | `~/.config/systemd/user/hermes-gateway.service` |

### Shell Config (`.bashrc`)

- **Line 161**: `export HERMES_HOME=/mnt/CharlieII/hermes`
- **Line 170-171**: `eval "$(hermes completion bash)"`

### Systemd Service

The gateway runs as a user service installed via `hermes gateway install`.

```bash
systemctl --user status hermes-gateway
```

### Key Commands

```bash
cd /home/charlie/hermes-agent
source .venv/bin/activate
hermes          # interactive CLI
hermes --tui    # terminal UI
hermes gateway run  # gateway foreground
hermes config edit  # edit config
hermes model        # switch provider/model
```

## Hermes WebUI

### Paths

| Item | Path |
|------|------|
| **Code repo** | `/home/charlie/hermes-webui/` |
| **Repo .env** | Symlink → `/mnt/CharlieII/hermes/webui/env.conf` |
| **Config file** | `/mnt/CharlieII/hermes/webui/env.conf` |
| **State dir** (sessions, settings, models_cache) | `/mnt/CharlieII/hermes/webui/` |
| **Bind address** | `0.0.0.0:8787` (LAN: `192.168.75.30:8787`) |
| **Systemd service** | `~/.config/systemd/user/hermes-webui.service` |

### Config Content (`env.conf`)

```ini
HERMES_WEBUI_PORT=8787
HERMES_WEBUI_HOST=0.0.0.0
HERMES_WEBUI_STATE_DIR=/mnt/CharlieII/hermes/webui
HERMES_WEBUI_DEFAULT_WORKSPACE=/mnt/CharlieII
```

### Startup Chain

```
systemctl --user start hermes-webui
  └─ ExecStart = /home/charlie/hermes-webui/start.sh
       └─ set -a; source REPO_ROOT/.env (symlink → env.conf); set +a
            └─ python3 bootstrap.py --no-browser
                 └─ imports api.config (reads os.getenv)
                      └─ server.py (ThreadingHTTPServer on :8787)
```

### Recovery After Re-clone

If the repo is re-cloned, re-create the symlink:

```bash
cd /home/charlie/hermes-webui
ln -s /mnt/CharlieII/hermes/webui/env.conf .env
```

### Verification

```bash
systemctl --user is-enabled hermes-webui && systemctl --user is-active hermes-webui
curl -s http://localhost:8787/health  # → {"status": "ok"}
ss -tlnp | grep 8787                  # → LISTEN 0.0.0.0:8787
```

## Update Management

### Release-tag strategy

Charlie prefers to pin both repos to **release tags** (not master) and update only when intentionally choosing to. This avoids daily "N updates behind" banners caused by tracking master's constant churn.

| Repo | Current state | Tag format | Latest (as of session) |
|------|---------------|------------|----------------------|
| **Agent** (`hermes-agent/`) | master (bfc84bdc6) — NOT on a tag | `v2026.5.7`, `v2026.4.30`, ... (weekly) | `v2026.5.7` |
| **WebUI** (`hermes-webui/`) | `v0.51.33` (already a release tag) | `v0.51.33`, `v0.51.32`, ... (semver) | `v0.51.33` |

To pin agent to a release tag:
```bash
cd /home/charlie/hermes-agent
git fetch --tags
git checkout v2026.X.Y
```

### Update checker pitfall

The WebUI update checker (`api/updates/_check_repo()`) compares `HEAD` against `origin/master` — NOT against release tags. Even when checked out at a tag, `@{upstream}` fails (detached HEAD has no upstream) and it falls back to `origin/master`. Result: **any tag that is behind master shows "N updates behind"**, even though the user is intentionally on a stable release. This is the source of the daily nag.

The mechanism:
```
boot.js → GET /api/updates/check → updates.py:_check_repo()
  → git fetch origin
  → git rev-list --count HEAD..origin/master  ← compares against branch, not tags
```

### Disabling update checks

The nag can be stopped in WebUI Settings → Preferences → **"Check for updates"** toggle (default: on). When disabled:
- Backend route returns `{"disabled": true}` instead of running git fetch
- Frontend guard (`_bootSettings.check_for_updates: false`) skips the API call
- The update banner never appears

This is the recommended companion to the release-tag strategy: pin to a tag, disable the check, and manually update when choosing to.

### Manual update flow

When ready to update to a newer release:
```bash
cd /home/charlie/hermes-agent
git fetch --tags
git tag --sort=-v:refname | head -5         # see latest tags
git checkout v2026.X.Y                       # latest weekly tag

cd /home/charlie/hermes-webui
git fetch --tags
git tag --sort=-v:refname | head -5
git checkout v0.51.XX                        # latest semver tag
```

Then restart the WebUI: `systemctl --user restart hermes-webui`

## Cron Jobs (release-tag pinning)

### Weekly release update

A cron job runs every Friday 17:00 to pin both repos to their latest release tags and deliver a report to Telegram.

| Field | Value |
|-------|-------|
| **Job name** | `Weekly release update` |
| **Job ID** | `72970d80e71d` |
| **Schedule** | `0 17 * * 5` (Friday 17:00) |
| **Mode** | `no_agent: true` — runs script, delivers stdout verbatim |
| **Delivery** | `telegram:8614941405` (Charlie's DM) |
| **Script** | `~/.hermes/scripts/weekly-release-update.sh` |

The script:
1. Fetches latest tags from both `hermes-agent` and `hermes-webui`
2. Compares current HEAD to latest tag via `git merge-base --is-ancestor`
3. If current HEAD is NOT exactly the latest tag, checks it out
4. If WebUI was updated, restarts its systemd service
5. Outputs a concise report like:
   > 📅 Release Update Report — 2026-05-15 17:00
   > 🤖 Agent: already at latest v2026.5.7 ✅
   > 🌐 WebUI: v0.51.33 → v0.51.34 ✅ (service restarted)

### Pitfall: script path must be relative

Cron job scripts MUST reside in `~/.hermes/scripts/` and be referenced by filename only.
An absolute or `~`-qualified path causes: `Script path must be relative to ~/.hermes/scripts/.`

### List existing cron jobs

```bash
hermes cron list
```

## Config Centralization Principle

All config and runtime data lives on the NFS mount `/mnt/CharlieII/`, NOT in the code repos:

```
/home/charlie/.bashrc                  ← HERMES_HOME export
/mnt/CharlieII/hermes/                 ← Agent data (config.yaml, .env, sessions, skills, state.db)
/mnt/CharlieII/hermes/webui/env.conf   ← WebUI config (symlinked from repo .env)
/mnt/CharlieII/hermes/webui/           ← WebUI state (sessions, settings, models_cache)
```

This makes the stack survive repo re-clones, OS reinstalls, and machine migrations — only the NFS mount and the thin shell config need to be preserved.

## Backup & Version Control

The `$HERMES_HOME` directory can be version-controlled to back up critical config and session data. The NFS share is already NAS-backed, but git gives you point-in-time recovery and diff history.

### Cleanup before init

Before initializing git, remove ephemeral artifacts:

```bash
cd /mnt/CharlieII/hermes
rm -f auth.lock gateway.lock gateway.pid processes.json
rm -f interrupt_debug.log .update_check .restart_last_processed.json
rm -f state.db-wal state.db-shm
rm -rf pastes/ plans/ hooks/ pairing/ workspace/  # empty/unused dirs
rm -f config.yaml.bak.*  # stale backup snapshots
```

### What to track vs ignore

| Track (critical) | Ignore (ephemeral/secrets) |
|---|---|
| `config.yaml` | `.env` — **API keys, NEVER commit** |
| `SOUL.md` | `auth.json` — OAuth tokens |
| `channel_directory.json` | `cache/` — model caches (123MB) |
| `gateway_state.json` | `audio_cache/` — TTS audio (4MB) |
| `memories/` | `logs/` — agent/gateway logs |
| `state.db` — session store (74MB) | `bin/` — binaries |
| `sessions/` — session JSONL (56MB) | `webui/` — auto-managed WebUI state |
| `checkpoints/` — conversation checkpoints (13MB) | `profiles/` — auto-generated model profiles |
| `kanban.db`, `response_store.db` | `images/`, `image_cache/` |
| `cron/`, `scripts/`, `plugins/` | `.local/` — XDG local state |
| `skills/` (agent-managed, but worth snapshotting) | `sandboxes/`, `skins/` |

### Setup git

```bash
cd /mnt/CharlieII/hermes
git init
cp /mnt/CharlieII/claude/skills/chsh-sk-hermes-setup/references/hermes-backup-gitignore .gitignore
git add .
git commit -m "Initial backup: hermes runtime config + sessions"
```

### Push to GitHub (private repo)

```bash
# Create repo (one-time)
gh repo create chshzh/hermes --private --description "HERMES_HOME runtime data backup"

# Push
git branch -m main
git remote add origin https://github.com/chshzh/hermes.git
git push -u origin main
```

### Updating (periodic backup)

```bash
cd /mnt/CharlieII/hermes
rm -f state.db-wal state.db-shm          # remove transient WAL before add
git add -A
git commit -m "backup $(date +%F)"
git push
```

### NFS permission pitfall

Checkpoint HEAD files and some skill/plugin files may have `0000` (no-read) permissions from NFS. Fix before add:

```bash
find checkpoints skills plugins -type f ! -perm -400 -exec chmod 644 {} +
```

### Pitfalls

- **`.env` contains API keys.** Add it to `.gitignore` before the first commit. If you accidentally commit it, use `git filter-branch` or `git rm --cached` + force push (if remote exists).
- **`state.db` is 74MB binary SQLite.** Git handles it as a single blob — every clone pulls the full file. For remote repos, consider Git LFS. For local NAS backup, plain git is fine.
- **`state.db-wal` and `state.db-shm`** are SQLite WAL files that only exist while the agent is running. Remove them before init to avoid committing transient state.
- **Don't gitignore `state.db` itself.** The user explicitly wants session data backed up. If you need to exclude it temporarily (e.g. for a fast push), use `git update-index --skip-worktree state.db` instead of gitignore.

## Related Wiki Pages

- [[hermes-architecture]] — Source code internals: agent loop, tools, gateway, skills system
- [[hermes-setup-on-linux-with-nas-nfs-backup]] — NAS NFS architecture and machine recovery procedure
