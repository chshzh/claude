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

---

## Reference Implementations

Before writing any code, browse both repos to understand the patterns in use:

| Repo | Patterns to reference |
|------|-----------------------|
| [`nordic-wifi-webdash`](https://github.com/chshzh/nordic-wifi-webdash) | Web dashboard integration, HTTP server over Wi-Fi, `app_httpd` library wrapper, HTML/CSS/JS serving from littlefs, Wi-Fi station mode management |
| [`nordic-wifi-memfault`](https://github.com/chshzh/nordic-wifi-memfault) | Memfault OTA + cloud integration, `app_memfault` library wrapper, MQTT app module, Zbus channel design, NVS credential storage, SMF state machine for connectivity |

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
# App module with state machine (SMF + Zbus)
# → reference: nordic-wifi-memfault/src/modules/app_connectivity/

# Library wrapper module
# → reference: nordic-wifi-memfault/src/modules/app_memfault/
# → reference: nordic-wifi-webdash/src/modules/app_httpd/

# Main.c + Kconfig + CMakeLists.txt patterns
# → reference: nordic-wifi-memfault/CMakeLists.txt, Kconfig, prj.conf
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
