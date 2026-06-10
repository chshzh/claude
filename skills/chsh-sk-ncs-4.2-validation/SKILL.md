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
nrfutil sdk-manager toolchain launch --ncs-version=${NCS_VERSION:-v3.3.0} -- \
  west flash -d build/ --dev-id <SN>

# Use --recover only for first-time flash on nRF54LM20DK or after AP protection erase
# WARNING: --recover and --erase wipe all flash including NVS/WiFi credentials
```

If multiple boards are available, assign roles:
- **Reference board** — same HW with the previous known-good firmware
- **Test board** — board under evaluation

Compare boot logs side-by-side when available. The first divergent line identifies the failure boundary.

### A2. Capture UART output

**Primary**: Invoke `chsh-ag-terminal` — it opens the UART port, captures the boot log, and returns the full output. Tell it the board type and it will pick the right VCOM and baud rate.

```
chsh-ag-terminal: capture boot log on nRF7002DK
chsh-ag-terminal: capture boot log on nRF54LM20DK
```

**Fallback** (if chsh-ag-terminal is unavailable): call `mcp_nordic-mcp_nordicsemi_workflow_ncs` to load `nordicsemi_uart_monitor.py`, then:
```bash
python3 nordicsemi_uart_monitor.py --port /dev/cu.usbmodem... --baud 115200
```

Capture the full boot log. Use this as evidence in the test report.

> **nRF54LM20DK note**: UART30 = VCOM1. Run `nrfutil device list` to identify VCOMs.

### A3. Send test commands via UART

**Primary**: Invoke `chsh-ag-terminal` with the commands to send. It sends each command and returns the response.

```
chsh-ag-terminal: send "wifi scan" and capture output
chsh-ag-terminal: send "wifi connect -s <SSID> -k 1 -p <password>" then "wifi status"
```

Common test commands:
```
wifi scan
wifi connect -s <SSID> -k 1 -p <password>
wifi status
net iface
kernel threads
```

Paste the relevant output lines as evidence for each test case — not generic "it worked".

### A4. Run the loop test for stability acceptance criteria

For any TC involving connectivity, boot reliability, or stability, always run a loop test:

**Primary**: Invoke `chsh-ag-terminal` in loop mode — it resets the board, captures the log, and repeats N times without a local script:

```
chsh-ag-terminal: run loop test 10 iterations on nRF7002DK
chsh-ag-terminal: run loop test 20 iterations on nRF54LM20DK
```

**Fallback** (if chsh-ag-terminal is unavailable): run Mode F from **chsh-sk-ncs-3.2-debug** using `loop_test.py`:
```bash
python3 ~/.claude/skills/chsh-sk-ncs-3.2-debug/scripts/loop_test.py 10   # acceptance gate
python3 ~/.claude/skills/chsh-sk-ncs-3.2-debug/scripts/loop_test.py 20   # release gate
```

The loop test is **mandatory** for any PRD acceptance criterion that includes:
- "reliably", "consistently", "always", or a success rate threshold
- WiFi connection, BLE pairing, or network reconnect
- Boot time measurements

Record pass rate and iteration count in the test report evidence.

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
> release with **chsh-sk-ncs-3.5-release**?"

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
| Tag and publish release | `chsh-sk-ncs-3.5-release` |
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
