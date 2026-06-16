---
name: chsh-sk-ncs-4.1-verification
description: >-
  Load when running Phase 4.1 Verification for an NCS project — code review,
  clean build, and documentation consistency audit. No hardware required.
---

# chsh-sk-ncs-4.1-verification — Phase 4.1: Verification (no hardware)

Phase 4.1 of the NCS project lifecycle. Runs after implementation is complete,
before any release or demo, and after any merge to main. No hardware required —
can run in CI.

```
4.1 Verification
├── Code review (structure, config, standards)
├── Build verification (README "### Build" commands only)
└── Documentation consistency audit
```

> **Knowledge sources**: Call `mcp_nordic-mcp_nordicsemi_workflow_ncs` at the start of each session — loads `nrfutil-manual` and `embedded-code-guidance-ncs-zephyr`. Use `mcp_nordic-mcp_nordicsemi_search_sources` before checking any Kconfig symbol or board capability.

**Output**: `docs/qa-test/VERIFICATION-YYYY-MM-DD-HH-MM.md`

---

## Step 0 — Check Inputs

```bash
# Extract version chain — record all three before proceeding
# PRD_VERSION: last timestamp row in the PRD Changelog table
grep -E "^\| [0-9]{4}" docs/pm-prd/PRD.md | tail -1

# SPECS_VERSION: last timestamp row in overview.md Changelog table
grep -E "^\| [0-9]{4}" docs/dev-specs/overview.md | tail -1

# Code version: prj.conf carries both version tags
grep -E "ZEGO_APP_(PRD|SPECS)_VERSION" prj.conf

# Remaining inputs
ls docs/dev-specs/                                         # spec files present
git status --short                                         # check for uncommitted changes
grep -n "^### Build" README.md                            # locate canonical build commands
```

Record `PRD_VERSION` (latest PRD Changelog entry), `SPECS_VERSION` (latest overview.md Changelog entry), and the version tags in `prj.conf` — all three go into every report header.

> **Note on uncommitted changes**: if `git status` shows modified source files, the code version is ahead of the last commit. Build and test anyway — but note in the report that the build reflects uncommitted state. The version chain must satisfy:

```
PRD_VERSION  →  specs/overview.md written against PRD_VERSION
                  ↓
             SPECS_VERSION  →  src/main.c SPECS_VERSION tag matches
                                  ↓
                             README features reflect current PRD features
```

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
- `CONFIG_SHELL`: acceptable in **developer template** projects where the UART shell is a documented feature. Flag as P1 only if the project README does not document shell usage or if it is production firmware (not a template).
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

**Build command policy (strict):**
- Use only the commands listed in the application's `README.md` under `### Build`.
- Do not invent or simplify build commands (no ad-hoc `west build -p`, no custom flags)
  unless the README command itself requires parameter substitution.
- If README has multiple board targets, run each command exactly as documented.
- If `### Build` is missing, fail verification as documentation gap (P1) and stop build checks.

```bash
# Run exactly the build commands from README "### Build" section.
# Example policy:
#   1) copy each documented command verbatim
#   2) execute via the configured NCS toolchain launcher
#   3) record pass/fail, warnings, and binary size for each target
```

Zero **compiler** warnings required. Record binary size. Any compiler warning = P1.

> **Known non-P1 Kconfig notices**: `warning: Experimental symbol WIFI_NM_WPA_SUPPLICANT is enabled` and similar NCS Wi-Fi experimental warnings appear on every nRF70 build and are expected. Do not flag these as P1 — they are upstream NCS status notices, not project issues. Only flag new or project-specific Kconfig warnings.

### 4.1.3 Documentation Consistency Audit

Work through the version chain in order. A failure at an earlier step blocks the steps below it.

**Step A — PRD version → Specs**

```bash
# 1. Read the latest PRD Changelog entry to get PRD_VERSION
grep -E "^\| [0-9]{4}" docs/pm-prd/PRD.md | tail -1

# 2. Confirm overview.md PRD Version field matches PRD_VERSION
grep "PRD Version" docs/dev-specs/overview.md
```

| Check | Pass condition |
|-------|----------------|
| Specs reference current PRD version | `docs/dev-specs/overview.md` header/changelog explicitly references `PRD_VERSION`; no older PRD version cited |
| PRD FR/NFR coverage | Every Functional Requirement and Non-Functional Requirement in PRD traceable to at least one spec requirement |

