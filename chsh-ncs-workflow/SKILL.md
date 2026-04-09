---
name: chsh-ncs-workflow
description: Orchestrates the full NCS project lifecycle. Detects where a project stands, guides through each phase (PRD → Specs → Code → QA+Test), and routes feedback back to the right phase. Use as the single entry point for any NCS project work.
---

# chsh-ncs-workflow — NCS Project Lifecycle Orchestrator

This skill runs the full lifecycle defined in `NCS-Project-WORKFLOW.md`.
It detects the current state of a project, tells you exactly where you are,
and guides you through each phase — invoking the right skill at each step.

Full workflow reference: [`~/.claude/skills/NCS-Project-WORKFLOW.md`](../NCS-Project-WORKFLOW.md)

---

## Step 0 — Scan Project State

Run this before anything else. Provide the project path if not already in it.

```bash
# Documents
ls docs/product/PRD.md 2>/dev/null         && echo "PRD: YES"     || echo "PRD: NO"
ls docs/engineering/specs/overview.md 2>/dev/null && echo "SPECS: YES"   || echo "SPECS: NO"
ls docs/qa/QA-*.md 2>/dev/null | sort | tail -1    && echo "QA: YES"     || echo "QA: NO"
ls docs/qa/TEST-*.md 2>/dev/null | sort | tail -1  && echo "TEST: YES"   || echo "TEST: NO"

# Code
ls src/main.c 2>/dev/null           && echo "CODE: YES"    || echo "CODE: NO"
ls src/modules/ 2>/dev/null         && echo "MODULES: YES" || echo "MODULES: NO"

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
- Specs are **stale** if the `PRD Version` field in `docs/engineering/specs/overview.md` is older than the latest PRD Changelog entry.
- Code is **stale** if `SPECS_VERSION` in `src/main.c` is older than the latest entry in `docs/engineering/specs/overview.md`'s Changelog.

Ask the user: *"Where would you like to start?"* — or proceed with the recommended step.

---

## Phase 1 — Product Definition  `skill: chsh-pm-prd`

**Enter Phase 1 when:**
- No PRD exists yet (new project or undocumented existing project)
- A feature is being added or changed
- Code changed but PRD was not updated

**Run:**
> Load skill **chsh-pm-prd** and follow its workflow.

**Phase 1 outputs:**
- `docs/product/PRD.md` with new Changelog entry

**On completion**, ask:
> "PRD is updated. Proceed to **Phase 2 — Specs** with chsh-dev-spec? (yes / stop)"

---

## Phase 2 — Technical Design  `skill: chsh-dev-spec`

**Enter Phase 2 when:**
- PRD exists and was just updated (or is newer than current specs)
- No engineering specs exist yet
- User explicitly requests spec work

**Run:**
> Load skill **chsh-dev-spec** and follow its workflow.

**Phase 2 outputs:**
- `docs/engineering/specs/overview.md` — spec index, PRD-to-spec mapping
- `docs/engineering/specs/architecture.md` — module map, Zbus channels, memory budget
- `docs/engineering/specs/<module>.md` — one per feature module
- `docs/engineering/config.yaml` — project context

Each spec's `PRD Version` field must match the latest PRD Changelog timestamp.

**On completion**, ask:
> "Specs are ready. Proceed to **Phase 3 — Implementation** with chsh-dev-project? (yes / stop)"

---

## Phase 3 — Implementation  `skill: chsh-dev-project`

**Enter Phase 3 when:**
- Specs exist and are approved
- Specs were updated and code needs to catch up
- A bug fix is needed (no spec change)

**Run:**
> Load skill **chsh-dev-project** and follow its workflow.

**Phase 3 outputs:**
- `src/modules/<name>/` — one per spec module
- `prj.conf`, `CMakeLists.txt`, `Kconfig` wired up
- `src/main.c` with `SPECS_VERSION` matching latest `overview.md` Changelog entry
- Passing `west build`

**On completion**, ask:
> "Implementation done. Proceed to **Phase 4 — QA & Test** with chsh-pm-review? (yes / stop)"

---

## Phase 4 — QA & Test  `skill: chsh-pm-review`

**Enter Phase 4 when:**
- Implementation is complete or updated
- Before any release or demo
- After any merge to main

**Run:**
> Load skill **chsh-pm-review** and follow its workflow.
>
> | Document | When | Hardware? |
> |----------|------|-----------|
> | `TEST-YYYY-MM-DD-HH-MM.md` | **Always** — every cycle | Yes |
> | `QA-YYYY-MM-DD-HH-MM.md` | **Release / demo only** | No |

**Phase 4 outputs:**
- QA Report with 0–100 score and critical/warning issues
- Test Report with pass/fail per PRD acceptance criterion and UART evidence

**Feedback routing after Phase 4:**

| Finding | Route back to |
|---------|--------------|
| P0 critical code issue | Phase 3 — fix code |
| Spec gap / undocumented behaviour | Phase 2 — update spec, then Phase 3 |
| New requirement found | Phase 1 — add to PRD, then Phase 2, then Phase 3 |
| All P0 tests pass | ✅ Ready for release |

Ask after routing:
> "Issues identified. Return to **Phase <N>** to address them? (yes / stop)"

---

## Full Cycle Summary

```
┌─────────────────────────────────────────────────────────────┐
│  chsh-ncs-workflow  (this skill — orchestrator)             │
├─────────────────────────────────────────────────────────────┤
│  Phase 1  chsh-pm-prd      →  docs/product/PRD.md           │
│     ↓                                                        │
│  Phase 2  chsh-dev-spec    →  docs/engineering/specs/       │
│     ↓                                                        │
│  Phase 3  chsh-dev-project →  src/  +  passing build        │
│     ↓                                                        │
│  Phase 4  chsh-pm-review   →  docs/qa/QA-*.md               │
│                               docs/qa/TEST-*.md              │
│     ↓ issues found                                           │
│  ↩ loop back to Phase 1 / 2 / 3 as appropriate              │
└─────────────────────────────────────────────────────────────┘
```

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
