# NCS Project Skill - Document Index

Quick navigation guide for all ncs-project documentation.

---

## 🎯 Start Here

| Document | Purpose | When to Use |
|----------|---------|-------------|
| [README.md](README.md) | Skill overview | First time using the skill |
| [SKILL.md](SKILL.md) | Quick reference guide | Regular development work |
| [FEATURE_QUICK_REF.md](FEATURE_QUICK_REF.md) | Feature selection guide | Planning new projects |

---

## 📚 Detailed Guides

### Core Guides

| Guide | Topics | Token Count |
|-------|--------|-------------|
| [FEATURE_SELECTION.md](guides/FEATURE_SELECTION.md) | Complete feature documentation (12 features) | ~15,000 |
| [ARCHITECTURE_PATTERNS.md](guides/ARCHITECTURE_PATTERNS.md) | **NEW!** Multi-threaded vs SMF+zbus patterns | ~20,000 |
| [WIFI_GUIDE.md](guides/WIFI_GUIDE.md) | Wi-Fi development patterns | ~8,000 |
| [PROJECT_STRUCTURE.md](guides/PROJECT_STRUCTURE.md) | File organization | ~5,000 |
| [CONFIG_GUIDE.md](guides/CONFIG_GUIDE.md) | Configuration management | ~5,000 |

### Feature Documentation

**In FEATURE_SELECTION.md**, find complete details for:

**Wi-Fi Features:**
- Wi-Fi Shell
- Wi-Fi STA (Station)
- Wi-Fi SoftAP (Access Point)
- Wi-Fi P2P (Wi-Fi Direct)

**Network Protocols:**
- UDP (User Datagram Protocol)
- TCP (Transmission Control Protocol)
- MQTT (Message Queuing Telemetry Transport)
- HTTP Client (RESTful APIs)
- HTTPS Server (Web interface)
- CoAP (Constrained Application Protocol)

