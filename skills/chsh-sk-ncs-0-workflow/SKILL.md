---
name: chsh-sk-ncs-0-workflow
description: Use as the single entry point for any NCS project work. Orchestrates the full NCS project lifecycle — detects where a project stands, guides through each phase (PRD → Specs → Code → QA+Test), and routes feedback back to the right phase.
---

# chsh-sk-ncs-0-workflow — NCS Project Lifecycle Orchestrator

Single entry point for any NCS project work. Scans the project state, presents a
status dashboard, and guides you through each phase — invoking the right skill at each step.

---

## The Four-Phase Cycle

```
┌────────────────────────────────────────────────────────────────┐
│  PHASE 1 — PRODUCT DEFINITION         skill: chsh-sk-ncs-1-prd       │
│                                                                 │
│  Product Manager defines device requirements:                  │
│  • What should the device do, for whom, and why?               │
│  • Which Wi-Fi modes, UI behaviours, connectivity features?    │
│  • Success metrics and release criteria                        │
│                                                                 │
│  Input:  stakeholder ideas, user feedback, bug reports         │
│  Output: docs/pm-prd/PRD.md  (Changelog updated)             │
└───────────────────────────┬────────────────────────────────────┘
                            │ PRD approved → triggers Phase 2
                            ▼
┌────────────────────────────────────────────────────────────────┐
│  PHASE 2 — TECHNICAL DESIGN           skill: chsh-sk-ncs-2-spec     │
│                                                                 │
│  Developer Engineer translates PRD into technical specs:       │
│  • Architecture choice (SMF+Zbus or multi-threaded)            │
│  • docs/dev-specs/overview.md                          │
│  • docs/dev-specs/architecture.md                      │
│  • docs/dev-specs/<module>.md (one per feature)        │
│  • docs/dev-specs/config.yaml (optional project context)           │
│                                                                 │
│  Input:  docs/pm-prd/PRD.md                                   │
│  Output: docs/dev-specs/*.md (PRD Version field set)   │
└───────────────────────────┬────────────────────────────────────┘
                            │ Specs approved → triggers Phase 3
                            ▼
┌────────────────────────────────────────────────────────────────┐
│  PHASE 3 — IMPLEMENTATION   3.1-coding · 3.2-debug · 3.3-memopt        │
│                                                                 │
│  3.1 Coding: src/modules/, prj.conf, CMakeLists.txt, Kconfig   │
│  3.2 Debug:  fix runtime failures, crashes, WiFi issues        │
│  3.3 Memopt: RAM/Flash budget, stack overflow, memory usage    │
│                                                                 │
│  Input:  docs/dev-specs/*.md                           │
│  Output: code, clean build, UART log evidence                  │
└───────────────────────────┬────────────────────────────────────┘
                            │ Implementation done → triggers Phase 4
                            ▼
┌────────────────────────────────────────────────────────────────┐
│  PHASE 4 — VERIFICATION & VALIDATION (V&V)                                   │
│           4.1: chsh-sk-ncs-4.1-verification · 4.2: chsh-sk-ncs-4.2-validation │
│                                                                 │
│  4.1 Verification (always, no HW): code review, build, docs    │
│  4.2 Validation via EEDP (HW): GPIO, UART, Saleae, JLink, Router  │
│  • Output: docs/qa-test/ (VERIFICATION + VALIDATION)                │
│                                                                 │
│  P0 → Phase 3  |  Spec gap → Phase 2  |  New req → Phase 1    │
└────────────────────────────────────────────────────────────────┘
```

---

## Step 0 — Scan Project State

Run this before anything else. Provide the project path if not already in it.

```bash
# Documents
ls docs/pm-prd/PRD.md 2>/dev/null              && echo "PRD: YES"     || echo "PRD: NO"
ls docs/dev-specs/overview.md 2>/dev/null && echo "SPECS: YES" || echo "SPECS: NO"
ls docs/qa-test/VALIDATION-*.md 2>/dev/null | sort | tail -1   && echo "VALIDATION: YES"    || echo "VALIDATION: NO"

# Code
ls src/main.c 2>/dev/null    && echo "CODE: YES"    || echo "CODE: NO"
ls src/modules/ 2>/dev/null  && echo "MODULES: YES" || echo "MODULES: NO"

# Spec version in code
grep "SPECS_VERSION" src/main.c 2>/dev/null || echo "SPECS_VERSION: not set"

# Recent activity
git log --oneline -5
```

Present the project status dashboard to the user:

```
═══════════════════════════════════════════════
  PROJECT STATUS: <project name>
═══════════════════════════════════════════════
  Phase 1 — PRD        [ YES / NO / STALE ]
  Phase 2 — Specs      [ YES / NO / STALE ]
  Phase 3 — Code       [ YES / NO / STALE ]
  Phase 4 — V&V          [ YES / NO ]
═══════════════════════════════════════════════
  → Recommended next step: <phase + reason>
```

