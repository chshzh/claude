---
title: Embedded System General Debugging
created: 2026-05-07
updated: 2026-05-07v2
type: concept
tags: [debugging, embedded, ncs, zephyr, vpr, spi, mspi, uart]
sources: []
confidence: high
---

# Embedded System General Debugging

Lessons distilled from a multi-session sQSPI/MSPI bus driver debugging effort on
nRF54LM20DK + nRF7002 EB-II (NCS v3.3.0). The session started from zero (no MSPI
bus backend existed) and ended with a stable 32 MHz Quad-SPI connection that passes
20/20 consecutive WiFi connect cycles.

See also: [ncs-app-versioning](ncs-app-versioning.md), [mcp-nrflow-tools](mcp-nrflow-tools.md)

---

## 1. Establish a Working Baseline First

Before writing any new code, confirm that the same hardware with the known-good path
(SPI reference board) works end-to-end. Run it on a second device if needed. This
gives you a reference output to compare against, removes hardware suspicion, and
prevents wasted time debugging unfamiliar failure modes.

**Applied here:** SPI board (serial `0010518513281`) used as reference; all UART
output and WiFi connect behaviour confirmed before starting sQSPI work.

---

## 2. Reduce the Number of Variables at Once

When debugging an unknown failure, change only one thing at a time. It is tempting to
apply multiple "fixes" together, but when something stops working (or starts working),
you will not know which change caused it.

**Applied here:** Frequency reduction (24 → 8 MHz), barrier re-trigger, and DMA
timeout recovery were each tried separately before combining. The aggressive 1024-spin
periodic re-trigger caused a regression (0/10 from 4/5); stepping back revealed it was
the `__SSB` call in `nrf_sqspi_abort()` that corrupted VPR state.

---

## 3. Use a Second Device for Comparison

Having two identical setups — one working, one under test — allows you to:
- Compare UART logs byte-for-byte to find the first divergence.
- Rule out hardware damage or board-specific failure.
- Test a hypothesis on the working board first without risking the test board.

**Applied here:** Board A (SPI, VCOM0) vs Board B (sQSPI, VCOM0 via UART30). The boot log
divergence was the first observable symptom: Board A showed clean WiFi init; Board B
showed `RPU boot signature check failed` at barrier timeout.

---

## 4. Log the Right Things at the Right Verbosity

Early in debugging, maximize logging: print every barrier count, every timeout, every
retry. Once the root cause is understood, remove or gate verbose logs because they:
- Add latency in the hot path (printk in barrier handlers stalls the CPU).
- Flood the serial monitor and obscure the failure signal.
- Increase FLASH/RAM usage unnecessarily.

**Applied here:** Added `printk` with `h0`, `h1`, `tc` to every barrier timeout. After
understanding the VPR one-behind pattern, all timeout printks were removed and replaced
with a flag (`_xsb_timeout_tc`) that callers can check.

---

## 5. Read the Error — Then Read the Symptom

Error messages are proxies. The real question is: *what invariant was violated to
produce this error?*

| Message seen | Actual cause |
|---|---|
| `RPU boot signature check failed` | First CSB barrier never completed — VPR didn't process trigger |
| `Invalid memory address` | Address sent to nRF7002 was corrupted (transfer used wrong state) |
| `UMAC init timed out` | Multiple chained failures — every xfer after first timeout fails |
| `nrf_sqspi_xfer() failed: 0bad000b` (BUSY) | `transfer_in_progress` not cleared after previous timeout |

Chained failures: *once the first transfer fails, all subsequent ones fail too.* Fix
the first failure to recover the cascade.

---

## 6. Barrier / Synchronization Primitives — Key Patterns

When a host CPU communicates with a co-processor (VPR, DSP, MCU) via shared memory:

1. **Write memory before triggering** — always use `__DSB()` / `dmb` / `dsb` to ensure
   writes are architecturally committed before the trigger reaches the co-processor.
