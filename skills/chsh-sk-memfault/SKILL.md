---
name: chsh-sk-memfault
description: Use when uploading .elf symbol files to Memfault, creating an OTA release, deploying to a cohort, aborting deployments, querying device state, or decoding crash traces for nordic-wifi-memfault. Covers both Memfault MCP (read-only fleet/trace queries) and CLI (symbol upload, OTA release, deploy) via chsh-ag-memfault.
---

# chsh-sk-memfault

Orchestrates Memfault symbol upload, OTA release, and deployment tasks for
`nordic-wifi-memfault` by delegating to the `chsh-ag-memfault` subagent.

> **Execution model**: This skill is a dispatcher. All Memfault API calls,
> approval gates, and file operations are handled by `chsh-ag-memfault`.
> This skill's job is to: determine what the user wants → optionally rebuild →
> then hand off to the agent.

---

## Project constants

| DK | `--software-type` | ELF | OTA payload |
|----|-------------------|-----|-------------|
| nrf54lm20dk | `nrf54lm20dk-fw` | `build_nrf54lm20dk/nordic-wifi-memfault/zephyr/zephyr.elf` | `build_nrf54lm20dk/nordic-wifi-memfault/zephyr/zephyr.signed.bin` |
| nrf7002dk | `nrf7002dk-fw` | `build_nrf7002dk/nordic-wifi-memfault/zephyr/zephyr.elf` | `build_nrf7002dk/nordic-wifi-memfault/zephyr/zephyr.signed.bin` |

Project root: `$(west topdir)/nordic-wifi-memfault`

### Two Memfault projects — local credentials only cover `nord-project`

CI builds firmware for two Memfault projects: **`nord-project`** (`nrf-test` / `nordic` org)
and **`terr-project`**. Credentials stored in `overlay-app-memfault-project-info.conf` are
for `nord-project` only. `terr-project` credentials live exclusively in GitHub Secrets
(`TERR_KEY`). Unless the user supplies terr-project credentials explicitly, only run
Workflow B for `nord-project`.

GitHub release artifacts follow this naming pattern:
```
nordic-wifi-memfault-{board}-{project}-{version}.{ext}
```
Examples for `3.3.0.2`:
- `nordic-wifi-memfault-nrf7002dk-nord-project-3.3.0.2.elf`
- `nordic-wifi-memfault-nrf7002dk-nord-project-3.3.0.2.signed.bin`
- `nordic-wifi-memfault-nrf54lm20dk-nrf7002ebii-nord-project-3.3.0.2.elf`
- `nordic-wifi-memfault-nrf54lm20dk-nrf7002ebii-nord-project-3.3.0.2.signed.bin`

Download pattern (replace `<VER>` with tag name):
```bash
mkdir -p /tmp/fw/memfault-<VER>
gh release download <VER> --repo chshzh/nordic-wifi-memfault \
  --pattern "*.elf" --pattern "*.signed.bin" -D /tmp/fw/memfault-<VER>/
```

---

## Default Workflow (project folder invocation)

When the user invokes this skill with a project folder reference (e.g.
`@<ncs-version>/nordic-wifi-memfault`) **without specifying a workflow**, execute
this sequence automatically — no further clarification needed:

1. **Rebuild** both boards in parallel (Step 2)
2. **Flash** both boards in parallel (Step 2b)
3. **Upload symbols without version** — Workflow A (Step 3)

This default applies to `nordic-wifi-memfault` at
`$(west topdir)/nordic-wifi-memfault`.

---

## Quick reference

| Scenario | Workflow | Rebuild needed? |
|----------|----------|-----------------|
| Default (project folder, no explicit task) | Rebuild → Flash → A | Yes |
| Debug crash (symbol-only) | A | Only if artifacts are missing |
| Release (symbols + OTA + deploy) | B | Yes if re-releasing same version |
| Stop an OTA update mid-rollout | C | No |

---

## Step 1 — Determine task

Read the user's request and decide:

| User says | Target workflow |
|-----------|----------------|
| Project folder only, no specific task | Default: Rebuild → Flash → Workflow A |
| "upload symbols", "debug crash", "decode coredump" | Workflow A |
| "release", "publish", "upload OTA", "deploy" | Workflow B |
| "disable release", "abort OTA", "stop update", "pull deployment" | Workflow C |
| "rebuild and release / update" | Rebuild first (Step 2), then Workflow B |

---

## Step 2 — Rebuild (only if requested or artifacts are missing)

