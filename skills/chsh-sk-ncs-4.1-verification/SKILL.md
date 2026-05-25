---
name: chsh-sk-ncs-4.1-verification
description: >-
  Load when running Phase 4 Verification & Validation (V&V) for an NCS project.
  Covers 4.1 Verification (code review, build, docs audit — no hardware) and
  4.2 Validation (hardware tests via EEDP — GPIO, UART, Saleae, JLink, Router,
  PPK2, Wireshark — hardware required).
---

# chsh-sk-ncs-4.1-verification — Phase 4: Verification & Validation (V&V)

Phase 4 of the NCS project lifecycle. Run after implementation is complete or
updated, before any release or demo, and after any merge to main.

```
Phase 4 — Verification & Validation (V&V)
├── 4.1 Verification (always)         ← code review, build, docs audit (no hardware)
│   ├── Code review (structure, config, standards)
│   ├── Build verification (west build -p)
│   └── Documentation consistency audit
│
├── 4.2 Validation (always, hardware required)
│   ├── GPIO Shell → button sequences, LED state
│   ├── UART log → assert log content
│   ├── Saleae → protocol timing analysis
│   ├── JLink → flash/reset/coredump
│   ├── Router → Wi-Fi reconnect, packet loss simulation
│   ├── PPK2 → power budget verification
│   └── Wireshark → 802.11 frame analysis
│
└── Feedback routing (P0→Phase 3, Spec gap→Phase 2, New req→Phase 1)
```

**Outputs**:
- 4.1: `docs/qa/VERIFICATION-YYYY-MM-DD-HH-MM.md`
- 4.2: `docs/qa/VALIDATION-YYYY-MM-DD-HH-MM.md`

---

## Step 0 — Check Inputs

```bash
cat docs/pm-prd/PRD.md              # acceptance criteria
ls docs/dev-specs/                  # spec files
grep "SPECS_VERSION" src/main.c     # spec version tag
west build --build-dir build/ 2>&1 | tail -5   # confirm build baseline
```

Note the PRD Changelog version and Specs version — both go into all report headers.

---

## 4.1 — Verification

No hardware required. Can be run independently at any point.

### 4.1.1 Code Review

Inspect each area below. Flag each finding with severity (P0 / P1 / P2).

**Structure**
- `src/modules/` layout matches `docs/dev-specs/architecture.md`
- Required top-level files present: `CMakeLists.txt`, `Kconfig`, `prj.conf`
- No hardcoded paths in `CMakeLists.txt`; modules gated by `CONFIG_APP_*`

**Config**
- `prj.conf` free of conflicts; overlay files named consistently
- No unguarded `CONFIG_SHELL`, `CONFIG_LOG_BACKEND_*` in release build
- All overlay files documented in README

**Standards**
- Zbus usage: no direct publish from ISR context (use `k_msgq` or `atomic`)
- `SYS_INIT` order: drivers initialized before app modules
- All return codes checked; Zbus publish errors logged
- No stack-allocated buffers > 512 B in ISR context

**Security** (any finding = P0 — stop, route to Phase 3 immediately)

| Risk | Where to check | Pass condition |
|------|---------------|----------------|
| Hardcoded WiFi credentials | `prj.conf`, `overlay-*.conf`, `*.c` | No literal passwords in source |
| WiFi credentials in VCS | `git log --all -- prj.conf` | No real passwords committed |
| Debug features in release build | `prj.conf` | `CONFIG_SHELL` etc. guarded or disabled |
| HTTP instead of HTTPS | MQTT/HTTP URLs in code | All remote endpoints TLS (`mqtts://`, 8883/443) |
| Memfault project key in source | `prj.conf`, `*.c` | Key only via `CONFIG_MEMFAULT_NCS_PROJECT_KEY` |

### 4.1.2 Build Verification

```bash
west build -p
```

Zero warnings required. Record binary size. Any warning = P1.

### 4.1.3 Documentation Consistency Audit

| Check | Pass condition |
|-------|----------------|
| `SPECS_VERSION` in `src/main.c` | Matches latest `docs/dev-specs/overview.md` Changelog entry |
| Spec modules | Every `docs/dev-specs/<module>.md` has a `src/modules/<name>/` counterpart |
| README feature list | Reflects current PRD features (no stale or missing entries) |
| PRD acceptance criteria in specs | Each FR/NFR traceable to at least one spec requirement |

