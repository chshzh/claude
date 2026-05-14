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

## Related Skills

| Task | Skill |
|------|-------|
| Build / flash / west commands | `chsh-sk-ncs-env` |
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
