---
name: chsh-sk-hermes-setup
description: Complete reference for Charlie's Hermes Agent + Hermes WebUI installation on Ubuntu 24.04 VM (192.168.75.30). Covers code paths, venv, config locations, env vars, systemd services, startup chains, and the NFS-based config centralization scheme. Use when working on the Hermes stack, troubleshooting startup, or referencing installation layout.
---

# Hermes Agent + WebUI Installation Layout

This skill documents the complete installation layout for Charlie's self-hosted Hermes stack on a UGREEN NAS + Ubuntu VM setup.

## Overview

```
                            UGREEN DX4600 NAS (192.168.75.10)
                            ├── Lucky (万吉) v2.27.2 — reverse proxy, TLS, DDNS
                            └── NFS share /volume1/CharlieII → /mnt/CharlieII (VM mount)

                            Ubuntu 24.04 VM (192.168.75.30)
                            ├── Hermes Agent — core AI agent
                            ├── Hermes WebUI — browser UI
                            └── noVNC (port 9999) + ttyd (port 8080)
```

## Hermes Agent

### Paths

| Item | Path |
|------|------|
| **Code repo** | `/home/charlie/hermes-agent/` |
| **Python venv** | `/home/charlie/hermes-agent/.venv/` |
| **HERMES_HOME** (config, sessions, skills) | `/mnt/CharlieII/hermes/` |
| **Agent config** | `~/.hermes/config.yaml` + `~/.hermes/.env` |
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

## Lucky (万吉) Reverse Proxy

- **Version**: 2.27.2 (latest, up to date)
- **Running on**: NAS (192.168.75.10)
- **Listening**: port 443 with Let's Encrypt TLS (`*.chsh.uk`)
- **Admin panel**: https://lucky.chsh.uk (port 16601 on NAS)
- **Lucky = 万吉** — same project, Chinese name vs English name
- **Reverse proxy rules** for: hermes.chsh.uk, nas.chsh.uk, lucky.chsh.uk, haos.chsh.uk, nav.chsh.uk, router.chsh.uk, vm.chsh.uk, etc.
- **DDNS**: Cloudflare
- **Upgrade**: via Lucky admin → "上传新版本" button

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
