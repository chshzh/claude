---
name: chsh-sk-ncs-memory
description: Optimize and debug memory usage in Nordic nRF Connect SDK (NCS) applications. Use when analyzing RAM/Flash usage, reducing memory footprint, debugging stack overflows, or investigating memory-related crashes in nRF projects.
---

# NCS Memory Optimization

## Critical Rules

1. **Set up NCS environment first** — use `chsh-sk-ncs-env`
2. **Build Release mode** for accurate Flash measurements (Debug builds are larger)
3. **Measure before optimizing** — Thread Analyzer and `heap_monitor` first

---

## Workflow

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

| Thread type | Margin |
|---|---|
| Simple worker | 1.2–1.3× |
| Protocol / parser | 1.5× |
| Heavy logging | 1.5–2.0× |
| Touched by ISRs | ≥1.5× |

Example (from the output above):
- `net_socket_service`: 2140 × 1.5 ≈ 3210 → round up to 3.5 KB
- `rx_q[0]`: 940 × 1.5 ≈ 1410 → 2 KB is sufficient
- `hostap_handler`: 5668 × 1.6 ≈ 9070 → keep 8 KB or bump to 9 KB

```
# prj.conf — stack sizing (keep rationale in comments)
CONFIG_MAIN_STACK_SIZE=2048
CONFIG_NET_RX_STACK_SIZE=2048
CONFIG_NET_TX_STACK_SIZE=2048
CONFIG_HTTP_SERVER_STACK_SIZE=3072
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
2. Flash and exercise all code paths
3. Record the highest peak value from UART logs
4. Production heap = peak × 1.5 (or at minimum 1.2×)

```
<inf> heap_monitor: System Heap: used=51712/98304 (52%) peak=64752/98304 (65%)
<wrn> heap_monitor: System Heap: used=86500/98304 (88%) peak=86500/98304 (88%)
```

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

---

## Self-Update Policy

At the **end of each conversation**, review what was discovered and check whether any facts in this skill are new, corrected, or outdated (e.g. new heap sizing data, new Kconfig options, updated stack margin guidelines).

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
