---
name: chsh-sk-ncs-3.3-memopt
description: Use when analyzing RAM/Flash usage, reducing memory footprint, debugging stack overflows, or investigating memory-related crashes in nRF projects. Optimizes and debugs memory usage in Nordic NCS applications.
---

# NCS Memory Optimization

## Critical Rules

1. **Set up NCS environment first** — use `chsh-sk-ncs-env`
2. **Build Release mode** for accurate Flash measurements (Debug builds are larger)
3. **Measure before optimizing** — Thread Analyzer and `heap_monitor` first

> **Knowledge sources**: Call `mcp_nordic-mcp_nordicsemi_workflow_ncs` at the start of each session — loads `nrfutil-manual` and `embedded-code-guidance-ncs-zephyr`. Use `mcp_nordic-mcp_nordicsemi_search_sources` for Kconfig symbols and memory-related config options.

---

## Workflow

### Step 0 — Prefer the validation report as the sizing input

Before measuring anything yourself, check for `docs/qa-test/VALIDATION_REPORT.md`. If it has a
populated **Memory Watermarks** section, that is the **primary** input — those peaks were
captured by ZView during the validation high-memory round under worst-case concurrent load
(`chsh-sk-ncs-4.2-validation`), which is exactly the scenario you want to size against.

- Use its per-board peak thread stacks and heap peaks directly; its columns map 1:1 onto the
  Thread Stack / Heap Analysis tables below.
- Apply the sizing formulas (Step 3 / Heap Sizing) to those peaks and update `prj.conf`.
- Honour any "thread not active during the round" note — keep the prior value for that thread.
- Record provenance in the report `Source` field: `VALIDATION_REPORT.md v<version>`.

Fall back to your own capture (Steps 1–4 below: Thread Analyzer + `heap_monitor`) only when no
validation report exists, or to re-measure a thread the validation round didn't exercise.

### Step 1 — Build and check summary

```sh
west build -b <board>
```

Build output shows the memory summary. Also generate detailed reports:

```sh
west build -t rom_report    # Flash usage by symbol
west build -t ram_report    # RAM usage by symbol
west build -t puncover      # Interactive HTML visualization
```

```sh
arm-none-eabi-nm --size-sort -S build/zephyr/zephyr.elf | tail -20
```

### Step 2 — Identify hotspots

**Flash hotspots:** large functions, debug strings, logging, unused subsystems, crypto/networking stacks

**RAM hotspots:** thread stacks (often oversized), heap, static buffers, global variables

### Step 3 — Apply optimizations

See the sections below. Apply one category at a time, rebuild, verify.

### Step 4 — Validate on hardware

Memory usage differs between Debug and Release builds and between QEMU and real hardware.
Always measure on the target board with the Release build and representative workload.

---

## Live ZView — interactive memory viewer

ZView reads thread stack watermarks and heap usage **over SWD (J-Link), no UART/RTT, no on-target
code** — ideal for watching memory live while you exercise the app during optimization.
(For unattended peak capture during validation, `chsh-sk-ncs-4.2-validation` uses `record`/`dump`.)

### 0. Check it's installed (remind to install if not)

```sh
nrfutil sdk-manager toolchain launch --ncs-version=${NCS_VERSION:-v3.3.0} -- west zview --help
```

If this errors (extension or deps missing), remind the user to install ZView, then stop:
- It ships as a west module at `modules/tools/zview`. If absent, add to the west manifest and
  `west update zview` (see its `README.md`); the J-Link runner needs `pylink-square`.

### 1. Resolve ELF + J-Link target per board

This project builds with **sysbuild**, so the application ELF is under the project-named
sub-image directory, not the top-level `zephyr/`:

| Board | J-Link target | ELF |
|-------|---------------|-----|
| nRF7002DK (nRF5340) | `nRF5340_xxAA` | `build_nrf7002dk/nordic-wifi-memfault/zephyr/zephyr.elf` |
| nRF54LM20DK | `nRF54LM20A_M33` | `build_nrf54lm20dk/nordic-wifi-memfault/zephyr/zephyr.elf` |

