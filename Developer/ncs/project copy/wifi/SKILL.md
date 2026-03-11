````skill
---
name: ncs-wifi
description: Wi-Fi development for Nordic nRF Connect SDK - station, SoftAP, P2P, and raw packet modes
parent: ncs-project
---

# Wi-Fi Development Subskill

Complete Wi-Fi support for Nordic nRF70-series devices.

## 🎯 Quick Access

### Wi-Fi Modes (Choose ONE per project)

**Station Mode** (Connect to Access Point):
```bash
cp ~/.claude/skills/Developer/ncs/project/wifi/configs/wifi-sta.conf .
```

**SoftAP Mode** (Create Access Point):
```bash
cp ~/.claude/skills/Developer/ncs/project/wifi/configs/wifi-softap.conf .
```

**P2P Mode** (Wi-Fi Direct):
```bash
cp ~/.claude/skills/Developer/ncs/project/wifi/configs/wifi-p2p.conf .
```

**Raw Packet Mode** (Monitor/Promiscuous):
```bash
cp ~/.claude/skills/Developer/ncs/project/wifi/configs/wifi-raw.conf .
```

## 📖 Documentation

- **[WIFI_GUIDE.md](wifi/guides/WIFI_GUIDE.md)**: Comprehensive Wi-Fi development guide (~8,000 tokens)
  - All Wi-Fi modes explained
  - Memory requirements
  - Configuration details
  - Best practices
  - Troubleshooting

- **[RECONNECTION_PATTERNS.md](wifi/guides/RECONNECTION_PATTERNS.md)**: Network reconnection best practices
  - WiFi auto-reconnect after router power cycle
  - Network stack stabilization delays
  - Application protocol state management (MQTT, HTTPS)
  - Idempotent network notifications
  - Complete integration examples

## ⚙️ Configuration Files

Located in `wifi/configs/`:
- `wifi-sta.conf` - Station mode (connect to AP)
- `wifi-softap.conf` - SoftAP mode (create AP)
- `wifi-p2p.conf` - P2P/Wi-Fi Direct mode
- `wifi-raw.conf` - Monitor/raw packet mode

## 🔧 Common Requirements

All Wi-Fi projects need:
```properties
CONFIG_WIFI=y
CONFIG_WIFI_NRF70=y
CONFIG_WIFI_READY_LIB=y
CONFIG_HEAP_MEM_POOL_SIZE=80000  # Minimum!
```

## 📡 SoftAP Playbook (New)

- **Reserve the 192.168.7.0/24 subnet** – Configure SoftAP DHCP with the gateway at `192.168.7.1` and lease range `192.168.7.2-192.168.7.254`. Using a non-home-router subnet prevents IP conflicts and matches the latest QA-approved docs.
- **Document the network everywhere** – README Quick Start tables, PRD networking sections, REST samples, and troubleshooting notes must all cite the same gateway/IP examples so validation scripts and support teams stay aligned.
- **Ship credential templates** – Track `overlay-wifi-credentials.conf.template`, mention it in README.md, and add the real `overlay-wifi-credentials.conf` to `.gitignore`. Never log SSIDs/passwords at boot; the SoftAP project removed those logs after QA flagged them.
- **Handle recovery** – When enabling SoftAP fails (radio busy, bad creds), add retry/back-off logic in your SMF/zbus states and capture the behavior in PRD acceptance criteria so QA can test it.

## 📊 Memory Requirements

| Mode | Flash | RAM | Heap |
|------|-------|-----|------|
| STA | 60KB | 50KB | 80KB min |
| SoftAP | 75KB | 60KB | 100KB min |
| P2P | 80KB | 65KB | 100KB min |
| Raw | 50KB | 45KB | 60KB min |

## 🚀 Usage

1. Copy desired Wi-Fi mode config
2. Build with overlay:
```bash
west build -p -b nrf7002dk/nrf5340/cpuapp -- \
  -DEXTRA_CONF_FILE="wifi-sta.conf"
```

For complete details, see [WIFI_GUIDE.md](wifi/guides/WIFI_GUIDE.md)

````