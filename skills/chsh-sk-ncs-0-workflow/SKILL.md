---
name: chsh-sk-ncs-0-workflow
description: Use when you need a project status dashboard or when a drift is detected (code ahead of specs, specs ahead of PRD). Scans PRD/Specs/Code version timestamps, identifies which artifact is newest, and routes to the right skill. Not needed for normal forward-flow work — each phase skill handles its own handoff.
---

# chsh-sk-ncs-0-workflow — NCS Project Lifecycle Orchestrator

Single entry point for any NCS project work. Scans the project state, presents a
status dashboard, and guides you through each phase — invoking the right skill at each step.

---

## The Four-Phase Cycle

```
┌──────────────────────────────────────────────────────────────────────────────┐
│  PHASE 1 — PRODUCT DEFINITION         skill: chsh-sk-ncs-1-prd               │
│                                                                              │
│  Product Manager defines device requirements:                                │
│  • What should the device do, for whom, and why?                             │
│  • Which Wi-Fi modes, UI behaviours, connectivity features?                  │
│  • Success metrics and release criteria                                      │
│                                                                              │
│  Input:  stakeholder ideas, user feedback, bug reports                       │
│  Output: docs/pm-prd/PRD.md  (Changelog updated)                             │
└───────────────────────────┬──────────────────────────────────────────────────┘
                            │ PRD approved → triggers Phase 2
                            ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│  PHASE 2 — TECHNICAL DESIGN           skill: chsh-sk-ncs-2-spec              │
│                                                                              │
│  Developer Engineer translates PRD into technical specs:                     │
│  • Architecture choice (SMF+Zbus or multi-threaded)                          │
│  • docs/dev-specs/overview.md                                                │
│  • docs/dev-specs/architecture.md                                            │
│  • docs/dev-specs/<module>.md (one per feature)                              │
│  • docs/dev-specs/config.yaml (optional project context)                     │
│                                                                              │
│  Input:  docs/pm-prd/PRD.md                                                  │
│  Output: docs/dev-specs/*.md (PRD Version field set)                         │
└───────────────────────────┬──────────────────────────────────────────────────┘
                            │ Specs approved → triggers Phase 3
                            ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│  PHASE 3 — IMPLEMENTATION   3.1-coding · 3.2-debug · 3.3-memopt              │
│                                                                              │
│  3.1 Coding: src/modules/, prj.conf, CMakeLists.txt, Kconfig                 │
│  3.2 Debug:  fix runtime failures, crashes, WiFi issues                      │
│  3.3 Memopt: RAM/Flash budget, stack overflow, memory usage                  │
│                                                                              │
│  Input:  docs/dev-specs/*.md                                                 │
│  Output: code, clean build, UART log evidence                                │
└───────────────────────────┬──────────────────────────────────────────────────┘
                            │ Implementation done → triggers Phase 4
                            ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│  PHASE 4 — VERIFICATION & VALIDATION (V&V)                                   │
│           4.1: chsh-sk-ncs-4.1-verification · 4.2: chsh-sk-ncs-4.2-validation│
│                                                                              │
│  4.1 Verification (always, no HW): code review, build, docs                  │
│  4.2 Validation via EEDP (HW): GPIO, UART, Saleae, JLink, Router             │
│  • Output: docs/qa-test/ (VERIFICATION + VALIDATION)                         │
│                                                                              │
│  P0 → Phase 3  |  Spec gap → Phase 2  |  New req → Phase 1                   │
└──────────────────────────────────────────────────────────────────────────────┘
```

---

## Step 0 — Scan Project State

Run this before anything else. Provide the project path if not already in it.

```bash
# Existence check
ls docs/pm-prd/PRD.md 2>/dev/null             && echo "PRD: YES"        || echo "PRD: NO"
ls docs/dev-specs/overview.md 2>/dev/null     && echo "SPECS: YES"      || echo "SPECS: NO"
ls src/main.c 2>/dev/null                     && echo "CODE: YES"       || echo "CODE: NO"
ls docs/qa-test/VALIDATION-*.md 2>/dev/null | sort | tail -1 && echo "VALIDATION: YES" || echo "VALIDATION: NO"

# Version extraction (only when all three exist)
PRD_VER=$(grep -m1 '| [0-9]' docs/pm-prd/PRD.md 2>/dev/null | awk -F'|' '{print $2}' | tr -d ' ')
SPEC_VER=$(grep -m1 '| [0-9]' docs/dev-specs/overview.md 2>/dev/null | awk -F'|' '{print $2}' | tr -d ' ')

# Code version = last git commit timestamp that touched src/
# If src/ has uncommitted changes, commit them first (use chsh-sk-ncs-3.4-git-commit)
CODE_VER=$(git log --format="%ci" -- src/ 2>/dev/null | head -1 | awk '{print $1"-"$2}' | sed 's/://g; s/-[0-9]*$//; s/-/\//3; s/-/:/')

echo "PRD_VER:  $PRD_VER"
echo "SPEC_VER: $SPEC_VER"
echo "CODE_VER: $CODE_VER"
```

### Version format

