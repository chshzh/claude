---
name: chsh-sk-ncs-3.2-debug
description: Debug NCS/Zephyr applications on Nordic hardware. Covers UART log capture, board reset, multi-device comparison testing, loop testing for stability validation, barrier/co-processor debugging, and crash analysis. Use when debugging a boot failure, WiFi issue, driver crash, kernel hang, or any firmware reliability problem on nRF devices. For initial environment setup use chsh-sk-ncs-env first.
---

# chsh-sk-ncs-3.2-debug — NCS Embedded Debugging Workflow

Systematic debugging for NCS/Zephyr applications on Nordic hardware.
Covers the full range from simple UART log capture to co-processor barrier synchronization.

> **Prerequisite**: A working NCS toolchain (`nrfutil sdk-manager`) and at least one connected Nordic board.
> If not set up, run **chsh-sk-ncs-env** first.

> **Knowledge sources**: Always call `mcp_nrflow_nordicsemi_workflow_ncs` at the start
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

  | Board config | Log VCOM | Port suffix | rtscts |
  |---|---|---|---|
  | nRF7002DK | VCOM1 | `...3` e.g. `/dev/tty.usbmodem0010507879623` | False |
  | nRF54LM20DK + nRF7002EB2 | VCOM0 | `...1` e.g. `/dev/tty.usbmodem0010518067141` | True |
  | nRF54LM20DK (no shield) | VCOM1 | `...3` e.g. `/dev/tty.usbmodem0010518067143` | False |
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
  D) Other                   — specify board name and port

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

> **Board UART/VCOM reference** (for context — the subagent applies this automatically):
>
> | Board config | Log VCOM | macOS port suffix | rtscts | Notes |
> |---|---|---|---|---|
> | nRF54LM20DK + nRF7002EB2 | VCOM0 | `...1` | True | Shield overlay disables UART20; app logs on UART30/VCOM0 |
> | nRF54LM20DK (no shield) | VCOM1 | `...3` | False | UART20/VCOM1 is the application log port |
> | nRF7002DK | VCOM1 | `...3` | False | Application logs on VCOM1; VCOM0 is NET core |
>
> Port suffix pattern: VCOM0 → last digit `1`, VCOM1 → last digit `3`.

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

Announce each step in chat before the Task() call, e.g.:
> `→ Board: reset → wifi scan → wifi connect → wifi status → net iface`

```
Task(
  subagent_type="chsh-ag-terminal",
  description="WiFi debug — capture log + run commands",
  prompt="""
  Board:    <confirmed board>
  Port:     <confirmed port>
  Log file: <NCS_LOG from Step 0.6>

  Append each received line (with [HH:MM:SS.mmm] timestamp) to Log file as it arrives.

  1. Reset the board, capture boot log until uart:~$
  2. Send: wifi scan       (wait for scan results, max 15 s)
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
| USAGE FAULT / silent reboot during WiFi reconnect | `sysworkq` stack overflow in reconnect path | Increase `CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE` to ≥ 6144; see Mode C4–C5 |

### B4. Test: WiFi credentials stored but AP not available

This scenario validates reconnect stability when a provisioned AP is unreachable (powered
off, out of range, SSID changed). It is **required before release** for any WiFi
provisioning app — the reconnect path uses significantly more sysworkq stack than
steady-state operation and will not be caught by the standard Happy Path test or by
Thread Analyzer during normal connected operation.

**Setup:**
1. Provision the device with valid WiFi credentials.
2. Power off the AP (or rename its SSID to something non-existent).
3. Reboot the device — it will boot, find stored credentials, and attempt to connect.

**Expected behaviour:**
1. Connection times out: `[WiFi] Reason: Connection timed out (-ETIMEDOUT)`
2. Retry is scheduled: `scheduling retry` / `attempting to connect`
3. **No USAGE FAULT** — device continues running through the retry loop.
4. When the AP is restored, device connects and resumes normal operation.

**Failure signature (sysworkq stack overflow):**
```
***** USAGE FAULT *****  Stack overflow (context area not valid)
  Faulting instruction address (r15/pc): 0x...  ← decodes to wpa_cli_cmd
  RESETREAS: 0x00000040  ← SREQ (software reset by fault handler)
```

See Mode C4–C5 for the full diagnosis workflow. Fix: `CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=6144`.

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

### C4. Stack overflow via "hidden fault dump" pattern

Zephyr's default `CONFIG_LOG_MODE_DEFERRED=y` queues log messages. When a USAGE FAULT
(stack overflow) triggers a software reset at full queue capacity, the fault dump is
dropped and the device reboots silently. Symptoms:

- Log shows `--- N messages dropped ---` immediately before the reboot line
- `RESETREAS: 0x00000040` — SREQ (software reset by `NVIC_SystemReset()` in fault handler)
- `Thread Analyzer` shows peak usage near or at the configured stack size

**Workaround:** switch to immediate mode to make the fault dump print synchronously:

```kconfig
CONFIG_LOG_MODE_DEFERRED=n
CONFIG_LOG_MODE_IMMEDIATE=y
# Immediate mode causes each log call to process in the calling thread, which
# significantly increases stack depth. Increase thread_analyzer auto stack:
CONFIG_THREAD_ANALYZER_AUTO_STACK_SIZE=4096
```

Rebuild and flash. The next fault will print the full register dump including PC, LR,
PSP, and RESETREAS before the reboot. Revert both changes after the fault is diagnosed.

### C5. Decode fault dump — addr2line and thread identification

**Always use the Zephyr SDK `addr2line` — NOT `arm-none-eabi-addr2line`:**

```bash
# NCS v3.3.0 macOS toolchain path
ADDR2LINE="/opt/nordic/ncs/toolchains/0c0f19d91c/opt/zephyr-sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-addr2line"
ELF="<app>/build_<board>/zephyr/zephyr.elf"