**Advanced Features:**
- Memfault (Cloud monitoring & OTA)
- BLE Provisioning (Credential setup)
**Architecture Patterns:**
- Simple Multi-Threaded (Traditional approach)
- SMF + zbus Modular (Nordic's recommended pattern for complex systems)


Each includes: Config requirements, code examples, memory needs, dependencies.

---

## 🗂️ Templates

| Template | Purpose | Location |
|----------|---------|----------|
| LICENSE | Nordic 5-Clause license | [templates/LICENSE](templates/LICENSE) |
| .gitignore | NCS-specific ignore patterns | [templates/.gitignore](templates/.gitignore) |
| README_TEMPLATE.md | Project README structure | [templates/README_TEMPLATE.md](templates/README_TEMPLATE.md) |
| PRD_TEMPLATE.md | Product Requirements Document with feature selection | [templates/PRD_TEMPLATE.md](templates/PRD_TEMPLATE.md) |

---

## ⚙️ Configuration Files

### Wi-Fi Mode Configs (Choose ONE)

| Config | Wi-Fi Mode | Location |
|--------|-----------|----------|
| wifi-sta.conf | Station (connect to AP) | [configs/wifi-sta.conf](configs/wifi-sta.conf) |
| wifi-softap.conf | SoftAP (create AP) | [configs/wifi-softap.conf](configs/wifi-softap.conf) |
| wifi-p2p.conf | P2P/Wi-Fi Direct | [configs/wifi-p2p.conf](configs/wifi-p2p.conf) |
| wifi-raw.conf | Monitor/raw packets | [configs/wifi-raw.conf](configs/wifi-raw.conf) |

### Feature Overlays (Choose ANY)

| Overlay | Feature | Location |
|---------|---------|----------|
| overlay-wifi-shell.conf | Wi-Fi Shell | [features/overlay-wifi-shell.conf](features/overlay-wifi-shell.conf) |
| overlay-udp.conf | UDP Protocol | [features/overlay-udp.conf](features/overlay-udp.conf) |
| overlay-tcp.conf | TCP Protocol | [features/overlay-tcp.conf](features/overlay-tcp.conf) |
| overlay-mqtt.conf | MQTT Messaging | [features/overlay-mqtt.conf](features/overlay-mqtt.conf) |
| overlay-http-client.conf | HTTP Client | [features/overlay-http-client.conf](features/overlay-http-client.conf) |
| overlay-https-server.conf | HTTPS Server | [features/overlay-https-server.conf](features/overlay-https-server.conf) |
| overlay-coap.conf | CoAP Protocol | [features/overlay-coap.conf](features/overlay-coap.conf) |
| overlay-memfault.conf | Memfault | [features/overlay-memfault.conf](features/overlay-memfault.conf) |
| overlay-ble-prov.conf | BLE Provisioning | [features/overlay-ble-prov.conf](features/overlay-ble-prov.conf) |

---

## 🔍 Review Tools

| Tool | Purpose | Location |
|------|---------|----------|
| check_project.sh | Automated validation | [ProductManager/ncs/review/check_project.sh](ProductManager/ncs/review/check_project.sh) |
| CHECKLIST.md | Review checklist | [ProductManager/ncs/review/CHECKLIST.md](ProductManager/ncs/review/CHECKLIST.md) |
| QA_TEMPLATE.md | QA report template | [ProductManager/ncs/review/QA_TEMPLATE.md](ProductManager/ncs/review/QA_TEMPLATE.md) |
| IMPROVEMENT_GUIDE.md | Template feedback | [ProductManager/ncs/review/IMPROVEMENT_GUIDE.md](ProductManager/ncs/review/IMPROVEMENT_GUIDE.md) |

---

## 📖 Examples

| Example | Description | Location |
|---------|-------------|----------|
| basic_app | Minimal NCS app | [examples/basic_app/](examples/basic_app/) |

---

## 🔄 Workflow Guides

### For New Projects
1. **Plan**: Read [FEATURE_QUICK_REF.md](FEATURE_QUICK_REF.md)
2. **Document**: Fill [PDR_TEMPLATE.md](templates/PDR_TEMPLATE.md)
3. **Setup**: Copy templates from [templates/](templates/)
4. **Configure**: Copy configs from [configs/](configs/) and [features/](features/)
5. **Develop**: Reference [guides/](guides/) as needed
6. **Review**: Use [review/](review/) tools

### For Feature Selection
1. **Quick lookup**: [FEATURE_QUICK_REF.md](FEATURE_QUICK_REF.md)
2. **Detailed info**: [guides/FEATURE_SELECTION.md](guides/FEATURE_SELECTION.md)
3. **Copy overlays**: From [features/](features/) directory

### For Wi-Fi Development
1. **Overview**: [WIFI_GUIDE.md](guides/WIFI_GUIDE.md)
2. **Mode config**: Choose from [configs/](configs/)
3. **Additional features**: Add from [features/](features/)

---

## 📊 Quick Reference Tables

### Common Feature Combinations

| Use Case | Features | Memory (approx) |
|----------|----------|-----------------|
| IoT Sensor | STA + MQTT + Memfault | Flash 260KB, RAM 120KB, Heap 100KB |
| Smart Home | STA + HTTP Client + BLE Prov + Memfault | Flash 280KB, RAM 130KB, Heap 100KB |
| Config Portal | SoftAP + HTTPS Server | Flash 200KB, RAM 110KB, Heap 128KB |
| P2P Transfer | P2P + TCP + UDP | Flash 120KB, RAM 80KB, Heap 100KB |

See [FEATURE_QUICK_REF.md](FEATURE_QUICK_REF.md) for build commands.

### Build Command Pattern

**Supported WiFi Boards**:
- `nrf7002dk/nrf5340/cpuapp` - nRF7002 DK (built-in WiFi)
- `nrf54l15dk/nrf54l15/cpuapp` - nRF54L15 DK + nRF7002EB
- `nrf54lm20dk/nrf54lm20/cpuapp` - nRF54LM20 DK + nRF7002EB

```bash
# Replace <board> with one of the above
west build -p -b <board> -- \
  -DEXTRA_CONF_FILE="<wifi-mode>.conf;<feature1>.conf;<feature2>.conf"
```

---

## 🆘 Getting Help

| Need Help With | See Document |
|----------------|--------------|
| Skill overview | [README.md](README.md) |
| Quick reference | [SKILL.md](SKILL.md) |
| Feature selection | [FEATURE_QUICK_REF.md](FEATURE_QUICK_REF.md) |
| Feature details | [guides/FEATURE_SELECTION.md](guides/FEATURE_SELECTION.md) |
| Wi-Fi specifics | [guides/WIFI_GUIDE.md](guides/WIFI_GUIDE.md) |
| Project structure | [guides/PROJECT_STRUCTURE.md](guides/PROJECT_STRUCTURE.md) |
| Configuration | [guides/CONFIG_GUIDE.md](guides/CONFIG_GUIDE.md) |
| Project review | [ProductManager/ncs/review/CHECKLIST.md](ProductManager/ncs/review/CHECKLIST.md) |

---

## 📱 External Resources

- [NCS Documentation](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/index.html)
- [Zephyr Documentation](https://docs.zephyrproject.org/latest/)
- [Nordic DevZone](https://devzone.nordicsemi.com/)
- [Memfault](https://memfault.com/)

---

## 🎓 Learning Path

**New to NCS?**
1. Read [README.md](README.md)
2. Try [examples/basic_app/](examples/basic_app/)
3. Study [guides/PROJECT_STRUCTURE.md](guides/PROJECT_STRUCTURE.md)

**Building Wi-Fi Project?**
1. Read [FEATURE_QUICK_REF.md](FEATURE_QUICK_REF.md)
2. Check boxes in [PDR_TEMPLATE.md](templates/PDR_TEMPLATE.md)
3. Study relevant sections in [FEATURE_SELECTION.md](guides/FEATURE_SELECTION.md)
4. Copy overlays from [configs/](configs/) and [features/](features/)

**Want to Master NCS?**
1. Read all [guides/](guides/)
2. Try different feature combinations
3. Study [WIFI_GUIDE.md](guides/WIFI_GUIDE.md) in depth
4. Practice with [review/](review/) tools

---

## 📂 Directory Structure

```
ncs-project/
├── INDEX.md                        ← You are here
├── README.md                       ← Start here
├── SKILL.md                        ← Quick reference
├── FEATURE_QUICK_REF.md           ← Feature selection
├── ENHANCEMENT_SUMMARY.md         ← What's new
│
├── templates/                      ← Copy to projects
├── configs/                        ← Wi-Fi modes
├── features/                       ← Feature overlays
├── guides/                         ← Detailed docs
├── review/                         ← QA tools
└── examples/                       ← Reference code
```

---

**Last Updated**: 2026-01-30
**Skill Version**: 2.0.0 (Feature Selection Update)
