---
name: chsh-sk-ncs-4.2-validation
description: >-
  Load when running hardware validation for an NCS project — after 4.1 static verification,
  after a feature, or before a release (not release-only). Derives test scenarios from PRD
  acceptance criteria into a reviewable VALIDATION_PLAN.md, executes them on hardware
  shell-first via chsh-ag-terminal, captures peak thread/heap watermarks with ZView over SWD,
  and produces VALIDATION_REPORT.md (which feeds chsh-sk-ncs-3.3-memopt). Hardware required.
---

# chsh-sk-ncs-4.2-validation — Hardware Validation (Plan → Execute → Report)

Confirms PRD acceptance criteria are satisfied **at runtime**, and captures accurate peak
memory watermarks under load. **Hardware required.**

**Run anytime** — most commonly right after Phase 4.1 static verification, but equally after
finishing a feature or before a release. This is not a release-only step.

Two living documents (fixed names, internal Changelog — like `PRD.md`/`architecture.md`):
- `docs/qa-test/VALIDATION_PLAN.md` — the reviewable test plan (from `VALIDATION_PLAN_TEMPLATE.md`)
- `docs/qa-test/VALIDATION_REPORT.md` — the results (from `VALIDATION_REPORT_TEMPLATE.md`)

> Static code review (security, format, quality, code-reading PRD check) belongs to
> **chsh-sk-ncs-4.1-verification** — not repeated here.
> EEDP platform / UART connection details — see **chsh-sk-ncs-3.2-debug**
> (board→VCOM table, port pre-checks, `chsh-ag-terminal` delegation).

---

## Entry Gate (run anytime, multi-board aware)

1. **Check inputs**:
   ```bash
   cat docs/pm-prd/PRD.md            # acceptance criteria source (note Changelog version)
   ls docs/dev-specs/overview.md     # spec version reference
   nrfutil device list               # which boards are ACTUALLY connected + serial numbers
   ```
   Don't assume a fixed board set — the user plugs in different DKs per run.

