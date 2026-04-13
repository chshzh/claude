---
name: chsh-dev-project
description: Implement NCS project code from engineering specs in docs/specs/. Scaffolds new projects, implements modules, and updates code when specs change. Use when specs are ready and you need to write or update code. For generating specs, use chsh-dev-spec first.
---

# chsh-dev-project — Code Implementation Workflow

Implements NCS project code from the engineering specs in `docs/specs/`.
Specs are the contract — **always read the spec before writing code**.

> **Prerequisite**: `docs/specs/architecture.md` and at least one module spec must exist.
> If specs are missing, run **chsh-dev-spec** first.

---

## Step 0 — Detect Context

Check what exists before choosing a mode:

```bash
cat docs/specs/architecture.md    # architecture overview
ls docs/specs/*.md                # all module specs
ls src/modules/ 2>/dev/null                   # existing modules
git log --oneline -5                          # recent commits
```

| Condition | Mode |
|-----------|------|
| Specs exist, no code yet | **A — New Project** |
| Specs changed (check Revision History), code needs updating | **B — Update Implementation** |
| Small bug fix, no spec change needed | **C — Code Fix** |

Ask the user to confirm the mode before proceeding.

---

## Mode A — New Project (Scaffold + Implement)

### A1. Read all specs

Load:
1. `docs/specs/architecture.md` — module map, Zbus channels, boot order, memory budget
2. Each `docs/specs/<module>.md`

### A2. Create project scaffold

```bash
mkdir -p src/modules docs/specs

# Base templates
cp ~/.claude/skills/chsh-dev-project/templates/LICENSE .
cp ~/.claude/skills/chsh-dev-project/templates/.gitignore .
cp ~/.claude/skills/chsh-dev-project/templates/README_TEMPLATE.md README.md

# Wi-Fi config (pick from spec)
cp ~/.claude/skills/chsh-dev-project/overlays/overlay-<mode>.conf .
```

### A3. Implement each module

Check the spec's **Module Type** field. Use the right pattern:

**Application module** (SMF+Zbus or multi-threaded):

1. Create `src/modules/<name>/` with:
   - `<name>.c` — state machine or thread loop + Zbus integration
   - `<name>.h` — public API, types, channel declarations
   - `Kconfig.<name>` — module Kconfig following the spec's Kconfig table
   - `CMakeLists.txt` — `zephyr_library_sources` guarded by `CONFIG_APP_<MODULE>_MODULE`

**Library wrapper module** (`app_<lib>/`):

1. Create `src/modules/app_<lib>/` with:
   - `app_<lib>.c` — library init (`SYS_INIT`), API calls, callback implementations,
     and Zbus publishing for app lifecycle integration. See **Library Wrapper Modules** below.
   - `app_<lib>.h` — public interface (if needed) guarded by Kconfig
   - `Kconfig.app_<lib>` — `CONFIG_APP_<LIB>_MODULE=y`, must `select CONFIG_<LIB>`
   - `CMakeLists.txt` — guarded by `CONFIG_APP_<LIB>_MODULE`

For both types:

2. Add module Kconfig to top-level `Kconfig`:
   ```kconfig
   rsource "src/modules/<name>/Kconfig.<name>"
   ```

3. Add module config block to `prj.conf`:
   ```
   # <Module name>
   CONFIG_APP_<MODULE>_MODULE=y
   <additional config from spec's Kconfig table>
   ```

4. Wire into `CMakeLists.txt`:
   ```cmake
   add_subdirectory(src/modules/<name>)
   ```

5. Wire Zbus subscriptions into `src/main.c` as needed.

### A4. Generate `src/modules/messages.h`

Contains all shared Zbus message structs from `architecture.md`.

### A4b. Record spec version in `src/main.c`

Read the latest version timestamp from `docs/specs/overview.md`'s Changelog
table and embed it as a startup log line so every UART capture is traceable to its spec:

