# MCP Debug Tools for Embedded Development

Discovered during 2026-05-10 brainstorm for an AI-driven embedded test harness.
Covers MCP servers for Saleae logic analyzers, JLink debug probes, serial debugging,
Memfault cloud, and router control.

---

## 1. Saleae Logic Analyzer MCP

Three GitHub projects (top pick first):

### Primary: `wegitor/logic-analyzer-ai-mcp` ★9

- **Install**: `npx -y wegitor/logic-analyzer-ai-mcp`
- **Tools**: capture start/stop, protocol decode (SPI, I2C, UART, GPIO), data export
- **Use cases**: SPI flash trace during barrier hangs, Wi-Fi enable sequence timing, UART baud mismatch detection
- **Config for Claude Code** (`claude_desktop_config.json`):
  ```json
  {
    "mcpServers": {
      "saleae": {
        "command": "npx",
        "args": ["-y", "wegitor/logic-analyzer-ai-mcp"]
      }
    }
  }
  ```
- **Config for Hermes** (`~/.hermes/config.yaml`):
  ```yaml
  mcp_servers:
    saleae:
      command: "npx"
      args: ["-y", "wegitor/logic-analyzer-ai-mcp"]
  ```

### Alternative: `AkiyukiOkayasu/saleae-logic2-automation-mcp` ★1

- Saleae Logic 2 automation, less tested

### Alternative: `ril3y/SaleaeMCP` ★0

- Saleae Logic 2 control via natural language

### Connection notes

Saleae Logic 2 must be running on the host machine. The MCP server connects to
the Saleae API over localhost. For remote setups (e.g. harness controller on
another machine), run Saleae + MCP on the host with Saleae hardware attached.

---

## 2. JLink Debug Probe MCP

### Primary: `cyj0920/jlink_mcp` ★7

- **Install**: `npx -y cyj0920/jlink_mcp`
- **Tools**: flash firmware, read/write memory, halt/resume CPU, register inspection
- **Use cases**: flashing test firmware, crash-state memory dump, multi-board programming
- **Config**:
  ```yaml
  mcp_servers:
    jlink:
      command: "npx"
      args: ["-y", "cyj0920/jlink_mcp"]
  ```
- Requires JLink software (SEGGER) installed on the system.

### Alternative: `Klievan/jlink-mcp` ★5

- Embedded probe MCP, broader scope

### Alternative: `es617/dbgprobe-mcp-server` ★4

- Generic debug probe (JLink, CMSIS-DAP, ST-Link, etc.)
- Good for mixed hardware setups

### Alternative: `the78mole/jlink-ppk2-mcp` ★0

- JLink + Nordic PPK2 combo
- Interesting for power profiling alongside debug

---

## 3. Serial-Agent MCP ★11

- **Install**: `npx -y Rance-OwO/Serial-Agent`
- **Tools**: auto-scan serial ports, HEX TX/RX, timestamps, auto-reconnect
- **Use cases**: richer UART monitoring, VSCode integration for log browsing
- **Config**:
  ```yaml
  mcp_servers:
    serial-agent:
      command: "npx"
      args: ["-y", "Rance-OwO/Serial-Agent"]
  ```

---

## 4. Memfault REST API

No MCP server exists. Access via direct REST:

**Base URL**: `https://api.memfault.com`
**API docs**: https://api-docs.memfault.com

**Auth options**:
- Email + Password: `POST /auth/me`
- API Key: header `Authorization: Memfault-API-Key <key>`
- Project Key (for data push): header `Memfault-Project-Key: <key>`

**Common endpoints**:

```bash
# List organizations
curl -u "email:password" https://api.memfault.com/api/v0/organizations

# List projects
curl -H "Authorization: Memfault-API-Key <key>" \
  https://api.memfault.com/api/v0/projects

# Push event data
curl -X POST https://api.memfault.com/api/v0/events \
  -H "Memfault-Project-Key: <key>" \
  -H "Content-Type: application/json" \
  -d '{...}'
```

**Lightweight MCP wrapper** (template, ~30 lines):

```python
# memfault_mcp.py
import json, urllib.request
from mcp.server import Server, NotificationOptions
from mcp.server.models import InitializationOptions

server = Server("memfault")

@server.list_tools()
async def handle_list_tools():
    return [
        {
            "name": "memfault_get_projects",
            "description": "List Memfault projects",
            "inputSchema": {"type": "object", "properties": {}}
        },
        {
            "name": "memfault_get_issues",
            "description": "List recent issues/crashes",
            "inputSchema": {
                "type": "object",
                "properties": {
                    "project_id": {"type": "string"},
                    "limit": {"type": "integer"}
                }
            }
        },
        # ... extend as needed
    ]
```

---

## 5. Router Control (Wi-Fi Test Orchestration)

### ASUS BE92U + Asuswrt-Merlin (current)

SSH-based control:

```bash
ROUTER="admin@192.168.1.1"

# Wi-Fi
ssh $ROUTER "service restart_wireless"
ssh $ROUTER "wl -i eth1 radio"          # 0=off, 1=on (2.4GHz)
ssh $ROUTER "wl -i eth2 radio"          # 5GHz

# Networking
ssh $ROUTER "arp -a"                    # connected clients
ssh $ROUTER "nvram show | grep ssid"   # current SSID config
ssh $ROUTER "nvram set wl_ssid=testbed && nvram commit && service restart_wireless"

# Firewall
ssh $ROUTER "iptables -L -n"
ssh $ROUTER "iptables -A FORWARD -s 192.168.1.100 -j DROP"
```

### OpenWRT option (richer API)

If switching to OpenWRT, leverage `ubus` for structured control:

```bash
ssh root@router "ubus call network.wireless status"
ssh root@router "wifi down && sleep 2 && wifi up"
ssh root@router "ubus call iwinfo scan '{'device':'radio0'}'"
ssh root@router "uci set wireless.@wifi-iface[0].ssid='testbed' && uci commit && wifi"
```

OpenWRT also supports `luci` HTTP API and can run custom Python scripts
exposing REST endpoints directly.

---

## Notes

- All MCP servers listed were discovered via GitHub search (May 2026).
- Star counts are approximate and may change.
- For Hermes Agent: configure under `mcp_servers` in `~/.hermes/config.yaml`.
- For Claude Code: configure under `mcpServers` in `claude_desktop_config.json`.
- For Cursor: configure in Cursor Settings → MCP Servers.