> Prerequisite Kconfig in the build: `CONFIG_INIT_STACKS=y`, `CONFIG_THREAD_MONITOR=y`,
> `CONFIG_THREAD_STACK_INFO=y` (+ `CONFIG_SYS_HEAP_RUNTIME_STATS=y` for heap, `CONFIG_THREAD_NAME=y`
> for names). If ZView shows empty threads/heaps, the build lacks these — rebuild with them.

### 2. Launch `live`

`live` is a full-screen TUI — it needs a real terminal, so the agent **cannot host it inline**.
Run it in your own terminal, or have the agent open it in a new macOS Terminal window:

```sh
# nRF7002DK  (get S/N with: nrfutil device list)
west zview live -e build_nrf7002dk/nordic-wifi-memfault/zephyr/zephyr.elf -r jlink -t nRF5340_xxAA -s <SN>

# nRF54LM20DK
west zview live -e build_nrf54lm20dk/nordic-wifi-memfault/zephyr/zephyr.elf -r jlink -t nRF54LM20A_M33 -s <SN>
```

Wrap in the toolchain launcher if `west` isn't on PATH:
`nrfutil sdk-manager toolchain launch --ncs-version=${NCS_VERSION:-v3.3.0} -- west zview live …`.
TUI keys: **ENTER** = drill into a thread/heap · **H** = heap fragmentation view · **Q** = quit.

> When the user asks to "open ZView", confirm which connected board (run `nrfutil device list`),
> then launch that board's command in a new Terminal window (e.g. macOS `osascript … Terminal …`).
> For a scripted/non-interactive readout instead, use `west zview dump … --json` and parse with `jq`.

---

## Flash Reduction

```
CONFIG_SIZE_OPTIMIZATIONS=y
CONFIG_LTO=y
CONFIG_ASSERT=n
CONFIG_LOG=n                    # Or CONFIG_LOG_DEFAULT_LEVEL=1 for errors only
CONFIG_SHELL=n
CONFIG_PRINTK=n
```

```sh
west build -b <board> -- -DCMAKE_BUILD_TYPE=MinSizeRel
```

Full config list: [reference/optimization-configs.md](reference/optimization-configs.md)

---

## Thread Stack Sizing

### 1. Enable Thread Analyzer

```
CONFIG_THREAD_ANALYZER=y
CONFIG_DEBUG_THREAD_INFO=y
CONFIG_THREAD_ANALYZER_USE_LOG=y
CONFIG_THREAD_ANALYZER_AUTO=y
CONFIG_THREAD_ANALYZER_AUTO_INTERVAL=5
CONFIG_THREAD_NAME=y
```

### 2. Flash, exercise all code paths, read UART output

```
[00:12:38] <inf> thread_analyzer:  hostap_handler      : STACK: unused 2524 usage 5668 / 8192 (69 %)
[00:12:38] <inf> thread_analyzer:  net_socket_service  : STACK: unused  260 usage 2140 / 2400 (89 %)
[00:12:38] <inf> thread_analyzer:  rx_q[0]             : STACK: unused 1108 usage  940 / 2048 (45 %)
```

### 3. Apply margin and update `prj.conf`

**Multi-board projects:** measure on **all** target boards and use the worst-case value across them. Stack usage can differ significantly between SoCs for the same thread (e.g., `mqtt_helper_thread` measured 2728 B on nRF5340 but 4040 B on nRF54LM20A — a 48 % difference that caused 95 % utilisation on the second board).

**Flat formula (preferred for consistency):**

```
new_size = floor(max_usage / 0.9)   # 10 % headroom
```

Embed the rationale directly in `prj.conf` so future readers can verify the math:

```
# Max usage 4040/0.9=4488
CONFIG_MQTT_HELPER_STACK_SIZE=4488
```

**Type-based guidelines (use when no measurement is available):**

| Thread type | Margin |
|---|---|
| Simple worker | 1.2–1.3× |
| Protocol / parser | 1.5× |
| Heavy logging | 1.5–2.0× |
| Touched by ISRs | ≥1.5× |

**Network I/O queues (`rx_q`, `tx_q`) — exception:** Do not apply the formula blindly. Measured usage reflects idle/steady-state only; a burst of fragmented packets can spike the stack temporarily. Keep `NET_RX_STACK_SIZE` and `NET_TX_STACK_SIZE` at ≥2048 B regardless of measured values and add a comment explaining why:

