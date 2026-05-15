---
name: chsh-sk-ncs-debug
description: Debug NCS/Zephyr applications on Nordic hardware. Covers UART log capture, board reset, multi-device comparison testing, loop testing for stability validation, barrier/co-processor debugging, and crash analysis. Use when debugging a boot failure, WiFi issue, driver crash, kernel hang, or any firmware reliability problem on nRF devices. For initial environment setup use chsh-sk-ncs-env first.
---

# chsh-sk-ncs-debug — NCS Embedded Debugging Workflow

Systematic debugging for NCS/Zephyr applications on Nordic hardware.
Covers the full range from simple UART log capture to co-processor barrier synchronization.

> **Prerequisite**: A working NCS toolchain (`nrfutil sdk-manager`) and at least one connected Nordic board.
> If not set up, run **chsh-sk-ncs-env** first.

> **Knowledge sources**: Always call `mcp_nrflow_nordicsemi_workflow_ncs` at the start
> of any debug session to load the `nrfutil-manual`, `nordicsemi_uart_monitor.py`, and
> `embedded-code-guidance-ncs-zephyr` resources into context.

---

## Step 0 — Gather Baseline Information (Mandatory)

Before investigating any failure, collect and document:

```bash
nrfutil device list          # connected boards + serial numbers
west boards | grep <soc>     # available board targets
cat docs/dev-specs/overview.md 2>/dev/null | head -10  # specs version
git -C <app> log --oneline -3  # recent changes
```

Required information:
- **Board HW**: board name + version (e.g., `nRF54LM20DK PCA10173 v0.7.0`)
- **NCS version**: e.g., `v3.3.0`
- **Firmware version**: git tag or commit hash
- **UART port**: correct VCOM (e.g., nRF54LM20DK+nRF7002EB2: UART30/VCOM0, `rtscts=True` — UART20 disabled by shield overlay)
- **Second reference board** (if available): same HW with known-good firmware

> **Rule**: If you have two boards, assign one as **reference** (known-good firmware)
> and one as **test** (under investigation). Compare their logs at the first divergence.

### Step 0.5 — Resolve Test Target (Required Before Any Serial Action)

Before invoking `chsh-ag-terminal` for any serial operation, the test target must be
unambiguously known. If the board and port are not already clear from the conversation
context, use `AskQuestion` to confirm:

```
Board options to present:
  A) nRF54LM20DK+nRF7002EB2 — VCOM0 port (suffix ...1, rtscts=True)  e.g. /dev/cu.usbmodem0010518696871
  B) nRF7002DK   — port unknown, needs nrfutil device list
  C) Other       — specify board name and port

Pre-filled default when context is silent: option A.
```

Only proceed with serial operations once **both** Board and Port are confirmed.
Do not guess the port — a wrong port produces misleading output silently.

---

## Step 1 — Choose Debug Mode

| Symptom | Mode |
|---------|------|
| Boot hangs, no UART output | A — Serial capture + reset |
| WiFi fails, drivers misbehave | B — UART logs + loop test |
| Crash / hardfault | C — Core dump / GDB |
| Co-processor / VPR hang | D — Barrier debugging |
| Memory overflow, stack corruption | E — Memory analysis → use **chsh-sk-ncs-memory** |
| Intermittent failure (not always) | F — Loop test |
| CI build fails / pre-built firmware broken | G — GitHub Actions monitoring + firmware test |
| Need protocol-level insight (SPI, I2C) | H — External Debug Tools via MCP (Saleae LA, JLink) |
| Manual debug insufficient; need automated HIL | I — Physical Test Harness (future: actuators + sensors) |

---

## Mode A — Serial Capture and Reset

> **Serial operations in this mode are delegated to `chsh-ag-terminal`.**
> Resolve the test target (Step 0.5) first, then invoke the subagent.

### A1. Connect to UART and capture boot log

Delegate to `chsh-ag-terminal` with the confirmed test target:

```
Task(
  subagent_type="chsh-ag-terminal",
  description="Capture boot log",
  prompt="""
  Board: <confirmed board>
  Port:  <confirmed port>

  1. Reset the board via nrfutil device reset
  2. Capture serial output until uart:~$ appears (30 s timeout)
  3. Return: full boot log + first error line if any
  """
)
```