### 4.1.4 Generate Verification Report

Create `docs/qa/VERIFICATION-YYYY-MM-DD-HH-MM.md`:

| Section | Content |
|---------|---------|
| Document Info | PRD version, Specs version, reviewer, date |
| Code Review | Findings per check, severity (P0/P1/P2) |
| Build Result | Pass/Fail, warning count, binary size |
| Docs Audit | SPECS_VERSION match, spec→code coverage gaps |
| Routing | P0 → Phase 3 / spec gap → Phase 2 / ✅ proceed to 4.2 |

---

## 4.2 — Validation

Hardware validation against PRD acceptance criteria. **Hardware required.**

**Test framework**: Load **chsh-sk-ncs-4.2-validation** for the full validation workflow
(flash → UART → map TCs → execute → loop test → report).

**EEDP platform**: **chsh-sk-ncs-3.2-debug** →
[`references/eedp-platform.md`](../chsh-sk-ncs-3.2-debug/references/eedp-platform.md)

**Test case library** (extend per project):

| TC | When to run | File |
|----|--------------|------|
| TC-WIFI-THROUGHPUT | PRD includes throughput acceptance criteria | [`tc-wifi-throughput.md`](../chsh-sk-ncs-4.2-validation/tc-wifi-throughput.md) |
| TC-MEMFAULT-LOG | PRD includes Memfault observability requirements | [`tc-memfault-log.md`](../chsh-sk-ncs-4.2-validation/tc-memfault-log.md) |

Output: `docs/qa/VALIDATION-YYYY-MM-DD-HH-MM.md`

---

## Feedback Routing

| Finding | Priority | Route |
|---------|----------|-------|
| Security finding (P0) | P0 | Phase 3 — fix code immediately |
| P0 test case fails (code bug) | P0 | Phase 3 (`chsh-sk-ncs-3.1-coding`) |
| Build failure or warning | P0/P1 | Phase 3 |
| PRD criterion mismatch (❌) | P0 | Phase 3 |
| Spec gap / undocumented behaviour | P1 | Phase 2 (`chsh-sk-ncs-2-spec`) |
| New requirement found | P1 | Phase 1 (`chsh-sk-ncs-1-prd`) → Phase 2 → Phase 3 |
| P1/P2 issues only | P2 | Phase 3 (next iteration) |
| All P0 checks pass (4.1) + all P0 TCs pass (4.2) | ✅ | Ready for release |

After reporting, ask:
> "Verification complete. Route P0 issues to the appropriate phase, or proceed
> to release with **chsh-sk-git-release**?"

---

## Gotchas

| Gotcha | Detail |
|--------|--------|
| clang-format version mismatch | Use NCS toolchain clang-format (`nrfutil sdk-manager toolchain launch -- clang-format`), not system clang-format |
| PRD "not visible" ≠ "not implemented" | Auto-reconnect via Zephyr WiFi manager has no visible app code — check `CONFIG_WIFI_CREDENTIALS` / `CONFIG_WIFI_NM` before flagging |
| Security grep false positive | `grep -r "password"` hits comments/docs — read context before flagging |
| Loop test mandatory for stability TCs | Never accept a single-pass result for "reliably" / "consistently" criteria |
| Flash `--recover` erases NVS | WiFi credentials lost — re-provision after using `--recover` |

---

## Related Skills

| Task | Skill |
|------|-------|
| Implement code (Phase 3) | `chsh-sk-ncs-3.1-coding` |
| UART capture, loop test, crash debug | `chsh-sk-ncs-3.2-debug` |
| AP simulation, SSID control | `chsh-sk-router-control` |
| Memfault log upload diagnostics | `chsh-sk-ncs-tc-memfault-log-debug` |
| Wi-Fi throughput benchmarking | `chsh-sk-ncs-tc-wifi-throughput` |
| Auto-fix clang-format violations | `chsh-sk-ncs-clang-format` |
| Memfault symbol upload / OTA | `chsh-sk-memfault-cli` |
| Tag and publish release | `chsh-sk-git-release` |
| Full lifecycle orchestration | `chsh-sk-ncs-0-workflow` |

## Self-Update Policy

At the **end of each conversation**, review what was discovered and check
whether any facts in this skill are new, corrected, or outdated (e.g. new
EEDP modules, security risk patterns, test case procedures, throughput baselines).

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