- **PRD version** = newest timestamp row in `docs/pm-prd/PRD.md` Changelog (`YYYY-MM-DD-HH-MM`)
- **Specs version** = newest timestamp row in `docs/dev-specs/overview.md` Changelog (`YYYY-MM-DD-HH-MM`)
- **Code version** = `git log --format="%ci" -- src/ | head -1` converted to `YYYY-MM-DD-HH-MM`
  - ⚠️ If `src/` has **uncommitted changes**, invoke **chsh-sk-ncs-3.4-git-commit** first, then re-read `CODE_VER`.

### Staleness decision table (all phases exist)

Compare the three versions. Whichever timestamp is **newest** is the source of truth;
the others must be brought forward.

| Newest artifact | What drifted | Required sync | Recommended flow |
|-----------------|-------------|---------------|-----------------|
| **PRD** | Specs and/or Code lag | Read PRD Changelog entries newer than SPEC_VER; update specs to match, then update code | **1 → 2 → 3.1 → 4.1** |
| **Specs** | PRD and/or Code lag | Read Specs Changelog entries newer than PRD_VER and CODE_VER; propagate intent back to PRD, then update code | **2 → 1 → 3.1 → 4.1** |
| **Code** | PRD and/or Specs lag | Read `git log --oneline src/` from CODE_VER back to SPEC_VER (and PRD_VER); infer intent from commits; update Specs then PRD | **3.1 → 2 → 1 → 4.1** |
| All equal | Nothing drifted | Skip to V&V | **4.1** |

> **Tie-breaking:** if two artifacts share the same version, treat the *third* as the source of
> truth if it is newer; otherwise treat the tie as "no drift" for that pair.

### Sync procedure by case

**Case A — PRD is newest (flow: 1 → 2 → 3.1 → 4.1)**

1. Read PRD Changelog rows newer than `SPEC_VER`.
2. Load **chsh-sk-ncs-2-spec**: update `overview.md` and affected `<module>.md` files.
   Set `PRD Version` to `PRD_VER`.
3. Load **chsh-sk-ncs-3.1-coding**: implement spec changes in `src/`.
4. Proceed to Phase 4.1.

**Case B — Specs are newest (flow: 2 → 1 → 3.1 → 4.1)**

1. Read Specs Changelog rows newer than `PRD_VER` *and* rows newer than `CODE_VER`.
2. Load **chsh-sk-ncs-1-prd**: surface any new requirements or behaviour changes back into
   `PRD.md` (add Changelog entry, bump PRD version).
3. Load **chsh-sk-ncs-3.1-coding**: implement the spec changes in `src/`.
4. Proceed to Phase 4.1.

**Case C — Code is newest (flow: 3.1 → 2 → 1 → 4.1)**

1. Run: `git log --oneline src/` — read commits from `HEAD` back to the commit whose
   timestamp matches `SPEC_VER` (or the nearest earlier commit).
2. Infer the engineering intent from those commits.
3. Load **chsh-sk-ncs-2-spec**: document the implemented behaviour in specs; bump
   `overview.md` Changelog.
4. Load **chsh-sk-ncs-1-prd**: if the commits introduced user-visible behaviour changes,
   update `PRD.md`; add Changelog entry.
5. Proceed to Phase 4.1.

---

Present the project status dashboard to the user:

```
═══════════════════════════════════════════════════════
  PROJECT STATUS: <project name>
═══════════════════════════════════════════════════════
  Phase 1 — PRD    [ YES / NO ]  version: YYYY-MM-DD-HH-MM
  Phase 2 — Specs  [ YES / NO ]  version: YYYY-MM-DD-HH-MM
  Phase 3 — Code   [ YES / NO ]  version: YYYY-MM-DD-HH-MM
  Phase 4 — V&V    [ YES / NO ]
═══════════════════════════════════════════════════════
  Newest artifact: <PRD | Specs | Code | all equal>
  → Recommended flow: <A | B | C | 4.1 only> — <reason>
═══════════════════════════════════════════════════════
```

Ask: *"Proceed with recommended flow, or choose a different starting point?"*

---

## Phase 1 — Product Definition  `skill: chsh-sk-ncs-1-prd`

**Enter Phase 1 when:**
- No PRD exists yet (new project or undocumented existing project)
- A feature is being added or changed
- Code changed but PRD was not updated

**Run:** Load skill **chsh-sk-ncs-1-prd** and follow its workflow.

**Output:** `docs/pm-prd/PRD.md` with new Changelog entry

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

---

## Phase 4 — Verification & Validation (V&V)  `chsh-sk-ncs-4.1-verification` · `chsh-sk-ncs-4.2-validation`

**Normal trigger:** The `chsh-sk-ncs-3.4-git-commit` / `chsh-ag-git` Step 8 `AskQuestion` offers to run 4.1 automatically after every commit in repos with `docs/qa-test/`. In normal forward-flow you do not need to enter Phase 4 manually.

**Manual entry when:**
- Skipped at commit time and want to run it now
- Before a release or demo
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
| All phases exist — PRD is newest | Step 0 Case A → flow 1→2→3.1→4.1 |
| All phases exist — Specs are newest | Step 0 Case B → flow 2→1→3.1→4.1 |
| All phases exist — Code is newest | Step 0 Case C → flow 3.1→2→1→4.1 |
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
| `chsh-sk-ncs-3.4-git-commit` | Preparing git commits | Clean, logical commit history |
| `chsh-sk-ncs-3.5-release` | Tagging a release, watching CI, publishing firmware to GitHub | GitHub release with published artifact |
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