**Staleness check:**
- PRD is **stale** if there are code commits newer than the latest PRD Changelog entry.
- Specs are **stale** if the `PRD Version` field in `overview.md` is older than the latest PRD Changelog entry.
- Code is **stale** if `SPECS_VERSION` in `src/main.c` is older than the latest `overview.md` Changelog entry.

Ask: *"Where would you like to start?"* — or proceed with the recommended step.

---

## Phase 1 — Product Definition  `skill: chsh-sk-ncs-1-prd`

**Enter Phase 1 when:**
- No PRD exists yet (new project or undocumented existing project)
- A feature is being added or changed
- Code changed but PRD was not updated

**Run:** Load skill **chsh-sk-ncs-1-prd** and follow its workflow.

**Output:** `docs/pm-prd/PRD.md` with new Changelog entry

**On completion**, ask:
> "PRD is updated. Proceed to **Phase 2 — Specs** with chsh-sk-ncs-2-spec? (yes / stop)"

---

## Phase 2 — Technical Design  `skill: chsh-sk-ncs-2-spec`

**Enter Phase 2 when:**
- PRD exists and was just updated (or is newer than current specs)
- No engineering specs exist yet

**Run:** Load skill **chsh-sk-ncs-2-spec** and follow its workflow.

**Outputs:**
- `docs/dev-specs/overview.md` — spec index, PRD-to-spec mapping
- `docs/dev-specs/architecture.md` — module map, Zbus channels, memory budget
- `docs/dev-specs/<module>.md` — one per feature module

Each spec's `PRD Version` field must match the latest PRD Changelog timestamp.

**On completion**, ask:
> "Specs are ready. Proceed to **Phase 3 — Implementation** with chsh-sk-ncs-3.1-coding? (yes / stop)"

---

## Phase 3 — Implementation

| Sub-phase | Skill | Enter when |
|-----------|-------|------------|
| **3.1 Coding** | `chsh-sk-ncs-3.1-coding` | Specs approved, code needs writing or updating |
| **3.2 Debug** | `chsh-sk-ncs-3.2-debug` | Runtime failure, crash, WiFi issue, boot hang |
| **3.3 Memopt** | `chsh-sk-ncs-3.3-memopt` | RAM/Flash overflow, stack corruption |

**⛔ Phase 3 guard — check before touching `src/`:**
Before writing any code, verify that all agreed design changes are captured in docs.
If the conversation contains discussed-but-undocumented changes:
- Requirements changed → route to **Phase 1** first (update PRD)
- Only spec/implementation details changed → route to **Phase 2** first (update specs)
Never touch `src/` until PRD and specs are current and approved.
A user saying "start implementation" does **not** bypass this guard.

**Enter Phase 3 when:**
- Specs exist and are approved
- Specs were updated and code needs to catch up
- A bug fix or runtime issue is found during testing

**Default flow:** Start with **3.1 Coding** → if runtime issues arise → **3.2 Debug** → if
memory budget exceeded → **3.3 Memopt** → back to 3.1. Iterate until clean build + UART
log evidence of correct behavior.

**On completion**, ask:
> "Implementation done. Proceed to **Phase 4 — V&V** with chsh-sk-ncs-4.1-verification? (yes / stop)"

---

## Phase 4 — Verification & Validation (V&V)  `chsh-sk-ncs-4.1-verification` · `chsh-sk-ncs-4.2-validation`

**Enter Phase 4 when:**
- Implementation is complete or updated
- Before any release or demo
- After any merge to main

**Run:** Load skill **chsh-sk-ncs-4.1-verification** and follow its workflow.

| Sub-phase | Document | Hardware? |
|-----------|----------|-----------|
| 4.1 Verification (always) | `docs/qa-test/VERIFICATION-YYYY-MM-DD-HH-MM.md` | No |
| 4.2 Validation (always) | `docs/qa-test/VALIDATION-YYYY-MM-DD-HH-MM.md` | Yes |

**Feedback routing after Phase 4:**

| Finding | Route back to |
|---------|---------------|
| Security finding or P0 code issue | Phase 3 — fix code |
| Spec gap / undocumented behaviour | Phase 2 — update spec, then Phase 3 |
| New requirement found | Phase 1 — add to PRD, then Phase 2, then Phase 3 |
| All P0 checks pass (4.1) + all P0 TCs pass (4.2) | ✅ Ready for release |

---

## Document Conventions

### Document Information header

Every `PRD.md` and engineering spec opens with a **Document Information** table whose
fields come **verbatim from the matching template** (`PRD_TEMPLATE.md`, `OVERVIEW_TEMPLATE.md`,
`ARCH_TEMPLATE.md`, `MODULE_TEMPLATE.md`) and stay identical from creation through every
maintenance edit — no ad-hoc variants like `Latest Version`.