```c
/* Spec version from docs/specs/overview.md */
#define SPECS_VERSION "YYYY-MM-DD-HH-MM"

/* In main() or SYS_INIT entry: */
LOG_INF("<project> built from specs v" SPECS_VERSION);
```

Update this define whenever `chsh-dev-spec` adds a new Changelog entry to `overview.md`.

### A5. Build and verify

```bash
west build -p -b <board>
```

Fix build errors. Verify that UART output matches the test points listed in each spec.

### A6. Handoff

> "Implementation complete. Run **chsh-qa-test** to validate against PRD and specs."

---

## Mode B — Update Implementation (Specs Changed)

### B1. Identify what changed

Compare the spec's **Revision History** table against the last time code was updated:

```bash
git log --oneline -- docs/specs/    # spec commit history
git log --oneline -- src/modules/              # code commit history
```

Ask the user which specs changed if not immediately clear.

### B2. Map spec changes to code impact

For each changed spec:

| Spec change | Code impact |
|-------------|-------------|
| New Kconfig flag | Add to `Kconfig.<name>` and `prj.conf` |
| State machine change | Update SMF states/transitions in `<name>.c` |
| Zbus channel added | Add channel definition in `<name>.h`, subscription in `<name>.c` |
| New API function | Implement in `<name>.c`, declare in `<name>.h` |
| Lib API call added/changed | Update `app_<lib>.c`; verify thread context and safety |
| Lib callback signature changed | Update callback implementation in `app_<lib>.c` |
| Module added | Follow Mode A steps for that module |
| Module removed | Remove `src/modules/<name>/`, remove from `prj.conf`, `Kconfig`, `CMakeLists.txt` |

### B3. Implement the changes

Apply only the code changes required by the spec delta — do not refactor unrelated code.
Update `SPECS_VERSION` in `src/main.c` to the latest timestamp from `overview.md`'s Changelog.

### B4. Build and verify

```bash
west build -p -b <board>
```

Verify that UART output still matches the test points in each spec (including unchanged modules).

### B5. Handoff

> "Implementation updated. Run **chsh-qa-test** to validate the new build."

---

## Mode C — Code Fix

For bug fixes or minor adjustments that do not require spec changes.

1. Describe the bug or issue clearly.
2. Identify the affected module(s).
3. Fix the code.
4. If the fix reveals a spec gap (undocumented behaviour), note it and offer to update the spec via **chsh-dev-spec** Mode B.
5. Build and verify.

---

## Coding Standards

Always follow these rules when generating NCS/Zephyr code:

### Architecture

> **Scope**: the chosen architecture pattern (SMF+Zbus or multi-threaded) applies to
> **application code only**. External libraries (Memfault SDK, Wi-Fi driver, BLE stack)
> run in their own internal threads and are **not** subject to this pattern.
> Wrapper modules (`app_<lib>/`) form the interface boundary.

- **SMF+Zbus**: each application module is a `SYS_INIT`-registered state machine; communicates only via Zbus channels
- **Multi-threaded**: each application module has its own `k_thread`; communicates via message queues or semaphores
- Never use global variables to share state between application modules — use Zbus channels

#### Threads ≠ Modules

A thread is an **execution context**. A module is **logic**. Do not conflate them:

- SMF driven from hardware callbacks (`dk_buttons_init`, net_mgmt, zbus listener) needs **no dedicated thread**. The state machine runs inside the callback's context.
- Only add a thread when the module must **block-wait on events** (e.g. `zbus_sub_wait`) or do **sustained, long-running processing**.
- A thread whose body is only `while(1) { k_sleep(K_MSEC(N)); }` with no blocking kernel object serves zero purpose — **remove it**.

#### zbus Observer Rules

