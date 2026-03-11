````skill
---
name: ncs-mem
description: Optimize and debug memory usage in Nordic nRF Connect SDK (NCS) applications. Use when analyzing RAM/Flash usage, reducing memory footprint, debugging stack overflows, or investigating memory-related crashes in nRF projects.
---

# Nordic nRF Connect SDK (NCS) Memory Optimization and Debugging

## Key Concepts

**Memory Types in Embedded Systems:**
- **Flash (ROM)**: Stores code, constants, and read-only data
- **RAM**: Runtime memory for stack, heap, global/static variables
- **Stack**: Per-thread memory for function calls and local variables
- **Heap**: Dynamic memory allocation pool

**Memory Analysis Tools:**
- **west build size reports**: Detailed memory usage breakdown
- **Puncover**: Interactive memory visualization tool
- **GDB memory inspection**: Runtime memory analysis
- **Thread Analyzer**: Stack usage monitoring
- **Memory mapping**: Linker scripts and partition manager

## Critical Rules

1. **Always ensure NCS environment is set up** before memory analysis (use ncs-env-setup skill)
2. **Build in Release mode** for accurate Flash size measurements (Debug builds are larger)
3. **Enable memory debugging features only during development** (they consume extra memory)
4. **Check partition manager configuration** for multi-image builds

## Workflow for Memory Analysis

### Step 1: Build and Generate Memory Reports

Build the project and examine memory usage:
```sh
west build -b <board>
```

The build output shows memory summary:
```
Memory region         Used Size  Region Size  %age Used
           FLASH:      123456 B       1 MB     11.77%
             RAM:       45678 B     256 KB     17.41%
```

### Step 2: Detailed Memory Analysis

**Generate detailed size report:**
```sh
west build -t rom_report      # Flash usage by symbol
west build -t ram_report      # RAM usage by symbol
west build -t puncover         # Interactive HTML visualization
```

**Examine symbol sizes:**
```sh
arm-none-eabi-nm --size-sort -S build/zephyr/zephyr.elf | tail -20
```

**View memory map:**
```sh
cat build/zephyr/zephyr.map
```

### Step 3: Identify Memory Hotspots

**Flash optimization targets:**
- Large functions or libraries
- Debug strings and logging
- Unused features/drivers
- Crypto/networking stacks

**RAM optimization targets:**
- Thread stacks (often oversized)
- Heap size
- Static buffers
- Global variables

### Step 4: Apply Optimizations

See "Common Memory Optimizations" section below.

## Common Memory Optimizations

### Flash Reduction Strategies

**1. Disable Unused Features**

Review and disable in `prj.conf`:
```
# Disable assertions in production
CONFIG_ASSERT=n

# Minimal logging (or disable completely)
CONFIG_LOG=n
# Or use minimal level
CONFIG_LOG_DEFAULT_LEVEL=1
CONFIG_LOG_MODE_MINIMAL=y

# Disable shell if not needed
CONFIG_SHELL=n

# Disable printk for production
CONFIG_PRINTK=n
CONFIG_EARLY_CONSOLE=n
```

**2. Optimize Compilation**

```
# Size optimization (default is usually speed)
CONFIG_SIZE_OPTIMIZATIONS=y
CONFIG_COMPILER_OPT="-Os"

# Link-time optimization
CONFIG_LTO=y
```

**3. Remove Debug Information**

Build in Release mode:
```sh
west build -b <board> -- -DCMAKE_BUILD_TYPE=Release
```

**4. Reduce Networking Stack Size**

```
# For minimal networking
CONFIG_NET_BUF_DATA_SIZE=128
CONFIG_NET_PKT_RX_COUNT=4
CONFIG_NET_PKT_TX_COUNT=4

# Disable IPv6 if only using IPv4
CONFIG_NET_IPV6=n
```

**5. Minimize Bluetooth Stack**

```
# Reduce Bluetooth buffers
CONFIG_BT_BUF_ACL_TX_COUNT=3
CONFIG_BT_BUF_ACL_TX_SIZE=27
CONFIG_BT_CTLR_DATA_LENGTH_MAX=27

# Disable unused Bluetooth features
CONFIG_BT_OBSERVER=n
CONFIG_BT_BROADCASTER=n
```

### RAM Reduction Strategies

**1. Optimize Thread Stack Sizes**

Analyze actual stack usage:
```
# Enable thread analyzer
CONFIG_THREAD_ANALYZER=y
CONFIG_THREAD_NAME=y
```

Then at runtime, check stack usage and adjust:
```c
// In code, enable periodic analysis
CONFIG_THREAD_ANALYZER_AUTO=y
CONFIG_THREAD_ANALYZER_AUTO_INTERVAL=60
```

**Thread Analyzer workflow**

1. **Enable the analyzer in `prj.conf`:**
  ```
  # Thread Analyzer
  CONFIG_THREAD_ANALYZER=y
  CONFIG_DEBUG_THREAD_INFO=y
  CONFIG_THREAD_ANALYZER_USE_LOG=y
  CONFIG_THREAD_ANALYZER_AUTO=y
  CONFIG_THREAD_ANALYZER_AUTO_INTERVAL=5
  ```
  `CONFIG_DEBUG_THREAD_INFO` keeps per-thread metadata so the analyzer can print names and stack bounds; `CONFIG_THREAD_ANALYZER_USE_LOG` routes the report through the log subsystem, which is easier to capture than printk on large projects.

2. **Exercise the firmware:** flash the build, let it run through as many user flows as possible (Wi-Fi connect/disconnect, HTTP traffic, button presses, etc.), then collect the UART/RTT log. Example output fragment after a long soak test:
  ```
  [00:12:38.249] <inf> thread_analyzer:  hostap_handler      : STACK: unused 2524 usage 5668 / 8192 (69 %)
  [00:12:38.250] <inf> thread_analyzer:  net_socket_service  : STACK: unused  260 usage 2140 / 2400 (89 %)
  [00:12:38.251] <inf> thread_analyzer:  rx_q[0]             : STACK: unused 1108 usage  940 / 2048 (45 %)
  ```

3. **Convert usage to new targets:** apply a high‑water margin based on thread role to decide the configured stack size. Practical guideline:

  | Thread type             | Margin |
  | ----------------------- | ------ |
  | Simple worker           | 1.2–1.3× |
  | Protocol / parser       | 1.5× |
  | Threads with heavy logging | 1.5–2.0× |
  | Threads touched by ISRs | ≥1.5× |

  Example calculations using the log above:

  - `net_socket_service`: $2140 \times 1.5 \approx 3210$ → round up to 3 KB or 3.5 KB.
  - `rx_q[0]`: $940 \times 1.5 \approx 1410$ → 2 KB stack is sufficient.
  - `hostap_handler`: $5668 \times 1.6 \approx 9070$ → keep 8 KB or bump to 9 KB if headroom is needed.

