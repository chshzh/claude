# Flash Memory Layout — <Project Name>

## Document Information

| Field | Value |
|-------|-------|
| Project | |
| Version | YYYY-MM-DD-HH-MM |
| PRD Version | YYYY-MM-DD-HH-MM |
| NCS Version | e.g. v3.3.0 |
| Target Board(s) | e.g. nRF7002DK, nRF54LM20DK + nRF7002EB2 |
| Status | Draft |

> `Version` = this spec's own latest edit time (`date +%Y-%m-%d-%H-%M`); bump it on **every** edit.
> `PRD Version` = the PRD Changelog timestamp this spec tracks.

---

## Changelog

| Version | Summary of changes |
|---|---|
| YYYY-MM-DD-HH-MM | Initial partition layout |

---

## Overview

NCS v3.3+ uses DeviceTree fixed-partitions (DTS overlays) instead of the legacy Partition Manager (PM).
Layouts below reflect the DTS-based configuration in `boards/<board>.overlay`.

`SB_CONFIG_PARTITION_MANAGER=n` must be set in `sysbuild.conf` to disable PM and let MCUboot use DTS directly.

---

## Flash Memory Layout

### <Board 1> (<SoC> — <X> KB/<X> MB internal flash)

> If this board has both internal flash and external flash, fill in both sub-sections.
> Delete the external flash sub-section if the board has no external flash.

#### Internal Flash (`<dts-node-label>`, e.g. `cpuapp_flash` or `cpuapp_rram`)

| Address | Partition | Size | Purpose |
|---------|-----------|------|---------|
| `0x00000` | `boot_partition` | XX KB | Bootloader (MCUboot) |
| `0x0XXXX` | `slot0_partition` | XXX KB | Primary app image |
| `0xXXXXX` | `storage_partition` | 8 KB | NVS — Wi-Fi credentials and small persistent app state |
| `0xXXXXX` | `<coredump_partition>` | 64 KB | Crash coredumps (`CONFIG_MEMFAULT_COREDUMP_STORAGE_CUSTOM=y` or `_RRAM=y`) |

#### External Flash (`<dts-node-label>`, e.g. `mx25r64` — X MB)

| Address | Partition | Size | Purpose |
|---------|-----------|------|---------|
| `0x000000` | `slot1_partition` | XXX KB | Secondary OTA slot |
| `0xXXXXXX` | `<extra_partition>` | X KB | *(add project-specific partitions here)* |

---

### <Board 2> (<SoC> — <X> KB/<X> MB internal flash)

#### Internal Flash (`<dts-node-label>`)

| Address | Partition | Size | Purpose |
|---------|-----------|------|---------|
| `0x000000` | `boot_partition` | XX KB | Bootloader (MCUboot) |
| `0x0XXXXX` | `slot0_partition` | XXXX KB | Primary app image |
| `0xXXXXXX` | `storage_partition` | 8 KB | NVS — Wi-Fi credentials and small persistent app state |
| `0xXXXXXX` | `<coredump_partition>` | 64 KB | Crash coredumps |

#### External Flash (`<dts-node-label>` — X MB)

| Address | Partition | Size | Purpose |
|---------|-----------|------|---------|
| `0x000000` | `slot1_partition` | XXXX KB | Secondary OTA slot |
| `0xXXXXXX` | `<extra_partition>` | X KB | *(add project-specific partitions here)* |

---

## Migration Notes (NCS 3.2 → 3.3)

> Fill this section only if migrating from Partition Manager. Delete if this is a new project.

| Aspect | Partition Manager (v3.2−) | DeviceTree (v3.3+) | Reason |
|--------|---------------------------|--------------------|--------|
| Configuration | YAML (`pm_static_*.yml`) | DTS overlays (`.overlay`) | DTS is Zephyr standard; PM is deprecated |
| Build system | Sysbuild PM subsystem | Standard MCUboot DTS parsing | Simplifies toolchain |
| Pad bytes | Explicit `mcuboot_pad` node | Eliminated; addresses aligned directly | Reduces complexity |
| OTA compatibility | PM-based images | DTS-based images | OTA across PM↔DTS boundary is **not supported** — reflash required |

> **Important**: firmware built under PM (v3.2) cannot be OTA-updated to DTS (v3.3) due to address shifts.
> Devices must be reflashed via hardware or a custom transition strategy.

### Legacy Layout (Partition Manager, NCS 3.2 and earlier)

#### <Board 1> — Legacy

| Address | Partition | Size | Purpose | Notes |
|---------|-----------|------|---------|-------|
| `0x00000` | `mcuboot` | XX KB | Bootloader | — |
| `0x0XXXX` | `mcuboot_pad` | 512 B | Image header padding | Removed in DTS |
| `0x0XXXX` | `app` | XXX KB | Main application | Now starts at `0x0XXXX` |
| `0xXXXXX` | `settings_storage` | 8 KB | NVS / credentials | — |
| `0xXXXXX` | `<coredump_partition>` | 64 KB | Crash coredumps | — |

---

## Storage Partition Capacity Notes

The `storage_partition` (8 KB) is shared by Zephyr settings subsystem and **all** modules
writing to NVS. Budget carefully:

| Consumer | Estimated size | Notes |
|----------|---------------|-------|
| Wi-Fi credentials (SSID + PSK) | ~256 B | Per stored AP |
| Mode selection (NVS key) | ~16 B | Single enum value |
| *(add project-specific entries)* | | |
| **Total** | **~X KB** | Must stay well below 8 KB |

> Raw diagnostic snapshots (e.g. log-state blobs) should **not** use `storage_partition`.
> Allocate a dedicated external-flash partition instead (see example: `mflt_log_state_partition`).

---

## DTS Overlay Checklist

For each board, ensure the following exist and are consistent:

- [ ] `boards/<board>.overlay` — app image partitions (`boot_partition`, `slot0_partition`, `storage_partition`, coredump partition)
- [ ] `sysbuild/mcuboot/boards/<board>.overlay` — MCUboot sees the same partition map
- [ ] `sysbuild.conf` contains `SB_CONFIG_PARTITION_MANAGER=n`
- [ ] `prj.conf` / board conf sets the coredump storage backend Kconfig (`CONFIG_MEMFAULT_COREDUMP_STORAGE_CUSTOM=y` or `_RRAM=y`)
- [ ] `slot0_partition` + `slot1_partition` are **identical in size** on both overlays (MCUboot requirement)
- [ ] Total internal flash allocation ≤ SoC flash capacity

---

## Related Specs

- [1-architecture.md](1-architecture.md) — memory budget and SoC selection
- [3-memopt.md](3-memopt.md) — RAM budget and headroom tracking
