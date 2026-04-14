---
name: chsh-ncs-workflow
description: Orchestrates the full NCS project lifecycle. Detects where a project stands, guides through each phase (PRD → Specs → Code → QA+Test), and routes feedback back to the right phase. Use as the single entry point for any NCS project work.
---

# chsh-ncs-workflow — NCS Project Lifecycle Orchestrator

Single entry point for any NCS project work. Scans the project state, presents a
status dashboard, and guides you through each phase — invoking the right skill at each step.

---

## The Four-Phase Cycle

```
┌────────────────────────────────────────────────────────────────┐
│  PHASE 1 — PRODUCT DEFINITION         skill: chsh-pm-prd       │
│                                                                 │
│  Product Manager defines device requirements:                  │
│  • What should the device do, for whom, and why?               │
│  • Which Wi-Fi modes, UI behaviours, connectivity features?    │
│  • Success metrics and release criteria                        │
│                                                                 │
│  Input:  stakeholder ideas, user feedback, bug reports         │
│  Output: docs/PRD.md  (Changelog updated)             │
└───────────────────────────┬────────────────────────────────────┘
                            │ PRD approved → triggers Phase 2
                            ▼
┌────────────────────────────────────────────────────────────────┐
│  PHASE 2 — TECHNICAL DESIGN           skill: chsh-dev-spec     │
│                                                                 │
│  Developer Engineer translates PRD into technical specs:       │
│  • Architecture choice (SMF+Zbus or multi-threaded)            │
│  • docs/specs/overview.md                          │
│  • docs/specs/architecture.md                      │
│  • docs/specs/<module>.md (one per feature)        │
│  • docs/specs/config.yaml (optional project context)           │
│                                                                 │
│  Input:  docs/PRD.md                                   │
│  Output: docs/specs/*.md (PRD Version field set)   │
└───────────────────────────┬────────────────────────────────────┘
                            │ Specs approved → triggers Phase 3
                            ▼
┌────────────────────────────────────────────────────────────────┐
│  PHASE 3 — IMPLEMENTATION             skill: chsh-dev-project  │
│                                                                 │
│  Developer Engineer implements code from specs:                │
│  • src/modules/<name>/ per module                              │
│  • prj.conf, CMakeLists.txt, Kconfig wired up                  │
│  • Build verified: west build -p -b <board>                    │
│  • SPECS_VERSION in main.c matches overview.md Changelog       │
│                                                                 │
│  Input:  docs/specs/*.md                           │
│  Output: code, passing build, UART log evidence                │
└───────────────────────────┬────────────────────────────────────┘
                            │ Implementation done → triggers Phase 4
                            ▼
┌────────────────────────────────────────────────────────────────┐
│  PHASE 4 — QA & FUNCTIONAL TEST       skill: chsh-qa-test    │
│                                                                 │
│  Test Report (always):                                         │
│  • One TC per PRD acceptance criterion, UART evidence          │
│  • Output: docs/TEST-YYYY-MM-DD-HH-MM.md                   │
│                                                                 │
│  QA Report (release/demo only):                                │
│  • Code quality: structure, config, standards, security        │
│  • Output: docs/QA-YYYY-MM-DD-HH-MM.md (0–100 score)       │
│                                                                 │
│  Issues loop back:                                             │
│  • P0 code bug → Phase 3   • Spec gap → Phase 2               │
│  • New requirement → Phase 1                                   │
└────────────────────────────────────────────────────────────────┘
```

---

## Step 0 — Scan Project State

Run this before anything else. Provide the project path if not already in it.

```bash
# Documents
ls docs/PRD.md 2>/dev/null              && echo "PRD: YES"     || echo "PRD: NO"
ls docs/specs/overview.md 2>/dev/null && echo "SPECS: YES" || echo "SPECS: NO"
ls docs/QA-*.md 2>/dev/null | sort | tail -1   && echo "QA: YES"    || echo "QA: NO"
ls docs/TEST-*.md 2>/dev/null | sort | tail -1  && echo "TEST: YES"  || echo "TEST: NO"

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
  Phase 4 — QA         [ YES / NO ]
           — Test      [ YES / NO ]
═══════════════════════════════════════════════
  → Recommended next step: <phase + reason>
```

**Staleness check:**
- PRD is **stale** if there are code commits newer than the latest PRD Changelog entry.
- Specs are **stale** if the `PRD Version` field in `overview.md` is older than the latest PRD Changelog entry.
- Code is **stale** if `SPECS_VERSION` in `src/main.c` is older than the latest `overview.md` Changelog entry.

Ask: *"Where would you like to start?"* — or proceed with the recommended step.