# Decode faulting instruction (PC) and calling function (LR)
$ADDR2LINE -e $ELF -f -p <PC_HEX>
$ADDR2LINE -e $ELF -f -p <LR_HEX>
```

**Identify crashing thread from PSP** using the map file:

```python
import re

MAP = "<app>/build_<board>/zephyr/zephyr.map"
PSP = 0x<psp_from_fault_dump>          # e.g. 0x20050570

stacks = []
with open(MAP) as f:
    for line in f:
        m = re.search(r'(0x[0-9a-f]+)\s+(0x[0-9a-f]+)\s+(.+)', line)
        if m:
            addr, size = int(m.group(1), 16), int(m.group(2), 16)
            if 0x20000000 <= addr <= 0x20100000 and size > 0:
                stacks.append((addr, size, m.group(3).strip()))

for addr, size, name in sorted(stacks):
    if addr <= PSP <= addr + size:
        print(f"MATCH: 0x{addr:08x}+0x{size:x}=0x{addr+size:08x}: {name}")
```

**Key register meanings:**

| Register | Meaning |
|----------|---------|
| `r15/pc` | Faulting instruction — decode with addr2line |
| `r14/lr` | Return address in caller — decode with addr2line |
| `psp` | Stack pointer at fault — use map script to identify thread |
| `EXC_RETURN=0xffffffed` | Thread mode, PSP, extended FP frame (~104 bytes stacked) |
| `RESETREAS=0x00000040` | SREQ — fault handler called `sys_reboot()` |

**Common Wi-Fi reconnect stack overflow (sysworkq):**

The reconnect path (`wpa_cli_cmd_remove_network`) runs on `sysworkq` and allocates
large on-stack buffers deep in the call chain:

| Frame | Stack allocation |
|-------|-----------------|
| `wpa_cli_cmd` (`wpa_cli_cmds.c`) | `buf[1024]` (CMD_BUF_LEN) |
| `_wpa_ctrl_command` (`wpa_cli_zephyr.c`) | `buf[512]` (CMD_BUF_LEN) |
| `zvfs_poll_internal` | `poll_events[CONFIG_ZVFS_POLL_MAX=20]` ≈ 400 B |
| Call overhead + saved regs | ~552 B |
| **Total peak (nRF54LM20DK)** | **~4488 B** |

Fix: `CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=6144` (formula: floor(4488/0.9)=4988 minimum).

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

Before launching, tell the developer:
> **Loop test starting — monitor progress live:**
> ```bash
> tail -f <NCS_LOG>           # serial output stream
> tail -f /tmp/loop_test_results.txt   # per-iteration pass/fail summary
> ```

```
Task(
  subagent_type="chsh-ag-terminal",
  description="Loop test — N iterations",
  run_in_background=True,   # loop tests are long; keep working while it runs
  prompt="""
  Board:    <confirmed board>
  Port:     <confirmed port>
  Log file: <NCS_LOG from Step 0.6>
  SSID:     <ssid>
  PSK:      <password>
  KEY_MGMT: 1   # 1=WPA2-PSK, 3=WPA3-SAE

  Run a loop test with <N> iterations (10 = acceptance, 20 = release).
  For each iteration:
    - Reset board, wait for uart:~$, send wifi connect, verify Connected + IP
    - Append each received serial line (timestamped) to Log file as it arrives
    - Append iteration result line to /tmp/loop_test_results.txt immediately after each iteration:
        [PASS] iter=N  time=Xs  ip=A.B.C.D
        [FAIL] iter=N  time=Xs  reason=<first error line>
  Return: pass rate summary (X/N passed) and any failure details.
  """
)
```

After the task completes, read `/tmp/loop_test_results.txt` for the full per-iteration log.

> **HWFC reference** (applied automatically by the subagent):
> nRF54LM20DK+nRF7002EB2: VCOM0/suffix `...1` → `rtscts=True` (app logs). UART20/VCOM1 → disabled.
> nRF7002DK: VCOM1/suffix `...3` → `rtscts=False` (app logs).
> nRF54LM20DK (no shield): VCOM1/suffix `...3` → `rtscts=False` (app logs).

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
| Commit after fixing | `chsh-sk-git` |
| Tag and publish a release after CI passes | `chsh-sk-git-release` |
| Migrate to a newer NCS version | `chsh-sk-ncs-migrate` |
| Physical button/LED test automation | `references/eedp-gpio-shell-approach.md` — zero-firmware GPIO shell approach |
| mcp.nrflow tools reference | wiki: `mcp-nrflow-tools` |
| General embedded debug patterns | wiki: `embedded-system-general-debugging` |
| CI/CD for NCS firmware | wiki: `github-actions-ncs-ci` |

---

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
