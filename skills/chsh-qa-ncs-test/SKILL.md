---
name: chsh-qa-ncs-test
description: Phase 4 of the NCS project lifecycle. Generates a Test Report (PRD acceptance criteria pass/fail, hardware required, always) and optionally a QA Report (code quality 0-100, no hardware, release/demo only). Use when validating a build against PRD and specs.
---

# chsh-qa-ncs-test — QA & Functional Test

Phase 4 of the NCS project lifecycle. Validates that the implementation is both
well-built (QA Report) and behaves as the PRD requires (Test Report).

> **One output file, two sections, different cadence:**
>
> | Section | Question | Hardware? | When required |
> |---------|----------|-----------|---------------|
> | Part A — Functional Test | "Does it behave as the PRD says?" | Yes — run on board | **Always** — after every implementation cycle |
> | Part B — Code Quality | "Is the code well-built?" | No — AI reviews code | **Release/demo only** — optional otherwise |
>
> Both sections go into a single dated file: `docs/qa-test/QA-YYYY-MM-DD-HH-MM.md`
> For routine cycles, fill Part A and mark Part B sections `[SKIP]`.

---

## Step 0 — Check Inputs

Before starting, verify:

```bash
cat docs/pm-prd/PRD.md                          # acceptance criteria source
ls docs/dev-specs/overview.md                # spec version reference
grep "SPECS_VERSION" src/main.c          # firmware spec version
west build --build-dir build/ 2>&1 | tail -5   # confirm build is clean
```

Note the PRD Changelog version and specs overview Changelog version — both go into the
Document Information of each report.

---

## Part A — Test Report (Functional Testing) — *Always Required*

**Hardware required. Run firmware on the target board.**
**Run after every implementation cycle — this is the proof that the PRD is satisfied.**

### A1. Flash the firmware

```bash
west flash --build-dir build/
```

Open a serial monitor and capture the boot log.

### A2. Map PRD acceptance criteria to test cases

Read `docs/pm-prd/PRD.md`. For every FR and NFR:
- Extract all acceptance criteria lines
- Assign each a TC ID: `TC-<FR number>-<sequence>` (e.g. `TC-001-01`)
- List them in the Test Report before starting any testing

### A3. Execute each test case

For each TC:
1. Follow the acceptance criterion literally
2. Record the result: ✅ Pass / ❌ Fail / ⚠️ Partial / ⬜ Not Run
3. Paste the relevant UART log lines as evidence

For failures, record expected vs actual behaviour and suggest routing:
- Code bug → Phase 3
- Spec gap → Phase 2
- Missing PRD requirement → Phase 1

### A4. Generate `docs/qa-test/QA-YYYY-MM-DD-HH-MM.md`

Use `QA_TEMPLATE.md` as the base. Fill in:
- Document Information (PRD Version, Specs Version, firmware build timestamp)
- Changelog entry for this run
- Part A: Summary table (total / passed / failed / blocked)
- Part A: Results grouped by FR
- Part A: NFR measurements with targets vs actuals
- Part A: Failed test detail section
- Part A: Full UART boot log and performance metrics
- Part B: Mark all sections `[SKIP — routine cycle]`

**Test Report quality criteria**:
- Every TC maps to an exact acceptance criterion from the PRD — no invented test cases
- Evidence is specific: paste the relevant UART log lines, not a generic "it worked"
- NFR rows always have a measured number (not just ✅) — e.g. "29 s" not just "pass"
- Failed TC detail includes expected vs actual behaviour and a routing recommendation
- The executive summary (total/pass/fail + one-sentence verdict) appears at the top before results

---

## Part B — QA Report (Code Quality) — *Release/Demo Only*

**No hardware required. AI reviews code and documents.**
**Skip for routine development cycles. Run before any release or demo.**

### B1. Review code systematically