> **CRITICAL — version string must match release version before building.**
> Before rebuilding, update `overlay-app-memfault-project-info.conf` to set
> `CONFIG_MEMFAULT_NCS_FW_VERSION="<target-version>"` (e.g. `"3.3.0.1"`).
> Uploading a binary whose embedded version string differs from the Memfault
> `--software-version` causes the device to report the wrong version after OTA.
> Confirm the overlay is correct before starting any build.

If the user asks to rebuild, or if the `.signed.bin` / `.elf` files are absent,
build both DKs first using `chsh-sk-ncs-env` ((use chsh-sk-ncs-env to resolve toolchain + bundle)):

```bash
BUNDLE_ID="0c0f19d91c"
export PATH="/opt/nordic/ncs/toolchains/${BUNDLE_ID}/bin:$PATH"
export GIT_EXEC_PATH=$(ls -d /opt/nordic/ncs/toolchains/${BUNDLE_ID}/Cellar/git/*/libexec/git-core)
source $(west topdir)/zephyr/zephyr-env.sh
cd $(west topdir)/nordic-wifi-memfault

# nrf54lm20dk
west build -b nrf54lm20dk/nrf54lm20a/cpuapp -d build_nrf54lm20dk -p \
  -- -DSHIELD=nrf7002eb2 \
     -DEXTRA_CONF_FILE="overlay-app-memfault-project-info.conf"

# nrf7002dk
west build -b nrf7002dk/nrf5340/cpuapp -d build_nrf7002dk -p \
  -- -DEXTRA_CONF_FILE="overlay-app-memfault-project-info.conf"
```

Run both builds in parallel (background each with `block_until_ms: 0`), then
await both before proceeding.

---

## Step 2b — Flash (only if the user asks to flash)

If the user asks to flash after building, flash both boards in parallel.

> **NEVER use `--recover` or `--erase` when flashing.** These flags wipe the
> entire flash including the NVS partition where Wi-Fi credentials are stored.
> Use plain `west flash` without any erase flag.

Known board serial numbers:

| Board | Serial |
|-------|--------|
| nRF54LM20DK (PCA10184) | `1051806714` |
| nRF7002DK (PCA10143) | `1050718454` |

```bash
BUNDLE_ID="0c0f19d91c"
export PATH="/opt/nordic/ncs/toolchains/${BUNDLE_ID}/bin:$PATH"
export GIT_EXEC_PATH=$(ls -d /opt/nordic/ncs/toolchains/${BUNDLE_ID}/Cellar/git/*/libexec/git-core)
source $(west topdir)/zephyr/zephyr-env.sh
cd $(west topdir)/nordic-wifi-memfault

# nrf54lm20dk — NO --recover, preserves NVS/Wi-Fi credentials
nrfutil sdk-manager toolchain launch --ncs-version=${NCS_VERSION:-v3.3.0} -- \
  west flash -d build_nrf54lm20dk --dev-id 1051806714

# nrf7002dk — NO --recover, preserves NVS/Wi-Fi credentials
nrfutil sdk-manager toolchain launch --ncs-version=${NCS_VERSION:-v3.3.0} -- \
  west flash -d build_nrf7002dk --dev-id 1050718454
```

Run both in parallel (background each with `block_until_ms: 0`), then await both.

---

## Step 3 — Delegate to chsh-ag-memfault

After any required rebuild completes, hand off to the agent:

```
Invoke subagent: chsh-ag-memfault
Task: <workflow letter(s) and version, e.g.:
  "Run Workflow B for version 3.3.0.1 — artifacts are already built."
  "Run Workflow C — list active deployments for 3.3.0.1 and ask which to disable."
  "Run Workflow A — upload symbols without version (crash debug).">
```

The agent handles all pre-flight checks, AskQuestion approval gates, API calls,
and reporting. Do not duplicate that logic here.

---

## Memfault MCP Tools (read-only observability)

The `mcp_memfault_*` tools expose the Memfault API for **read/query operations
only**. They complement the CLI workflow but **cannot** upload symbols, create
releases, or deploy OTA. Use them alongside the agent, not as a replacement.

| Tool | Use case |
|------|----------|
| `mcp_memfault_projects_list` | Confirm project slug before any operation |
| `mcp_memfault_device_get` | Inspect device state (cohort, software version, last seen) |
| `mcp_memfault_device_search` | SQL filter fleet — e.g. `software_version = '3.3.0.1'` |
| `mcp_memfault_device_getAttributes` | Read attribute metrics on a device |
| `mcp_memfault_device_listReboots` | Check recent reboot reasons on a device |
| `mcp_memfault_issue_get` | Fetch crash issue details by ID |
| `mcp_memfault_trace_get` | Decode crash trace (stacktrace + fault analysis + logs) |
| `mcp_memfault_metrics_list` | List available timeseries/attribute metric keys |