4. **Update `prj.conf`:** keep the sizing rationale near the config block so future changes stay aligned. For the SoftAP sample project, the analyzer-driven block looks like:
  ```
  # Memory configuration (sized from thread analyzer high-water marks)
  CONFIG_MAIN_STACK_SIZE=2048
  CONFIG_SHELL_STACK_SIZE=3072
  CONFIG_NET_RX_STACK_SIZE=2048
  CONFIG_NET_TX_STACK_SIZE=2048
  CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=2048
  CONFIG_NET_MGMT_EVENT_STACK_SIZE=2048
  CONFIG_HTTP_SERVER_STACK_SIZE=3072
  ```

  Re-run the analyzer after every significant feature change; if usage ever exceeds ~80% of the configured size, recalculate with the table above.

Reduce oversized stacks in `prj.conf`:
```
CONFIG_MAIN_STACK_SIZE=2048      # Default often 4096+
CONFIG_IDLE_STACK_SIZE=512       # Default often 1024+
CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=1024
CONFIG_BT_RX_STACK_SIZE=1024
```

**2. Optimize Heap Size**

**Understanding Heap Composition:**

In NCS/Zephyr, the total heap size comes from:
- Base size: `CONFIG_HEAP_MEM_POOL_SIZE`
- Subsystem add-ons: `CONFIG_HEAP_MEM_POOL_ADD_SIZE_*` (auto-calculated)

```sh
# Check what contributes to heap requirements
grep "HEAP_MEM_POOL_ADD_SIZE" build/zephyr/.config

# Common contributors:
# CONFIG_HEAP_MEM_POOL_ADD_SIZE_HOSTAP=41808       # WPA supplicant
# CONFIG_HEAP_MEM_POOL_ADD_SIZE_POSIX_THREADS=256
# CONFIG_HEAP_MEM_POOL_ADD_SIZE_SOCKETPAIR=296
# CONFIG_HEAP_MEM_POOL_ADD_SIZE_ZBUS=3072
```

**Sizing Guidelines:**

```
# Wi-Fi applications (WPA supplicant) - CRITICAL MINIMUM
# WPA supplicant requires stable base ≥64 KB for TLS contexts,
# PMK cache, and EAP state machines during handshakes.
# Symptoms below 64 KB: malloc failures, AP bring-up failures.
CONFIG_HEAP_MEM_POOL_SIZE=80000   # 80 KB base recommended
                                   # Total with add-ons ≈125 KB

# Non-Wi-Fi applications
CONFIG_HEAP_MEM_POOL_SIZE=2048    # Minimal if using malloc/new

# No dynamic allocation needed
CONFIG_HEAP_MEM_POOL_SIZE=0       # Disable completely
```

**Heap Minimum Enforcement:**

By default, Zephyr auto-bumps heap to sum of all `HEAP_MEM_POOL_ADD_SIZE_*`:
```
# To bypass auto-minimum (not recommended for Wi-Fi)
CONFIG_HEAP_MEM_POOL_IGNORE_MIN=y

# Standard behavior (recommended)
CONFIG_HEAP_MEM_POOL_IGNORE_MIN=n
```

**3. Optimize Buffer Sizes**

```
# Network buffers
CONFIG_NET_BUF_DATA_SIZE=128

# UART buffers
CONFIG_UART_ASYNC_TX_BUF_SIZE=256
CONFIG_UART_ASYNC_RX_BUF_SIZE=256
```

**4. Reduce Shell/Log Memory**

```
# Reduce logging buffer
CONFIG_LOG_BUFFER_SIZE=512

# Reduce shell backend buffer
CONFIG_SHELL_BACKEND_SERIAL_TX_RING_BUFFER_SIZE=64
CONFIG_SHELL_BACKEND_SERIAL_RX_RING_BUFFER_SIZE=64
```

### Heap Monitoring Module

> **Reference implementation**: `memfault-nrf7002dk` / `ncs-project-logo`  
> `src/modules/heap_monitor/` — production-ready, copy into any NCS project.

**Purpose:** Real-time heap usage tracking with standardised log output, peak
detection, threshold alerts, and optional Memfault heartbeat metrics.

**When to Use:**
- Sizing heap for production (measure peak under full workload, apply 1.5× margin)
- Debugging heap exhaustion
- Monitoring heap growth during long-running tests
- Detecting memory leaks

---

#### Module structure

```
src/modules/heap_monitor/
├── heap_monitor.c        # Implementation
├── CMakeLists.txt        # Conditionally added when CONFIG_HEAPS_MONITOR=y
├── Kconfig.heap_monitor  # Symbol definitions + log-level template
└── Kconfig.defaults      # Sensible defaults (off by default)
```

Wire into the project-level `Kconfig` and `CMakeLists.txt`:

```kconfig
# Kconfig — inside menu "Application configuration"
rsource "src/modules/heap_monitor/Kconfig.heap_monitor"
# ...
rsource "src/modules/heap_monitor/Kconfig.defaults"
```

```cmake
# CMakeLists.txt
add_subdirectory(src/modules/heap_monitor)
```

---

#### Enable with one switch

```kconfig
# prj.conf
CONFIG_HEAPS_MONITOR=y
CONFIG_HEAPS_MONITOR_LOG_LEVEL_INF=y
```

The module automatically selects `SYS_HEAP_LISTENER` + `SYS_HEAP_RUNTIME_STATS`
when `CONFIG_HEAP_MEM_POOL_SIZE > 0`, and selects `MBEDTLS_MEMORY_DEBUG` when
`CONFIG_MBEDTLS_ENABLE_HEAP=y`. No other `prj.conf` entries are needed.

The module depends on `(HEAP_MEM_POOL_SIZE > 0) || MBEDTLS_ENABLE_HEAP`, so it
can only be enabled when at least one heap is present.

---

#### Tuning knobs (all have defaults in `Kconfig.defaults`)

| Symbol | Default | Meaning |
|--------|---------|---------|
| `CONFIG_HEAPS_MONITOR_WARN_PCT` | 88 | Emit `LOG_WRN` when used% reaches this |
| `CONFIG_HEAPS_MONITOR_STEP_BYTES` | 512 | Fire alloc-path report only when high-water mark advances by this many bytes |
| `CONFIG_HEAPS_MONITOR_PERIODIC_INTERVAL_SEC` | 30 | Seconds between timed snapshots |

