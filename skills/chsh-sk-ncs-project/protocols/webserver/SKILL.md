---
name: chsh-sk-ncs-project-protocols-webserver
description: Static HTTP server pattern for Nordic NCS — serve HTML/CSS/JS from flash, expose REST APIs, and bridge to WebSockets. Use when building an on-device web UI or configuration portal hosted directly from the firmware.
---

# Static Web Server Subskill

Build complete web interfaces for Nordic devices with static resources, dynamic APIs, and real-time WebSocket communication.

## 🌐 What's Included

### Static Resources
- HTML, CSS, JavaScript (gzip-compressed)
- Images (PNG, JPEG, SVG)
- 3D models (GLTF/GLB)
- Fonts (WOFF2)
- Automatic ~70% size reduction via gzip

### Dynamic REST APIs
- JSON request/response
- POST endpoints for device control
- GET endpoints for status/data
- Authentication support

### WebSocket
- Real-time bidirectional communication
- Sensor data streaming
- Live updates
- Multiple simultaneous connections

### Network Discovery
- mDNS/DNS-SD support
- Access via `device.local` hostname
- Zero-configuration networking

## 🚀 Quick Start

```bash
# 1. Copy templates
mkdir -p src/static_web_resources
cp ~/.claude/skills/chsh-sk-ncs-project/protocols/webserver/templates/* src/
cp ~/.claude/skills/chsh-sk-ncs-project/protocols/webserver/overlay-static-webserver.conf .

# 2. Create web files in src/static_web_resources/
#    - index.html
#    - main.js
#    - styles.css

# 3. Update CMakeLists.txt (see template)

# 4. Build
west build -p -b nrf7002dk/nrf5340/cpuapp -- \
  -DEXTRA_CONF_FILE="wifi-sta.conf;overlay-static-webserver.conf"

# 5. Access web interface
# http://192.168.7.1/  (default SoftAP gateway)
# http://mydevice.local/  (if mDNS configured)

## 🧪 QA-Driven Documentation Tips

- **Mirror reality** – Keep REST examples, troubleshooting flows, and screenshots synchronized with the actual board build (button/LED counts, JSON payloads, default SSIDs). The SoftAP webserver QA pass required rewriting every 192.168.1.x reference to 192.168.7.1 to match firmware defaults.
- **Surface credential workflow** – Every README/PRD that exposes a web portal must remind developers to copy the credential overlay template locally and never commit real SSIDs/passwords.
- **Map hardware to UI** – List which buttons or LEDs power each UI element per board so reviewers can validate acceptance criteria without guessing.
```

## 📖 Complete Documentation

**[STATIC_WEBSERVER_GUIDE.md](STATIC_WEBSERVER_GUIDE.md)** (~15,000 tokens)
- Complete walkthrough
- Static resource embedding
- Dynamic REST API implementation
- WebSocket real-time streaming
- Security (HTTPS, authentication)
- Troubleshooting
- Performance optimization

## 🗂️ Templates

Located in `protocols/webserver/templates/`:
- `http_resources_template.c` - Complete HTTP resource definitions
- `http_resources_template.h` - Public API interface
- `CMakeLists_webserver.txt` - CMake configuration
- `sections-rom.ld` - Required linker script

## 📊 Memory Requirements

| Component | Flash | RAM | Heap |
|-----------|-------|-----|------|
| HTTP Server Base | 40KB | 30KB | 20KB |
| Static Files (per 10KB) | 3KB | - | - |
| WebSocket | +10KB | +10KB | +20KB |
| **Typical Total** | **100KB** | **80KB** | **90KB** |

Configure heap: `CONFIG_HEAP_MEM_POOL_SIZE=90000`

## 🎯 Based On

**Nordic Thingy91x Suitcase Demo** - Production-proven reference:
- Location: `/opt/nordic/ncs/myApps/Nordic_Thingy91x_Suitcase_Demo/`
- Features: 3D model viewer, LED control, sensor streaming, WiFi location

## ⚙️ Configuration Overlay

`overlay-static-webserver.conf` includes:
- HTTP server framework
- JSON parsing
- Network configuration
- mDNS/DNS-SD
- TLS support (HTTPS)
- Optimized memory settings

## 🔒 Security Features

- HTTPS/TLS support
- Certificate provisioning
- Authentication headers
- Input validation
- Secure defaults

## 🆘 Common Use Cases

- **Device Configuration Portals**: Wi-Fi setup, settings management
- **Real-Time Dashboards**: Sensor data visualization, live charts
- **OTA Update Interfaces**: Firmware management consoles
- **Smart Home Controls**: Device control panels
- **Debug Consoles**: Development and diagnostics

For complete implementation details, see [STATIC_WEBSERVER_GUIDE.md](STATIC_WEBSERVER_GUIDE.md)

## Self-Update Policy

At the **end of each conversation**, review what was discovered and check whether any facts in this skill are new, corrected, or outdated (e.g. new HTTP server patterns, REST API conventions, static file handling).

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
