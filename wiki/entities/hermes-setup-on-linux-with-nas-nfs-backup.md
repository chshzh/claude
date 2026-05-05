---
title: Hermes on Linux — NAS NFS Architecture & Recovery
created: 2026-05-03
updated: 2026-05-03
type: entity
tags: [self-hosted, nas, nfs, agent, webui, guide]
sources: []
---

## Overview

Hermes data lives on the UGreen DX4600 NAS, exported via NFS to any Linux machine on the LAN.
This means a new machine doesn't need to reinstall or reconfigure — it just mounts the NFS share
and everything is there: config, sessions, skills, memory, WebUI state, wiki.

## NAS NFS Architecture

```
UGreen DX4600 NAS (192.168.75.10)
  └── /volume1/CharlieII/          ← NFS export (3.6 TB)
        ├── hermes/                ← HERMES_HOME (1.8 GB)
        ├── hermes-webui/          ← WebUI git repo (20 MB)
        └── wiki/                  ← LLM Wiki (2.9 MB)
```

**Ubuntu 24.04 VM** (any machine on 192.168.75.0/24):

```bash
# /etc/fstab
192.168.75.10:/volume1/CharlieII /mnt/CharlieII nfs defaults,_netdev 0 0
```

- NFSv3, TCP, rsize/wsize 1 MB, hard mount
- 3.6 TB total, 1.2 TB used (33%)

## What Lives on the NAS

Everything that matters for Hermes is on the NFS share — nothing critical lives on the VM itself.

### `/mnt/CharlieII/hermes/` — HERMES_HOME

| Path | Contents |
|------|----------|
| `config.yaml` | Agent config (model, provider, gateway platforms, approvals) |
| `.env` | API keys, secrets, WIKI_PATH, env vars |
| `state.db` | SQLite session store (32 MB) |
| `sessions/` | JSON session transcripts |
| `skills/` | Installed skills (26 subdirs) |
| `memories/` | Persistent agent memory |
| `logs/` | Gateway and agent logs |
| `cron/` | Scheduled cron job definitions |
| `webui/` | WebUI runtime data (sessions, settings, workspaces) |

### `/mnt/CharlieII/hermes-webui/` — WebUI source

Git clone of `nesquena/hermes-webui`. The `.env` at repo root points `HERMES_WEBUI_STATE_DIR`
back to `/mnt/CharlieII/hermes/webui/`.

### `/mnt/CharlieII/wiki/` — LLM Wiki

The knowledge base. `WIKI_PATH=/mnt/CharlieII/wiki` in `.env`.

### `/mnt/CharlieII/hermes_backup/` — Archive

| Path | What |
|------|------|
| `data/` | Old Docker-volume hermes data (May 3) |
| `data(1)(1)/` | Older Docker backup |
| `docker-compose.yaml` | Previous Docker Compose config (deprecated) |
| `webui_backup/` | WebUI state backup (May 2) |

## Bringing Up a New Machine

1. **Mount the NFS share** — add the fstab line above, `mount /mnt/CharlieII`
2. **Install Hermes CLI** — `curl -fsSL https://raw.githubusercontent.com/NousResearch/hermes-agent/main/scripts/install.sh | bash`
3. **Set HERMES_HOME** — `export HERMES_HOME=/mnt/CharlieII/hermes` (persist in `~/.bashrc` or Hermes `.env`)
4. **Shell completion** — `eval "$(hermes completion bash)"` in `~/.bashrc` (not plain `hermes completion bash` — that only prints, doesn't register)
5. **WebUI** — `cd /mnt/CharlieII/hermes-webui && ./start.sh`
6. **Gateway** — `hermes gateway install && sudo loginctl enable-linger $USER && hermes gateway start`
7. **WebUI Settings** — enable `show_cli_sessions` to see CLI sessions in sidebar

That's it. No model config, no API keys, no skill reinstalls — it all comes from the NAS.

## Systemd Auto-Start (survives reboot)

Both gateway and WebUI run as systemd user services with linger enabled. They start on boot
without any login required.

### Gateway

```bash
hermes gateway install                  # creates systemd user service
sudo loginctl enable-linger $USER       # keep running after logout
```

Systemd unit: `~/.config/systemd/user/hermes-gateway.service`

Verify:

```bash
systemctl --user is-enabled hermes-gateway   # → enabled
systemctl --user is-active hermes-gateway    # → active
loginctl show-user $USER | grep Linger       # → Linger=yes
```

### WebUI

Systemd unit created manually (no built-in install command):

```ini
# ~/.config/systemd/user/hermes-webui.service
[Unit]
Description=Hermes WebUI
After=network.target

[Service]
Type=simple
ExecStart=/mnt/CharlieII/hermes-webui/start.sh
WorkingDirectory=/mnt/CharlieII/hermes-webui
Environment="HERMES_HOME=/mnt/CharlieII/hermes"
Restart=on-failure
RestartSec=10

[Install]
WantedBy=default.target
```

```bash
systemctl --user daemon-reload
systemctl --user enable --now hermes-webui
```

### Linger

Without linger, user services die on logout. `loginctl enable-linger $USER` decouples the
service lifecycle from login sessions — the service manager starts at boot and stays up.

```
No linger:   login → services start  |  logout → services die
Linger:      boot → services start   |  survive logouts and reboots
```

Both services show `enabled` + `active` and survive full system reboots.

## Key Environment Variables

Set in `/mnt/CharlieII/hermes/.env`:

```
HERMES_HOME=/mnt/CharlieII/hermes
WIKI_PATH=/mnt/CharlieII/wiki
```

Set in `/mnt/CharlieII/hermes-webui/.env`:

```
HERMES_WEBUI_STATE_DIR=/mnt/CharlieII/hermes/webui
HERMES_WEBUI_PORT=8787
HERMES_WEBUI_HOST=0.0.0.0
HERMES_WEBUI_DEFAULT_WORKSPACE=/mnt/CharlieII
```

## Pitfalls

1. **systemd service paths** — if the Hermes source was moved (e.g. Docker → native), the systemd unit file may point to stale paths. Check: `systemctl --user edit --full hermes-gateway`. All paths should resolve under `/mnt/CharlieII/hermes/`.

2. **Shell completion** — `hermes completion bash` only prints the script. Must wrap: `eval "$(hermes completion bash)"`.

3. **WebUI CLI sessions** — `show_cli_sessions` defaults to `false`. Must toggle in WebUI Settings.

4. **NFS permissions** — files on the NFS share may be owned by container UIDs (e.g. 1002:uucp). This is normal and doesn't block reads.

5. **Linger** — without `sudo loginctl enable-linger $USER`, the gateway systemd service dies on SSH logout.

## Related

- [[hermes-architecture]] — Hermes Agent source code architecture