```kconfig
# Development: verbose
CONFIG_HEAPS_MONITOR_STEP_BYTES=512
CONFIG_HEAPS_MONITOR_PERIODIC_INTERVAL_SEC=10

# Production: reduce log noise
CONFIG_HEAPS_MONITOR_STEP_BYTES=4096
CONFIG_HEAPS_MONITOR_PERIODIC_INTERVAL_SEC=60
CONFIG_HEAPS_MONITOR_WARN_PCT=90
```

---

#### Log output (standardised format for both heaps)

```
<inf> heap_monitor: System Heap: used=51712/98304 (52%) blocks=n/a, peak=64752/98304 (65%), peak_blocks=n/a
<inf> heap_monitor: mbedTLS Heap: used=19136/110592 (17%) blocks=49, peak=72684/110592 (65%), peak_blocks=197
<wrn> heap_monitor: System Heap: used=86500/98304 (88%) blocks=n/a, peak=86500/98304 (88%), peak_blocks=n/a
```

- **System heap**: Zephyr runtime stats expose bytes only, so `blocks=n/a`.
- **mbedTLS heap**: reports real `blocks` and `peak_blocks` from
  `mbedtls_memory_buffer_alloc_{cur,max}_get()`. The module accumulates a
  rolling all-time peak externally because `max_get()` resets each call.

---

#### Memfault integration (automatic)

When `CONFIG_APP_MEMFAULT_MODULE=y` the module calls
`MEMFAULT_METRIC_SET_UNSIGNED()` on every periodic snapshot — no extra
wiring needed. When Memfault is absent a stub no-op macro is used so the
module compiles cleanly in both configurations.

Register the metric keys in
`src/modules/app_memfault/config/memfault_metrics_heartbeat_config.def`:

```c
#if CONFIG_HEAPS_MONITOR
#if CONFIG_HEAP_MEM_POOL_SIZE > 0
MEMFAULT_METRICS_KEY_DEFINE(ncs_system_heap_total, kMemfaultMetricType_Unsigned)
MEMFAULT_METRICS_KEY_DEFINE(ncs_system_heap_used,  kMemfaultMetricType_Unsigned)
MEMFAULT_METRICS_KEY_DEFINE(ncs_system_heap_peak,  kMemfaultMetricType_Unsigned)
#endif
#if CONFIG_MBEDTLS_ENABLE_HEAP
MEMFAULT_METRICS_KEY_DEFINE(ncs_mbedtls_heap_total, kMemfaultMetricType_Unsigned)
MEMFAULT_METRICS_KEY_DEFINE(ncs_mbedtls_heap_used,  kMemfaultMetricType_Unsigned)
MEMFAULT_METRICS_KEY_DEFINE(ncs_mbedtls_heap_peak,  kMemfaultMetricType_Unsigned)
#endif
#endif
```

---

#### Old single-file approach (deprecated)

The previous pattern (`src/modules/memory/heap_monitor.c`, `CONFIG_APP_HEAP_MONITOR`,
`CONFIG_SYS_HEAP_LISTENER=y` manually in `prj.conf`) is superseded by the
module above. Do not use it in new projects.

---

**Heap Architecture Notes:**

**System Heap vs Dedicated Heaps:**

NCS/Zephyr uses multiple heaps:

1. **`_system_heap`** (monitored by heap_monitor):
   - General malloc/calloc/free
   - POSIX thread allocations
   - WPA supplicant heap (when CONFIG_NRF_WIFI_GLOBAL_HEAP=y)
   - Sized by `CONFIG_HEAP_MEM_POOL_SIZE + <subsystem add-ons>`

2. **Net Buffer Heaps** (NOT monitored):
   - Dedicated slab pools for network packets
   - Configured via `CONFIG_NET_BUF_RX_COUNT`, `CONFIG_NET_BUF_TX_COUNT`
   - Separate to avoid heap fragmentation and guarantee ISR-safe allocation
   - Cannot easily be moved to system heap without rewriting net_buf internals

3. **Wi-Fi Driver Heap** (when `CONFIG_NRF_WIFI_GLOBAL_HEAP=n`):
   - Dedicated region for firmware, DMA descriptors
   - Provides isolation and predictable performance
   - Use global heap only if severely memory-constrained

4. **mbedTLS Heap** (`CONFIG_MBEDTLS_ENABLE_HEAP=y`):
   - A fixed-size buffer carved out of RAM at boot, separate from `_system_heap`
   - Sized by `CONFIG_MBEDTLS_HEAP_SIZE` — this is the **total** capacity (no add-ons)
   - Used exclusively by mbedTLS for TLS session state, certificate chains, key material
   - Peaks during TLS handshakes; can spike to ~70 KB on Wi-Fi/HTTPS workloads
   - `CONFIG_MBEDTLS_MEMORY_DEBUG=y` is auto-selected by `CONFIG_HEAPS_MONITOR`
   - Sizing: `CONFIG_MBEDTLS_HEAP_SIZE` is the only knob; there are no auto add-ons
   - Recommended margin: 1.5× of measured peak (same as system heap for TLS workloads)
   - Example configuration:
     ```
     CONFIG_MBEDTLS_ENABLE_HEAP=y
     CONFIG_MBEDTLS_HEAP_SIZE=110592    # 150% of ~72 KB measured peak
     ```

**Why Net Buffers Don't Use System Heap:**
- **Latency**: ISR-level networking needs lock-free allocation
- **Fragmentation**: ~1 KB packets would fragment general heap
- **Determinism**: Fixed-size slabs guarantee allocation success
- **Alignment**: DMA requires strict alignment guarantees

**Recommendation**: Keep net buffers dedicated; monitor `_system_heap` and mbedTLS heap separately.

### Memfault Heartbeat Metrics Integration

> **Already handled by the heap_monitor module** when
> `CONFIG_APP_MEMFAULT_MODULE=y`. The section below is kept for reference
> if you need to wire heap metrics manually in a project that does not use
> the module.

Wire heap stats into Memfault so peak usage is observable in the cloud dashboard without needing a serial terminal.

**1. Define metric keys in `memfault_metrics_heartbeat_config.def`**

