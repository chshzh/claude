````skill
---
name: ncs-protocols
description: Network protocols for Nordic NCS - MQTT, HTTP, CoAP, UDP, TCP, WebServer
parent: ncs-project
---

# Network Protocols Subskill

Complete protocol support for Nordic NCS projects.

## 🌐 Available Protocols

### Static Web Server (NEW!)
**Full-featured HTTP server with static resources, REST APIs, and WebSockets**

- **[WebServer Subskill](protocols/webserver/SKILL.md)**: Complete web server implementation
- Based on Nordic Thingy91x Suitcase Demo
- Static HTML/CSS/JS with gzip compression  
- Dynamic REST API endpoints
- WebSocket real-time communication
- See [STATIC_WEBSERVER_GUIDE.md](protocols/webserver/STATIC_WEBSERVER_GUIDE.md) for complete guide

### MQTT
Industrial IoT messaging protocol

```bash
cp ~/.claude/skills/ProductManager/ncs/features/overlays/overlay-mqtt.conf .
```

**Features**: QoS 0/1/2, TLS, Last Will, persistent sessions  
**Memory**: Flash +45KB, RAM +30KB, Heap +40KB  
**Use for**: Cloud connectivity, sensor networks, home automation

### HTTP Client
RESTful API calls and web services

```bash
cp ~/.claude/skills/ProductManager/ncs/features/overlays/overlay-http-client.conf .
```

**Features**: GET/POST/PUT/DELETE, HTTPS (TLS), chunked encoding  
**Memory**: Flash +35KB, RAM +25KB, Heap +32KB  
**Use for**: Cloud APIs, firmware updates, data uploads

### CoAP
Constrained Application Protocol for IoT

```bash
cp ~/.claude/skills/ProductManager/ncs/features/overlays/overlay-coap.conf .
```

**Features**: UDP-based, block transfer, observe, DTLS  
**Memory**: Flash +40KB, RAM +28KB, Heap +24KB  
**Use for**: Resource-constrained devices, battery-powered sensors

### TCP
Reliable stream protocol

```bash
cp ~/.claude/skills/ProductManager/ncs/features/overlays/overlay-tcp.conf .
```

**Features**: Reliable, ordered delivery, connection-oriented  
**Memory**: Flash +20KB, RAM +15KB, Heap +16KB  
**Use for**: Reliable data transfer, custom protocols

### UDP
Fast datagram protocol

```bash
cp ~/.claude/skills/ProductManager/ncs/features/overlays/overlay-udp.conf .
```

**Features**: Low latency, connectionless, broadcast/multicast  
**Memory**: Flash +10KB, RAM +8KB, Heap +8KB  
**Use for**: Real-time data, video streaming, sensor broadcasts

## 📖 Documentation

Each protocol has detailed documentation in:
- `ProductManager/ncs/features/FEATURE_SELECTION.md` - Configuration and code examples
- `protocols/<protocol>/` - Protocol-specific guides (when available)
- **[RECONNECTION_PATTERNS.md](../wifi/guides/RECONNECTION_PATTERNS.md)** - Network state management
  - Application protocol reconnection (MQTT, HTTPS)
  - State machine patterns
  - Stabilization delays and retry logic
  - Handling WiFi disconnect/reconnect events

## 🚀 Usage

```bash
# Combine protocols as needed
west build -p -b nrf7002dk/nrf5340/cpuapp -- \
  -DEXTRA_CONF_FILE="wifi-sta.conf;overlay-mqtt.conf;overlay-http-client.conf"
```

## 📊 Protocol Comparison

| Protocol | Use Case | Overhead | TLS Support |
|----------|----------|----------|-------------|
| **WebServer** | Web UI, dashboards | High | Yes (HTTPS) |
| **MQTT** | IoT messaging | Medium | Yes |
| **HTTP** | REST APIs | Medium | Yes (HTTPS) |
| **CoAP** | Constrained devices | Low | Yes (DTLS) |
| **TCP** | Reliable custom | Low | Via TLS sockets |
| **UDP** | Real-time data | Very Low | Via DTLS |

For complete details, see [FEATURE_SELECTION.md](../../../../ProductManager/ncs/features/FEATURE_SELECTION.md)

````