2. **Build the per-board addressing map** (single source of truth for every phase). For each
   connected board the project has a config/build for, resolve from the **chsh-sk-ncs-3.2-debug**
   board table:

   | Board | `--dev-id` (SN) | VCOM port + rtscts | Baud | J-Link target | ELF (sysbuild: under app sub-image) | Shell |
   |-------|------|------|------|------|------|------|
   | nRF7002DK | from `device list` | `…3`, rtscts=False | 115200 | `nRF5340_xxAA` | `build_nrf7002dk/<app>/zephyr/zephyr.elf` | off |
   | nRF54LM20DK + nRF7002EB2 | from `device list` | `…1`, rtscts=True | 115200 | `nRF54LM20A_M33` | `build_nrf54lm20dk/<app>/zephyr/zephyr.elf` | on |

   > Sysbuild puts the app ELF under `build_<board>/<app-name>/zephyr/`, not the top-level
   > `build_<board>/zephyr/` (that's the MCUboot/merged domain). Use the app sub-image ELF for ZView.

   Skip (with a logged note) any connected board the project has no build/config for.

3. **Negotiate scope and plan complexity with `AskQuestion`** — before touching the PRD:
   - Which connected board(s) to validate? **Single-board runs are fully supported** — don't
     default to all boards. The user may want to test just one at a time.
   - Routine-functional vs release run?
   - Include the **ZView memory-watermark pass?**
   - **Plan complexity** (this shapes everything in V1):
     - *Simple / automated* — shell commands only, fully scripted, fast (minutes)
     - *Comprehensive / realistic* — real app usage scenarios, AP mode transitions, manual
       provisioning flows, interactive debugging with tools from `chsh-sk-ncs-3.2-debug`
       (RTT, coredump, logic analyser, etc.), steps that require the **human to operate
       the device or react to UI events**

   The right complexity depends on where in the dev cycle you are. A post-feature smoke test
   might be simple; a pre-release sign-off may need every real-world usage scenario exercised.
   If unsure, present a two-line summary of each option and let the user pick.

---

## Phase V1 — Plan → `docs/qa-test/VALIDATION_PLAN.md` (collaborative)

**Build this plan with the human using `AskQuestion` throughout — do not silently draft a
complete plan and present it for rubber-stamp approval.** The complexity negotiated in the Entry
Gate determines how many collaborative checkpoints you make:

- *Simple*: draft a Coverage Matrix + single round, show to user, get approval → done.
- *Comprehensive*: co-design each round — surface proposed scenarios, ask which debugging tools
  (from `chsh-sk-ncs-3.2-debug`) to include, identify where human interaction is required, ask
  whether to add AP-simulation rounds (`chsh-sk-router-control`) or throughput rounds, then get
  final approval.

Generate from `VALIDATION_PLAN_TEMPLATE.md`.

1. Read `docs/pm-prd/PRD.md` and cross-reference `docs/dev-specs/`. Build a **draft Coverage
   Matrix**: every FR/NFR and each suspected corner-case → a row, common-usage first then corner
   cases, each traceable to a PRD requirement ID. No invented tests.

2. **Present the draft Coverage Matrix to the user and ask:**
   - Are there features or edge cases missing?
   - Which rows require **human interaction** (e.g. provisioning a phone to the AP, navigating
     a captive portal, triggering a manual OTA check, observing LED behaviour)? — mark these
     `[HUMAN]` in the round definition so the plan makes clear when to pause for a human step.
   - For complex plans: which rows benefit from `chsh-sk-ncs-3.2-debug` tools (RTT log,
     coredump decode, logic analyser capture)?

3. Group into **test rounds — aim for 1–3, max 5**. Combine as many checks as possible into one
   continuous device session. **If more than 5 rounds are genuinely needed, stop and `AskQuestion`.**
4. Tag the round(s) that maximize concurrent memory pressure as **[ZView]** (see Phase V2 → ZView).
5. Per board: note shell availability (drives shell vs button/log execution) and track rounds per
   board. For `[HUMAN]` steps on no-shell boards, write the human instruction explicitly ("Press
   Button 2 now and observe LED2 blinks 3×").
6. **Present the complete draft plan and get explicit approval before executing.** For complex
   plans, ask one last time: "Any rounds to add, remove, or simplify before we execute?"

---

## Phase V2 — Execute (shell-first, ZView-instrumented)

### Flash (per board, per the addressing map)

```bash
# Standard flash — preserves NVS / Wi-Fi credentials. Target the right probe with --dev-id.
nrfutil sdk-manager toolchain launch --ncs-version=${NCS_VERSION:-v3.3.0} -- \
  west flash -d build_<board>/ --dev-id <SN>
# --recover / --erase wipe NVS + Wi-Fi creds — only for first flash or after AP-protect erase.
```

### Drive the test (shell-first)

Delegate serial to **`chsh-ag-terminal`** — never drive pyserial directly. Follow
**chsh-sk-ncs-3.2-debug** Steps 0.5–0.7 first (confirm target, set up the `tail -f` session log,
verify the port is FREE). Pass board + port + rtscts + log file to the subagent.

```
chsh-ag-terminal: on <board>/<port>, send "wifi connect -s <SSID> -k 1 -p <pw>" then "wifi status"
```

Shell-first evidence commands: `wifi status`, `net iface`, `kernel threads`, heap_monitor logs.
**Paste the real output lines** as evidence — not "it worked".

**No-shell boards (e.g. nRF7002DK, `CONFIG_SHELL=n`)**: drive via button triggers + UART log
parsing instead (e.g. Button 1 = heartbeat, Button 2 = OTA check; crash demos via long-press).

**Stability gate**: for any criterion saying "reliably/consistently/always" or a success-rate
threshold, run a loop test — `chsh-ag-terminal: run loop test N iterations on <board>` (10 =
acceptance, 20 = release), fallback Mode F in chsh-sk-ncs-3.2-debug. Record pass rate.

### ZView memory-watermark capture (for [ZView] rounds)

ZView reads thread stack/heap watermarks over SWD (J-Link) **without halting the CPU or using
UART** — so it runs concurrently with the `chsh-ag-terminal` UART session on the same DK (debug
port and VCOM are independent interfaces).

**Prerequisite** — the build under test must be compiled with (add a `overlay-zview.conf` if the
production `prj.conf` lacks them; these enable measurement without changing stack/heap *sizes*):
```
CONFIG_INIT_STACKS=y            # stack watermarks
CONFIG_THREAD_MONITOR=y         # thread discovery
CONFIG_THREAD_STACK_INFO=y      # thread metadata
CONFIG_SYS_HEAP_RUNTIME_STATS=y # heap peak / fragmentation (optional but wanted here)
CONFIG_THREAD_NAME=y            # readable thread names (optional)
```
If `west zview dump` returns empty threads/heaps, the build is missing these — rebuild with them.

**Procedure** (wrap `west` in the toolchain launcher as above; ELF = app sub-image, see map):
```bash
# 1. Start a recording over the full high-memory round (run in background while the shell drives load)
#    Pass -s <SN> to avoid the probe-selection dialog (get S/N with: nrfutil device list)
west zview record -e build_<board>/<app>/zephyr/zephyr.elf -r jlink -t <target> -s <SN> \
  -o /tmp/zview_<board>.ndjson.gz --duration <round_seconds>

# 2. Drive the high-memory stimulus over UART/shell concurrently
#    (concurrent TLS/HTTPS + MQTT, OTA download, coredump capture+upload, large log flush)

# 3. Extract peak watermarks from the recording (stderr=status, stdout=clean JSON for jq)
west zview dump -i /tmp/zview_<board>.ndjson.gz --frame <peak> --json \
  | jq '.threads[] | {name, alloc:.stack_size, watermark:.runtime.stack_watermark_percent}'
west zview dump -i /tmp/zview_<board>.ndjson.gz --frame <peak> --json | jq '.heaps'
```
Or a single live frame: `west zview dump -e <elf> -r jlink -t <target> -s <SN> --json`.
J-Link targets: nRF7002DK → `nRF5340_xxAA`; nRF54LM20DK → `nRF54LM20A_M33`.
For interactive watching, see `chsh-sk-ncs-3.3-memopt` → **Live ZView**.
Record the per-board peak thread stacks (name, Kconfig symbol, peak/alloc, %) and heap peaks
(system + mbedTLS) into the report's **Memory Watermarks** section, noting the producing round.

---

## Phase V3 — Report → `docs/qa-test/VALIDATION_REPORT.md`

Generate from `VALIDATION_REPORT_TEMPLATE.md`:
- Document Information (PRD/Specs/Plan version, firmware build, boards, ZView used) + Changelog entry
- Executive Summary (totals + verdict) at the top
- Per-FR results (TC | criterion | board | result | evidence) + NFR measured values
- Failed-test detail (expected vs actual + UART log + routing)
- **Memory Watermarks** section — the hand-off contract for chsh-sk-ncs-3.3-memopt (per-board
  peak thread stacks + heap peaks; columns mirror MEMOPT_REPORT). Skip-mark if no ZView pass.
- UART boot log + performance metrics + routing table

**Report quality**: every TC maps to an exact PRD criterion (none invented) and is traceable to
a spec; evidence is concrete log lines; NFR rows carry a measured number; Memory Watermarks cells
are filled (no empty `___`) when the ZView pass ran.

---

## Routing

| Finding | Priority | Route |
|---------|----------|-------|
| P0 TC fails (code bug) | P0 | Phase 3 (`chsh-sk-ncs-3.1-coding`) |
| P0 TC fails (spec gap) | P0 | Phase 2 (`chsh-sk-ncs-2-spec`) |
| Build/flash fails on a board | P0 | Phase 3 |
| PRD criterion ambiguous | P1 | Phase 1 (`chsh-sk-ncs-1-prd`) |
| Thread/heap at CRITICAL risk in Memory Watermarks | P1 | Phase 3.3 (`chsh-sk-ncs-3.3-memopt`) — feed this report |
| P1/P2 failures only | P2 | Phase 3 (next iteration) |
| All P0 TCs pass | ✅ | Ready for release (`chsh-sk-ncs-3.5-release`) |

After reporting, ask:
> "Validation complete. Route P0 issues to the appropriate phase, re-size memory with
> **chsh-sk-ncs-3.3-memopt** (it reads VALIDATION_REPORT.md), or proceed to release with
> **chsh-sk-ncs-3.5-release**?"

---

## Document Conventions

- Two living docs, fixed names: `docs/qa-test/VALIDATION_PLAN.md` and `docs/qa-test/VALIDATION_REPORT.md`.
  Each carries its own `Version` (bump on every edit) and a Changelog — not dated snapshots.
- Include PRD Version + Specs Version in Document Information for traceability.
- The Memory Watermarks columns must stay 1:1 with `chsh-sk-ncs-3.3-memopt/MEMOPT_REPORT_TEMPLATE.md`.

## Related Skills

| Task | Skill |
|------|-------|
| UART connection, board→VCOM table, port pre-checks, loop test | `chsh-sk-ncs-3.2-debug` |
| Serial command/log execution (delegated) | `chsh-ag-terminal` |
| Re-size heaps/threads from Memory Watermarks | `chsh-sk-ncs-3.3-memopt` |
| AP simulation, SSID control, reconnect tests | `chsh-sk-router-control` |
| Wi-Fi throughput benchmarking | `chsh-sk-ncs-tc-wifi-throughput` |
| Phase 4.1 static verification (no hardware) | `chsh-sk-ncs-4.1-verification` |
| Fix code for P0 failures | `chsh-sk-ncs-3.1-coding` |
| Tag and publish release | `chsh-sk-ncs-3.5-release` |
| Full lifecycle orchestration | `chsh-sk-ncs-0-workflow` |

## Gotchas
- ZView returns empty threads/heaps → build missing `CONFIG_INIT_STACKS` / `THREAD_MONITOR` / `THREAD_STACK_INFO`; rebuild with `overlay-zview.conf`.
- A thread inactive during the ZView round (e.g. OTA downloader never fired) has no true peak — flag it so 3.3 keeps the prior value.
- TODO: add one entry per real observed failure or routing false-positive.

## Self-Update Policy

At the **end of each conversation**, review what was discovered and check whether any facts in
this skill are new, corrected, or outdated (new test procedures, ZView flags, board/VCOM changes,
J-Link target descriptors).

If updates are warranted: collect changes with rationale, present a summary via `AskQuestion`,
apply approved updates. Do **not** modify this skill mid-conversation unless the user explicitly asks.
