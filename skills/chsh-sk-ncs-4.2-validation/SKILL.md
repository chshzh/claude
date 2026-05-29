---
name: chsh-sk-ncs-4.2-validation
description: >-
  Load when running Phase 4.2 Validation (hardware tests) for an NCS project.
  Derives test cases from PRD acceptance criteria and engineering specs, executes
  them on hardware via EEDP, and generates a dated VALIDATION report. Hardware required.
---

# chsh-sk-ncs-4.2-validation — Phase 4.2 Validation (Hardware Test Framework)

Phase 4.2 of the V&V lifecycle. Confirms PRD acceptance criteria are satisfied
at runtime. **Hardware required.**

Output: `docs/qa-test/VALIDATION-YYYY-MM-DD-HH-MM.md`

> For Phase 4.1 Verification (code review, build, docs audit) — use
> **chsh-sk-ncs-4.1-verification**.
>
> For EEDP platform setup and tool details — see **chsh-sk-ncs-3.2-debug**
> → [`references/eedp-platform.md`](../chsh-sk-ncs-3.2-debug/references/eedp-platform.md).

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

## Validation Framework

### A1. Flash the firmware

```bash
# Standard flash — preserves NVS and WiFi credentials
nrfutil sdk-manager toolchain launch --ncs-version=v3.3.0 -- \
  west flash -d build/ --dev-id <SN>

# Use --recover only for first-time flash on nRF54LM20DK or after AP protection erase
# WARNING: --recover and --erase wipe all flash including NVS/WiFi credentials
```

If multiple boards are available, assign roles:
- **Reference board** — same HW with the previous known-good firmware
- **Test board** — board under evaluation

Compare boot logs side-by-side when available. The first divergent line identifies the failure boundary.

### A2. Capture UART output

**Preferred (mcp.nordic-mcp)**: call `mcp_nordic-mcp_nordicsemi_workflow_ncs` to load the
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

**Preferred**: Delegate to `chsh-ag-terminal` — run Mode F from **chsh-sk-ncs-3.2-debug** for automated loop test execution without a local script.

**Manual fallback**: Copy `~/.claude/skills/chsh-sk-ncs-3.2-debug/scripts/loop_test.py` to `<app>/scripts/loop_test.py`, edit the constants at the top, then run:
```bash
python3 <app>/scripts/loop_test.py 10      # minimum for acceptance
python3 <app>/scripts/loop_test.py 20      # for release
```

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

Read `docs/pm-prd/PRD.md` and cross-reference `docs/dev-specs/`. For every FR and NFR:
- Extract all acceptance criteria lines
- Verify the criterion is traceable to a spec in `docs/dev-specs/`
- Assign each a TC ID: `TC-<FR number>-<sequence>` (e.g. `TC-001-01`)
- List them in the Test Report before starting any testing — this list IS the test plan

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

### A7. Generate `docs/qa-test/VALIDATION-YYYY-MM-DD-HH-MM.md`

Use `VALIDATION_TEMPLATE.md` as the base. Fill in:
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
- Every TC is traceable to a spec in `docs/dev-specs/` — no acceptance criterion without a design backing
- Evidence is specific: paste the relevant UART log lines, not a generic "it worked"
- NFR rows always have a measured number (not just ✅) — e.g. "29 s" not just "pass"
- Failed TC detail includes expected vs actual behaviour and a routing recommendation
- The executive summary (total/pass/fail + one-sentence verdict) appears at the top before results

---

## Routing

| Finding | Priority | Route |
|---------|----------|-------|
| P0 test case fails (code bug) | P0 | Phase 3 (`chsh-sk-ncs-3.1-coding`) |
| P0 test case fails (spec gap) | P0 | Phase 2 (`chsh-sk-ncs-2-spec`) |
| Build fails on target board | P0 | Phase 3 |
| PRD criterion ambiguous | P1 | Phase 1 (`chsh-sk-ncs-1-prd`) |
| P1/P2 test failures only | P2 | Phase 3 (next iteration) |
| All P0 TCs pass | ✅ | Ready for release |

After reporting, ask:
> "Test complete. Route P0 issues to the appropriate phase, or proceed to
> release with **chsh-sk-git-release**?"

---

## Document Conventions

- Output: `docs/qa-test/VALIDATION-YYYY-MM-DD-HH-MM.md` — dated snapshot, Part A only
- Keep only the latest report in `docs/qa-test/`
- Include PRD Version and Specs Version in Document Information for traceability
- Use `VALIDATION_TEMPLATE.md` (Part A sections only) as the base for every new report

## Related Skills

| Task | Skill |
|------|-------|
| EEDP platform setup + debug tools | `chsh-sk-ncs-3.2-debug` · [eedp-platform.md](../chsh-sk-ncs-3.2-debug/references/eedp-platform.md) |
| AP simulation, SSID control | `chsh-sk-router-control` |
| Memfault log upload diagnostics | `chsh-sk-ncs-tc-memfault-log-debug` |
| Wi-Fi throughput benchmarking | `chsh-sk-ncs-tc-wifi-throughput` |
| Phase 4.1 Verification (no hardware) | `chsh-sk-ncs-4.1-verification` |
| Fix code for P0 failures | `chsh-sk-ncs-3.1-coding` |
| Tag and publish release | `chsh-sk-git-release` |
| Full lifecycle orchestration | `chsh-sk-ncs-0-workflow` |

## Gotchas
- TODO: add one entry per real observed failure or routing false-positive

## Self-Update Policy

At the **end of each conversation**, review what was discovered and check
whether any facts in this skill are new, corrected, or outdated (e.g. new
test case procedures, EEDP tool updates, Memfault API changes, throughput
baseline changes).

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
