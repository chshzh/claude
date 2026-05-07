---
name: chsh-dev-ncs-debug
description: Debug NCS/Zephyr applications on Nordic hardware. Covers UART log capture, board reset, multi-device comparison testing, loop testing for stability validation, barrier/co-processor debugging, and crash analysis. Use when debugging a boot failure, WiFi issue, driver crash, kernel hang, or any firmware reliability problem on nRF devices. For initial environment setup use chsh-dev-ncs-env first.
---

# chsh-dev-ncs-debug — NCS Embedded Debugging Workflow

Systematic debugging for NCS/Zephyr applications on Nordic hardware.
Covers the full range from simple UART log capture to co-processor barrier synchronization.

> **Prerequisite**: A working NCS toolchain (`nrfutil sdk-manager`) and at least one connected Nordic board.
> If not set up, run **chsh-dev-ncs-env** first.

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
- **UART port**: correct VCOM (e.g., UART30 = VCOM0 on nRF54LM20DK with nRF7002 EB-II sQSPI shield)
- **Second reference board** (if available): same HW with known-good firmware

> **Rule**: If you have two boards, assign one as **reference** (known-good firmware)
> and one as **test** (under investigation). Compare their logs at the first divergence.

---

## Step 1 — Choose Debug Mode

| Symptom | Mode |
|---------|------|
| Boot hangs, no UART output | A — Serial capture + reset |
| WiFi fails, drivers misbehave | B — UART logs + loop test |
| Crash / hardfault | C — Core dump / GDB |
| Co-processor / VPR hang | D — Barrier debugging |
| Memory overflow, stack corruption | E — Memory analysis → use **chsh-dev-ncs-memory** |
| Intermittent failure (not always) | F — Loop test |
| CI build fails / pre-built firmware broken | G — GitHub Actions monitoring + firmware test |

---

## Mode A — Serial Capture and Reset

### A1. Connect to UART

**Preferred — always use `nordicsemi_uart_monitor.py`**:
```bash
# Load the script via mcp.nrflow:
# call mcp_nrflow_nordicsemi_workflow_ncs, read the nordicsemi_uart_monitor.py resource
python3 nordicsemi_uart_monitor.py --port /dev/cu.usbmodem... --baud 115200
```
This script handles reconnection, timestamps, and log capture automatically.

**Manual fallback** (only if mcp resource is unavailable):
```python
import serial
ser = serial.Serial("/dev/cu.usbmodem...", 115200, rtscts=True)  # rtscts for HWFC
```

> **nRF54LM20DK note**: UART30 = VCOM0 (Port 0, P0.6/P0.7). Run `nrfutil device list` to see
> all VCOM assignments. UART30 requires `hw-flow-control` in DTS and `rtscts=True` in Python.

### A2. Reset and capture boot log

```bash
nrfutil device reset --serial-number <SN>
```

Capture output for at least 10 seconds. Key things to look for:
- `*** Booting nRF Connect SDK` — firmware is running
- Module init log lines — which modules started
- First error line — what failed and when in boot sequence
- `uart:~$` — shell is available

### A3. Compare logs between reference and test boards

If a reference board is available:
1. Reset both boards simultaneously
2. Capture boot logs from both serial ports
3. Find the first line that differs — that is the failure boundary

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

```bash
uart:~$ wifi scan
uart:~$ wifi connect -s <SSID> -k 1 -p <password>
uart:~$ wifi status
uart:~$ net iface
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

### F1. Use the loop test template

A production-ready template lives in this skill at:
```
~/.claude/skills/chsh-dev-ncs-debug/scripts/loop_test.py
```

Copy it to your app, edit the constants at the top, then run:
```bash
python3 <app>/loop_test.py 10      # 10 iterations
python3 <app>/loop_test.py 20      # 20 for release
```

The script:
1. Resets the board via `nrfutil device reset`
2. Opens serial with HWFC (`rtscts=True`)
3. Waits for shell prompt (`uart:~$`)
4. Sends `wifi connect` command
5. Waits for `Connected`
6. Verifies IP via `wifi status`
7. Reports pass/fail per iteration + summary

### F2. Script customization

Edit the constants at the top of `loop_test.py`:

```python
SERIAL_PORT = "/dev/cu.usbmodem..."
BAUD = 115200
BOARD_SERIAL = "1051869687"
SSID = "YOUR_SSID"
PSK = "YOUR_PASSWORD"
KEY_MGMT = 1       # 1=WPA2-PSK, 2=WPA3-SAE
ITERATIONS = 10    # default, overrideable via argv
BOOT_TIMEOUT = 30
CONNECT_TIMEOUT = 30
```

Override iterations from command line:
```bash
python3 loop_test.py 5    # quick smoke test
python3 loop_test.py 20   # release validation
```

### F3. Multi-device loop test

To test both reference and test boards in the same loop:

```python
# Run loop_test.py on both boards in parallel
import subprocess, sys

ref = subprocess.Popen(["python3", "loop_test.py", "10",
                        "--port", "/dev/cu.ref_port",
                        "--serial", "ref_sn"])
test = subprocess.Popen(["python3", "loop_test.py", "10",
                         "--port", "/dev/cu.test_port",
                         "--serial", "test_sn"])
ref.wait(); test.wait()
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

# Flash
west flash --hex-file /tmp/fw/merged.hex --recover --dev-id <SN>
```

Or use **nRF Connect for Desktop → Programmer** (no local toolchain needed).

### G4. Verify boot over UART

```bash
# Always use nordicsemi_uart_monitor.py (prefer over raw serial)
python3 nordicsemi_uart_monitor.py --port /dev/cu.usbmodem... --baud 115200
```

Expected sequence:
1. `*** Booting nRF Connect SDK` — MCUboot + firmware running
2. Module init lines — confirm sQSPI / WiFi driver loading
3. `uart:~$` — shell ready

Quick functional check:
```sh
uart:~$ wifi scan
uart:~$ wifi status
uart:~$ kernel threads
```

### G5. Fix-push-verify loop

```
edit → commit → push → gh run watch → (pass) → gh release download latest → west flash → verify
                                     → (fail) → gh run view --log-failed → fix → repeat
```

Minimum loop time with caching: ~5 min. Without cache: ~20 min.

---

## Related Skills and Resources

| Task | Go to |
|------|-------|
| First-time NCS setup | `chsh-dev-ncs-env` |
| Optimize RAM/Flash | `chsh-dev-ncs-memory` |
| QA and test reporting | `chsh-qa-ncs-test` |
| Commit after fixing | `chsh-dev-git-commit` |
| mcp.nrflow tools reference | wiki: `mcp-nrflow-tools` |
| General embedded debug patterns | wiki: `embedded-system-general-debugging` |
| CI/CD for NCS firmware | wiki: `github-actions-ncs-ci` |
