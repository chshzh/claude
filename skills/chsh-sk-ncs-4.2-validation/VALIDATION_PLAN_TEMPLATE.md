# Validation Plan — <Project Name>

## Document Information

| Field | Value |
|-------|-------|
| Project | |
| Version | YYYY-MM-DD-HH-MM |
| PRD Version | YYYY-MM-DD-HH-MM |
| Specs Version | YYYY-MM-DD-HH-MM |
| NCS Version | e.g. v3.3.0 |
| Boards under test | e.g. nRF7002DK, nRF54LM20DK + nRF7002EB2 |
| ZView memory pass | Yes / No |
| Run type | Routine functional / Release |
| Status | Draft / Approved / Executed |

---

## Changelog

| Version | Summary of changes |
|---|---|
| YYYY-MM-DD-HH-MM | Initial plan |

---

## Boards Under Test

> Built from `nrfutil device list` at run time — only boards actually connected and
> with a project config/build. One row per board; this is the addressing source of truth.

| Board | Serial (`--dev-id`) | VCOM port | rtscts | J-Link target | ELF path (sysbuild app sub-image) | Shell |
|-------|---------------------|-----------|--------|---------------|-----------------------------------|-------|
| nRF7002DK | | `/dev/tty.usbmodem…3` | False | `nRF5340_xxAA` | `build_nrf7002dk/<app>/zephyr/zephyr.elf` | ❌ off |
| nRF54LM20DK + nRF7002EB2 | | `/dev/tty.usbmodem…1` | True | `nRF54LM20A_M33` | `build_nrf54lm20dk/<app>/zephyr/zephyr.elf` | ✅ on |

> Shell off ⇒ drive that board via buttons + UART log parsing. Shell on ⇒ drive via shell commands.

---

## Coverage Matrix

> Every PRD feature and each suspected corner-case maps to a round. Common-usage rows
> first, corner cases after. Every row traces to a PRD requirement ID — no invented tests.

| Requirement | Acceptance criterion (short) | Type | Round | Priority |
|-------------|------------------------------|------|-------|----------|
| FR-001 | <criterion> | Common | R1 | P0 |
| FR-002 | <criterion> | Common | R1 | P0 |
| FR-00x | <corner / failure-mode> | Corner | R2 | P1 |
| NFR-01 | Connection time < 30 s | NFR | R1 | P0 |
| NFR-0x | Memory headroom under load | NFR | R3 (ZView) | P0 |

---

## Test Rounds

> **1–3 rounds, max 5.** Combine as many checks as possible into one continuous device
> session. If more than 5 rounds are genuinely needed, stop and ask the user.
> Tag any round that maximizes concurrent memory pressure as **[ZView]** — those drive
> the memory-watermark capture.

### Round R1 — <name> · boards: <which> · [ZView: No]

- **Goal / requirements covered**: FR-001, FR-002, NFR-01
- **Setup**: <flash, credentials, AP state, router-control if needed>
- **Steps** (shell-first; button/log fallback for no-shell boards):
  ```
  wifi connect -s <SSID> -k 1 -p <password>
  wifi status
  net iface
  kernel threads
  ```
- **Expected**: <observable pass condition per criterion>
- **Stability**: loop test N iterations if any criterion says "reliably/consistently".

### Round R2 — <name> · boards: <which> · [ZView: No]

- **Goal / requirements covered**: <corner cases, failure/recovery>
- **Steps**: <commands / button sequence>
- **Expected**: <pass condition>

### Round R3 — <name> · boards: <which> · **[ZView: Yes — high memory]**

- **Goal**: drive worst-case concurrent memory load while ZView records watermarks.
- **High-memory stimulus** (combine as many as apply): concurrent TLS/HTTPS + MQTT
  sessions, OTA firmware download, coredump capture + upload, large log flush.
- **ZView capture**: `west zview record` over J-Link runs for the full round; peaks
  extracted afterward into the report's Memory Watermarks section.
- **Expected**: no allocation failures, no stack overflow; record peak stack/heap per board.

---

## High-Memory Coverage Rationale

> One or two sentences: which scenarios are believed to be the worst-case memory consumers
> for this project (from the specs) and why the ZView round exercises them concurrently —
> so the captured watermarks are the true peaks that `chsh-sk-ncs-3.3-memopt` will size against.

---

## Approval

Plan reviewed and approved to execute: **Yes / No** — <who, when>