The subagent handles port selection, HWFC (rtscts), reconnection, and timestamping
internally. Do not run pyserial or nordicsemi_uart_monitor.py directly.

> **nRF54LM20DK + nRF7002EB2 HWFC reference** (for context — the subagent applies this automatically):
> The nrf7002eb2 shield overlay disables UART20 (SPI pin conflict) and sets `zephyr,shell-uart = &uart30`.
> UART30/VCOM0 (shell `uart:~$`) → `rtscts=True`. UART20/VCOM1 → **disabled, do not use**.
> Always use VCOM0 for the shell prompt when nRF7002EB2 is active.

### A2. Interpret the returned boot log

Key lines to look for in the subagent's response:

| Line | Meaning |
|------|---------|
| `*** Booting nRF Connect SDK` | Firmware started |
| `uart:~$` | Shell ready |
| `RPU boot signature check failed` | VPR/co-processor failure |
| `UMAC init timed out` | Cascaded from earlier failure |

### A3. Compare logs between reference and test boards

If a reference board is available, invoke `chsh-ag-terminal` twice (or in parallel
with `run_in_background=True`) — once per board. The first divergent log line is the
failure boundary.

---

## Mode B — UART Logs and WiFi Debugging

### B1. Enable verbose logging

Add to `prj.conf` or `boards/<board>.conf`:
```kconfig
CONFIG_LOG=y
CONFIG_LOG_DEFAULT_LEVEL=3    # INFO
CONFIG_LOG_BUFFER_SIZE=8192
CONFIG_WIFI_LOG_LEVEL_DBG=y   # for WiFi issues
CONFIG_NET_LOG=y              # for network issues
```

Rebuild:
```bash
nrfutil sdk-manager toolchain launch --ncs-version=v3.3.0 -- \
  west build -b <board> -d <app>/build <app> -- -DSHIELD=<shield>
```

### B2. Connect, reset, capture, issue commands

> **Delegate to `chsh-ag-terminal`** — do not run serial commands manually.

```
Task(
  subagent_type="chsh-ag-terminal",
  description="WiFi debug — capture log + run commands",
  prompt="""
  Board: <confirmed board>
  Port:  <confirmed port>

  1. Reset the board, capture boot log until uart:~$
  2. Send: wifi scan
  3. Send: wifi connect -s <SSID> -k 1 -p <password>
  4. Send: wifi status
  5. Send: net iface
  6. Return: full output for each command
  """
)
```

### B3. Interpret common WiFi error patterns

| Log line | Likely cause | Fix |
|----------|-------------|-----|
| `RPU boot signature check failed` | First CSB barrier never ACK'd by VPR | Check VPR firmware load, memory layout |
| `UMAC init timed out` | Cascaded from earlier barrier failure | Find the first failure, not the last |
| `nrf_sqspi_xfer() failed: 0bad000b` | `transfer_in_progress` stuck | Abort + clear state before retry |
| `Invalid memory address` | Transfer ran with corrupt state | State not reset after previous failure |
| `wlan0: CTRL-EVENT-DISCONNECTED` | Association succeeded but data path failed | Check DMA timeout recovery logic |
| `FF:FF:FF:FF:FF:FF` MAC | OTP blank — needs `CONFIG_WIFI_RANDOM_MAC_ADDRESS=y` | Add to Kconfig |

---

## Mode C — Crash and Hardfault Analysis

### C1. Enable crash logging

```kconfig
CONFIG_FAULT_DUMP=2
CONFIG_EXTRA_EXCEPTION_INFO=y
CONFIG_DEBUG_COREDUMP=y
CONFIG_DEBUG_COREDUMP_BACKEND_LOGGING=y
```

### C2. Capture the crash dump

```
FATAL ERROR: BUS FAULT
   Executing thread (tid: 0x20003abc, name: "wifi_mgmt"):
   ...
   Faulting instruction address (r15/pc): 0x0005e2f4
```

