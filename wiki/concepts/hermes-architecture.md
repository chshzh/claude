---
title: Hermes Agent — Architecture & Feature Overview
created: 2026-05-01
updated: 2026-05-01
type: concept
tags: [hermes, architecture, ai-agent, framework, codebase-analysis]
sources: []
confidence: high
---

# Hermes Agent — Architecture & Feature Overview

> **Reading time:** ~10 minutes | **Analyzed version:** v0.11.0 | **Codebase:** ~625,000 lines Python, ~1,300 source files, ~860 test files

## What Is Hermes?

Hermes Agent is an open-source AI agent framework by [Nous Research](https://github.com/NousResearch/hermes-agent). It's an autonomous coding and task-execution agent that runs in your terminal, on messaging platforms (Telegram, Discord, Slack, etc.), and in IDEs — with the same agent personality and full tool access everywhere. Think Claude Code or OpenAI Codex, but provider-agnostic, self-improving, and with persistent memory.

Key differentiators:
- **Skills system** — learns from experience, saves reusable procedures as documents that load into future sessions. Gets better at your specific tasks over time.
- **Persistent memory** — remembers who you are, preferences, environment, and lessons learned across sessions.
- **Multi-platform gateway** — same agent on Telegram, Discord, Slack, WhatsApp, Signal, Matrix, Email, and 10+ other platforms.
- **Provider-agnostic** — works with 20+ LLM providers (OpenRouter, Anthropic, OpenAI, DeepSeek, Google, xAI, local models, and more). Swap mid-task.

## Codebase at a Glance

```
hermes-agent/                          (~625K lines, 1303 source .py files)
├── run_agent.py         13,815 lines — Core agent conversation loop (AIAgent)
├── cli.py               11,640 lines — Interactive CLI (prompt_toolkit, TUI)
├── model_tools.py          803 lines — Tool discovery & dispatch layer
├── hermes_state.py       2,094 lines — SQLite session store (FTS5 search)
│
├── agent/                29,746 lines — Prompt building, memory, compression, model routing
│   ├── prompt_builder.py       — System prompt assembly (personality, skills, context)
│   ├── context_compressor.py   — Automatic context window compression (auxiliary LLM)
│   ├── memory_manager.py       — Orchestrates built-in + plugin memory providers
│   ├── credential_pool.py      — Multi-key API credential rotation
│   ├── context_engine.py       — Token-aware context window management
│   └── transports/             — Provider-specific API adapters (Anthropic, Bedrock, ChatCompletions)
│
├── tools/                51,386 lines — One file per tool, self-registering
│   ├── registry.py             — Central tool registry (auto-discovered)
│   ├── terminal_tool.py        — Shell execution + background processes
│   ├── browser_tool.py         — Browser automation (Browserbase/Camofox/CDP)
│   ├── file_tools.py           — File read/write/search/patch
│   ├── memory_tool.py          — Durable cross-session memory
│   ├── delegate_tool.py        — Subagent task delegation
│   ├── cronjob_tools.py        — Scheduled task management
│   ├── session_search_tool.py  — Search past conversations
│   └── ... (67 tool files total)
│
├── gateway/              19,756 lines — Messaging platform integrations
│   ├── run.py                                    — Gateway lifecycle (12,718 lines)
│   └── platforms/                               — 28 platform adapters
│       ├── telegram.py, discord.py, slack.py     — Major messaging platforms
│       ├── whatsapp.py, signal.py, matrix.py     — Encrypted messengers
│       ├── wecom.py, feishu.py, dingtalk.py      — Enterprise Chinese platforms
│       ├── weixin.py                             — WeChat
│       └── api_server.py, webhook.py             — HTTP/API integrations
│
├── hermes_cli/           65,173 lines — CLI subcommands, config, setup
│   ├── commands.py             — Slash command registry (/help, /model, /tools, etc.)
│   ├── config.py               — Default config, env vars, settings schema
│   ├── main.py                 — argparse entry point
│   ├── setup.py                — Interactive setup wizard
│   └── skills_hub.py           — Skill discovery, install, search
│
├── cron/                 2,330 lines — Background job scheduler
│   ├── scheduler.py            — Tick-based due-job executor
│   └── jobs.py                 — Job storage and lifecycle
│
├── acp_adapter/                  — IDE integration (Agent Communication Protocol)
├── tui_gateway/                  — Terminal UI gateway (WS-based)
└── tests/                ~860 test files — pytest suite (#:~:text=3000, but our count shows ~860 files)
```

## Core Architecture

### The Agent Loop (`run_agent.py` — `AIAgent`)

The heart of Hermes. A single class, `AIAgent`, that orchestrates the entire conversation:

```
run_conversation(user_message) → final_response
  │
  ├─ 1. Build system prompt
  │     ├─ Personality (persona / SOUL.md)
  │     ├─ Skills index (available skills with conditions)
  │     ├─ Memory injection (user profile + saved facts)
  │     ├─ Platform hints (Telegram/Discord/CLI-specific context)
  │     └─ Context files (AGENTS.md, .cursorrules from working directory)
  │
  ├─ 2. Conversation loop (max_turns, default 90)
  │     ├─ Call LLM (OpenAI-format messages + tool schemas)
  │     ├─ If tool_calls → dispatch each via handle_function_call()
  │     │   └─ Append tool results as messages → continue loop
  │     └─ If text response → return final answer
  │
  ├─ 3. Context compression (automatic, near token limit)
  │     └─ Compress middle turns with auxiliary LLM (cheap/fast model)
  │         preserves head (system prompt) + tail (recent messages)
  │
  └─ 4. Post-turn: sync memory, save session to SQLite
```

### State & Persistence (`hermes_state.py`)

All sessions live in a SQLite database at `~/.hermes/state.db`:

| Table | Purpose |
|-------|---------|
| `sessions` | Metadata: ID, source (cli/telegram/discord), model, tokens, cost, title |
| `messages` | Full message history: role, content, tool_calls, tool_call_id |
| `messages_fts` | FTS5 full-text index → powers `session_search` |
| `schema_version` | Currently v11 |

Key design: WAL mode for concurrent reads + one writer (gateway with multiple platforms). Session splitting on compression via `parent_session_id` chains.

### Tool System (`tools/` + `model_tools.py`)

Tools are the agent's hands. Each tool is a single file in `tools/` that self-registers:

```python
# tools/example_tool.py
registry.register(
    name="example_tool",
    toolset="example",
    schema={...},            # JSON Schema for the LLM
    handler=lambda args: ..., # Execution function
    check_fn=check_requirements,  # Only show if deps met
    requires_env=["API_KEY"],
)
```

67 tools across 30 toolsets. Key toolsets:

| Toolset | Tools |
|---------|-------|
| `terminal` | Shell commands, background processes, PTY |
| `browser` | Browser automation (CDP, Browserbase, Camofox) |
| `file` | read_file, write_file, search_files, patch |
| `memory` | Durable cross-session memory |
| `delegation` | delegate_task (subagent spawning) |
| `cronjob` | Schedule recurring tasks |
| `messaging` | send_message (cross-platform delivery) |
| `web` | Web search + content extraction |

Tools are **platform-gated**: enable/disable per source (CLI vs Telegram vs Discord) via `hermes tools`. Changes take effect on next session (`/reset`).

### Skills System

Skills are Hermes' procedural memory — reusable documents that teach the agent how to do specific tasks. They live in `~/.hermes/skills/` (or `data/skills/` in Docker):

```yaml
---
name: openwrt-wireguard
description: Deploy WireGuard VPN on OpenWrt
version: 1.0.0
---
# Step-by-step procedure...
```

Loaded per-turn from `agent/skill_preprocessing.py`. The skills index is injected into the system prompt with conditions (e.g., "load this skill when user mentions WireGuard"). Skills can be:
- **Hub-installed** from the community registry
- **User-created** via `skill_manage` tool
- **Auto-patched** by the agent when outdated or wrong

### Memory System (`agent/memory_manager.py`)

Two-tier persistent memory:

| Tier | Storage | Content |
|------|---------|---------|
| User Profile | `~/.hermes/user_profile.md` | Who the user is — name, role, devices, preferences |
| Memory Entries | `~/.hermes/memory.md` | Durable facts — environment, conventions, lessons |

Pluggable backends: built-in (markdown files), Honcho, Mem0. Only one external provider allowed at a time. Memory is injected into every turn as fenced context blocks.

### Context Compression (`agent/context_compressor.py`)

When conversation approaches the model's context limit:
1. **Tool output pruning** — old tool results replaced with placeholder
2. **LLM summarization** — middle turns compressed by auxiliary model
3. **Tail protection** — recent N tokens preserved intact
4. **Handoff framing** — summary prefixed with "different assistant" framing to prevent confused continuity

Uses a cheap/fast model for summarization (configurable via `auxiliary` settings).

### Gateway Architecture (`gateway/`)

The gateway is a long-running daemon that bridges Hermes to messaging platforms:

```
GatewayRunner (gateway/run.py)
  ├─ Platform Adapters (one per configured platform)
  │   ├─ Telegram: python-telegram-bot, handles DMs + groups + topics
  │   ├─ Discord: nextcord, handles guilds + threads + voice
  │   ├─ Slack: socket mode + events API
  │   └─ ... 25+ more
  │
  ├─ Agent Cache (LRU, 128 max, 1h idle TTL)
  │   └─ Per-session AIAgent instances cached for reuse
  │
  ├─ Cron Tick (every 60s)
  │   └─ Checks for due jobs, spawns agent runs
  │
  ├─ Delivery Engine
  │   └─ Routes agent responses → correct platform + chat + thread
  │
  └─ Session Context
      └─ Tracks per-chat session IDs, auto-resume on interruption
```

Platform adapters normalize messages into a common format and handle platform-specific quirks (telegram topics, discord threads, slack blocks, wecom encryption).

### Cron Scheduler (`cron/`)

Jobs run in fresh agent sessions. File-lock based (no DB dependency):
- Supports cron expressions and human-readable schedules ("every 2h", "30m")
- Per-job: model override, toolset restriction, working directory, script pre-run
- Output auto-delivered to configured channel (Telegram, Discord, etc.)
- Context chaining: job B receives job A's output as context

## Configuration

### Key Files

| File | Purpose |
|------|---------|
| `~/.hermes/config.yaml` | All settings (model, tools, gateway, compression, etc.) |
| `~/.hermes/.env` | Secrets: API keys, tokens, credentials |
| `~/.hermes/webui/settings.json` | Web UI preferences |
| `~/.hermes/auth.json` | OAuth tokens, credential pools |

### Profiles

Run multiple independent Hermes instances with isolated configs via `hermes profile`. Each profile gets its own `~/.hermes/profiles/<name>/` with separate sessions, skills, and memory.

### Credential Pools

Rotate across multiple API keys for the same provider. Exhausted keys are skipped automatically. Managed via `hermes auth`.

## Transport & Provider Architecture

Hermes abstracts provider differences through transports:

```
run_agent.py
  └─ agent/transports/
       ├─ chat_completions.py  → OpenAI-compatible (OpenRouter, DeepSeek, xAI, local, 15+ providers)
       ├─ anthropic.py         → Anthropic native (Claude models, prompt caching)
       ├─ bedrock.py           → AWS Bedrock
       └─ codex.py             → OpenAI Codex responses API
```

Tool call parsing adapts to provider quirks via `environments/tool_call_parsers/` — dedicated parsers for DeepSeek, GLM, Qwen, Mistral, Llama, Kimi K2, and more.

## Deployment Modes

| Mode | Command | Use Case |
|------|---------|----------|
| Interactive CLI | `hermes` | Development, one-off tasks |
| One-shot | `hermes chat -q "..."` | Scripts, CI/CD, cron |
| Gateway (foreground) | `hermes gateway run` | Testing, debugging |
| Gateway (service) | `hermes gateway install` | Production, always-on |
| MCP Server | `hermes mcp serve` | IDE integration |
| ACP Server | `hermes acp` | Agent Communication Protocol |
| Web Dashboard | `hermes dashboard` | Browser-based chat UI |
| Docker | `docker compose up` | Isolated deployment |

Docker runs three containers typically: `hermes-gateway`, `hermes-web-server`, and `redis` (for pub/sub).

## Working Directory Integration

When run from a project directory, Hermes automatically discovers and injects:
- `AGENTS.md` / `CLAUDE.md` / `.cursorrules` — project-specific instructions
- Git context — branch, recent commits, changed files
- These files are scanned for prompt injection before loading (regex patterns for "ignore instructions", secret exfiltration, invisible Unicode)

## Key Statistics

| Metric | Value |
|--------|-------|
| Total Python source lines | ~625,000 |
| Source files | ~1,300 |
| Test files | ~860 |
| Platform adapters | 28 |
| Supported LLM providers | 20+ |
| Core tools | 67 |
| Available toolsets | ~30 |
| Largest module | `run_agent.py` (13,815 lines) |
| Gateway runner | `gateway/run.py` (12,718 lines) |
| CLI module | `cli.py` (11,640 lines) |
| State schema version | v11 |

## Release History

> **Cadence:** Median 5 days between releases (range 2–7). Git uses date tags (`v2026.4.23`); semantic versions are in `pyproject.toml`.

| Version | Date | Days Since Prev | Highlights |
|---------|------|-----------------|------------|
| v0.2.0 | 2026-03-12 | — | Formal versioning begins; curated first-release changelog |
| v0.3.0 | 2026-03-17 | 5 | |
| v0.4.0 | 2026-03-23 | 6 | |
| v0.5.0 | 2026-03-28 | 5 | |
| v0.6.0 | 2026-03-30 | 2 | |
| v0.7.0 | 2026-04-03 | 4 | |
| v0.8.0 | 2026-04-08 | 5 | |
| v0.9.0 | 2026-04-13 | 5 | |
| v0.10.0 | 2026-04-16 | 3 | |
| **v0.11.0** | 2026-04-23 | 7 | Current — 28 platforms, 20+ providers, skills, profiles, credential pools |

*(v0.1.0 from 2024-10-18 was the pre-restructure alpha on the old `research` branch — not part of this release lineage.)*

## Design Principles Observable in the Code

1. **Self-registration pattern** — tools auto-discover via `registry.register()`, no manual import lists
2. **Platform gating** — tools/skills/settings can differ per platform (CLI vs Telegram vs Discord)
3. **Prompt caching awareness** — no mid-session tool or system prompt changes (would invalidate LLM cache)
4. **Lazy imports** — 240ms `openai.OpenAI` import deferred via proxy pattern
5. **Thread-safe async bridging** — persistent event loops per thread for tool handlers
6. **File-based locking** — cron scheduler uses `fcntl` locks, no DB dependency for scheduling
7. **Security scanning** — context files scanned for prompt injection, secrets, invisible Unicode before injection
8. **Multi-key rotation** — credential pools with automatic exhaustion tracking

## Related

- [[svg-pptx-agent-generation]] — Case study: how Hermes generates SVG diagrams and PPTX presentations from a single prompt (using tools: `write_file`, `delegate_task`, `terminal`, PptxGenJS)
- [[hermes-setup-on-linux-with-nas-nfs-backup]] — NAS NFS deployment: UGreen NAS hosts all Hermes data, new machines recover by mounting NFS
