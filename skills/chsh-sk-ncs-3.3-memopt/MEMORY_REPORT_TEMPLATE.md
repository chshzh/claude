# Memory Optimization Report — <Project Name>

## Document Information

| Field | Value |
|-------|-------|
| Project | |
| Version | YYYY-MM-DD-HH-MM |
| NCS Version | e.g. v3.3.0 |
| Target Board(s) | e.g. nRF7002DK, nRF54LM20DK + nRF7002EB2 |
| Method | Thread Analyzer (`CONFIG_THREAD_ANALYZER_AUTO`) + heap_monitor, steady-state after <describe scenario: Wi-Fi connect, upload cycle, etc.> |
| Status | Draft / Applied |

---

## Changelog

| Version | Summary of changes |
|---|---|
| YYYY-MM-DD-HH-MM | Initial measurement and sizing applied to prj.conf |

---

## Sizing Rules

| Resource | Formula | Headroom |
|----------|---------|---------|
| Thread stacks | `floor(max_usage / 0.9)` | 10 % |
| System heap | `floor(peak / 0.8)` | 20 % |
| mbedTLS heap | `floor(peak / 0.8)` | 20 % |

> Adapt headroom targets as needed. Use worst-case across all measured boards.
> Keep `NET_RX_STACK_SIZE` and `NET_TX_STACK_SIZE` at Zephyr defaults (2048 B) to absorb burst spikes unless explicitly profiled under load.

---

## Thread Stack Analysis

| Thread name | Kconfig | \<Board A\> usage | \<Board B\> usage | Max | New size | Old size | Δ | Risk |
|-------------|---------|-----------------|-----------------|-----|----------|----------|---|------|
| `<thread_name>` | `<CONFIG_SYMBOL>` | X / Y (Z %) | X / Y (Z %) | X | **new** | old | ±Δ | CRITICAL / HIGH / MEDIUM / LOW / — |

**Risk levels:**
- **CRITICAL** — ≥ 90 % utilization
- **HIGH** — 85–89 %
- **MEDIUM** — 80–84 %
- **LOW** — < 80 % but changed from previous measurement
- **—** — unchanged, no action needed

> Add a note for any thread not active during capture (e.g. OTA downloader):
> **Note on `<SYMBOL>`:** thread was not active during capture; previous measurement retained.

---

## Heap Analysis

| Heap | \<Board A\> | \<Board B\> | Peak (worst) | New size | Old size | Δ |
|------|------------|------------|-------------|----------|----------|---|
| System heap (`HEAP_MEM_POOL_SIZE`) | used X / peak **Y** / Z **(P %)** | used X / peak Y / Z (P %) | Y | **new** | old | ±Δ |
| mbedTLS heap (`MBEDTLS_HEAP_SIZE`) | used X / peak Y / Z (P %) | used X / peak **Y** / Z **(P %)** | Y | **new** | old | ±Δ |

> Add or remove heap rows as needed (e.g. no mbedTLS if TLS not used).
> Flag any heap at > 85 % utilization — at that level a single burst allocation can exhaust it.

---

## Summary of Changes Applied to `prj.conf`

| Kconfig | Old | New | Δ B | Reason |
|---------|-----|-----|-----|--------|
| `CONFIG_<SYMBOL>` | old | **new** | ±Δ | reason |

**Net stack RAM change:** ±X B  
**Net heap RAM change:** ±X B  
**Total net RAM change:** ±X B (~Y KB)

---

## ISR Stack

| Board | ISR0 usage | Allocated | Utilization |
|-------|-----------|-----------|-------------|
| \<Board A\> | X B | Y B | Z % |
| \<Board B\> | X B | Y B | Z % |

> `CONFIG_ISR_STACK_SIZE` default is 2048 B. Only increase if utilization exceeds 80 %.

---

## Open Issues

| # | Description | Owner | Target |
|---|-------------|-------|--------|
| 1 | Rebuild and flash all boards after changes to confirm no regression | Team | Next build cycle |
| 2 | <any thread not captured under full load — re-run after that scenario> | Team | <date> |
| 3 | <any heap approaching ceiling — monitor threshold> | Team | Ongoing |