**Step B — Specs version → Code** *(only if Step A passes)*

```bash
# 1. Read the latest Specs Changelog entry to get SPECS_VERSION
grep -E "^\| [0-9]{4}" docs/dev-specs/overview.md | tail -1

# 2. Confirm prj.conf carries the matching tags
grep -E "ZEGO_APP_(PRD|SPECS)_VERSION" prj.conf
```

| Check | Pass condition |
|-------|----------------|
| `CONFIG_ZEGO_APP_SPECS_VERSION` in `prj.conf` | Matches the latest `docs/dev-specs/overview.md` Changelog entry |
| `CONFIG_ZEGO_APP_PRD_VERSION` in `prj.conf` | Matches the latest `docs/pm-prd/PRD.md` Changelog entry |
| Spec modules → code | Every `docs/dev-specs/<module>.md` has a `src/modules/<name>/` counterpart |

**Step C — PRD features → README** *(only if Steps A & B pass)*

```bash
# Extract feature list from PRD (FR section) and compare against README
grep -E "^\*\*?FR[0-9]|^-.*feature" docs/pm-prd/PRD.md | head -20
grep -i "feature\|capability\|support" README.md | head -20
```

| Check | Pass condition |
|-------|----------------|
| README feature list | Every PRD feature (FR items) mentioned in README; no stale or removed features listed |
| No undocumented features in README | README does not describe features absent from the PRD |

### 4.1.4 Generate Verification Report

Create `docs/qa-test/VERIFICATION-YYYY-MM-DD-HH-MM.md` using `VERIFICATION_TEMPLATE.md` as the base:

| Section | Content |
|---------|---------|
| Document Info | PRD version, Specs version, reviewer, date |
| Code Review | Findings per check, severity (P0/P1/P2) |
| Build Result | Pass/Fail, warning count, binary size |
| Docs Audit | Version chain: PRD→Specs (Step A), Specs→Code (Step B), PRD features→README (Step C); coverage gaps |
| Routing | P0 → Phase 3 / spec gap → Phase 2 / ✅ proceed to 4.2 |

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
| All P0 checks pass | ✅ | Proceed to hardware validation (`chsh-sk-ncs-4.2-validation`) |

After reporting, ask:
> "Verification complete. Proceed to hardware validation with **chsh-sk-ncs-4.2-validation**, or route issues to the appropriate phase?"

---

## Gotchas

| Gotcha | Detail |
|--------|--------|
| clang-format version mismatch | Use NCS toolchain clang-format (`nrfutil sdk-manager toolchain launch -- clang-format`), not system clang-format |
| PRD "not visible" ≠ "not implemented" | Auto-reconnect via Zephyr WiFi manager has no visible app code — check `CONFIG_WIFI_CREDENTIALS` / `CONFIG_WIFI_NM` before flagging |
| Security grep false positive | `grep -r "password"` hits comments/docs — read context before flagging |
| Version grep matches table header | `grep -m1 '\| [0-9]'` captures the `\| Version \|` header row, not a real version. Use `grep -E '^\| [0-9]{4}'` to match only timestamp rows. |
| PRD Changelog entries out of order | Authors sometimes append entries non-chronologically. Always read **all** rows and pick the lexicographically latest timestamp, not just the last row. |
| Uncommitted changes affect code version | `git log` reflects the last commit, not on-disk state. Run `git status --short` — if modified files exist, note in the report that the build reflects uncommitted code. |
| SoftAP default password `12345678` | This is the WPS PIN / developer hotspot passphrase, not a user's Wi-Fi password. Context matters — do not flag as a security issue for developer templates. |

---

## Related Skills

| Task | Skill |
|------|-------|
| Implement code (Phase 3) | `chsh-sk-ncs-3.1-coding` |
| Debug firmware failures | `chsh-sk-ncs-3.2-debug` |
| Auto-fix clang-format violations | `chsh-sk-ncs-clang-format` |
| Hardware validation (Phase 4.2) | `chsh-sk-ncs-4.2-validation` |
| Tag and publish release | `chsh-sk-ncs-3.5-release` |
| Full lifecycle orchestration | `chsh-sk-ncs-0-workflow` |

## Self-Update Policy

At the **end of each conversation**, review what was discovered and check
whether any facts in this skill are new, corrected, or outdated (e.g. new
security risk patterns, Zephyr coding standards, or documentation checks).

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
