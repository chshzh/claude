---
title: Hermes File Map — Where Everything Lives & What to Customize
created: 2026-05-05
updated: 2026-05-05
type: concept
tags: [hermes, deployment, config, self-hosted, guide]
sources: []
confidence: high
---

# Hermes File Map — Where Everything Lives & What to Customize

> **Purpose:** Understand the physical layout of Hermes Agent + WebUI on disk, and know
> exactly which files you edit vs which are auto-managed.

## Three Repositories, Two Roles

```
/mnt/CharlieII/                        ← NFS share root (NAS-backed, survives VM rebuilds)
├── hermes/              HERMES_HOME   ← Runtime data: config, state, skills, memory
├── hermes-webui/        Git repo      ← WebUI code + .env + start.sh
└── wiki/                WIKI_PATH     ← LLM Wiki knowledge base

/home/charlie/
├── hermes-agent/        Git repo      ← Hermes Agent source (cloned from NousResearch)
└── .config/systemd/user/             ← Systemd unit files (VM-local, not on NAS)
    ├── hermes-webui.service
    └── hermes-gateway.service
```

**Key insight:** The agent source (`hermes-agent/`) lives on the VM's local disk.
Everything that matters for your setup — config, API keys, sessions, skills, memory —
lives on the NAS at `/mnt/CharlieII/hermes/`. Rebuild the VM, remount the NFS share,
re-clone `hermes-agent`, and you're back.

## HERMES_HOME: `/mnt/CharlieII/hermes/`

This is the data directory. All runtime state lives here.

| Path | Type | Customize? | What it is |
|------|------|-----------|------------|
| `config.yaml` | File | **YES — edit this** | Agent config: model, provider, toolsets, gateway platforms, personality, approvals |
| `.env` | File | **YES — edit this** | API keys, secrets, `WIKI_PATH`, `HERMES_HOME` |
| `SOUL.md` | File | **YES — edit this** | Agent personality / system prompt |
| `state.db` | File | No (auto) | SQLite session store (~32 MB). Managed by agent, never touch. |
| `skills/` | Dir | No (agent-managed) | Installed skills. Agent creates/patches via `skill_manage`. |
| `plugins/` | Dir | Add plugins here | Plugins like session-find. |
| `checkpoints/` | Dir | No (auto) | Conversation checkpoints for resumption. |
| `cron/` | Dir | No (auto via cronjob tool) | Cron job definitions. |
| `audio_cache/` | Dir | No (auto) | TTS audio files. |
| `webui/` | Dir | No (WebUI auto-creates) | WebUI runtime: sessions, settings, workspaces. Config via WebUI `.env`, not here. |
| `logs/` | Dir | No (auto) | Gateway + agent logs. |
| `cache/` | Dir | No (auto) | Misc cache files. |
| `auth.json` | File | No (auto via `hermes auth`) | OAuth tokens, credential pools. |
| `gateway_state.json` | File | No (auto) | Gateway runtime state. |

### config.yaml — What you customize

```yaml
model:
  default: deepseek-v4-pro      # Your default model
  provider: deepseek             # Provider name
  base_url: https://api.deepseek.com/v1

agent:
  max_turns: 90                 # Max tool-calling iterations
  personalities:                # Custom agent voices
    helpful: ...
    concise: ...
```

Use `hermes config` to change settings safely, or edit directly. Backups are auto-created as `config.yaml.bak.*`.

### .env — What you customize

```
# API keys for LLM providers
DEEPSEEK_API_KEY=sk-...
OPENROUTER_API_KEY=sk-or-...

# Paths
HERMES_HOME=/mnt/CharlieII/hermes
WIKI_PATH=/mnt/CharlieII/wiki
```

