# Loop Testing for Stability

Scripted loop test to confirm fix stability. 10 passes = acceptance gate, 20 = release gate.
Referenced from [chsh-sk-ncs-3.2-debug Mode F](../SKILL.md#mode-f--loop-testing-for-stability).


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