| Observer type | Run context | Rules |
|---|---|---|
| `ZBUS_LISTENER_DEFINE` | Publisher's thread (VDED) | **ISR-equivalent**: must finish in microseconds. **Forbidden**: `k_sleep`, blocking I/O, mutex with non-zero timeout, `zbus_chan_pub` with non-zero timeout. |
| `ZBUS_ASYNC_LISTENER_DEFINE` | System work queue | OK for deferred light work (< ~20 ms). Safe to call `k_work_submit/k_work_schedule`. Gets message copy. |
| `ZBUS_MSG_SUBSCRIBER_DEFINE` | Dedicated thread loops on `zbus_sub_wait_msg` | Use when module must block-wait with zero data loss. |
| `ZBUS_SUBSCRIBER_DEFINE` | Dedicated thread loops on `zbus_sub_wait` | Use when occasional loss is acceptable (notification only). |

**For deferred work triggered by a zbus listener**, never `k_sleep()` — schedule a work item instead:
```c
/* In the listener (VDED context) — safe: */
k_work_schedule(&my_work, K_MSEC(1000));

/* In the work handler (system work queue) — safe to do I/O: */
static void my_work_fn(struct k_work *w) { /* start server, etc. */ }
static K_WORK_DELAYABLE_DEFINE(my_work, my_work_fn);
```

#### SYS_INIT Safety Rules

- **Never use `K_FOREVER`** in a `SYS_INIT` callback — if the awaited resource never arrives, boot hangs permanently.
- Use a bounded timeout: `k_sem_take(&sem, K_SECONDS(30))`; on failure: `LOG_ERR(…); return -ETIMEDOUT;`
- `SYS_INIT` callbacks run sequentially on the main thread; an infinite block prevents all subsequent init from executing.

### Library Wrapper Modules (`app_<lib>/`)

A wrapper module bridges an external library into the application architecture:

1. **Call the library API** — initialise the library, configure it, and invoke its functions.
   Match the call site to the thread context the library expects (check its documentation).
2. **Implement library callbacks** — provide the callback/hook functions the library requires
   (e.g. `memfault_metrics_heartbeat_collect_data()`). These run in the **library's thread context**,
   not the application's. Keep them short; signal the app via a semaphore or Zbus message if
   further work is needed.
3. **Zbus integration** — subscribe to Zbus channels for app lifecycle events the library needs
   (e.g. Wi-Fi connected → trigger upload). Publish Zbus messages when significant library events occur.
4. **Thread safety** — do not access app-layer globals from inside a library callback without
   proper synchronisation. Use `k_sem`, `k_mutex`, or atomic operations.
5. **No SMF required** — library wrapper modules typically do not use a state machine; their
   lifecycle is driven by library callbacks and Zbus events. Remove the State Machine section
   from the spec if it does not apply.

```
src/modules/app_<lib>/
├── app_<lib>.c         ← library init, API calls, callback implementations
├── app_<lib>.h         ← public API (if any), Kconfig guard
├── Kconfig.app_<lib>   ← CONFIG_APP_<LIB>_MODULE=y, selects CONFIG_<LIB>
└── CMakeLists.txt      ← guarded by CONFIG_APP_<LIB>_MODULE
```

### File structure (one module)
```
src/modules/<name>/
├── <name>.c           # state machine + Zbus subscriber/publisher
├── <name>.h           # public API, structs, channel declarations
├── Kconfig.<name>     # CONFIG_APP_<NAME>_MODULE + sub-options
└── CMakeLists.txt     # zephyr_library_sources if CONFIG_APP_<NAME>_MODULE
```

### Kconfig pattern
```kconfig
config APP_<NAME>_MODULE
    bool "Enable <Name> module"
    default y
    select <DEPENDENCY>

config APP_<NAME>_LOG_LEVEL
    int "<Name> module log level"
    default 3
    depends on APP_<NAME>_MODULE
```

### prj.conf strategy
- Base config (logging, shell, DK library) in `prj.conf`
- One section per module, derived from spec's Kconfig table
- Credentials in git-ignored `overlay-credentials.conf`
- Template tracked as `overlay-credentials.conf.template`

### Memory requirements (Wi-Fi projects)
- `CONFIG_HEAP_MEM_POOL_SIZE` ≥ 80000 (critical)
- `CONFIG_MAIN_STACK_SIZE` ≥ 4096
- Always verify with heap_monitor module

