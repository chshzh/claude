---
name: chsh-sk-ncs-3.2-debug
description: Use when debugging a boot failure, WiFi issue, driver crash, kernel hang, or any firmware reliability problem on nRF devices. Covers UART log capture, board reset, multi-device comparison, loop testing, co-processor debugging, and crash analysis. For environment setup, use chsh-sk-ncs-env first.
---

# chsh-sk-ncs-3.2-debug — NCS Embedded Debugging Workflow

Systematic debugging for NCS/Zephyr applications on Nordic hardware.
Covers the full range from simple UART log capture to co-processor barrier synchronization.

> **Prerequisite**: A working NCS toolchain (`nrfutil sdk-manager`) and at least one connected Nordic board.
> If not set up, run **chsh-sk-ncs-env** first.

> **Knowledge sources**: Always call `mcp_nordic-mcp_nordicsemi_workflow_ncs` at the start
> of any debug session to load the `nrfutil-manual`, `nordicsemi_uart_monitor.py`, and
> `embedded-code-guidance-ncs-zephyr` resources into context.

---

## Debug Tool Inventory

| Tool | Layer | When to use |
|------|-------|-------------|
| **Serial Terminal** | Logs | Default — always first; captures boot log + shell commands |
| **addr2line** | Crash | Decode PC/LR from fault dump to source file + line |
| **GDB** | Crash / Memory | Backtrace, local variables, memory inspection |
| **Thread Analyzer** | Memory | Stack usage per thread; overflow detection |
| **Saleae Logic Analyzer** | Protocol | SPI/I2C/UART bus capture and timing analysis |
| **JLink / Debug Probe** | Probe | Flash, halt CPU, read/write memory registers |
| **Router Control** | Network | AP restart, SSID block, reconnect simulation |
| **PPK2** | Power | Current measurement for power budget |
| **nrfutil device** | Flash/Reset | Flash firmware, reset board, enumerate VCOMs |
| **GitHub Actions** | CI | Monitor CI builds, download pre-built `.hex` |

Advanced tools (Saleae, JLink, Router, PPK2) are documented in
[`references/mcp-debug-tools.md`](references/mcp-debug-tools.md) and
[`references/eedp-platform.md`](references/eedp-platform.md).

---

## Default Debugging Workflow

**Always start with the serial terminal.** Most bugs are visible in boot/runtime logs
without any additional tooling.

```
Issue reported
      │
      ▼
[Step 0] Baseline — confirm board, VCOM port, NCS version
      │
      ▼
[Mode A] Serial capture + reset — read boot log
      │
      ├── WiFi fails / driver errors in log ──→ Mode B (verbose Kconfig + shell commands)
      ├── Crash / USAGE FAULT in log ─────────→ Mode C (addr2line, GDB)
      ├── Co-processor / sQSPI hang ──────────→ Mode D (barrier debug)
      ├── Intermittent failure ───────────────→ Mode F (loop test)
      ├── Stack overflow / OOM ──────────────→ chsh-sk-ncs-3.3-memopt
      │
      └── UART logs insufficient?
            ├── Protocol timing (SPI/I2C)  → Saleae MCP
            ├── Memory at crash point      → JLink MCP
            ├── Network / AP simulation    → chsh-sk-router-control
            └── Full reference: references/mcp-debug-tools.md
```

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
- **UART port**: correct VCOM — see table below:

  | Board config | Log VCOM | Port suffix | rtscts | Notes |
  |---|---|---|---|---|
  | nRF7002DK | VCOM1 | `...3` e.g. `/dev/tty.usbmodem0010507879623` | False | Application logs on VCOM1; VCOM0 is NET core |
  | nRF54LM20DK + nRF7002EB2 | VCOM0 | `...1` e.g. `/dev/tty.usbmodem0010518067141` | True | Shield overlay disables UART20; app logs on UART30/VCOM0 |
  | nRF54LM20DK (no shield) | VCOM1 | `...3` e.g. `/dev/tty.usbmodem0010518067143` | False | UART20/VCOM1 is the application log port |
  | nRF5340 Audio DK + nRF7002EK | VCOM0 | `...1` e.g. `/dev/tty.usbmodem0010501119811` | False | Application logs on VCOM0; PCA10121 board |

  Port suffix pattern: VCOM0 → last digit `1`, VCOM1 → last digit `3`.
- **Second reference board** (if available): same HW with known-good firmware

> **Rule**: If you have two boards, assign one as **reference** (known-good firmware)
> and one as **test** (under investigation). Compare their logs at the first divergence.