Score each category:
- Project structure (required files, directory layout)
- Core files quality (CMakeLists.txt, Kconfig, prj.conf, main.c)
- Configuration (Wi-Fi config, build config)
- Code quality (coding standards, error handling, memory, thread safety)
- Documentation (README, code comments) — see README quality criteria below
- Wi-Fi implementation (mode-specific correctness, event handling)
- Security (credentials, network security, debug features)
- Build verification (clean build, no warnings, binary size)

**README quality criteria** (applied when scoring Documentation):
- **Cognitive funnel**: starts broad (what/why), goes narrow (how). An evaluator reaches a working device via Quick Start without reading past that section.
- **Dual audience**: evaluator path (pre-built hex, ≤5 min) is clearly separated from developer path (build from source).
- **Completeness**: one-liner description, screenshot or demo image, features list, project structure tree, Quick Start, build/flash commands, buttons & LEDs table, documentation links.
- **Length**: 3,000–7,000 characters (excluding code blocks). Under 1,500 chars is too sparse; over 10,000 should be split into separate docs.
- **No emojis in section headings** — professional tone consistent with Nordic SDK documentation style.
- **Tested**: Quick Start steps verified by someone other than the developer.

### B2. Fill in Part B of the report

Open the `docs/qa-test/QA-YYYY-MM-DD-HH-MM.md` already created in Part A.
Replace the `[SKIP]` placeholders in Part B with the actual review content.
Record the overall score (0–100) and verdict (PASS / PASS WITH ISSUES / REWORK / FAIL).

---

## Part C — Review Summary & Routing

After generating the report(s), present a combined summary:

```
═══════════════════════════════════════════════
  PHASE 4 REVIEW SUMMARY
═══════════════════════════════════════════════
  Test Result: __/__TCs passed  [PASS / FAIL]
  QA Score:    ___/100  [PASS / REWORK / FAIL]  ← if run
═══════════════════════════════════════════════
  Critical issues requiring action:
  • [P0] <issue> → route to Phase <N>
═══════════════════════════════════════════════
```

Then ask:
> "Review complete. Route P0 issues back to the appropriate phase?
> Reply **yes** to continue with chsh-dev-ncs-workflow, or **no** to stop here."

---

## Scoring (QA Report)

| Score | Grade |
|-------|-------|
| 90–100 | Excellent — ready for release |
| 80–89 | Good — minor improvements needed |
| 70–79 | Satisfactory — notable issues |
| 60–69 | Needs work — significant problems |
| < 60 | Fail — major rework required |

## Routing Rules

| Finding | Priority | Route to |
|---------|----------|---------|
| Build fails | P0 | Phase 3 |
| P0 test case fails | P0 | Phase 3 (or Phase 2 if spec gap) |
| Security issue (hardcoded credentials) | P0 | Phase 3 |
| QA score < 70 | P1 | Phase 3 |
| Spec does not cover a test scenario | P1 | Phase 2 |
| PRD acceptance criterion is ambiguous | P1 | Phase 1 |
| P1/P2 test failures only | P2 | Phase 3 (next iteration) |

---

## Files in This Skill

| File | Purpose |
|------|---------|
| `QA_TEMPLATE.md` | Combined Phase 4 report template — Part A (functional test) + Part B (code quality) |

## Document Conventions

- **Phase 4 Report**: `docs/qa-test/QA-YYYY-MM-DD-HH-MM.md` — dated snapshot containing Part A always, Part B when running a release/demo review
- Keep all runs for history
- Both Parts include PRD Version and Specs Version in Document Information for traceability

## Related Skills

- `chsh-dev-ncs-workflow` — full lifecycle orchestrator; routes back here after Phase 3
- `chsh-pm-ncs-prd` — update `docs/pm-prd/PRD.md` if requirements need changing
- `chsh-dev-spec` — update `docs/dev-specs/` if design gaps are found
- `chsh-dev-ncs-project` — fix code for P0 issues

## Self-Update Policy

At the **end of each conversation**, review what was discovered and check whether any facts in this skill are new, corrected, or outdated (e.g. new test report patterns, acceptance criteria templates, QA scoring rubrics).

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