Use the ELF to decode:
```bash
arm-zephyr-eabi-addr2line -e build/zephyr/zephyr.elf 0x0005e2f4
```

### C3. GDB attach

```bash
nrfutil sdk-manager toolchain launch --ncs-version=v3.3.0 -- \
  west debug -d <app>/build
```

```gdb
(gdb) bt full            # backtrace with locals
(gdb) info thread        # all threads
(gdb) frame <n>          # switch frame
(gdb) p <variable>       # print variable
```

---

## Mode D — Co-processor / VPR Barrier Debugging

For sQSPI / MSPI soft peripheral barrier hangs on nRF54L series.

### D1. Identify the pattern

VPR barrier hang symptoms:
- Busy-wait spins forever (thread stuck in `__XSBx` macro)
- `h0 != h1` in handshake registers: `h0=N, h1=N-1` means VPR one barrier behind

### D2. Debug sequence

Add instrumentation (temporary, remove before release):
```c
_xsb_timeout_tc = 0;
__CSB(p_qspi->p_reg);
if (_xsb_timeout_tc) {
    printk("CSB timeout tc=%u h0=%u h1=%u\n",
           _xsb_timeout_tc,
           sp_handshake_get(p_qspi->p_reg, 0),
           sp_handshake_get(p_qspi->p_reg, 1));
}
```

### D3. Root cause checklist

- [ ] `__DSB()` called before task trigger? (required for AHB/APB write visibility)
- [ ] VPR still in event loop entry when trigger fires? (add graduated re-trigger)
- [ ] Abort sends `__SSB`? (do NOT — if VPR is stuck, SSB also hangs)
- [ ] Events cleared after abort? (`nrf_qspi2_event_clear` for all DMA events)
- [ ] `transfer_in_progress` reset? (needed or next `nrf_sqspi_xfer` returns BUSY)

### D4. Safe abort pattern

```c
void abort_transfer(void) {
    nrf_qspi2_core_disable(p_reg);          // stop DMA, no barriers needed
    nrf_qspi2_event_clear(p_reg, NRF_QSPI2_EVENT_DMA_DONE);
    nrf_qspi2_event_clear(p_reg, NRF_QSPI2_EVENT_DMA_ABORTED);
    nrf_qspi2_event_clear(p_reg, NRF_QSPI2_EVENT_DMA_DONEJOB);
    p_cb->transfer_in_progress = false;
    p_cb->prepared_pending = false;
    // DO NOT call __SSB — it will also hang
}
```

---

## Mode F — Loop Testing for Stability

Use a scripted loop test whenever a failure is intermittent or after any fix to confirm
stability. The rule of thumb: **10 passes minimum, 20 for a release claim**.

### F1. Run the loop test via chsh-ag-terminal

> **Delegate to `chsh-ag-terminal`** — it contains the full loop test template
> and handles HWFC, reset, and per-iteration pass/fail reporting.

```
Task(
  subagent_type="chsh-ag-terminal",
  description="Loop test — N iterations",
  run_in_background=True,   # loop tests are long; keep working while it runs
  prompt="""
  Board: <confirmed board>
  Port:  <confirmed port>
  SSID:  <ssid>
  PSK:   <password>
  KEY_MGMT: 1   # 1=WPA2-PSK, 3=WPA3-SAE

  Run a loop test with <N> iterations (10 = acceptance, 20 = release).
  For each iteration: reset board, wait for uart:~$, send wifi connect, verify Connected + IP.
  Write per-iteration results to /tmp/loop_test_results.txt.
  Return: pass rate summary (X/N passed) and any failure details.
  """
)
```

After the task completes, read `/tmp/loop_test_results.txt` for the full per-iteration log.

> **HWFC reference** (applied automatically by the subagent):
> nRF54LM20DK+nRF7002EB2: UART20/VCOM1 → **disabled**. UART30/VCOM0 → `rtscts=True` (shell).

### F2. Iteration counts

| Count | Purpose |
|-------|---------|
| 5 | Quick smoke test |
| 10 | Acceptance gate (minimum) |
| 20 | Release gate |

### F3. Multi-device loop test