### Step 0.5 — Resolve Test Target (Required Before Any Serial Action)

Before invoking `chsh-ag-terminal` for any serial operation, the test target must be
unambiguously known. If the board and port are not already clear from the conversation
context, use `AskQuestion` to confirm:

```
Board options to present:
  A) nRF54LM20DK+nRF7002EB2  — VCOM0 (suffix ...1, rtscts=True)  e.g. /dev/tty.usbmodem0010518067141
  B) nRF7002DK               — VCOM1 (suffix ...3, rtscts=False) e.g. /dev/tty.usbmodem0010507879623
  C) nRF54LM20DK (no shield) — VCOM1 (suffix ...3, rtscts=False) e.g. /dev/tty.usbmodem0010518067143
  D) nRF5340 Audio DK + nRF7002EK — VCOM0 (suffix ...1, rtscts=False) e.g. /dev/tty.usbmodem0010501119811
  E) Other                   — specify board name and port

Pre-filled default when context is silent: option A.
```

Only proceed with serial operations once **both** Board and Port are confirmed.
Do not guess the port — a wrong port produces misleading output silently.

### Step 0.6 — Developer Monitoring Setup (Required)

Before any serial operation, create a session log file and tell the developer how to watch it live.

**1. Create the session log (run via Shell tool):**

```bash
NCS_LOG=/tmp/ncs_debug_$(date +%Y%m%d_%H%M%S).log
touch "$NCS_LOG"
echo "Session log: $NCS_LOG"
```

**2. Print this block in your chat response so the developer can open it:**

> **Live monitoring:** Open a new terminal tab and run:
> ```bash
> tail -f /tmp/ncs_debug_<timestamp>.log
> ```
> All serial output streams here as the agent captures it.

**3. Pass `NCS_LOG` to every `chsh-ag-terminal` prompt** via the `Log file:` field.
The subagent appends each received line (with timestamp) to the file as it arrives —
not in a batch at the end.

**4. Announce each shell command before sending it.**
Before each `Task(chsh-ag-terminal, ...)` call, write one line in your chat response:

> `→ Board: wifi connect -s MySSID -k 1`

This lets the developer correlate chat progress with what appears in `tail -f`.

### Step 0.7 — Check Port Availability (Required Before Any Serial Operation)

Before invoking `chsh-ag-terminal`, verify the target port is free.
A port held by another process causes a silent failure or misleading output.

**Run via Shell tool:**

```bash
PORT="<confirmed port from Step 0.5>"
if lsof "$PORT" 2>/dev/null | grep -q .; then
    echo "BUSY"
    lsof "$PORT" 2>/dev/null | awk 'NR>1 {printf "  process: %s  pid: %s\n", $1, $2}'
else
    echo "FREE"
fi
```

**If result is BUSY — stop and show this reminder in your chat response:**

---

> **Serial port occupied** — `<PORT>` is currently held by **`<process>`** (PID `<pid>`).
>
> This is typically your serial terminal (screen, minicom, nRF Terminal, nRF Connect for Desktop, or similar).
> The agent cannot connect until it is released.
>
> **Please:**
> 1. Disconnect or close your serial terminal application
> 2. Reply here when done — I will re-check and continue
>
> _Tip: If you want to keep monitoring logs while I work, see Step 0.6 — I write all output to a log file you can `tail -f` instead._

---

Do NOT invoke `chsh-ag-terminal` until the developer confirms. Then re-run the `lsof`
check to confirm the port is now FREE before proceeding.

**If result is FREE** — proceed to Step 1.

---

## Step 1 — Choose Debug Mode

| Symptom | Mode | Primary Tool |
|---------|------|--------------|
| Boot hangs, no UART output | A — Serial capture + reset | `chsh-ag-terminal` |
| WiFi fails, drivers misbehave | B — UART logs + WiFi debug | `chsh-ag-terminal` |
| Crash / hardfault in log | C — Crash analysis | `addr2line`, GDB |
| Co-processor / VPR hang | D — Barrier debugging | UART + instrumentation |
| Stack overflow / memory corruption | → **chsh-sk-ncs-3.3-memopt** | Thread Analyzer |
| Intermittent failure | F — Loop test | `chsh-ag-terminal` |
| CI build fails / pre-built broken | → [`references/github-actions-debug.md`](references/github-actions-debug.md) | `gh` CLI |
| Protocol-level insight (SPI, I2C) | → [`references/mcp-debug-tools.md`](references/mcp-debug-tools.md) | Saleae MCP |
| Memfault logs missing from cloud | → **chsh-sk-ncs-tc-memfault-log-debug** | UART + Memfault API |
| Automated HIL (buttons/LEDs) | → [`references/eedp-platform.md`](references/eedp-platform.md) | EEDP / GPIO Shell |

