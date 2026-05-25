# UART Logs and WiFi Debugging

Verbose Kconfig, task templates, error pattern table, and reconnect stability test.
Referenced from [chsh-sk-ncs-3.2-debug Mode B](../SKILL.md#mode-b--uart-logs-and-wifi-debugging).


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