Invoke `chsh-ag-terminal` twice with `run_in_background=True`, once per board, then
wait for both results. Supply each with its own board/port/SN. Compare pass rates
to isolate whether the failure is hardware-specific or firmware-general.

---

## Mode J — Memfault Log File Debugging

Use when: device uploads data to Memfault (last_seen updates) but no Log Files appear
in the "Log Files" tab on the device page.

### J1. Distinguish chunk upload from log file creation

| Symptom | Cause |
|---------|-------|
| `last_seen` updates every ~60s, no log files | Rate limiting (most common) |
| `last_seen` never updates | Upload failing (DNS, TLS, auth) |
| Log files from old builds only | Test builds hit rate limit before developer mode enabled |
| `Memfault upload complete` in serial but no files | Rate limiting (chunks accepted, log file dropped silently) |

### J2. Check for rate limiting first

The Memfault platform silently drops log files that exceed ~1/hour per device.
The firmware **receives HTTP 202** for all chunk uploads — there is no firmware-side warning.

**How to detect:** Memfault UI → Device page → Developer Mode tab → Processing Errors.

**Fix for testing:** Enable **Server-Side Developer Mode** on the Device page.
After enabling, log files should appear within 60–90s.

**Production guidance:** Set `CONFIG_MEMFAULT_PERIODIC_UPLOAD_INTERVAL_SECS=3600`
(1 hour) to stay within the rate limit.

### J3. Check `memfault_log_trigger_collection()` call sites

If `memfault_log_trigger_collection()` is called in `on_connect()`, it freezes the
log buffer right after reconnect. New logs (including the reconnect success messages)
are then silently dropped by `prv_try_free_space()` until the upload completes.

```bash
grep -rn "memfault_log_trigger_collection" src/
# Should appear ONLY in disconnect handlers, never in on_connect()
```

### J4. Verify via API

```python
import requests, base64
KEY = '<project_key>'
TOKEN = '<org_token>'
headers = {'Authorization': f'Bearer {TOKEN}'}
r = requests.get('https://api.memfault.com/api/v0/organizations/<org>/projects/<proj>'
                 '/devices/<serial>/log-files', headers=headers)
files = r.json()['data']
print(f'Total: {len(files)}, latest: {files[0]["created_date"] if files else "none"}')
```

---

## Common Debug Kconfig

```kconfig
# Logging
CONFIG_LOG=y
CONFIG_LOG_DEFAULT_LEVEL=3
CONFIG_LOG_BUFFER_SIZE=8192

# Crash analysis
CONFIG_FAULT_DUMP=2
CONFIG_EXTRA_EXCEPTION_INFO=y
CONFIG_DEBUG_COREDUMP=y

# WiFi verbose
CONFIG_WIFI_LOG_LEVEL_DBG=y
CONFIG_NRF_WIFI_LOG_LEVEL_DBG=y

# Network
CONFIG_NET_LOG=y
CONFIG_NET_STATISTICS=y

# Memory debug
CONFIG_STACK_SENTINEL=y
CONFIG_THREAD_ANALYZER=y
```

---

## Build and Flash Quick Reference

```bash
# Build (pristine)
nrfutil sdk-manager toolchain launch --ncs-version=v3.3.0 -- \
  west build -b <board> -p -d <app>/build <app> -- -DSHIELD=<shield>

# Flash (nRF54LM20DK needs --recover for first flash)
nrfutil sdk-manager toolchain launch --ncs-version=v3.3.0 -- \
  west flash -d <app>/build --recover --dev-id <SN>

# Flash (nRF7002DK, standard)
nrfutil sdk-manager toolchain launch --ncs-version=v3.3.0 -- \
  west flash -d <app>/build --erase --dev-id <SN>

# Reset only
nrfutil device reset --serial-number <SN>

# List devices
nrfutil device list
```

---

## Mode G — GitHub Actions Monitoring and Pre-Built Firmware Test

Use when a CI build fails, the pre-built firmware is broken, or you need to verify the CI
→ flash → test loop end-to-end.

### G1. Monitor GitHub Actions runs

