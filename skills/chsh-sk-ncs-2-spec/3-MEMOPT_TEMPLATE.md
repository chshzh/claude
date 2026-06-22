# Memory Optimization Report — <Project Name>

## Document Information

| Field | Value |
|-------|-------|
| Project | |
| Version | YYYY-MM-DD-HH-MM |
| NCS Version | e.g. v3.3.0 |
| Target Board(s) | e.g. nRF7002DK, nRF54LM20DK + nRF7002EB2 |
| Method | ZView watermark live measurements; steady-state after Wi-Fi STA connect + Memfault upload + MQTT/HTTPS cycle |
| Status | Draft |

> `Version` = this doc's own latest edit time (`date +%Y-%m-%d-%H-%M`); bump it on **every** edit.
> No `PRD Version` field — this doc tracks code, not product requirements.

---

## Changelog

| Version | Summary of changes |
|---|---|
| YYYY-MM-DD-HH-MM | Initial measurement pass |

---

## Sizing Rules

| Resource | Formula | Headroom |
|----------|---------|---------|
| Thread stacks (watermark < 5120 B) | `ceil(watermark / 0.8)` | 20 % |
| Thread stacks (watermark ≥ 5120 B) | `ceil(watermark / 0.9)` | 10 % |
| Heaps | `ceil(peak / 0.8)` | 20 % |

`NET_RX_STACK_SIZE` and `NET_TX_STACK_SIZE` are kept at the Zephyr default (2048 B) regardless
of measurement to absorb network burst spikes.

---

## Headroom Targets

| Resource | Minimum headroom |
|----------|-----------------|
| Internal Flash | > 10 % of SoC flash |
| RAM (total) | > 5 % of SoC RAM |
| `storage_partition` (8 KB NVS) | > 2 KB free |

---

## Thread Stack Analysis

> Fill one row per thread/WQ. Use worst-case value across all boards.
> Omit threads not measured in this pass — add a note explaining why.

| Thread / WQ | Kconfig | <Board A> watermark (B) | <Board B> watermark (B) | Worst-case | Rule | New size | Old size | Δ (B) |
|-------------|---------|------------------------|------------------------|------------|------|----------|----------|-------|
| `sysworkq` | `SYSTEM_WORKQUEUE_STACK_SIZE` | — | — | — | ÷0.8 | | | |
| `main` | `MAIN_STACK_SIZE` | — | — | — | ÷0.9 | | | |
| *(add threads)* | | | | | | | | |
| `rx_q` | `NET_RX_STACK_SIZE` | — | — | — | kept | 2048 | 2048 | 0 |
| `tx_q` | `NET_TX_STACK_SIZE` | — | — | — | kept | 2048 | 2048 | 0 |

---

## Heap Analysis

> List every heap separately. Split dedicated K_HEAPs from the system heap when
> `NRF_WIFI_GLOBAL_HEAP=n` / `WIFI_NM_WPA_SUPPLICANT_GLOBAL_HEAP=n` is set.

| Heap | ZView pool name | <Board A> watermark (B) | <Board B> watermark (B) | Worst-case | New size | Old size | Δ (B) |
|------|-----------------|------------------------|------------------------|------------|----------|----------|-------|
| System heap | `_system_heap` | — | — | — | | | |
| mbedTLS heap | — | — | — | — | | | |
| *(add project heaps)* | | | | | | | |

---

## ISR Stack

| Board | ISR usage (B) | Allocated (B) | Utilization |
|-------|--------------|---------------|-------------|
| <Board A> | — | 2048 | — |
| <Board B> | — | 2048 | — |

> `CONFIG_ISR_STACK_SIZE` defaults to 2048 B. Increase only if usage exceeds 80 %.

---

## Flash & RAM Budget

> Fill after a clean build with `west build`. Read from `zephyr.map` or build output.

| Board | Flash used | Flash avail | Flash headroom | RAM used | RAM avail | RAM headroom |
|-------|-----------|------------|----------------|---------|----------|--------------|
| <Board A> | — KB | — KB | — % | — KB | — KB | — % |
| <Board B> | — KB | — KB | — % | — KB | — KB | — % |

---

## Summary of Changes Applied

| Kconfig | Old | New | Δ (B) | Reason |
|---------|-----|-----|-------|--------|
| `CONFIG_<SYMBOL>` | — | — | — | watermark X B |

**Net stack RAM change:** ± X B  
**Net heap RAM change:** ± X B  
**Total net RAM change:** ± X B

---

## Open Issues

| # | Description | Owner | Target |
|---|-------------|-------|--------|
| 1 | Re-measure after first OTA download cycle to capture `DOWNLOADER_STACK_SIZE` peak | — | Next OTA test |
| *(add items)* | | | |
