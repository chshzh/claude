---
name: chsh-sk-router-control
description: Control lab routers (ASUS RT-BE92U / Zyxel EX5700) over SSH — enable/disable wireless SSIDs, simulate AP reboots, block/unblock clients, and restart services. Use when writing Wi-Fi reconnection tests, simulating network failures, or controlling the wireless environment for nRF device testing.
---

# Router Control

## Available Routers

| Key | Model | System | IP | Used SSID |
|-----|-------|--------|----|-----------|
| `asus-rt-be92u` | RT-BE92U | Asuswrt-Merlin | 192.168.92.1 | BE92U_5G |
| `zyxel-ex5700` | Zyxel EX5700 (Telenor) | OpenWrt 25.12.4 | 192.168.75.1 | EX75_5G |

## Connection

Load connection details from `config.json` in this skill's directory.

```python
import json
import paramiko
from pathlib import Path


def load_router_info(router="asus-rt-be92u"):
    path = Path.home() / ".claude" / "skills" / "chsh-sk-router-control" / "config.json"
    routers = json.loads(path.read_text())
    return routers[router]

def router_ssh(router="asus-rt-be92u"):
    info = load_router_info(router)
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(info["host"], username=info["username"], password=info["password"], timeout=10)
    return ssh

def run(ssh, cmd):
    _, stdout, stderr = ssh.exec_command(cmd)
    return stdout.read().decode().strip() or stderr.read().decode().strip()
```

---

## Interface Map — ASUS RT-BE92U

| Interface | SSID | Band | Notes |
|-----------|------|------|-------|
| `wl0` | *(encrypted backhaul)* | 2.4 GHz | Do not touch |
| `wl0.1` | `BE92U_2.4G` | 2.4 GHz | User-facing 2.4G |
| `wl0.2` | `BE92U_IoT` | 2.4 GHz | IoT SSID (currently disabled) |
| `wl1` | *(encrypted backhaul)* | 5 GHz | Do not touch |
| `wl1.1` | `BE92U_5G` | 5 GHz | User-facing 5G ← **primary test target** |
| `wl1.2` | `BE92U_IoT` | 5 GHz | IoT SSID (currently disabled) |
| `wl2` | *(encrypted backhaul)* | 6 GHz | Do not touch |
| `wl2.1` | `BE92U_6G` | 6 GHz | User-facing 6G |

---

## Enable / Disable a Specific SSID

### Instant (no wireless restart, ~0s) — preferred for tests

```bash
# Disable BE92U_5G
wl -i wl1.1 bss down

# Enable BE92U_5G
wl -i wl1.1 bss up

# Check state  →  prints "up" or "down"
wl -i wl1.1 bss
```

This takes effect immediately. Other SSIDs are unaffected.

### Python helper

```python
import time

def ssid_down(ssh, iface="wl1.1"):
    run(ssh, f"wl -i {iface} bss down")

def ssid_up(ssh, iface="wl1.1"):
    run(ssh, f"wl -i {iface} bss up")

def ssid_state(ssh, iface="wl1.1"):
    return run(ssh, f"wl -i {iface} bss")   # "up" or "down"

# Usage — simulate 5s AP outage
ssh = router_ssh()
ssid_down(ssh)
time.sleep(5)
ssid_up(ssh)
assert ssid_state(ssh) == "up"
ssh.close()
```

### Persistent (survives reboot, ~10s wireless restart)

Use only when you want the change to survive a router power cycle:

```bash
# Persist disabled
nvram set wl1.1_bss_enabled=0
nvram commit
service restart_wireless   # ~10s, all clients reconnect

# Persist enabled
nvram set wl1.1_bss_enabled=1
nvram commit
service restart_wireless
```

---

## Enable / Disable Entire Radio Band

Kills all SSIDs on that band (backhaul + user SSIDs). Useful for severe failure simulation.

```bash
# Disable 5GHz radio
wl -i wl1 radio off

# Enable 5GHz radio
wl -i wl1 radio on

# Check  →  0x0000 = ON, 0x0001 = OFF
wl -i wl1 radio
```

---

## Simulate AP Reboot

```python
def restart_wireless(ssh):
    """Full wireless restart — all clients disconnect and reconnect (~10s)."""
    run(ssh, "service restart_wireless")
```

---

## Block / Unblock a Specific Client

```bash
# Block by MAC address
iptables -A FORWARD -m mac --mac-source AA:BB:CC:DD:EE:FF -j DROP

# Unblock
iptables -D FORWARD -m mac --mac-source AA:BB:CC:DD:EE:FF -j DROP
```

---

## Verify SSID State

```python
def check_all_ssids(ssh):
    for iface in ["wl0.1", "wl1.1", "wl2.1"]:
        ssid  = run(ssh, f"nvram get {iface}_ssid")
        state = run(ssh, f"wl -i {iface} bss")
        print(f"{iface}: {ssid!r:20} → {state}")
```

---

## Quick Test Patterns

### Reconnection test loop

```python
import time

def reconnect_stress(ssh, cycles=5, down_secs=5):
    for i in range(cycles):
        print(f"Cycle {i+1}: bringing BE92U_5G down")
        ssid_down(ssh)
        time.sleep(down_secs)
        ssid_up(ssh)
        time.sleep(3)  # allow reconnect
        assert ssid_state(ssh) == "up", "SSID failed to come back"
        print(f"Cycle {i+1}: OK")
```

### Teardown guard (always restore on test exit)

```python
import contextlib

@contextlib.contextmanager
def ssid_down_ctx(iface="wl1.1"):
    ssh = router_ssh()
    try:
        ssid_down(ssh, iface)
        yield ssh
    finally:
        ssid_up(ssh, iface)
        ssh.close()

# Usage
with ssid_down_ctx() as ssh:
    # device under test should detect disconnect here
    time.sleep(10)
# BE92U_5G is guaranteed back up here
```

---

## Notes

- `wl -i <iface> bss down/up` is instantaneous and safe — no nvram write, no service restart.
- `nvram commit` writes to flash — avoid running it frequently (flash wear).
- The router SSH session persists; reuse one `paramiko.SSHClient` per test session.
- `service restart_wireless` disconnects **all** wireless clients including the development Mac if it's on Wi-Fi.

---

## Self-Update Policy

At the **end of each conversation**, review what was discovered and check
whether any facts in this skill are new, corrected, or outdated (e.g. router
firmware updates, new SSH commands, SSID name changes, network topology changes).

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