```c
/* Heap metrics — report total capacity, current usage, and high-water mark */
MEMFAULT_METRICS_KEY_DEFINE(ncs_system_heap_total, kMemfaultMetricType_Unsigned)
MEMFAULT_METRICS_KEY_DEFINE(ncs_system_heap_used,  kMemfaultMetricType_Unsigned)
MEMFAULT_METRICS_KEY_DEFINE(ncs_system_heap_peak,  kMemfaultMetricType_Unsigned)
#if CONFIG_MBEDTLS_ENABLE_HEAP
MEMFAULT_METRICS_KEY_DEFINE(ncs_mbedtls_heap_total, kMemfaultMetricType_Unsigned)
MEMFAULT_METRICS_KEY_DEFINE(ncs_mbedtls_heap_used,  kMemfaultMetricType_Unsigned)
MEMFAULT_METRICS_KEY_DEFINE(ncs_mbedtls_heap_peak,  kMemfaultMetricType_Unsigned)
#endif
```

**2. Populate metrics inside `heap_monitor.c` (periodic work + boot)**

```c
#ifdef CONFIG_MEMFAULT
#include <memfault/metrics/metrics.h>
#else
/* Stub so the same code compiles without Memfault */
#define MEMFAULT_METRIC_SET_UNSIGNED(...) ((void)0)
#endif
```c
/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/heap_listener.h>
#include <zephyr/sys/mem_stats.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(app_heap_monitor, CONFIG_LOG_DEFAULT_LEVEL);

extern struct k_heap _system_heap;

BUILD_ASSERT(CONFIG_HEAP_MEM_POOL_SIZE >= 65536,
	     "CONFIG_HEAP_MEM_POOL_SIZE must stay >= 64 KB for WPA supplicant "
	     "stability");

static uint32_t last_reported_high;
static uint32_t last_warn_pct;
static bool boot_report_pending = true;

static void heap_report(const char *trigger)
{
	struct sys_memory_stats stats;
	if (sys_heap_runtime_stats_get((struct sys_heap *)&_system_heap.heap,
				       &stats) != 0) {
		return;
	}

	const uint32_t total =
		(uint32_t)(stats.allocated_bytes + stats.free_bytes);
	if (total == 0U) {
		return;
	}

	const uint32_t current_high = (uint32_t)stats.max_allocated_bytes;
	const uint32_t used = (uint32_t)stats.allocated_bytes;
	const uint32_t free_bytes = (uint32_t)stats.free_bytes;
	const uint32_t pct = (current_high * 100U) / total;
	const bool warn = pct >= CONFIG_APP_HEAP_MONITOR_WARN_PCT;
	bool progressed = false;

	if (current_high > last_reported_high) {
		progressed = (current_high - last_reported_high) >=
			     CONFIG_APP_HEAP_MONITOR_STEP_BYTES;
	}

	const bool new_warn = warn && (pct > last_warn_pct);
	const bool should_report = progressed || new_warn || boot_report_pending;
	if (!should_report) {
		return;
	}

	last_reported_high = MAX(last_reported_high, current_high);
	boot_report_pending = false;

	if (warn) {
		last_warn_pct = pct;
		LOG_WRN("Heap %s: peak=%u bytes (%u%% of %u), used=%u, free=%u",
			trigger, current_high, pct, total, used, free_bytes);
	} else {
		LOG_INF("Heap %s: peak=%u bytes (%u%% of %u), used=%u, free=%u",
			trigger, current_high, pct, total, used, free_bytes);
	}
}

static void heap_listener_alloc(uintptr_t heap_id, void *mem, size_t bytes)
{
	ARG_UNUSED(mem);
	ARG_UNUSED(bytes);

	if (heap_id != HEAP_ID_FROM_POINTER(&_system_heap)) {
		return;
	}

	heap_report("alloc");
}

HEAP_LISTENER_ALLOC_DEFINE(app_heap_listener_alloc,
			   HEAP_ID_FROM_POINTER(&_system_heap),
			   heap_listener_alloc);

static int app_heap_monitor_init(void)
{
	heap_report("boot");
	return 0;
}