2. **Co-processor may miss events if too fast** — if the co-processor is still
   executing its event loop entry path when a trigger arrives, it can miss it. Graduated
   re-triggers (at 100K / 500K / 2M spins) are safer than a single aggressive one.
3. **Never send a second barrier while the co-processor is stuck on the first** — if the
   co-processor stalled, sending `__SSB` (stop barrier) will also stall because it
   requires the co-processor to ACK.
4. **Abort without barriers** — on timeout: disable the peripheral, clear events, reset
   flags. Do not send additional synchronization commands.

---

## 7. Loop Testing Is the Only Reliable Quality Gate

A single pass is not evidence of correctness. A hardware timing issue may fail 1-in-5
or 1-in-20 times. Use a scripted loop test:

```python
for i in range(N):
    reset_board()
    wait_for_boot()
    send_wifi_connect_command()
    check_connected()
    record_pass_or_fail()
```

Target: N = 10 minimum, N = 20 for a release claim.

**Applied here:** WiFi connected once but then only 1/10 manually. The scripted loop
test exposed 4/5 → 0/10 → 5/5 → 10/10 regressions and fixes in rapid succession.
No scripted loop = no reliable quality signal.

---

## 8. Read Hardware Manuals for Co-processor Interfaces

The VPR/FLPR in nRF54L series is a RISC-V VPR (Vector Processing Resource). Its task
trigger registers are write-only; the host cannot read back whether the trigger was
consumed. The handshake register pair (`handshake[0]`, `handshake[1]`) is the only
shared-memory acknowledgement mechanism. Reading the PS (Product Specification) section
for VPR and the sQSPI application note before writing code would have avoided ~4
sessions of barrier-hang debugging.

---

## 9. Firmware + Driver Co-development Checklist

When adding a new bus backend to an existing driver stack:

- [ ] Check the existing backends (spis_if.c) to understand the full API surface required
- [ ] Verify register addresses in the device tree match the physical hardware
- [ ] Confirm memory region (softperipheral_ram) does not overlap linker sections
- [ ] Verify UART console VCOM number — wrong VCOM = no output, looks like board crash
- [ ] Test with minimum frequency first, then increase
- [ ] Run loop test at each frequency milestone before claiming stability

---

## 10. UART Connection — Always Use nordicsemi_uart_monitor.py

**Preferred**: Use `nordicsemi_uart_monitor.py` from the `nordicsemi_uart_monitor.py`
resource (loaded via `mcp_nrflow_nordicsemi_workflow_ncs`). It handles reconnection,
timestamps, and log capture without manual serial port management.

```bash
python3 nordicsemi_uart_monitor.py --port /dev/cu.usbmodem... --baud 115200
```

**Fallback** (manual Python serial) — only when mcp resource is unavailable:

```python
import serial, subprocess, time, re

def reset(serial_number):
    subprocess.run(["nrfutil", "device", "reset", "--serial-number", serial_number])

def read_until(ser, pattern, timeout):
    end = time.time() + timeout
    buf = ""
    while time.time() < end:
        chunk = ser.read(4096)
        if chunk:
            buf += chunk.decode("utf-8", errors="replace")
        if re.search(pattern, buf):
            return True, buf
    return False, buf

# For each iteration:
# 1. reset_board()
# 2. open serial (rtscts=True if hw flow control)
# 3. read_until(ser, r"uart:~\$", BOOT_TIMEOUT)
# 4. ser.write(b"wifi connect ...\r\n")
# 5. read_until(ser, r"Connected", CONNECT_TIMEOUT)
# 6. record result
```

Key points:
- Open the serial port *after* the reset to avoid VCOM enumeration races.
- Use `rtscts=True` when HWFC is enabled (nRF54LM20DK UART30).
- Close the port after each iteration so the next reset can re-enumerate.
- Drain the serial buffer before sending the next command.
- **UART30 = VCOM0** on nRF54LM20DK (Port 0, P0.6/P0.7); do not confuse with VCOM1.
