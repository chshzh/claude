# GitHub Actions Debug Reference

Use when a CI build fails, the pre-built firmware is broken, or you need to
verify the CI → flash → test loop end-to-end.

## Monitor GitHub Actions Runs

```bash
# List recent runs
gh run list --repo <owner>/<repo> --limit 5

# Watch until completion (updates every 30 seconds)
gh run watch <run-id> --repo <owner>/<repo> --interval 30

# Get failed step logs
gh run view <run-id> --repo <owner>/<repo> --log-failed
```

## Common CI Failure Patterns

| Failure | Root cause | Fix |
|---------|-----------|-----|
| `curl: (22) 404` on nrfutil | URL no longer valid | Switch to `ghcr.io/nrfconnect/sdk-nrf-toolchain:<ver>` container |
| `west: command not found` | Toolchain not in container PATH | Use container image, not manual install |
| `git am` fails "already applied" | Patch cached with repos | Add idempotent check: `git am --check "$p" && git am "$p"` |
| `merged.hex not found` | No MCUboot sysbuild | Fallback to `zephyr/zephyr.hex`; check `sysbuild.conf` |
| Permission denied on git ops | UID mismatch in container | Add `git config --global --add safe.directory '*'` |
| Release step fails | Missing `permissions: contents: write` | Add at workflow top level |

## Download and Flash Pre-Built Firmware

```bash
# Download latest release artifact
gh release download latest --repo <owner>/<repo> --pattern "*.hex" -D /tmp/fw/
ls /tmp/fw/   # confirm filename — use descriptive name, not merged.hex

# Flash (standard — preserves NVS and WiFi credentials)
nrfutil sdk-manager toolchain launch --ncs-version=v3.3.0 -- \
  west flash --hex-file /tmp/fw/<project>-<board>-<shield>-ncs<version>.hex --dev-id <SN>

# Use --recover only for first flash or if AP protection is blocking access
# WARNING: --recover does a full chip erase — wipes NVS including WiFi credentials
```

Or use **nRF Connect for Desktop → Programmer** (no local toolchain needed).

## Verify Boot over UART

Delegate to `chsh-ag-terminal` after flashing:

```
Task(
  subagent_type="chsh-ag-terminal",
  description="Post-flash boot verification",
  prompt="""
  Board:    <confirmed board>
  Port:     <confirmed port>
  Log file: <NCS_LOG>

  Append each received line (with [HH:MM:SS.mmm] timestamp) to Log file as it arrives.

  1. Reset the board
  2. Capture boot log until uart:~$ (30 s timeout)
  3. Send: wifi scan; wifi status; kernel threads
  4. Return: full output; confirm 'Booting nRF Connect SDK' and uart:~$ appeared
  """
)
```

Expected sequence: `*** Booting nRF Connect SDK` → module init → `uart:~$` (shell ready).

## Fix-Push-Verify Loop

```
edit → commit → push → gh run watch → (pass) → gh release download latest → west flash → verify
                                     → (fail) → gh run view --log-failed → fix → repeat
```

Minimum loop time with caching: ~5 min. Without cache: ~20 min.
