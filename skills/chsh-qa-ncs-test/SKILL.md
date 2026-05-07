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

> For debug-focused testing (barrier hangs, crash analysis, VPR), use **chsh-dev-ncs-debug** alongside this skill.

### A1. Flash the firmware

```bash
nrfutil sdk-manager toolchain launch --ncs-version=v3.3.0 -- \
  west flash -d build/ --recover --dev-id <SN>
```

If multiple boards are available, assign roles:
- **Reference board** — same HW with the previous known-good firmware
- **Test board** — board under evaluation

Compare boot logs side-by-side when available. The first divergent line identifies the failure boundary.

### A2. Capture UART output

**Preferred (mcp.nrflow)**: call `mcp_nrflow_nordicsemi_workflow_ncs` to load the
`nordicsemi_uart_monitor.py` script, then:

```bash
python3 nordicsemi_uart_monitor.py --port /dev/cu.usbmodem... --baud 115200
```

**Manual** (if needed):
```python
import serial
ser = serial.Serial("/dev/cu.usbmodem...", 115200, rtscts=True)
# rtscts=True required when hw-flow-control is enabled (e.g. nRF54LM20DK UART30)
```

Capture the full boot log. Use this as evidence in the test report.

> **nRF54LM20DK note**: UART30 = VCOM1. Run `nrfutil device list` to identify VCOMs.

### A3. Send test commands via UART

For each test case, send the required shell commands and capture the response:

```bash
uart:~$ wifi scan
uart:~$ wifi connect -s <SSID> -k 1 -p <password>
uart:~$ wifi status
uart:~$ net iface
uart:~$ kernel threads    # thread health
```

Paste the relevant output lines as evidence for each test case — not generic "it worked".

### A4. Run the loop test for stability acceptance criteria

For any TC involving connectivity, boot reliability, or stability, always run a loop test:

```bash
python3 <app>/scripts/loop_test.py 10      # minimum for acceptance
python3 <app>/scripts/loop_test.py 20      # for release
```

If no `loop_test.py` exists in the app, copy the template from
`~/.claude/skills/chsh-dev-ncs-debug/scripts/loop_test.py` and edit the constants at
the top.

The loop test is **mandatory** for any PRD acceptance criterion that includes:
- "reliably", "consistently", "always", or a success rate threshold
- WiFi connection, BLE pairing, or network reconnect
- Boot time measurements

Record pass rate and iteration count in the test report evidence.

**Adjustable iteration count**: pass as the first argument:
```bash
python3 loop_test.py 5    # quick smoke test
python3 loop_test.py 10   # acceptance gate
python3 loop_test.py 20   # release gate
```

### A5. Map PRD acceptance criteria to test cases

Read `docs/pm-prd/PRD.md`. For every FR and NFR:
- Extract all acceptance criteria lines
- Assign each a TC ID: `TC-<FR number>-<sequence>` (e.g. `TC-001-01`)
- List them in the Test Report before starting any testing

### A6. Execute each test case

For each TC:
1. Follow the acceptance criterion literally
2. Send the required UART commands and capture output
3. Record the result: ✅ Pass / ❌ Fail / ⚠️ Partial / ⬜ Not Run
4. Paste the relevant UART log lines as evidence

For failures, record expected vs actual behaviour and suggest routing:
- Code bug → Phase 3
- Spec gap → Phase 2
- Missing PRD requirement → Phase 1

### A7. Generate `docs/qa-test/QA-YYYY-MM-DD-HH-MM.md`

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

- `chsh-dev-ncs-debug` — UART log capture, loop test scripts, multi-device comparison, barrier debugging
- `chsh-dev-ncs-workflow` — full lifecycle orchestrator; routes back here after Phase 3
- `chsh-pm-ncs-prd` — update `docs/pm-prd/PRD.md` if requirements need changing
- `chsh-dev-spec` — update `docs/dev-specs/` if design gaps are found
- `chsh-dev-ncs-project` — fix code for P0 issues
- `chsh-dev-git-release` — once tests pass, tag and publish a release

## Self-Update Policy

At the **end of each conversation**, review what was discovered and check whether any facts in this skill are new, corrected, or outdated (e.g. new test report patterns, acceptance criteria templates, QA scoring rubrics).

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