---

## Phase 1 — Product Definition  `skill: chsh-pm-prd`

**Enter Phase 1 when:**
- No PRD exists yet (new project or undocumented existing project)
- A feature is being added or changed
- Code changed but PRD was not updated

**Run:** Load skill **chsh-pm-prd** and follow its workflow.

**Output:** `docs/PRD.md` with new Changelog entry

**On completion**, ask:
> "PRD is updated. Proceed to **Phase 2 — Specs** with chsh-dev-spec? (yes / stop)"

---

## Phase 2 — Technical Design  `skill: chsh-dev-spec`

**Enter Phase 2 when:**
- PRD exists and was just updated (or is newer than current specs)
- No engineering specs exist yet

**Run:** Load skill **chsh-dev-spec** and follow its workflow.

**Outputs:**
- `docs/specs/overview.md` — spec index, PRD-to-spec mapping
- `docs/specs/architecture.md` — module map, Zbus channels, memory budget
- `docs/specs/<module>.md` — one per feature module

Each spec's `PRD Version` field must match the latest PRD Changelog timestamp.

**On completion**, ask:
> "Specs are ready. Proceed to **Phase 3 — Implementation** with chsh-dev-project? (yes / stop)"

---

## Phase 3 — Implementation  `skill: chsh-dev-project`

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
- A bug fix is needed (no spec change)

**Run:** Load skill **chsh-dev-project** and follow its workflow.

**Outputs:**
- `src/modules/<name>/` — one per spec module
- `prj.conf`, `CMakeLists.txt`, `Kconfig` wired up
- `src/main.c` with `SPECS_VERSION` matching latest `overview.md` Changelog entry
- Passing `west build`

**On completion**, ask:
> "Implementation done. Proceed to **Phase 4 — QA & Test** with chsh-qa-test? (yes / stop)"

---

## Phase 4 — QA & Test  `skill: chsh-qa-test`

**Enter Phase 4 when:**
- Implementation is complete or updated
- Before any release or demo
- After any merge to main

**Run:** Load skill **chsh-qa-test** and follow its workflow.

| Document | When | Hardware? |
|----------|------|-----------|
| `TEST-YYYY-MM-DD-HH-MM.md` | **Always** — every cycle | Yes |
| `QA-YYYY-MM-DD-HH-MM.md` | **Release / demo only** | No |

**Feedback routing after Phase 4:**

| Finding | Route back to |
|---------|--------------|
| P0 critical code issue | Phase 3 — fix code |
| Spec gap / undocumented behaviour | Phase 2 — update spec, then Phase 3 |
| New requirement found | Phase 1 — add to PRD, then Phase 2, then Phase 3 |
| All P0 tests pass | ✅ Ready for release |

---

## Document Conventions

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
docs/TEST-2026-04-09-14-30.md
docs/QA-2026-04-09-14-30.md
```

---

## Document Ownership

| Document | Location | Owned by | Answers |
|----------|----------|----------|---------|
| `PRD.md` | `docs/` | Product Manager | What & why — features, behaviour, success metrics |
| `overview.md` | `docs/specs/` | Developer | Spec index, PRD-to-spec map, design decisions |
| `architecture.md` | `docs/specs/` | Developer | System design, module map, memory budget |
| `<module>.md` | `docs/specs/` | Developer | State machines, Kconfig, APIs |
| `TEST-*.md` | `docs/` | Tester / PM | PRD acceptance criteria pass/fail, UART evidence |
| `QA-*.md` | `docs/` | Reviewer / PM | Code quality score (0–100), issues |

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
| Before a demo or release | Phase 4 (both QA + Test) |

---

## Skill Reference

| Skill | Invoke when | Output |
|-------|-------------|--------|
| `chsh-ncs-workflow` | **Starting any project work** | Status dashboard + phase guidance |
| `chsh-pm-prd` | Defining or updating product requirements | `docs/PRD.md` |
| `chsh-dev-spec` | Translating PRD to engineering specs | `docs/specs/*.md` |
| `chsh-dev-project` | Implementing code from specs | `src/`, `prj.conf`, passing build |
| `chsh-qa-test` | Validating a build against PRD and specs | `TEST-*.md` + `QA-*.md` |
| `chsh-git-commit` | Preparing git commits | Clean, logical commit history |
| `chsh-dev-mem-opt` | Diagnosing memory usage | Heap / stack recommendations |

## Self-Update Policy

At the **end of each conversation**, review what was discovered and check whether any facts in this skill are new, corrected, or outdated (e.g. new phase transitions, skill routing changes, project lifecycle patterns).

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