- **`Version`** = the document's **own** latest edit timestamp (the current time, = its
  newest Changelog row). Bump it on every edit.
- **`PRD Version`** (specs only) = the PRD Changelog timestamp the spec is synced to.
- `Version` and `PRD Version` are different timestamps and must **not** be set equal —
  if they match, `Version` was filled in wrong.

### Living documents — Changelog table

`PRD.md` and all engineering specs are **living documents** with a single canonical filename.
Changes are tracked in a built-in Changelog table:

```markdown
## Changelog

| Version          | Summary of changes          |
|------------------|-----------------------------|
| 2026-04-09-10-00 | Initial draft               |
| 2026-04-15-14-30 | Added P2P mode requirements |
```

- Version is a timestamp `YYYY-MM-DD-HH-MM` — time included so same-day edits are distinguishable.
- Never delete rows — append-only.
- Git provides the full diff; the Changelog is the human-readable log.

### Audit snapshots — dated filenames

QA and Test reports are **point-in-time snapshots**. Each run creates a new dated file:

```
docs/qa-test/VALIDATION-2026-04-09-14-30.md
```

---

## Document Ownership

| Document | Location | Owned by | Answers |
|----------|----------|----------|---------|
| `PRD.md` | `docs/pm-prd/` | Product Manager | What & why — features, behaviour, success metrics |
| `overview.md` | `docs/dev-specs/` | Developer | Spec index, PRD-to-spec map, design decisions |
| `architecture.md` | `docs/dev-specs/` | Developer | System design, module map, memory budget |
| `<module>.md` | `docs/dev-specs/` | Developer | State machines, Kconfig, APIs |
| `VALIDATION-*.md` | `docs/qa-test/` | Tester / PM + Reviewer | PRD acceptance criteria pass/fail, UART evidence, code quality score |

**Division of responsibility:**
- PRD: behaviour in user terms — no Kconfig flags, no memory numbers.
- Specs: translate PRD into implementation detail — Kconfig, state machines, APIs.
- Developer needs both: PRD for "what", specs for "how".

---

## Quick-Entry Rules

| Situation | Start at |
|-----------|---------|
| Brand new project | Phase 1 |
| Existing code, no docs | Phase 1 (Mode A with code scan) |
| PRD done, no specs | Phase 2 |
| Specs done, no code | Phase 3 |
| Code done, need validation | Phase 4 |
| Small bug fix only | Phase 3 (Mode C) |
| Feature request | Phase 1 (Mode B) |
| Post-merge validation | Phase 4 |
| PRD stale after code changes | Phase 1 (Mode D) |
| Specs stale after PRD change | Phase 2 (Mode B) |
| Architecture change | Phase 1 + 2 |
| Before a demo or release | Phase 4 (V&V) |

---

## Skill Reference

| Skill | Invoke when | Output |
|-------|-------------|--------|
| `chsh-sk-ncs-0-workflow` | **Starting any project work** | Status dashboard + phase guidance |
| `chsh-sk-ncs-1-prd` | Defining or updating product requirements | `docs/pm-prd/PRD.md` |
| `chsh-sk-ncs-2-spec` | Translating PRD to engineering specs | `docs/dev-specs/*.md` |
| `chsh-sk-ncs-3.1-coding` | Implementing code from specs | `src/`, `prj.conf`, passing build |
| `chsh-sk-ncs-3.2-debug` | Debugging firmware failures, UART log analysis | Root cause identified and fixed |
| `chsh-sk-ncs-4.1-verification` | Phase 4.1 Verification (code review, build, docs audit — no hardware) | `docs/qa-test/VERIFICATION-*.md` |
| `chsh-sk-ncs-4.2-validation` | Phase 4.2 Validation (hardware tests via EEDP against PRD acceptance criteria) | `docs/qa-test/VALIDATION-*.md` |
| `chsh-sk-git-commit` | Preparing git commits | Clean, logical commit history |
| `chsh-sk-git-release` | Tagging a release, publishing firmware, downloading and flashing pre-built `.hex` | GitHub release with verified artifact |
| `chsh-sk-ncs-migrate` | Upgrading the project to a newer NCS version (single hop or multi-hop) | Migrated app, clean build, verified on hardware |
| `chsh-sk-ncs-3.3-memopt` | Diagnosing memory usage | Heap / stack recommendations |

## Gotchas
- TODO: add one entry per real observed failure or routing false-positive

## Self-Update Policy

At the **end of each conversation**, review what was discovered and check whether any facts in this skill are new, corrected, or outdated (e.g. new phase transitions, skill routing changes, project lifecycle patterns).

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
