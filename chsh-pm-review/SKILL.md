---
name: chsh-pm-review
description: Phase 4 of the NCS project lifecycle. Generates two documents: a QA Report (code quality, 0-100 score, no hardware needed) and a Test Report (PRD acceptance criteria pass/fail, hardware required). Use when validating a build against PRD and specs.
---

# chsh-pm-review — QA & Functional Test

Phase 4 of the NCS project lifecycle. Validates that the implementation is both
well-built (QA Report) and behaves as the PRD requires (Test Report).

> **Two documents, two questions, different cadence:**
>
> | Document | Question | Hardware? | When required | Output |
> |----------|----------|-----------|---------------|--------|
> | Test Report | "Does it behave as the PRD says?" | Yes — run on board | **Always** — after every implementation cycle | `docs/qa/TEST-YYYY-MM-DD-HH-MM.md` |
> | QA Report | "Is the code well-built?" | No — AI reviews code | **Release/demo only** — optional otherwise | `docs/qa/QA-YYYY-MM-DD-HH-MM.md` |

---

## Step 0 — Check Inputs

Before starting, verify:

```bash
cat docs/product/PRD.md                          # acceptance criteria source
ls docs/engineering/specs/overview.md            # spec version reference
grep "SPECS_VERSION" src/main.c                  # firmware spec version
west build --build-dir build/ 2>&1 | tail -5     # confirm build is clean
```

Note the PRD Changelog version and Specs overview Changelog version — both go into the
Document Information of each report.

---

## Part A — QA Report (Code Quality) — *Release/Demo Only*

**No hardware required. AI reviews code and documents.**
**Skip this part for routine development cycles. Run before any release or demo.**

### A1. Run automated check

```bash
~/.claude/skills/chsh-pm-review/check_project.sh /path/to/project
```

### A2. Manual review — follow `CHECKLIST.md`

Score each category systematically.

### A3. Generate `docs/qa/QA-YYYY-MM-DD-HH-MM.md`

Use `QA_TEMPLATE.md` as the base. Fill in all sections:

- Project structure (required files, directory layout)
- Core files quality (CMakeLists.txt, Kconfig, prj.conf, main.c)
- Configuration (Wi-Fi config, build config)
- Code quality (coding standards, error handling, memory, thread safety)
- Documentation (README, code comments)
- Wi-Fi implementation (mode-specific correctness, event handling)
- Security (credentials, network security, debug features)
- Build verification (clean build, no warnings, binary size)

**Do NOT include PRD acceptance criterion testing here** — that goes in the Test Report.

Record the overall score (0–100) and verdict (PASS / PASS WITH ISSUES / REWORK / FAIL).

---

## Part B — Test Report (Functional Testing) — *Always Required*

**Hardware required. Run firmware on the target board.**
**Run after every implementation cycle — this is the proof that the PRD is satisfied.**

### B1. Flash the firmware

```bash
west flash --build-dir build/
```

Open a serial monitor and capture the boot log.

### B2. Map PRD acceptance criteria to test cases

Read `docs/product/PRD.md`. For every FR and NFR:
- Extract all acceptance criteria lines
- Assign each a TC ID: `TC-<FR number>-<sequence>` (e.g. `TC-001-01`)
- List them in the Test Report before starting any testing

### B3. Execute each test case

For each TC:
1. Follow the acceptance criterion literally
2. Record the result: ✅ Pass / ❌ Fail / ⚠️ Partial / ⬜ Not Run
3. Paste the relevant UART log lines as evidence

For failures, record expected vs actual behaviour and suggest routing:
- Code bug → Phase 3
- Spec gap → Phase 2
- Missing PRD requirement → Phase 1

### B4. Generate `docs/qa/TEST-YYYY-MM-DD-HH-MM.md`

Use `TEST_TEMPLATE.md` as the base. Fill in:
- Document Information (PRD Version, Specs Version, firmware build timestamp)
- Summary table (total / passed / failed / blocked)
- Results grouped by FR
- NFR measurements with targets vs actuals
- Failed test detail section
- Full UART boot log in appendix

---

## Part C — Review Summary & Routing

After generating the Test Report (and QA Report if applicable), present a combined summary:

```
═══════════════════════════════════════════════
  PHASE 4 REVIEW SUMMARY
═══════════════════════════════════════════════
  QA Score:    ___/100  [PASS / REWORK / FAIL]
  Test Result: __/__TCs passed  [PASS / FAIL]
═══════════════════════════════════════════════
  Critical issues requiring action:
  • [P0] <issue> → route to Phase <N>
  • [P0] <issue> → route to Phase <N>
═══════════════════════════════════════════════
```

Then ask:
> "Review complete. Route P0 issues back to the appropriate phase?
> Reply **yes** to continue with chsh-ncs-workflow, or **no** to stop here."

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
| `QA_TEMPLATE.md` | Code quality review template (Part A output) |
| `TEST_TEMPLATE.md` | Functional test report template (Part B output) |
| `CHECKLIST.md` | Manual review checklist for QA scoring |
| `check_project.sh` | Automated project health check script |
| `IMPROVEMENT_GUIDE.md` | How to feed review findings back into templates |

## Document Conventions

- **QA Report**: `docs/qa/QA-YYYY-MM-DD-HH-MM.md` — code quality audit snapshot
- **Test Report**: `docs/qa/TEST-YYYY-MM-DD-HH-MM.md` — functional test snapshot
- Both are dated snapshots (not living documents); keep all runs for history
- Reference the PRD Changelog version and Specs overview version in both headers

## Related Skills

- `chsh-ncs-workflow` — full lifecycle orchestrator; routes back here after Phase 3
- `chsh-pm-prd` — update `docs/product/PRD.md` if requirements need changing
- `chsh-dev-spec` — update engineering specs if design gaps are found
- `chsh-dev-project` — fix code for P0 issues