### Security
- Never hardcode credentials in source or `prj.conf`
- Credentials go in `overlay-credentials.conf` (git-ignored)
- Copyright year: use **current year** (2026) in all new files

---

## Reference: Reusable Modules

Copy these from the `ncs-project-logo` reference project:

| Module | Enable flag | Purpose |
|--------|-------------|---------|
| `src/modules/heap_monitor/` | `CONFIG_HEAPS_MONITOR=y` | System + mbedTLS heap tracking |
| `src/modules/app_memfault/` | `CONFIG_APP_MEMFAULT_MODULE=y` | Memfault OTA + metrics |
| `src/modules/network/` | `CONFIG_WIFI_MODULE=y` | Wi-Fi STA connection manager |
| `src/modules/wifi_prov_over_ble/` | `CONFIG_WIFI_STA_PROV_OVER_BLE_ENABLED=y` | BLE credential provisioning |
| `src/modules/app_https_client/` | `CONFIG_APP_HTTPS_CLIENT_MODULE=y` | Periodic HTTPS GET |
| `src/modules/app_mqtt_client/` | `CONFIG_APP_MQTT_CLIENT_MODULE=y` | Persistent MQTT over TLS |
| `src/modules/button/` | `CONFIG_BUTTON_MODULE=y` | SMF button with short/long press |

## Reference: Wi-Fi Config Files

```bash
~/.claude/skills/chsh-dev-project/overlays/
├── overlay-wifi-sta.conf       # Station mode
├── overlay-wifi-softap.conf    # SoftAP mode
├── overlay-wifi-p2p.conf       # P2P / Wi-Fi Direct
└── overlay-wifi-raw.conf       # Monitor / raw packets
```

## Reference: Architecture Templates

```bash
~/.claude/skills/chsh-dev-project/architecture/smf-zbus/
├── templates/          # module_template_smf.c/h, Kconfig.module_template, messages.h
└── modules/            # button_example/, sensor_example/ (ready to copy)

~/.claude/skills/chsh-dev-project/architecture/simple-multithreaded/
└── templates/          # module_template_simple.c/h, Kconfig.module_template_simple
```

## Reference: Guides

- [ARCHITECTURE_PATTERNS.md](architecture/guides/ARCHITECTURE_PATTERNS.md) — SMF+Zbus vs multi-threaded deep dive
- [WIFI_GUIDE.md](wifi/guides/WIFI_GUIDE.md) — Wi-Fi modes, reconnection, event handling
- [RECONNECTION_PATTERNS.md](wifi/guides/RECONNECTION_PATTERNS.md) — STA retry and back-off
- [CONFIG_GUIDE.md](guides/CONFIG_GUIDE.md) — prj.conf strategy
- [PROJECT_STRUCTURE.md](guides/PROJECT_STRUCTURE.md) — recommended file layout

## Reference: Sub-Skills

| Sub-skill | When to load |
|-----------|-------------|
| `chsh-dev-project/debug` | Debugging crashes, RTT logging, GDB |
| `chsh-dev-project/env-setup` | Toolchain setup, west init/update |
| `chsh-dev-project/architecture` | Architecture pattern selection and templates |
| `chsh-dev-project/protocols` | MQTT, CoAP, HTTP, TCP/UDP details |
| `chsh-dev-project/protocols/webserver` | Static HTTP server, REST API patterns |
| `chsh-dev-project/wifi` | Wi-Fi STA/SoftAP/P2P implementation details |

## Critical Requirements (NCS Wi-Fi projects)

- `CONFIG_WIFI=y` + `CONFIG_WIFI_NRF70=y`
- `CONFIG_WIFI_READY_LIB=y` (safe initialization — never skip)
- `CONFIG_HEAP_MEM_POOL_SIZE` ≥ 80000
- Network stack (IPv4, TCP, DHCP) configured
- All net events handled (CONNECTED, DISCONNECTED, IPV4_ADDR_ADD)
- NO hardcoded credentials anywhere