SYS_INIT(app_heap_monitor_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
```

**2. Add Kconfig Options**

Create `src/modules/memory/Kconfig` (or add to existing `Kconfig`):
```kconfig
config APP_HEAP_MONITOR
	bool "Enable application heap monitoring"
	depends on HEAP_MEM_POOL_SIZE > 0
	select SYS_HEAP_LISTENER
	select SYS_HEAP_RUNTIME_STATS
	help
	  Monitor system heap usage and log when peak allocation
	  crosses configurable thresholds.

if APP_HEAP_MONITOR

config APP_HEAP_MONITOR_WARN_PCT
	int "Warn threshold percentage"
	range 50 100
	default 88
	help
	  Log a warning when heap usage exceeds this percentage.

config APP_HEAP_MONITOR_STEP_BYTES
	int "Reporting step size in bytes"
	range 128 8192
	default 512
	help
	  Log when peak heap advances by at least this many bytes.
	  Smaller values = more frequent logs (for debugging).
	  Larger values = less noise (for production).

endif # APP_HEAP_MONITOR
```

**3. Update CMakeLists.txt**

Add to `src/CMakeLists.txt`:
```cmake
# Memory monitoring module
target_sources_ifdef(CONFIG_APP_HEAP_MONITOR app PRIVATE
    modules/memory/heap_monitor.c
)
```

**4. Enable in prj.conf**

```
# Heap monitoring configuration
CONFIG_SYS_HEAP_LISTENER=y
CONFIG_SYS_HEAP_RUNTIME_STATS=y
CONFIG_APP_HEAP_MONITOR=y
CONFIG_APP_HEAP_MONITOR_WARN_PCT=88
CONFIG_APP_HEAP_MONITOR_STEP_BYTES=512
```

**Configuration Trade-offs:**

```
# Development: Fine-grained monitoring
CONFIG_APP_HEAP_MONITOR_STEP_BYTES=512   # Log every 512 B increase

# Production: Reduce log noise
CONFIG_APP_HEAP_MONITOR_STEP_BYTES=4096  # Log every 4 KB increase

# Aggressive warning
CONFIG_APP_HEAP_MONITOR_WARN_PCT=75

# Conservative warning
CONFIG_APP_HEAP_MONITOR_WARN_PCT=90
```

**Log Output Examples:**

```
[00:00:01.234] <inf> app_heap_monitor: Heap boot: peak=12288 bytes (12% of 100000), used=12000, free=88000
[00:00:05.678] <inf> app_heap_monitor: Heap alloc: peak=24576 bytes (24% of 100000), used=23000, free=77000
[00:01:30.456] <wrn> app_heap_monitor: Heap alloc: peak=90112 bytes (90% of 100000), used=89000, free=11000
```

**Interpreting Results:**
- **peak**: Highest heap usage ever reached (high-water mark)
- **used**: Current allocation (can fluctuate)
- **free**: Available heap space
- Monitor peak during typical workload to size production heap

**Heap Architecture Notes:**

**System Heap vs Dedicated Heaps:**

NCS/Zephyr uses multiple heaps:

1. **`_system_heap`** (monitored by heap_monitor):
   - General malloc/calloc/free
   - POSIX thread allocations
   - WPA supplicant heap (when CONFIG_NRF_WIFI_GLOBAL_HEAP=y)
   - Sized by `CONFIG_HEAP_MEM_POOL_SIZE + <subsystem add-ons>`

2. **Net Buffer Heaps** (NOT monitored):
   - Dedicated slab pools for network packets
   - Configured via `CONFIG_NET_BUF_RX_COUNT`, `CONFIG_NET_BUF_TX_COUNT`
   - Separate to avoid heap fragmentation and guarantee ISR-safe allocation
   - Cannot easily be moved to system heap without rewriting net_buf internals

3. **Wi-Fi Driver Heap** (when `CONFIG_NRF_WIFI_GLOBAL_HEAP=n`):
   - Dedicated region for firmware, DMA descriptors
   - Provides isolation and predictable performance
   - Use global heap only if severely memory-constrained

4. **mbedTLS Heap** (`CONFIG_MBEDTLS_ENABLE_HEAP=y`):
   - A fixed-size buffer carved out of RAM at boot, separate from `_system_heap`
   - Sized by `CONFIG_MBEDTLS_HEAP_SIZE` — this is the **total** capacity (no add-ons)
   - Used exclusively by mbedTLS for TLS session state, certificate chains, key material
   - Peaks during TLS handshakes; can spike to ~60 KB on Wi-Fi/HTTPS workloads
   - Enable `CONFIG_MBEDTLS_MEMORY_DEBUG=y` to activate peak/current tracking APIs
   - Monitoring APIs (requires `CONFIG_MBEDTLS_MEMORY_DEBUG=y`):
     ```c
     #include <mbedtls/memory_buffer_alloc.h>

     size_t cur_used, cur_blocks;
     size_t max_used, max_blocks;
     mbedtls_memory_buffer_alloc_cur_get(&cur_used, &cur_blocks);
     mbedtls_memory_buffer_alloc_max_get(&max_used, &max_blocks);
     // Note: max_used resets between calls — call periodically and track externally
     ```
   - Sizing: `CONFIG_MBEDTLS_HEAP_SIZE` is the only knob; there are no auto add-ons
   - Recommended margin: 1.5× of measured peak (same as system heap for TLS workloads)
   - Example configuration:
     ```
     CONFIG_MBEDTLS_ENABLE_HEAP=y
     CONFIG_MBEDTLS_HEAP_SIZE=90112    # 150% of ~60 KB measured peak
     CONFIG_MBEDTLS_MEMORY_DEBUG=y     # Enable in dev; costs ~10 KB Flash
     ```

**Why Net Buffers Don't Use System Heap:**
- **Latency**: ISR-level networking needs lock-free allocation
- **Fragmentation**: ~1 KB packets would fragment general heap
- **Determinism**: Fixed-size slabs guarantee allocation success
- **Alignment**: DMA requires strict alignment guarantees

**Recommendation**: Keep net buffers dedicated; monitor `_system_heap` and mbedTLS heap separately.

### Memfault Heartbeat Metrics Integration

Wire heap stats into Memfault so peak usage is observable in the cloud dashboard without needing a serial terminal.

**1. Define metric keys in `memfault_metrics_heartbeat_config.def`**

```c
/* Heap metrics — report total capacity, current usage, and high-water mark */
MEMFAULT_METRICS_KEY_DEFINE(ncs_system_heap_total, kMemfaultMetricType_Unsigned)
MEMFAULT_METRICS_KEY_DEFINE(ncs_system_heap_used,  kMemfaultMetricType_Unsigned)
MEMFAULT_METRICS_KEY_DEFINE(ncs_system_heap_peak,  kMemfaultMetricType_Unsigned)
#if CONFIG_MBEDTLS_ENABLE_HEAP
MEMFAULT_METRICS_KEY_DEFINE(ncs_mbedtls_heap_total, kMemfaultMetricType_Unsigned)
MEMFAULT_METRICS_KEY_DEFINE(ncs_mbedtls_heap_used,  kMemfaultMetricType_Unsigned)
MEMFAULT_METRICS_KEY_DEFINE(ncs_mbedtls_heap_peak,  kMemfaultMetricType_Unsigned)
#endif
```

**2. Populate metrics inside `heap_monitor.c` (periodic work + boot)**

```c
#ifdef CONFIG_MEMFAULT
#include <memfault/metrics/metrics.h>
#else
/* Stub so the same code compiles without Memfault */
#define MEMFAULT_METRIC_SET_UNSIGNED(...) ((void)0)
#endif

static void update_system_heap_metrics(uint32_t total, uint32_t used, uint32_t peak)
{
    MEMFAULT_METRIC_SET_UNSIGNED(ncs_system_heap_total, total);
    MEMFAULT_METRIC_SET_UNSIGNED(ncs_system_heap_used,  used);
    MEMFAULT_METRIC_SET_UNSIGNED(ncs_system_heap_peak,  peak);
}

#if defined(CONFIG_MBEDTLS_ENABLE_HEAP) && defined(CONFIG_MBEDTLS_MEMORY_DEBUG)
static void update_mbedtls_heap_metrics(size_t total, size_t used, size_t peak)
{
    MEMFAULT_METRIC_SET_UNSIGNED(ncs_mbedtls_heap_total, (uint32_t)total);
    MEMFAULT_METRIC_SET_UNSIGNED(ncs_mbedtls_heap_used,  (uint32_t)used);
    MEMFAULT_METRIC_SET_UNSIGNED(ncs_mbedtls_heap_peak,  (uint32_t)peak);
}
#endif

/* Call both from periodic_heap_work_fn() and app_heap_monitor_init() */
static void report_all_heaps(void)
{
    struct sys_memory_stats stats;
    if (sys_heap_runtime_stats_get((struct sys_heap *)&_system_heap.heap, &stats) == 0) {
        uint32_t total = (uint32_t)(stats.allocated_bytes + stats.free_bytes);
        update_system_heap_metrics(total,
                                   (uint32_t)stats.allocated_bytes,
                                   (uint32_t)stats.max_allocated_bytes);
    }

#if defined(CONFIG_MBEDTLS_ENABLE_HEAP) && defined(CONFIG_MBEDTLS_MEMORY_DEBUG)
    size_t cur_used, cur_blocks, max_used, max_blocks;
    mbedtls_memory_buffer_alloc_cur_get(&cur_used, &cur_blocks);
    mbedtls_memory_buffer_alloc_max_get(&max_used, &max_blocks);
    update_mbedtls_heap_metrics(CONFIG_MBEDTLS_HEAP_SIZE, cur_used, max_used);
#endif
}
```

**Key architectural decisions:**
- Metrics are set from `heap_monitor.c` (which already owns the stats calls) — no new collection path needed
- `ncs_*_heap_total` for system heap is derived at runtime (`allocated + free`); for mbedTLS it is the compile-time constant `CONFIG_MBEDTLS_HEAP_SIZE`
- `mbedtls_memory_buffer_alloc_max_get()` returns peak since last reset, not rolling max — call it from the periodic task to capture each window's peak
- Guard with `#ifdef CONFIG_MEMFAULT` so the heap monitor compiles cleanly in builds without Memfault

**Enable in `prj.conf`:**
```
# Memfault metrics (enable alongside heap monitor)
CONFIG_MEMFAULT=y
CONFIG_MEMFAULT_NCS_IMPLEMENTATION=y
```

## Memory Debugging

### Debug Stack Overflows

**Enable stack protection:**
```
CONFIG_STACK_SENTINEL=y        # Detects overflow
CONFIG_USERSPACE=y             # Memory protection (if HW supports)
CONFIG_THREAD_STACK_INFO=y     # Stack usage info
CONFIG_MPU_STACK_GUARD=y       # MPU protection (ARM Cortex-M)
```

**Monitor stack usage:**
```c
#include <zephyr/kernel.h>

void check_stack_usage(void) {
    size_t unused;
    k_thread_stack_space_get(k_current_get(), &unused);
    printk("Thread %s: %zu bytes unused\n", 
           k_thread_name_get(k_current_get()), unused);
}
```

**Use Thread Analyzer:**
```
CONFIG_THREAD_ANALYZER=y
CONFIG_THREAD_ANALYZER_USE_PRINTK=y
CONFIG_THREAD_ANALYZER_AUTO=y
```

View output via UART/RTT to see per-thread stack usage.

### Debug Heap Issues

**Enable heap debugging:**
```
CONFIG_HEAP_MEM_POOL_SIZE=8192
CONFIG_SYS_HEAP_VALIDATE=y     # Validate heap on alloc/free
CONFIG_SYS_HEAP_LISTENER=y     # Enable heap event listeners
CONFIG_SYS_HEAP_RUNTIME_STATS=y # Track heap statistics
```

**Runtime Heap Monitoring:**

Option 1: Use heap_monitor module (see "Heap Monitoring Module" section above)

Option 2: Manual shell commands (if shell enabled):
```c
// With CONFIG_KERNEL_SHELL=y
uart:~$ kernel stacks    # Shows stack usage
uart:~$ kernel uptime    # System uptime
// Note: No built-in heap shell command, use heap_monitor module
```

Option 3: Direct API calls:
```c
#include <zephyr/sys/heap_listener.h>
#include <zephyr/sys/mem_stats.h>

extern struct k_heap _system_heap;

void check_heap_usage(void) {
    struct sys_memory_stats stats;
    
    if (sys_heap_runtime_stats_get((struct sys_heap *)&_system_heap.heap,
                                   &stats) == 0) {
        printk("Heap: %zu used, %zu free, peak %zu\n",
               stats.allocated_bytes,
               stats.free_bytes,
               stats.max_allocated_bytes);
    }
}
```

**Wi-Fi Application Heap Requirements:**

For applications using WPA supplicant (Wi-Fi connectivity):

```
# CRITICAL MINIMUM - Never go below for Wi-Fi apps
CONFIG_HEAP_MEM_POOL_SIZE=65536   # 64 KB absolute minimum

# RECOMMENDED for stable operation
CONFIG_HEAP_MEM_POOL_SIZE=80000   # 80 KB base
# Total heap with subsystem add-ons ≈ 125 KB

# Explanation:
# - WPA supplicant (hostapd) adds ~42 KB via HEAP_MEM_POOL_ADD_SIZE_HOSTAP
# - TLS contexts, PMK cache, EAP state machines spike during handshakes
# - Below 64 KB base: malloc failures → AP bring-up failures
# - This is empirical requirement, not from Nordic docs
```

**Build-time heap validation:**
```c
// In your heap_monitor.c or memory module
BUILD_ASSERT(CONFIG_HEAP_MEM_POOL_SIZE >= 65536,
             "CONFIG_HEAP_MEM_POOL_SIZE must stay >= 64 KB for WPA supplicant");
```

**Checking Heap Contributors:**
```sh
# See what subsystems add to heap requirement
grep "HEAP_MEM_POOL_ADD_SIZE" build/zephyr/.config

# Example output:
# CONFIG_HEAP_MEM_POOL_ADD_SIZE_HOSTAP=41808
# CONFIG_HEAP_MEM_POOL_ADD_SIZE_POSIX_THREADS=256
# CONFIG_HEAP_MEM_POOL_ADD_SIZE_SOCKETPAIR=296
# CONFIG_HEAP_MEM_POOL_ADD_SIZE_ZBUS=3072
# Sum = 45,688 bytes (auto-enforced minimum)
```

**Heap vs Net Buffer Memory:**

Not all "network memory" uses the system heap:
```
# System heap (monitored by heap_monitor)
CONFIG_HEAP_MEM_POOL_SIZE=80000

# Net buffer SLABS (separate, not in heap, not monitored)
CONFIG_NET_BUF_RX_COUNT=16      # RX slab pool
CONFIG_NET_BUF_TX_COUNT=32      # TX slab pool
CONFIG_NET_PKT_RX_COUNT=16      # Packet descriptors
CONFIG_NET_PKT_TX_COUNT=24

# These are pre-allocated fixed pools, NOT heap allocations
# Do not try to move them to heap - breaks ISR safety and fragments memory
```

### Debug Memory Corruption

**Enable memory protection:**
```
CONFIG_USERSPACE=y
CONFIG_MPU_STACK_GUARD=y
CONFIG_HW_STACK_PROTECTION=y
CONFIG_THREAD_STACK_INFO=y
```

**Use address sanitizers (if available):**
```
# Some platforms support AddressSanitizer
CONFIG_ASAN=y
```

## Partition Manager (Multi-Image Builds)

For builds with bootloader (MCUboot) + app + network core:

**View partition layout:**
```sh
cat build/partitions.yml
```

**Adjust partition sizes:**

Create `pm_static.yml` or `pm_static_<board>.yml`:
```yaml
mcuboot:
  address: 0x0
  size: 0x10000
app:
  address: 0x10000
  size: 0x70000
mcuboot_pad:
  address: 0x80000
  size: 0x200
```

**Check partition usage:**
```sh
# After build, check fit
west build -t partition_manager_report
```

## Memory Profiling Tools

### Puncover - Interactive Memory Visualization

Generate HTML visualization:
```sh
west build -t puncover
# Opens browser with interactive memory breakdown
```

Features:
- Symbol-level Flash/RAM usage
- Clickable tree view by subsystem
- Size trends across builds

### Static Analysis

```sh
# Detailed ELF analysis
arm-none-eabi-size -A build/zephyr/zephyr.elf

# Section sizes
arm-none-eabi-objdump -h build/zephyr/zephyr.elf

# Symbol table
arm-none-eabi-nm -S --size-sort build/zephyr/zephyr.elf
```

### Runtime Analysis with GDB

```sh
west debug
# In GDB:
(gdb) info mem          # Memory regions
(gdb) x/100x 0x20000000 # Examine RAM
(gdb) print &_heap_start
(gdb) print &_heap_end
```

## Common Memory Issues

### Issue: Flash Overflow

**Symptoms:** Build fails with "region `FLASH' overflowed"

**Solutions:**
1. Enable size optimizations: `CONFIG_SIZE_OPTIMIZATIONS=y`
2. Disable debug features: `CONFIG_ASSERT=n`, `CONFIG_LOG=n`
3. Build in Release mode: `-DCMAKE_BUILD_TYPE=Release`
4. Disable unused subsystems (BT features, networking, etc.)
5. Consider device with more Flash

### Issue: RAM Overflow

**Symptoms:** Build fails with "region `RAM' overflowed"

**Solutions:**
1. Reduce thread stack sizes (analyze with Thread Analyzer first)
2. Reduce heap: `CONFIG_HEAP_MEM_POOL_SIZE`
3. Reduce buffers (network, UART, shell, logging)
4. Use static allocation instead of heap
5. Consider device with more RAM

### Issue: Stack Overflow at Runtime

**Symptoms:** Hard faults, crashes, corruption, unexpected behavior

**Solutions:**
1. Enable `CONFIG_STACK_SENTINEL=y` to detect
2. Use Thread Analyzer to measure actual usage
3. Increase problematic thread's stack size
4. Review recursive functions or large local arrays
5. Enable `CONFIG_MPU_STACK_GUARD=y` for protection

### Issue: Heap Exhaustion

**Symptoms:** malloc/k_malloc returns NULL, out-of-memory errors

**Solutions:**
1. Increase `CONFIG_HEAP_MEM_POOL_SIZE`
2. Check for memory leaks (missing free calls)
3. Use static allocation when possible
4. Enable heap validation: `CONFIG_SYS_HEAP_VALIDATE=y`
5. Profile heap usage with sys_heap_runtime_stats_get()

### Issue: High Memory Usage from Logging

**Symptoms:** Large Flash/RAM usage, logging subsystem in rom_report/ram_report

**Solutions:**
1. Reduce log level: `CONFIG_LOG_DEFAULT_LEVEL=1` (errors only)
2. Use deferred mode with smaller buffer
3. Disable logs in production: `CONFIG_LOG=n`
4. Use minimal logging: `CONFIG_LOG_MODE_MINIMAL=y`
5. Remove LOG_MODULE_REGISTER from unused modules

## Best Practices

### Development Phase
```
# Enable all debugging features
CONFIG_ASSERT=y
CONFIG_LOG=y
CONFIG_LOG_DEFAULT_LEVEL=4
CONFIG_THREAD_ANALYZER=y
CONFIG_STACK_SENTINEL=y
CONFIG_SYS_HEAP_VALIDATE=y
```

### Production Phase
```
# Minimize memory usage
CONFIG_SIZE_OPTIMIZATIONS=y
CONFIG_LTO=y
CONFIG_ASSERT=n
CONFIG_LOG=n              # Or LOG_DEFAULT_LEVEL=1
CONFIG_PRINTK=n
CONFIG_SHELL=n
# Build: west build -b <board> -- -DCMAKE_BUILD_TYPE=MinSizeRel
```

### Continuous Monitoring

1. Track memory usage across builds
2. Set CI/CD thresholds (e.g., alert if Flash > 80%)
3. Review rom_report/ram_report regularly
4. Profile on target hardware (Debug vs Release builds differ)

## Example Workflows

### Basic Memory Analysis
```sh
# 1. Build the project
west build -b nrf52840dk_nrf52840

# 2. Check memory summary (already shown in build output)

# 3. Generate detailed reports
west build -t rom_report      # View Flash usage
west build -t ram_report      # View RAM usage
west build -t puncover         # Interactive HTML report

# 4. Identify largest symbols
arm-none-eabi-nm --size-sort -S build/zephyr/zephyr.elf | tail -30
```

### Reduce Flash by 50%
```sh
# 1. Start with current usage
west build -b nrf52840dk_nrf52840
# Note Flash usage (e.g., 250 KB)

# 2. Apply optimizations to prj.conf
# - CONFIG_SIZE_OPTIMIZATIONS=y
# - CONFIG_LTO=y
# - CONFIG_LOG=n
# - CONFIG_ASSERT=n
# - CONFIG_SHELL=n

# 3. Rebuild in Release mode
west build -b nrf52840dk_nrf52840 -p -- -DCMAKE_BUILD_TYPE=MinSizeRel

# 4. Verify new usage (should be ~125 KB)
```

### Debug Stack Overflow
```sh
# 1. Enable stack debugging in prj.conf:
# CONFIG_STACK_SENTINEL=y
# CONFIG_THREAD_ANALYZER=y
# CONFIG_THREAD_ANALYZER_AUTO=y
# CONFIG_THREAD_NAME=y

# 2. Rebuild and flash
west build -b nrf52840dk_nrf52840
west flash

# 3. Monitor UART/RTT output for:
# - "Stack overflow detected" from sentinel
# - Thread analyzer periodic reports

# 4. Increase stack for problematic thread
# CONFIG_<THREAD>_STACK_SIZE=<larger_value>
```

### Optimize Thread Stacks
```sh
# 1. Enable thread analyzer
# CONFIG_THREAD_ANALYZER=y
# CONFIG_THREAD_ANALYZER_AUTO=y
# CONFIG_THREAD_ANALYZER_AUTO_INTERVAL=30
# CONFIG_THREAD_NAME=y

# 2. Build, flash, and run application through typical use cases
west build -b nrf52840dk_nrf52840
west flash

# 3. View RTT/UART output showing stack usage per thread
# Example output:
#   sysworkq  : unused 512 usage 512 / 1024 (50 %)
#   bt_rx     : unused 256 usage 768 / 1024 (75 %)

# 4. Adjust stack sizes in prj.conf based on actual usage + margin
# CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=1536  # Was 1024, needs more
# CONFIG_BT_RX_STACK_SIZE=896              # Was 1024, can reduce
```

### Add Heap Monitor to Project

Complete workflow to add heap monitoring capability:

```sh
# 1. Create directory structure
mkdir -p src/modules/memory

# 2. Create heap_monitor.c (see "Heap Monitoring Module" section for full code)
cat > src/modules/memory/heap_monitor.c << 'EOF'
/* Copy full heap_monitor.c implementation from skill documentation */
EOF

# 3. Create or update Kconfig
cat > src/modules/memory/Kconfig << 'EOF'
config APP_HEAP_MONITOR
	bool "Enable application heap monitoring"
	depends on HEAP_MEM_POOL_SIZE > 0
	select SYS_HEAP_LISTENER
	select SYS_HEAP_RUNTIME_STATS
	help
	  Monitor system heap usage and log peak allocation.

if APP_HEAP_MONITOR
config APP_HEAP_MONITOR_WARN_PCT
	int "Warn threshold percentage"
	range 50 100
	default 88
config APP_HEAP_MONITOR_STEP_BYTES
	int "Reporting step size in bytes"
	range 128 8192
	default 512
endif
EOF

# 4. Update CMakeLists.txt to include module
echo 'target_sources_ifdef(CONFIG_APP_HEAP_MONITOR app PRIVATE modules/memory/heap_monitor.c)' >> src/CMakeLists.txt

# 5. Source Kconfig in main Kconfig file
echo 'source "src/modules/memory/Kconfig"' >> Kconfig

# 6. Enable in prj.conf
cat >> prj.conf << 'EOF'

# Heap monitoring (development)
CONFIG_SYS_HEAP_LISTENER=y
CONFIG_SYS_HEAP_RUNTIME_STATS=y
CONFIG_APP_HEAP_MONITOR=y
CONFIG_APP_HEAP_MONITOR_WARN_PCT=88
CONFIG_APP_HEAP_MONITOR_STEP_BYTES=512
EOF

# 7. Build and test
west build -b <board> -p
west flash

# 8. Monitor UART/RTT for heap logs
# Expected output:
# <inf> app_heap_monitor: Heap boot: peak=12288 bytes (12% of 100000)...
# <inf> app_heap_monitor: Heap alloc: peak=24576 bytes (24% of 100000)...
```

### Size Wi-Fi Application Heap

Step-by-step heap sizing for Wi-Fi projects:

```sh
# 1. Start with recommended minimum
# In prj.conf:
CONFIG_HEAP_MEM_POOL_SIZE=80000  # 80 KB base

# 2. Check build-time heap calculation
west build -b nrf7002dk_nrf5340_cpuapp -p 2>&1 | grep HEAP
# Look for CMake message showing auto-calculated minimum

# 3. Verify subsystem contributions
grep "HEAP_MEM_POOL_ADD_SIZE" build/zephyr/.config
# Sum all values - should be ~45 KB for typical Wi-Fi app

# 4. Enable heap monitor (see above workflow)

# 5. Build, flash, exercise all features
west flash
# - Connect to Wi-Fi
# - Run HTTP traffic
# - Test all application features
# - Let run for extended period

# 6. Review heap logs for peak usage
# Look for highest "peak" value reported:
# <inf> app_heap_monitor: Heap alloc: peak=95232 bytes (76% of 125000)

# 7. Size production heap
# Production = (observed_peak * 1.2) rounded up
# Example: 95232 * 1.2 = 114,278 → round to 115000 or 120000

# 8. Update prj.conf with justified value
CONFIG_HEAP_MEM_POOL_SIZE=120000  # Sized from peak 95 KB + 25% margin

# 9. Add validation (in heap_monitor.c or memory module)
BUILD_ASSERT(CONFIG_HEAP_MEM_POOL_SIZE >= 65536,
             "Wi-Fi apps require >= 64 KB heap for WPA supplicant");

# 10. Disable heap monitor in production overlay
# overlay-release.conf:
CONFIG_APP_HEAP_MONITOR=n
```

## Notes

- Memory usage varies significantly between Debug and Release builds
- Always test on actual hardware (QEMU/emulation differs)
- Stack overflow is the most common memory issue in embedded systems
- When in doubt, use Thread Analyzer before adjusting stack sizes
- Partition Manager is used for multi-core and bootloader scenarios
- Some optimizations (LTO, size opts) increase build time
- Memory-mapped peripherals don't count against Flash/RAM limits
- Use `west build -t menuconfig` to explore memory-related Kconfig options
- Device Tree overlays can affect memory layout (reserved regions)

### Heap-Specific Notes

- **Wi-Fi applications MUST have ≥64 KB base heap** for WPA supplicant stability
  - Recommended: 80 KB base (results in ~125 KB total with subsystem add-ons)
  - Below 64 KB: malloc failures during TLS handshakes → AP bring-up failures
  - This is empirical requirement from testing, not from Nordic documentation

- **Heap size = Base + Auto Add-ons**
  - Base: `CONFIG_HEAP_MEM_POOL_SIZE`
  - Add-ons: Sum of all `CONFIG_HEAP_MEM_POOL_ADD_SIZE_*` (auto-calculated)
  - Unless `CONFIG_HEAP_MEM_POOL_IGNORE_MIN=y`, Zephyr enforces auto minimum

- **Net buffers are NOT in system heap**
  - Uses dedicated slab pools (`CONFIG_NET_BUF_RX/TX_COUNT`)
  - Separate for ISR safety, determinism, and fragmentation avoidance
  - Don't try to move to heap - breaks architecture

- **Heap monitoring is essential for sizing**
  - Use heap_monitor module (see skill for implementation)
  - Track peak usage under full workload
  - Size production heap = (observed_peak × 1.2) rounded up
  - Disable monitoring in production builds to save resources

- **Wi-Fi driver heap strategy**
  - Default: Dedicated heap (recommended)
  - `CONFIG_NRF_WIFI_GLOBAL_HEAP=y`: Uses system heap (only if severely constrained)
  - Dedicated provides isolation, predictable performance, easier debugging

- **Build-time validation**
  - Add `BUILD_ASSERT(CONFIG_HEAP_MEM_POOL_SIZE >= 65536, ...)` for Wi-Fi apps
  - Prevents accidental configuration regressions
  - Catches issues at build time vs runtime

````