This file is NOT the same as `hermes-agent/.env` (which is the repo's example file).
The live `.env` is at `$HERMES_HOME/.env`.

## WebUI: `/home/charlie/hermes-webui/` (repo) + state dir

The WebUI has two pieces:

| Piece | Path | Customize? |
|-------|------|-----------|
| Source code | `/home/charlie/hermes-webui/` | No (git clone) |
| **`.env`** | `/home/charlie/hermes-webui/.env` | **YES — edit this** |
| Systemd unit | `~/.config/systemd/user/hermes-webui.service` | **YES — edit paths if repo moves** |
| State directory | `/mnt/CharlieII/hermes/webui/` | No (auto-managed by WebUI) |

### WebUI .env

```
HERMES_WEBUI_STATE_DIR=/mnt/CharlieII/hermes/webui
HERMES_WEBUI_PORT=8787
HERMES_WEBUI_HOST=0.0.0.0            # 0.0.0.0 = LAN-accessible
HERMES_WEBUI_DEFAULT_WORKSPACE=/mnt/CharlieII
```

The `.env` is gitignored by the repo — safe to add API keys if needed.
Only four variables matter. The server auto-discovers the Hermes agent venv.

### WebUI systemd unit

```ini
# ~/.config/systemd/user/hermes-webui.service
[Unit]
Description=Hermes WebUI
After=network.target

[Service]
Type=simple
ExecStart=/home/charlie/hermes-webui/start.sh
WorkingDirectory=/home/charlie/hermes-webui
Environment="HERMES_HOME=/mnt/CharlieII/hermes"
Restart=on-failure
RestartSec=10

[Install]
WantedBy=default.target
```

Update the three paths if you move the WebUI repo.

## Hermes Agent Source: `/home/charlie/hermes-agent/`

This is a plain `git clone` of `NousResearch/hermes-agent`. You do NOT customize files here.
The agent reads its config from `$HERMES_HOME/config.yaml`, not from the repo.

The only exception: if you run the agent from this directory, it discovers `AGENTS.md`
and injects project context. But that's automatic — no editing needed.

## Systemd Services

Both run as **user** services with linger enabled (survive logout and reboot):

```bash
systemctl --user status hermes-gateway
systemctl --user status hermes-webui
sudo loginctl enable-linger charlie     # one-time setup
```

| Service | Unit file | Created by |
|---------|-----------|------------|
| `hermes-gateway` | `~/.config/systemd/user/hermes-gateway.service` | `hermes gateway install` |
| `hermes-webui` | `~/.config/systemd/user/hermes-webui.service` | Manually created |

The gateway unit is auto-generated and should NOT be hand-edited. The WebUI unit is manual — edit if paths change.

## Customization Checklist

When setting up Hermes on a new machine, these are the files you touch:

| # | File | What to set |
|---|------|-------------|
| 1 | `$HERMES_HOME/.env` | API keys, `HERMES_HOME`, `WIKI_PATH` |
| 2 | `$HERMES_HOME/config.yaml` | Model, provider, toolsets, personality |
| 3 | `$HERMES_HOME/SOUL.md` | Agent personality / behavior rules |
| 4 | `hermes-webui/.env` | Port, host, state dir, workspace |
| 5 | `~/.config/systemd/user/hermes-webui.service` | Paths (only if repo moves) |
| 6 | `~/.bashrc` | `eval "$(hermes completion bash)"` for tab completion |

Everything else — `state.db`, `skills/`, `checkpoints/`, `webui/`, `cron/`, `auth.json` —
is auto-managed by the agent and WebUI. Don't edit those.

## How It All Connects

```
hermes CLI  ──reads──→  $HERMES_HOME/config.yaml
                        $HERMES_HOME/.env
                        $HERMES_HOME/SOUL.md
                        $HERMES_HOME/skills/
                        $HERMES_HOME/state.db

hermes-webui ──serves──  port 8787
              ──reads──  hermes-webui/.env  →  $HERMES_WEBUI_STATE_DIR (webui sessions)
              ──reads──  $HERMES_HOME/state.db  →  CLI sessions (if show_cli_sessions on)
              ──shows──  $HERMES_WEBUI_DEFAULT_WORKSPACE  →  file browser sidebar

systemd ──starts──  hermes-webui.service (bind to port, survive reboot)
        ──starts──  hermes-gateway.service (messaging, cron, survive reboot)
```

## Related

- [[hermes-architecture]] — Source code internals: agent loop, tools, gateway, skills system
- [[hermes-setup-on-linux-with-nas-nfs-backup]] — NAS NFS architecture and machine recovery procedure