**Typical use after Workflow B (deploy):** run `mcp_memfault_device_search` with
`software_version = '<new-version>'` to confirm devices have updated, or
`mcp_memfault_device_listReboots` to check for unexpected reboots post-OTA.

**Typical use after Workflow A (symbols):** run `mcp_memfault_trace_get` with
`include_logs: true` to decode a crash trace directly in the conversation —
no separate symbol-server lookup needed once symbols are uploaded.

Default project slug: `nrf-test`

---

## Pitfalls

### Flashing with --recover / --erase Wipes Wi-Fi Credentials

`west flash --recover` and `west flash --erase` erase the entire flash including
the NVS partition where Wi-Fi credentials are stored. Always use plain `west flash`
(no erase flag) when credentials should be preserved.

### Log File Rate Limiting (silent failure)

Memfault **rate-limits log file ingestion** on the backend — officially "no more than
once per hour per device" per the docs (`memfault_log_trigger_collection()` guidance).

**Key behaviour:**
- The firmware **does not see a 429**. Chunks are accepted at `/api/v0/chunks/{serial}`
  with **HTTP 202** even when the assembled log file is then dropped due to rate limiting.
- There is **no firmware-side warning** (no serial log, no error code from `post_data()`).
- `last_seen` continues updating every upload interval — device appears healthy.
- The only visible evidence is the **Memfault Platform UI → Device page → Developer Mode
  tab → Processing Errors** section.

**Common trigger:** `CONFIG_MEMFAULT_PERIODIC_UPLOAD_INTERVAL_SECS=60` causes
`memfault_log_trigger_collection()` to fire 60×/hour — 60× the recommended limit.

**Fix for testing:** Enable **Server-Side Developer Mode** on the Device page
(bypasses rate limits for that device). Do not use 60s interval in production.
Production recommendation: `MEMFAULT_PERIODIC_UPLOAD_INTERVAL_SECS=3600`.

### `memfault_log_trigger_collection()` — correct usage pattern

**On disconnect:** do NOT call `memfault_log_trigger_collection()`. Instead, the
persist-once work item saves the ring-buffer state to external flash after a 10 s delay.
Calling trigger on disconnect would freeze the RAM buffer for brief reconnects where the
ring buffer survives intact — unnecessarily dropping new logs.

**On connect (after flash restore only):** call `memfault_log_trigger_collection()` once,
immediately after `memfault_log_state_restore_on_connect()` returns 0. The ring buffer
was restored from flash and has no saved trigger watermark (power cycle removed it from
RAM). This is the only correct place to call it on connect.

**Never call it unconditionally on every connect.** That freezes the buffer immediately
after reconnect, silently dropping all post-reconnect `LOG_INF` messages until the upload
completes (via `prv_try_free_space()` returning false while `triggered=true`).

### Confirming logs are flowing

After any Memfault log change, always enable Developer Mode on the device and check:
```python
import requests, base64
KEY = '<project_key>'
headers = {'Authorization': 'Basic ' + base64.b64encode(f':{KEY}'.encode()).decode()}
r = requests.get('https://api.memfault.com/api/v0/organizations/<org>/projects/<proj>'
                 '/devices/<serial>/log-files', headers=headers)
print(r.json()['data'][:3])  # newest first
```
Expected: new entries within 60–90s of device boot (when developer mode is on).

---

## Related Skills

| Task | Skill |
|------|-------|
| Build / flash / west commands | `chsh-sk-ncs-env` |
| Debug UART logs, capture disconnect traces | `chsh-sk-ncs-3.2-debug` |
| Git commit + push after release | `chsh-sk-git-commit` |
| Cut a GitHub release with firmware artifacts | `chsh-sk-git-release` |

---

## Gotchas
- TODO: add one entry per real observed failure or routing false-positive

## Self-Update Policy

At the **end of each conversation** involving Memfault work, check whether any
of the following changed and update this skill or `chsh-ag-memfault` accordingly:

- New Memfault API behaviour discovered (e.g. new error codes, missing fields)
- New workflow steps added (e.g. new cohort, new board target)
- Build commands or toolchain bundle IDs changed
- Credentials file location or format changed

Apply updates immediately if the change is clear; ask the user if uncertain.