---

## Mode A — Serial Capture and Reset

> **Serial operations in this mode are delegated to `chsh-ag-terminal`.**
> Resolve the test target (Step 0.5) first, then invoke the subagent.

### A1. Connect to UART and capture boot log

Announce in chat before delegating: `→ Board: reset + capture boot log`

Delegate to `chsh-ag-terminal` with the confirmed test target:

```
Task(
  subagent_type="chsh-ag-terminal",
  description="Capture boot log",
  prompt="""
  Board:    <confirmed board>
  Port:     <confirmed port>
  Log file: <NCS_LOG from Step 0.6>

  1. Reset the board via nrfutil device reset
  2. Capture serial output until uart:~$ appears (30 s timeout)
  3. Append each received line (with [HH:MM:SS.mmm] timestamp) to Log file as it arrives
  4. Return: full boot log + first error line if any
  """
)
```

The subagent handles port selection, HWFC (rtscts), reconnection, and timestamping
internally. Do not run pyserial or nordicsemi_uart_monitor.py directly.

> **Board UART/VCOM reference**: see the table in Step 0. The subagent applies the correct VCOM and rtscts automatically.

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

### A4. Alternative — Capture directly in the Cursor terminal (short sessions)

When you want serial output to appear in the **Cursor terminal itself** — without
delegating to a subagent — use the Shell tool with this Python snippet.
Output is visible to the developer live and also written to the session log.

```python
# Run via Shell tool (block_until_ms: 0 to background; Ctrl+C or kill to stop)
python3 - <<'EOF'
import serial, sys, datetime, signal

PORT   = "/dev/tty.usbmodemXXXXX"   # replace with confirmed port
BAUD   = 115200
RTSCTS = False   # True for nRF54LM20DK + nRF7002EB2
LOG    = "/tmp/ncs_debug.log"        # replace with NCS_LOG from Step 0.6

signal.signal(signal.SIGINT, lambda *_: sys.exit(0))

with serial.Serial(PORT, BAUD, rtscts=RTSCTS, timeout=1) as s, open(LOG, "a") as f:
    print(f"[monitor] {PORT} @ {BAUD} rtscts={RTSCTS} → {LOG}", flush=True)
    while True:
        line = s.readline().decode("utf-8", errors="replace").rstrip()
        if line:
            ts  = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]
            out = f"[{ts}] {line}"
            print(out, flush=True)
            f.write(out + "\n")
            f.flush()
EOF
```

> Use `block_until_ms: 0` so the shell stays live while you work. Stop it with a
> kill command once the capture window is done.

---

## Mode B — UART Logs and WiFi Debugging

See [`references/wifi-debug.md`](references/wifi-debug.md) for verbose Kconfig, task template, common error patterns, and reconnect stability test (B4).

> **Quick tip**: `FF:FF:FF:FF:FF:FF` MAC → add `CONFIG_WIFI_RANDOM_MAC_ADDRESS=y`. USAGE FAULT during WiFi reconnect → `CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=6144` (see [crash-analysis.md C4–C5](references/crash-analysis.md)).

---

## Mode C — Crash and Hardfault Analysis

See [`references/crash-analysis.md`](references/crash-analysis.md) for the full workflow (C1–C5): enable crash logging, decode with addr2line, GDB attach, hidden-fault-dump workaround, thread identification from PSP.

> **Quick tip**: Always use the Zephyr SDK `arm-zephyr-eabi-addr2line` (not `arm-none-eabi-addr2line`). Enable `CONFIG_LOG_MODE_IMMEDIATE=y` to surface hidden fault dumps.

---

## Mode D — Co-processor / VPR Barrier Debugging

For sQSPI / MSPI soft peripheral barrier hangs on nRF54L series.
See [`references/barrier-debug.md`](references/barrier-debug.md) for the full workflow (D1–D4).

> **Quick tip**: Never call `__SSB` in the abort path — if VPR is stuck, SSB also hangs. Use `nrf_qspi2_core_disable()` instead.

---

## Mode F — Loop Testing for Stability

See [`references/loop-testing.md`](references/loop-testing.md) for the full task template, iteration counts, and multi-device test setup.

> **Quick tip**: Use `run_in_background=True` for the subagent task. 10 iterations = acceptance gate; 20 = release gate.

---

