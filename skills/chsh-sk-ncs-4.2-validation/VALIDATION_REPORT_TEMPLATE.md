# Validation Report — <Project Name>

## Document Information

| Field | Value |
|-------|-------|
| Project | |
| Version | YYYY-MM-DD-HH-MM |
| PRD Version | YYYY-MM-DD-HH-MM |
| Specs Version | YYYY-MM-DD-HH-MM |
| Plan Version | YYYY-MM-DD-HH-MM *(VALIDATION_PLAN.md this run executed)* |
| Tester | |
| Boards under test | e.g. nRF7002DK, nRF54LM20DK + nRF7002EB2 |
| NCS Version | e.g. v3.3.0 |
| Firmware built | YYYY-MM-DD-HH-MM |
| ZView memory pass | Yes / No |
| Status | Draft / Pass / Pass with issues / Fail |

---

## Changelog

| Version | Summary of changes |
|---|---|
| YYYY-MM-DD-HH-MM | Initial validation run |

---

## Executive Summary

| Total TCs | Passed | Failed | Blocked | Not Run |
|-----------|--------|--------|---------|---------|
| | | | | |

**Overall Result:**
- [ ] ✅ PASS — all P0 acceptance criteria met
- [ ] ⚠️ PASS WITH ISSUES — P1/P2 failures only
- [ ] ❌ FAIL — one or more P0 criteria failed

**Verdict**: _one sentence_

---

## Test Environment

> One block per board under test (from VALIDATION_PLAN.md Boards Under Test).

```
Board:        nRF7002DK   (SN <dev-id>)
Flash:        west flash -d build_nrf7002dk/ --dev-id <SN>
Serial:       /dev/tty.usbmodem…3  @ 115200  rtscts=False   (shell off — button/log driven)

Board:        nRF54LM20DK + nRF7002EB2   (SN <dev-id>)
Flash:        west flash -d build_nrf54lm20dk/ --dev-id <SN>
Serial:       /dev/tty.usbmodem…1  @ 115200  rtscts=True    (shell on)
```

---

## Test Results

> Each TC maps to a PRD acceptance criterion (TC-<FR>-<seq>). Evidence = real UART/shell
> lines, not "it worked". Track per board where behaviour differs.

### FR-001 — <Feature Title>

| TC | Acceptance Criterion (from PRD) | Board | Result | Evidence |
|----|---------------------------------|-------|--------|----------|
| TC-001-01 | <criterion text> | nRF7002DK | ✅ / ❌ / ⚠️ / ⬜ | |
| TC-001-01 | <criterion text> | nRF54LM20DK | ✅ / ❌ / ⚠️ / ⬜ | |

**Notes / Observations:**

---

### NFR — Non-Functional Requirements

| TC | Requirement (from PRD) | Metric | Measured | Result |
|----|------------------------|--------|----------|--------|
| TC-NFR-01 | Connection time < 30 s | 30 s | ___ s | ✅ / ❌ |
| TC-NFR-02 | Stable over N reboots | N | ___ / N pass | ✅ / ❌ |

---

## Failed Tests Detail

> Fill in only for ❌ Fail or ⚠️ Partial.

### TC-XXX-YY — <criterion text> · <board>

**Expected**: <what PRD says should happen>
**Actual**: <what happened>

**UART / shell log:**
```
<relevant lines>
```

**Root cause (if known)**:
**Suggested fix / routing**:
- [ ] Code fix → Phase 3 (`chsh-sk-ncs-3.1-coding`)
- [ ] Spec gap → Phase 2 (`chsh-sk-ncs-2-spec`)
- [ ] PRD change → Phase 1 (`chsh-sk-ncs-1-prd`)

---

## Memory Watermarks (ZView)

> **Hand-off contract for `chsh-sk-ncs-3.3-memopt`.** Captured by ZView over SWD during the
> high-memory round(s) of VALIDATION_PLAN.md — these are true peaks under worst-case load.
> Columns mirror MEMOPT_REPORT_TEMPLATE so 3.3 can ingest directly. Skip this section
> (mark `[SKIP — no ZView pass]`) only if the ZView memory pass was not run.
>
> Capture: `west zview record -e build_<board>/<app>/zephyr/zephyr.elf -r jlink -t <target> -o cap.ndjson.gz --duration <N>`
> (J-Link targets: nRF7002DK → `nRF5340_xxAA`, nRF54LM20DK → `nRF54LM20A_M33`)
> then `west zview dump -i cap.ndjson.gz --frame <peak> --json`. Producing round: **R_**.

### Thread Stacks — peak usage

| Thread name | Kconfig symbol | <Board A> peak / alloc | <Board B> peak / alloc | Max % | Risk |
|-------------|----------------|------------------------|------------------------|-------|------|
| `<thread>` | `CONFIG_<SYMBOL>` | X / Y (Z %) | X / Y (Z %) | Z % | CRITICAL / HIGH / MEDIUM / LOW / — |

**Risk:** CRITICAL ≥90 % · HIGH 85–89 % · MEDIUM 80–84 % · LOW <80 %.

### Heaps — peak usage

| Heap | Kconfig symbol | <Board A> peak / size | <Board B> peak / size | Max % |
|------|----------------|-----------------------|-----------------------|-------|
| System heap | `CONFIG_HEAP_MEM_POOL_SIZE` | X / Y (Z %) | X / Y (Z %) | Z % |
| mbedTLS heap | `CONFIG_MBEDTLS_HEAP_SIZE` | X / Y (Z %) | X / Y (Z %) | Z % |

> Note any thread not active during the high-memory round (e.g. OTA downloader if OTA
> didn't fire) — its watermark is not a true peak; flag it so 3.3 retains the prior value.

---

## UART Boot Log

> Full boot sequence per board, power-on to ready, as evidence.

```
<paste>
```

---

## Performance Metrics

| Metric | Board | Target | Measured | Pass? |
|--------|-------|--------|----------|-------|
| Boot to ready | | < ___ ms | ___ ms | |
| Wi-Fi connect time | | < ___ s | ___ s | |

---

## Routing

| Finding | Priority | Route |
|---------|----------|-------|
| P0 TC fails (code bug) | P0 | Phase 3 (`chsh-sk-ncs-3.1-coding`) |
| P0 TC fails (spec gap) | P0 | Phase 2 (`chsh-sk-ncs-2-spec`) |
| Build/flash fails on a board | P0 | Phase 3 |
| PRD criterion ambiguous | P1 | Phase 1 (`chsh-sk-ncs-1-prd`) |
| Thread/heap at CRITICAL risk | P1 | Phase 3.3 (`chsh-sk-ncs-3.3-memopt`) — feed this report |
| P1/P2 failures only | P2 | Phase 3 (next iteration) |
| All P0 TCs pass | ✅ | Ready for release (`chsh-sk-ncs-3.5-release`) |
