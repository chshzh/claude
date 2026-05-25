---
name: chsh-sk-ncs-3.1-coding
description: >-
  Load when implementing NCS project code from engineering specs. Reads specs
  in docs/dev-specs/ and uses nordic-wifi-webdash and nordic-wifi-memfault as
  reference implementations. Use when specs are ready and code needs to be
  written or updated.
---

# chsh-sk-ncs-3.1-coding — NCS Code Implementation

Implements NCS firmware from the engineering specs in `docs/dev-specs/`.
Uses two real project repositories as reference implementations.

> **Prerequisite**: `docs/dev-specs/architecture.md` and at least one module
> spec must exist. If specs are missing, run **chsh-sk-ncs-2-spec** first.

> **Knowledge sources**: Call `mcp_nrflow_nordicsemi_workflow_ncs` to load `embedded-code-guidance-ncs-zephyr` — covers NCS code style, Kconfig patterns, and driver idioms. Use `mcp_nrflow_nordicsemi_search_sources` for API lookups, board targets, and driver symbol names.

---

## Reference Implementations

Before writing any code, browse both repos to understand the patterns in use:

| Repo | Patterns to reference |
|------|-----------------------|
| [`nordic-wifi-webdash`](https://github.com/chshzh/nordic-wifi-webdash) | **SMF+Zbus** modular architecture, multi-mode Wi-Fi (SoftAP/STA/P2P_GO/P2P_CLIENT), `mode_selector` NVS+shell pattern, HTTP webserver with gzip static assets from flash, REST API design, event-triggered service start on zbus (`CLIENT_CONNECTED_CHAN`) |
| [`nordic-wifi-memfault`](https://github.com/chshzh/nordic-wifi-memfault) | **SYS_INIT+Zbus** event-driven modules (no SMF), Memfault metrics/coredump/OTA, BLE Wi-Fi credential provisioning, NTP time sync, disconnect-time log persist to external flash, optional MQTT/HTTPS/CDR modules via Kconfig flags, button-driven debug flows |

Use the `github_repo` or `fetch_webpage` tool to read specific files from these
repos when implementing a similar module.

---

## Step 0 — Read Inputs

```bash
cat docs/dev-specs/architecture.md   # module map, Zbus channels, boot order
ls docs/dev-specs/                   # all module specs
git log --oneline -5                 # recent commits
ls src/modules/ 2>/dev/null          # existing modules
```

Identify what needs to be created vs updated:
- **New project**: all modules are new → follow Steps 1–2 for each
- **Specs changed**: compare spec Revision History against `git log -- docs/dev-specs/` → implement only changed modules
- **Bug fix**: no spec change → go directly to the affected module

---

## Step 1 — Browse Reference Implementations

For each module in the spec, find the closest analogue in the reference repos:

```
# SMF module (state machine + Zbus publish)
# → reference: nordic-wifi-webdash/src/modules/button/   (SMF 3-state, BUTTON_CHAN publish)
# → reference: nordic-wifi-webdash/src/modules/led/      (SMF 2-state per LED, LED_CMD_CHAN/LED_STATE_CHAN)

# SYS_INIT + Zbus listener/subscriber module (no SMF)
# → reference: nordic-wifi-memfault/src/modules/network/          (WIFI_CHAN + NETWORK_CHAN publish)
# → reference: nordic-wifi-memfault/src/modules/app_memfault/core/ (zbus subscriber, upload on connect)

# Library wrapper module (wraps external SDK or Zephyr subsystem)
# → reference: nordic-wifi-memfault/src/modules/app_memfault/     (Memfault SDK wrapper with core/metrics/ota/cdr)
# → reference: nordic-wifi-webdash/src/modules/webserver/         (Zephyr HTTP server + REST API wrapper)

# Main.c + Kconfig + CMakeLists.txt patterns
# → reference: nordic-wifi-memfault/CMakeLists.txt, Kconfig, prj.conf  (shell disabled, ZMS settings)
# → reference: nordic-wifi-webdash/CMakeLists.txt, Kconfig, prj.conf   (shell enabled, NVS settings)
```

Look for: how Zbus channels are declared, how `SYS_INIT` is used, how Kconfig
guards modules, how callbacks are wired to the rest of the app.

---

## Step 2 — Implement

For each module from the spec:

1. Create `src/modules/<name>/` with:
   - `<name>.c` — state machine/thread loop, Zbus integration, callback implementations
   - `<name>.h` — public API and type declarations (if needed)
   - `Kconfig.<name>` — module Kconfig with `CONFIG_APP_<MODULE>_MODULE`
   - `CMakeLists.txt` — `zephyr_library_sources` guarded by `CONFIG_APP_<MODULE>_MODULE`

2. Wire into top-level:
   - `Kconfig`: `rsource "src/modules/<name>/Kconfig.<name>"`
   - `prj.conf`: `CONFIG_APP_<MODULE>_MODULE=y` + required Kconfig from spec
   - `CMakeLists.txt`: `add_subdirectory(src/modules/<name>)`

3. Embed the spec version in `src/main.c`:
   ```c
   #define SPECS_VERSION "YYYY-MM-DD-HH-MM"  /* from docs/dev-specs/overview.md Changelog */
   LOG_INF("<project> built from specs v" SPECS_VERSION);
   ```

4. Build and verify:
   ```bash
   west build -p -b <board>
   ```
   Fix all warnings. Confirm UART output matches spec test points.

5. Handoff:
   > "Implementation complete. Run **chsh-sk-ncs-4.1-verification** for Phase 4
   > Verification & Test (static review + hardware validation)."

---

## Gotchas

| Gotcha | Detail |
|--------|--------|
| Zbus publish in ISR context | `zbus_chan_pub()` is not ISR-safe — use a work queue or `k_msgq` to defer from ISR |
| `SYS_INIT` order for library wrappers | `APPLICATION` level runs after `POST_KERNEL`; use explicit priority numbers when one lib depends on another |
| Kconfig `depends on` vs `select` | Use `depends on` for optional features; use `select` only when the module unconditionally requires a lib (mirrors reference repos) |
| `prj.conf` overlay order | `EXTRA_CONF_FILE` overlays win; put test-only settings in overlays, not `prj.conf` |
| `west build -p` vs incremental | Always use `-p` when switching board or overlay set; incremental builds can silently keep stale objects |

---

## Related Skills

| Task | Skill |
|------|-------|
| Generate engineering specs | `chsh-sk-ncs-2-spec` |
| Phase 4 Verification & Test | `chsh-sk-ncs-4.1-verification` |
| Debug build failures, UART analysis | `chsh-sk-ncs-3.2-debug` |
| Commit after implementation | `chsh-sk-git-commit` |
| Full lifecycle orchestration | `chsh-sk-ncs-0-workflow` |

## Self-Update Policy

At the **end of each conversation**, review what was discovered and check
whether any facts in this skill are new, corrected, or outdated (e.g. new
reference repo patterns, Zbus gotchas, Kconfig conventions found during
implementation).

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