## Mode J — Memfault Log Debug

See **chsh-sk-ncs-tc-memfault-log-debug** for diagnosing why Memfault log files
are missing from the cloud (rate limiting, CBOR corruption, stale triggered state,
`memfault_log_trigger_collection()` call site issues).

Quick check: Memfault UI → Device page → Developer Mode tab → Processing Errors.
If logs are silently dropped, enable **Server-Side Developer Mode** on the Device page.

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

# Flash (standard — preserves NVS and WiFi credentials)
nrfutil sdk-manager toolchain launch --ncs-version=v3.3.0 -- \
  west flash -d <app>/build --dev-id <SN>

# Flash (first-time only, or when AP protection blocks access)
# WARNING: --recover does a full chip erase — wipes NVS including WiFi credentials
nrfutil sdk-manager toolchain launch --ncs-version=v3.3.0 -- \
  west flash -d <app>/build --recover --dev-id <SN>

# Reset only
nrfutil device reset --serial-number <SN>

# List devices
nrfutil device list
```

---

## Mode G — GitHub Actions and Pre-Built Firmware

For CI monitoring, firmware download, and the fix-push-verify loop — see
[`references/github-actions-debug.md`](references/github-actions-debug.md).

---

## Mode H — External Debug Tools via MCP

When UART logs alone are insufficient, use MCP-controlled hardware tools.
See [`references/mcp-debug-tools.md`](references/mcp-debug-tools.md) for:
- **Saleae**: SPI/I2C/UART bus capture and protocol decode
- **JLink**: flash, halt CPU, read/write memory
- **Router Control**: AP simulation, SSID block (see also `chsh-sk-router-control`)

---

## Mode I — Physical Test Harness

For automated HIL testing (button presses, LED state, board reset) via ESP32-S3
REST API + MCP — see [`references/eedp-platform.md`](references/eedp-platform.md).

---

## Related Skills and Resources

| Task | Go to |
|------|-------|
| Serial terminal — capture logs, send commands, loop test | `chsh-ag-terminal` subagent |
| First-time NCS setup | `chsh-sk-ncs-env` |
| Optimize RAM/Flash | `chsh-sk-ncs-3.3-memopt` || Review expected module behavior from spec | `chsh-sk-ncs-2-spec` || Phase 4 Verification & Test (static + EEDP hardware) | `chsh-sk-ncs-4.1-verification` |
| EEDP platform setup + all modules | [`references/eedp-platform.md`](references/eedp-platform.md) · wiki: `eedp-platform` |
| Commit after fixing | `chsh-sk-git-commit` |
| Tag and publish a release after CI passes | `chsh-sk-git-release` |
| Migrate to a newer NCS version | `chsh-sk-ncs-migrate` |
| Physical button/LED test automation | `references/eedp-gpio-shell-approach.md` — zero-firmware GPIO shell approach |
| mcp.nordic-mcp tools reference | wiki: `mcp-nrflow-tools` |
| Decode Memfault crash trace (stacktrace + logs) | `mcp_memfault_trace_get` with `include_logs: true` |
| Check device reboot history on Memfault | `mcp_memfault_device_listReboots` |
| General embedded debug patterns | wiki: `embedded-system-general-debugging` |
| CI/CD for NCS firmware | wiki: `github-actions-ncs-ci` |

---

## Gotchas

- **DNS-SD -EAGAIN race (mDNS additional records)**: mDNS PTR responses include the A record as an additional record arriving ~100–200µs after the instance-name callback. If code calls `dns_get_addr_info` unconditionally after receiving the instance name, it gets `-EAGAIN (-11)` because the prior query slot is still occupied by pending callbacks. Symptom: headset log shows `A record: x.x.x.x` followed immediately by `DNS-SD discovery attempt N failed (err -11)` on all 3 retries. Fix: `k_sleep(K_MSEC(50))` after instance name to drain remaining callbacks, then guard the A-record query with `if (!ctx.addr_received)`.

- **Identifying gateway vs headset role at runtime**: If both boards are already past boot, `nrfutil device list` does not indicate role. Reset both boards simultaneously (parallel threads) and read ~15 lines each — the `fw_info: Compiled as AUDIO GATEWAY / HEADSET LEFT` line appears at ~460–470ms into boot on nRF5340 Audio DK.

## Self-Update Policy

At the **end of each conversation**, review what was discovered and check
whether any facts in this skill are new, corrected, or outdated (e.g. new
debug modes, EEDP module changes, UART port assignments, new crash patterns,
or MCP tool updates).

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