```bash
# List recent runs
gh run list --repo <owner>/<repo> --limit 5

# Watch until completion (updates every 30 seconds)
gh run watch <run-id> --repo <owner>/<repo> --interval 30

# Get failed step logs
gh run view <run-id> --repo <owner>/<repo> --log-failed
```

### G2. Common CI failure patterns

| Failure | Root cause | Fix |
|---------|-----------|-----|
| `curl: (22) 404` on nrfutil | URL no longer valid | Switch to `ghcr.io/nrfconnect/sdk-nrf-toolchain:<ver>` container |
| `west: command not found` | Toolchain not in container PATH | Use container image, not manual install |
| `git am` fails "already applied" | Patch cached with repos | Add idempotent check: `git am --check "$p" && git am "$p"` |
| `merged.hex not found` | No MCUboot sysbuild | Fallback to `zephyr/zephyr.hex`; check `sysbuild.conf` |
| Permission denied on git ops | UID mismatch in container | Add `git config --global --add safe.directory '*'` |
| Release step fails | Missing `permissions: contents: write` | Add at workflow top level |

### G3. Download and flash pre-built firmware

```bash
# Download latest release artifact
gh release download latest --repo <owner>/<repo> --pattern "*.hex" -D /tmp/fw/
ls /tmp/fw/   # confirm filename — use descriptive name, not merged.hex

# Flash (nRF54LM20DK needs --recover)
west flash --hex-file /tmp/fw/<project>-<board>-<shield>-ncs<version>.hex --recover --dev-id <SN>
```

Or use **nRF Connect for Desktop → Programmer** (no local toolchain needed).

### G4. Verify boot over UART

> **Delegate to `chsh-ag-terminal`** after flashing.

```
Task(
  subagent_type="chsh-ag-terminal",
  description="Post-flash boot verification",
  prompt="""
  Board: <confirmed board>
  Port:  <confirmed port>

  1. Reset the board
  2. Capture boot log until uart:~$ (30 s timeout)
  3. Send: wifi scan
  4. Send: wifi status
  5. Send: kernel threads
  6. Return: full output; confirm 'Booting nRF Connect SDK' and uart:~$ appeared
  """
)
```

Expected sequence in the returned log:
1. `*** Booting nRF Connect SDK` — MCUboot + firmware running
2. Module init lines — confirm sQSPI / WiFi driver loading
3. `uart:~$` — shell ready

### G5. Fix-push-verify loop

```
edit → commit → push → gh run watch → (pass) → gh release download latest → west flash → verify
                                     → (fail) → gh run view --log-failed → fix → repeat
```

Minimum loop time with caching: ~5 min. Without cache: ~20 min.

---

## Mode H — External Debug Tools via MCP (Saleae, JLink, Serial-Agent)

Use when UART logs alone aren't enough — you need protocol-level insight (SPI traces),
direct debug-probe control, or richer serial tooling. Three MCP servers bridge the gap
between AI and physical debug hardware.

