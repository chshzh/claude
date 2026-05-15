---
name: chsh-sk-memfault
description: Upload Memfault symbol files and perform OTA release workflow for nordic-wifi-memfault on nrf54lm20dk and nrf7002dk. Use when uploading .elf symbol files to Memfault, creating an OTA release, deploying a release to a cohort, or disabling/aborting active release deployments. Delegates execution to the chsh-ag-memfault subagent.
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

Project root: `/opt/nordic/ncs/v3.3.0/nordic-wifi-memfault`

---

## Quick reference

| Scenario | Workflow | Rebuild needed? |
|----------|----------|-----------------|
| Debug crash (symbol-only) | A | Only if artifacts are missing |
| Release (symbols + OTA + deploy) | B | Yes if re-releasing same version |
| Stop an OTA update mid-rollout | C | No |

---

## Step 1 — Determine task

Read the user's request and decide:

| User says | Target workflow |
|-----------|----------------|
| "upload symbols", "debug crash", "decode coredump" | Workflow A |
| "release", "publish", "upload OTA", "deploy" | Workflow B |
| "disable release", "abort OTA", "stop update", "pull deployment" | Workflow C |
| "rebuild and release / update" | Rebuild first (Step 2), then Workflow B |

---

## Step 2 — Rebuild (only if requested or artifacts are missing)

If the user asks to rebuild, or if the `.signed.bin` / `.elf` files are absent,
build both DKs first using `chsh-sk-ncs-env` (toolchain v3.3.0, bundle `0c0f19d91c`):

```bash
BUNDLE_ID="0c0f19d91c"
export PATH="/opt/nordic/ncs/toolchains/${BUNDLE_ID}/bin:$PATH"
export GIT_EXEC_PATH=$(ls -d /opt/nordic/ncs/toolchains/${BUNDLE_ID}/Cellar/git/*/libexec/git-core)
source /opt/nordic/ncs/v3.3.0/zephyr/zephyr-env.sh
cd /opt/nordic/ncs/v3.3.0/nordic-wifi-memfault

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

## Pitfalls

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

### `memfault_log_trigger_collection()` in `on_connect()` is harmful

Calling `memfault_log_trigger_collection()` in the reconnect handler (`on_connect()`)
**freezes the log buffer immediately after reconnect**, blocking new log writes until
the upload completes. This causes post-reconnect `LOG_INF` messages to be silently
dropped by `prv_try_free_space()` (which returns `false` when `triggered=true`).

**Correct pattern:** call `memfault_log_trigger_collection()` **only on disconnect**
(WiFi deassociation + network-layer loss), never on connect. The periodic upload handles
log collection for the normal case.

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
| Debug UART logs, capture disconnect traces | `chsh-sk-ncs-debug` |
| Git commit + push after release | `chsh-sk-git` |
| Cut a GitHub release with firmware artifacts | `chsh-sk-git-release` |

---

## Self-Update Policy

At the **end of each conversation** involving Memfault work, check whether any
of the following changed and update this skill or `chsh-ag-memfault` accordingly:

- New Memfault API behaviour discovered (e.g. new error codes, missing fields)
- New workflow steps added (e.g. new cohort, new board target)
- Build commands or toolchain bundle IDs changed
- Credentials file location or format changed

Apply updates immediately if the change is clear; ask the user if uncertain.
