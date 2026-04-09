# Skills Workflow — NCS Project Lifecycle

This document describes the end-to-end workflow connecting the four project skills.
It is a coordination reference; each skill has its own detailed instructions.

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
│  Output: docs/product/PRD.md  (with Revision History table)    │
└───────────────────────────┬────────────────────────────────────┘
                            │ PRD approved → triggers Phase 2
                            ▼
┌────────────────────────────────────────────────────────────────┐
│  PHASE 2 — TECHNICAL DESIGN           skill: chsh-dev-spec   │
│                                                                 │
│  Developer Engineer translates PRD into technical specs:       │
│  • Architecture choice (SMF+Zbus or multi-threaded)            │
│  • docs/engineering/specs/architecture.md                      │
│  • docs/engineering/specs/<module>.md (one per feature)        │
│  • docs/engineering/config.yaml (project context)              │
│  Each spec file has its own Revision History table.            │
│                                                                 │
│  Input:  docs/product/PRD.md                                   │
│  Output: docs/engineering/specs/*.md (approved by PM)          │
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
│  • Spec updated if implementation differs from design          │
│                                                                 │
│  Input:  docs/engineering/specs/*.md                           │
│  Output: code, passing build, UART log evidence                │
└───────────────────────────┬────────────────────────────────────┘
                            │ Implementation done → triggers Phase 4
                            ▼
┌────────────────────────────────────────────────────────────────┐
│  PHASE 4 — QA REVIEW                  skill: chsh-pm-review    │
│                                                                 │
│  Reviewer validates implementation against both PRD and specs: │
│  • Does code meet PRD acceptance criteria?                     │
│  • Does code match the engineering specs?                      │
│  • Automated check: chsh-pm-review/check_project.sh            │
│  • Manual review: chsh-pm-review/CHECKLIST.md                  │
│                                                                 │
│  Input:  code + docs/product/PRD.md + docs/engineering/        │
│  Output: docs/qa/QA-YYYY-MM-DD.md (scored, with issues)        │
│                                                                 │
│  Issues loop back:                                             │
│  • P0 critical issue → fix immediately (Phase 3)              │
│  • New requirement → update PRD (Phase 1)                      │
│  • Design change → update spec (Phase 2)                       │
└────────────────────────────────────────────────────────────────┘
```

---

## Document Conventions

### Living documents — Revision History table

`PRD.md` and all engineering specs are **living documents**. They have a single canonical
filename and track change history in a built-in table:

```markdown
## Changelog

| Version          | Summary of changes              |
|------------------|---------------------------------|
| 2026-04-09-10-00 | Initial draft                   |
| 2026-04-15-14-30 | Added P2P mode requirements     |
| 2026-04-20-09-15 | Revised success metrics         |
```

Rules:
- Version is a timestamp `YYYY-MM-DD-HH-MM` — includes time so multiple edits on the same day are distinguishable.
- Never delete rows — the table is append-only.
- Git provides the actual diff; the Changelog is the human-readable log.

### Audit snapshots — dated filenames

QA reports are **point-in-time audit snapshots**, not living documents. They use dated
filenames so you can track quality over time:

```
docs/qa/QA-2026-04-09.md
docs/qa/QA-2026-05-01.md
```

---

## Document Ownership

| Document | Location | Owned by | Answers |
|----------|----------|----------|---------|
| `PRD.md` | `docs/product/` | Product Manager | What & why — features, behaviour, success metrics |
| `architecture.md` | `docs/engineering/specs/` | Developer Engineer | System design, module map, memory budget |
| `<module>.md` | `docs/engineering/specs/` | Developer Engineer | How — state machines, Kconfig, APIs |
| `config.yaml` | `docs/engineering/` | Developer Engineer | Project context fed to chsh-dev-project |
| `QA-YYYY-MM-DD.md` | `docs/qa/` | Reviewer / PM | Scores, issues, pass/fail verdict |

**Division of responsibility:**
- PRD describes behaviour in user terms — no Kconfig flags, no memory numbers.
- Engineering specs translate PRD into implementation detail — Kconfig, state machines, APIs.
- The developer needs both: PRD for the "what", specs for the "how".

---

## Phase Trigger Rules

| Situation | Start at |
|-----------|---------|
| New product or major new feature | Phase 1 — update PRD first |
| Small code fix with no new user-visible behaviour | Phase 3 — fix code, update spec if needed |
| Architecture change | Phase 1 + 2 — update PRD section then spec |
| QA critical issue (P0) | Phase 3 — fix code |
| QA finding reveals missing requirement | Phase 1 — add to PRD, then Phase 2 for spec |
| Before a demo or release | Phase 4 — run QA review |
| After any merge to main | Phase 4 — refresh QA |

---

## Skill Quick Reference

| Skill | Invoke when | Output |
|-------|-------------|--------|
| `chsh-pm-prd` | Defining or updating product requirements | `docs/product/PRD.md` (new revision row) |
| `chsh-dev-spec` | Translating PRD to engineering specs | `docs/engineering/specs/*.md` (new revision rows) |
| `chsh-dev-project` | Implementing code from specs | `src/modules/`, `prj.conf`, passing build |
| `chsh-pm-review` | Validating a build against PRD and specs | `docs/qa/QA-YYYY-MM-DD.md` |
| `chsh-dev-commit` | Preparing git commits | Clean, logical commit history |
| `chsh-dev-mem-opt` | Diagnosing memory usage | Heap / stack recommendations |
