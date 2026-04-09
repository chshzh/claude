---
name: chsh-qa-test
description: Phase 4 of the NCS project lifecycle. Generates a Test Report (PRD acceptance criteria pass/fail, hardware required, always) and optionally a QA Report (code quality 0-100, no hardware, release/demo only). Use when validating a build against PRD and specs.
---

# chsh-qa-test — QA & Functional Test

Phase 4 of the NCS project lifecycle. Validates that the implementation is both
well-built (QA Report) and behaves as the PRD requires (Test Report).

> **Two documents, two questions, different cadence:**
>
> | Document | Question | Hardware? | When required | Output |
> |----------|----------|-----------|---------------|--------|
> | Test Report | "Does it behave as the PRD says?" | Yes — run on board | **Always** — after every implementation cycle | `docs/TEST-YYYY-MM-DD-HH-MM.md` |
> | QA Report | "Is the code well-built?" | No — AI reviews code | **Release/demo only** — optional otherwise | `docs/QA-YYYY-MM-DD-HH-MM.md` |

---

## Step 0 — Check Inputs

Before starting, verify:

```bash
cat docs/PRD.md                          # acceptance criteria source
ls docs/specs/overview.md                # spec version reference
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

Read `docs/PRD.md`. For every FR and NFR:
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

### A4. Generate `docs/TEST-YYYY-MM-DD-HH-MM.md`

Use `TEST_TEMPLATE.md` as the base. Fill in:
- Document Information (PRD Version, Specs Version, firmware build timestamp)
- Changelog entry for this run
- Summary table (total / passed / failed / blocked)
- Results grouped by FR
- NFR measurements with targets vs actuals
- Failed test detail section
- Full UART boot log in appendix

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
- Documentation (README, code comments)
- Wi-Fi implementation (mode-specific correctness, event handling)
- Security (credentials, network security, debug features)
- Build verification (clean build, no warnings, binary size)

### B2. Generate `docs/QA-YYYY-MM-DD-HH-MM.md`

Use `QA_TEMPLATE.md` as the base. Fill in all sections.
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
| `QA_TEMPLATE.md` | Code quality review template (Part B output) |
| `TEST_TEMPLATE.md` | Functional test report template (Part A output) |

## Document Conventions

- **Test Report**: `docs/TEST-YYYY-MM-DD-HH-MM.md` — functional test snapshot
- **QA Report**: `docs/QA-YYYY-MM-DD-HH-MM.md` — code quality audit snapshot
- Both are dated snapshots; keep all runs for history
- Both include PRD Version and Specs Version in Document Information for traceability

## Related Skills

- `chsh-ncs-workflow` — full lifecycle orchestrator; routes back here after Phase 3
- `chsh-pm-prd` — update `docs/PRD.md` if requirements need changing
- `chsh-dev-spec` — update `docs/specs/` if design gaps are found
- `chsh-dev-project` — fix code for P0 issues
