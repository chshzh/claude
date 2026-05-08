# Heap Monitor Module — Reference

## Module Location

Production-ready implementation in the `ncs-project-logo` reference project:

```
src/modules/heap_monitor/
├── heap_monitor.c        # Implementation
├── CMakeLists.txt        # Added when CONFIG_HEAPS_MONITOR=y
├── Kconfig.heap_monitor  # Symbol definitions
└── Kconfig.defaults      # Sensible defaults (off by default)
```

Wire into the project:

```kconfig
# Kconfig — inside menu "Application configuration"
rsource "src/modules/heap_monitor/Kconfig.heap_monitor"
rsource "src/modules/heap_monitor/Kconfig.defaults"
```

```cmake
# CMakeLists.txt
add_subdirectory(src/modules/heap_monitor)
```

## Enable

```
CONFIG_HEAPS_MONITOR=y
CONFIG_HEAPS_MONITOR_LOG_LEVEL_INF=y
```

The module automatically selects `SYS_HEAP_LISTENER` + `SYS_HEAP_RUNTIME_STATS`
when `CONFIG_HEAP_MEM_POOL_SIZE > 0`, and selects `MBEDTLS_MEMORY_DEBUG` when
`CONFIG_MBEDTLS_ENABLE_HEAP=y`. No other `prj.conf` entries are needed.

## Tuning Knobs

| Symbol | Default | Meaning |
|--------|---------|---------|
| `CONFIG_HEAPS_MONITOR_WARN_PCT` | 88 | Emit `LOG_WRN` when used% reaches this |
| `CONFIG_HEAPS_MONITOR_STEP_BYTES` | 512 | Report only when high-water mark advances by this many bytes |
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

## Log Output

```
<inf> heap_monitor: System Heap: used=51712/98304 (52%) blocks=n/a, peak=64752/98304 (65%), peak_blocks=n/a
<inf> heap_monitor: mbedTLS Heap: used=19136/110592 (17%) blocks=49, peak=72684/110592 (65%), peak_blocks=197
<wrn> heap_monitor: System Heap: used=86500/98304 (88%) blocks=n/a, peak=86500/98304 (88%), peak_blocks=n/a
```

- **System heap**: Zephyr runtime stats expose bytes only, `blocks=n/a`.
- **mbedTLS heap**: reports real `blocks` and `peak_blocks` from `mbedtls_memory_buffer_alloc_{cur,max}_get()`.

## Memfault Integration

When `CONFIG_APP_MEMFAULT_MODULE=y`, the module calls `MEMFAULT_METRIC_SET_UNSIGNED()` on every
periodic snapshot — no extra wiring needed. Register metric keys in
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

## Heap Architecture

NCS/Zephyr uses several distinct heaps:

### 1. System heap (`_system_heap`) — monitored by heap_monitor

- General `malloc`/`calloc`/`free`, POSIX thread allocations
- WPA supplicant (when `CONFIG_NRF_WIFI_GLOBAL_HEAP=y`)
- Sized by `CONFIG_HEAP_MEM_POOL_SIZE` + `HEAP_MEM_POOL_ADD_SIZE_*` (auto-calculated by Kconfig)
- **Wi-Fi apps: ≥64 KB base required; 80 KB recommended** (total ~125 KB with add-ons)

```sh
# Check what subsystems add to heap:
grep "HEAP_MEM_POOL_ADD_SIZE" build/zephyr/.config
# Common output:
# CONFIG_HEAP_MEM_POOL_ADD_SIZE_HOSTAP=41808       # WPA supplicant
# CONFIG_HEAP_MEM_POOL_ADD_SIZE_POSIX_THREADS=256
# CONFIG_HEAP_MEM_POOL_ADD_SIZE_ZBUS=3072
```

### 2. Net buffer slabs — NOT monitored, NOT in system heap

- Dedicated fixed-size slab pools for network packets
- Configured via `CONFIG_NET_BUF_RX_COUNT`, `CONFIG_NET_BUF_TX_COUNT`
- Separate for ISR safety, determinism, and fragmentation avoidance
- Do not attempt to move them to system heap

### 3. Wi-Fi driver heap (when `CONFIG_NRF_WIFI_GLOBAL_HEAP=n`)

- Dedicated region for firmware and DMA descriptors
- Provides isolation and predictable performance
- Use `CONFIG_NRF_WIFI_GLOBAL_HEAP=y` only if severely memory-constrained

### 4. mbedTLS heap (`CONFIG_MBEDTLS_ENABLE_HEAP=y`) — monitored by heap_monitor

- Fixed-size buffer carved out of RAM at boot, separate from `_system_heap`
- Sized exclusively by `CONFIG_MBEDTLS_HEAP_SIZE` (no auto add-ons)
- Peaks during TLS handshakes; can spike to ~70 KB on Wi-Fi/HTTPS workloads
- `CONFIG_MBEDTLS_MEMORY_DEBUG=y` is auto-selected by `CONFIG_HEAPS_MONITOR`
- Recommended margin: 1.5× of measured peak

```
CONFIG_MBEDTLS_ENABLE_HEAP=y
CONFIG_MBEDTLS_HEAP_SIZE=110592    # 150% of ~72 KB measured peak
```