> **Prerequisite**: MCP servers must be configured in `~/.hermes/config.yaml` (or the
> equivalent for Claude Code / Cursor's MCP config). See **native-mcp** skill for config
> format. The `references/mcp-debug-tools.md` file in this skill has ready-to-use config.

### H1. Saleae Logic Analyzer (protocol capture + decode)

```yaml
# ~/.hermes/config.yaml (Hermes) or claude_desktop_config.json (Claude Code)
mcp_servers:
  saleae:
    command: "npx"
    args: ["-y", "wegitor/logic-analyzer-ai-mcp"]
```

Available tool pattern: `mcp_saleae_*` — start capture, decode SPI/I2C/UART/GPIO,
export data. Use for:
- Looking at SPI flash transactions during sQSPI barrier hangs
- Verifying Wi-Fi enable/disable GPIO sequences
- Decoding UART traffic at the electrical level (baud mismatch?)

Alternatives with fewer stars but may suit specific needs:
- `AkiyukiOkayasu/saleae-logic2-automation-mcp` (★1)
- `ril3y/SaleaeMCP` (★0)

### H2. JLink Debug Probe (memory, flash, GDB)

```yaml
mcp_servers:
  jlink:
    command: "npx"
    args: ["-y", "cyj0920/jlink_mcp"]
```

Available tool pattern: `mcp_jlink_*` — flash firmware, read/write memory,
halt/resume CPU, inspect registers. Use for:
- Flashing test firmware without manual `west flash`
- Reading crash-state memory remotely
- Programming multiple boards in sequence during loop testing

Other options:
- `Klievan/jlink-mcp` (★5) — embedded probe MCP
- `es617/dbgprobe-mcp-server` (★4) — generic debug probe (JLink, CMSIS-DAP, ST-Link)
- `the78mole/jlink-ppk2-mcp` (★0) — JLink + Nordic PPK2 combo

### H3. Serial-Agent (richer UART + editor integration)

```yaml
mcp_servers:
  serial-agent:
    command: "npx"
    args: ["-y", "Rance-OwO/Serial-Agent"]
```

Available tool pattern: `mcp_serial_agent_*` — auto-scan ports, HEX TX/RX,
timestamps, auto-reconnect. Alternative to the raw `nordicsemi_uart_monitor.py`
approach, with editor integration for VSCode.

### H4. Memfault Cloud (crash analytics + OTA)

Memfault has a full REST API at `api.memfault.com` — no MCP server currently exists,
but you can either:
1. Call the REST API directly via `curl` with an API key
2. Build a lightweight MCP wrapper (20-30 lines):
```python
# memfault_mcp.py — minimal MCP server for Memfault API
import json, urllib.request
# ... wrap /api/v0/organizations, /api/v0/projects, /api/v0/events

```

> For full API reference: https://api-docs.memfault.com

### H5. Router Control (Wi-Fi test orchestration)

For testing Wi-Fi connectivity, control the testbed router via SSH:

```bash
# Merlin firmware
ssh admin@router "service restart_wireless"
ssh admin@router "arp -a"
ssh admin@router "nvram get wl0_ssid"

# OpenWRT
ssh root@router "wifi down && sleep 2 && wifi up"
ssh root@router "ubus call iwinfo scan '{'device':'wlan0'}'"
```

Router config: ASUS BE92U with Asuswrt-Merlin (current); can switch to OpenWRT
for richer `ubus`/`uci` control. See `references/mcp-debug-tools.md` for full
SSH command reference.

---

## Mode I — Physical Test Harness (Future / In-Design)

For fully automated hardware-in-the-loop testing without human hands. Covers the
physical interface layer that Mode H's MCP tools don't provide.

**Concept**: A controller (ESP32-S3) exposes a REST API + MCP endpoint for:\n- `/press/{fixture}/{button}` — energize stepper/solenoid to physically press a button\n- `/read/{fixture}/{led}` — read LED state via TCS34725 RGB sensor or LA probe\n- `/reset/{fixture}` — trigger board reset\n\n**Architecture**: AI-agnostic — HTTP REST API usable by Claude Code, Cursor, Cline,\nor any MCP-capable agent. The EEDP controller is a standalone service.\n\n**Full reference**: See **chsh-sk-eedp** skill for architecture, component selection,\nBOM, software design, and MCP integration patterns.

---

## Related Skills and Resources

| Task | Go to |
|------|-------|
| Serial terminal — capture logs, send commands, loop test | `chsh-ag-terminal` subagent |
| First-time NCS setup | `chsh-sk-ncs-env` |
| Optimize RAM/Flash | `chsh-sk-ncs-memory` || Review expected module behavior from spec | `chsh-sk-ncs-spec` || QA and test reporting | `chsh-sk-ncs-test` |
| Commit after fixing | `chsh-sk-git` |
| Tag and publish a release after CI passes | `chsh-sk-git-release` |
| Migrate to a newer NCS version | `chsh-sk-ncs-migrate` |
| Physical button/LED test automation | `references/eedp-gpio-shell-approach.md` — zero-firmware GPIO shell approach |
| mcp.nrflow tools reference | wiki: `mcp-nrflow-tools` |
| General embedded debug patterns | wiki: `embedded-system-general-debugging` |
| CI/CD for NCS firmware | wiki: `github-actions-ncs-ci` |