```
# Max usage 1168/0.9=1297; kept at 2048 for network burst headroom
CONFIG_NET_RX_STACK_SIZE=2048
```

Re-run the analyzer after every significant feature change.

---

## Heap Sizing

### Wi-Fi apps — critical minimum

```
CONFIG_HEAP_MEM_POOL_SIZE=80000   # 80 KB base; total ~125 KB with auto add-ons
```

Never go below 64 KB base for Wi-Fi apps — WPA supplicant malloc fails during TLS handshakes.

Add a build-time guard in your heap monitor or memory module:
```c
BUILD_ASSERT(CONFIG_HEAP_MEM_POOL_SIZE >= 65536,
             "Wi-Fi apps require >= 64 KB heap for WPA supplicant");
```

Check subsystem add-ons:
```sh
grep "HEAP_MEM_POOL_ADD_SIZE" build/zephyr/.config
```

### Size production heap from measured peak

1. Enable `heap_monitor` module (copy from `ncs-project-logo`, see [reference/heap-monitor.md](reference/heap-monitor.md))
2. Flash and exercise all code paths on **all target boards**
3. Read the `peak=` field from UART logs — **not** `used=` (live snapshot). Use the worst-case peak across all boards.
4. Production heap = `floor(peak / 0.8)` minimum (1.25× headroom); 1.5× if flash budget allows

```
<inf> heap_monitor: System Heap: used=51712/98304 (52%) peak=64752/98304 (65%)
<wrn> heap_monitor: System Heap: used=86500/98304 (88%) peak=86500/98304 (88%)
```

Formula with embedded rationale in `prj.conf`:

```
# Max usage 57928/0.8=72410
CONFIG_HEAP_MEM_POOL_SIZE=72410
```

> ⚠ If `peak` is above 85 % of the current ceiling you are one allocation spike away from exhaustion — raise the ceiling before the next measurement cycle.

---

## Common Issues

| Issue | Symptoms | Fix |
|---|---|---|
| Flash overflow | Build fails "region FLASH overflowed" | Enable `SIZE_OPTIMIZATIONS`, disable `LOG`/`ASSERT`/`SHELL`, build `MinSizeRel` |
| RAM overflow | Build fails "region RAM overflowed" | Reduce stacks (Thread Analyzer) + heap, reduce net buffers |
| Stack overflow | Hard fault, corruption, unexpected crash | Enable `STACK_SENTINEL`, use Thread Analyzer, increase offending thread's stack |
| Heap exhaustion | `k_malloc` / `malloc` returns NULL | Increase `HEAP_MEM_POOL_SIZE`, check for leaks, enable `SYS_HEAP_VALIDATE` |
| High log memory | Logging dominates rom/ram report | `CONFIG_LOG_DEFAULT_LEVEL=1`, `CONFIG_LOG_MODE_MINIMAL=y`, or `CONFIG_LOG=n` |

---

## Partition Manager (Multi-Image Builds)

```sh
cat build/partitions.yml                    # View partition layout
west build -t partition_manager_report      # Verify fit after changes
```

Create `pm_static.yml` (or `pm_static_<board>.yml`) for custom partition sizes.

---

## Reference Files

- [reference/heap-monitor.md](reference/heap-monitor.md) — heap monitor module, tuning knobs, log format, Memfault integration, heap architecture (system / net buffers / mbedTLS)
- [reference/optimization-configs.md](reference/optimization-configs.md) — full Flash/RAM config options, development vs production templates, profiling tools
- [MEMOPT_REPORT_TEMPLATE.md](MEMOPT_REPORT_TEMPLATE.md) — base for `MEMOPT_REPORT.md`; documents measured watermarks, sizing math, and the `prj.conf` changes applied (consumes `VALIDATION_REPORT.md` when present)

---

## Gotchas
- TODO: add one entry per real observed failure or routing false-positive

## Self-Update Policy

At the **end of each conversation**, review what was discovered and check whether any facts in this skill are new, corrected, or outdated (e.g. new heap sizing data, new Kconfig options, updated stack margin guidelines).

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
