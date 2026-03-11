---
name: ncs-project
description: Complete Nordic nRF Connect SDK project workflow - generate projects from templates, review quality with QA reports, and continuously improve through feedback loops
---

# NCS Project Workflow

Unified skill for managing the complete lifecycle of Nordic nRF Connect SDK projects.

## 🎯 PRD-Driven Development

**PRD.md is the single source of truth for every NCS project.**  

Before any action (generate, review, improve), always:
1. 📋 **Start with PRD.md** - Copy PRD_TEMPLATE.md to your project
2. ✅ **Select Features** - Check boxes for required features (100+ options)
3. 📝 **Define Requirements** - Document user stories and acceptance criteria
4. 🏗️ **Guide Development** - Use PRD.md to drive implementation
5. 🔍 **Review Against PRD** - Validate project meets documented requirements
6. 🔄 **Update PRD** - Keep it current as project evolves

## 🎯 Three Core Actions

### 1. **Generate** - Create New Projects
Start new NCS projects with proven templates and best practices built-in.  
**Product Manager creates PRD.md (business requirements), Developers create OpenSpec specs (technical implementation).**

### 2. **Review** - Validate Quality  
Systematically review projects against standards and generate detailed QA reports.  
**Compare implementation against PRD.md (business) and OpenSpec specs (technical).**

### 3. **Improve** - Evolve Templates
Update templates based on review findings for continuous improvement.  
**Update PRD_TEMPLATE.md (business) and OpenSpec config (technical) based on lessons learned.**

---

## 🧩 Workspace Application Setup

All customer-facing apps must be *workspace applications* so anyone can clone the repo and fetch the exact Nordic SDK revision in one command.

1. **Add `west.yml` at the repo root** (pin `sdk-nrf` revision and import Zephyr):
        ```yaml
        manifest:
               self:
                      path: my_app
               remotes:
                      - name: ncs
                             url-base: https://github.com/nrfconnect
               projects:
                      - name: nrf
                             remote: ncs
                             repo-path: sdk-nrf
                             revision: v3.2.1
                             import: true
        ```
2. **Document the workflow in README**:
        ```bash
        west init -l my_app
        west update -o=--depth=1 -n
        west build -p -b nrf7002dk/nrf5340/cpuapp
        ```
3. **CI/CD requirement** – add `.github/workflows/build.yml` that:
        - parses the NCS version from `west.yml`
        - runs `west init/update` inside the GitHub-hosted container `ghcr.io/nrfconnect/sdk-nrf-toolchain:${NCS_VERSION}`
        - executes format/static checks (`checkpatch`, `clang-format --dry-run`)
        - builds every supported configuration (matrix strategy)
        - uploads merged HEX artifacts for releases

Use `nordic_wifi_opus_audio_demo/.github/workflows/build.yml` as the reference implementation.

---

## 📋 Quick Command Reference

### Generate New Project

**Basic NCS Application**:
```bash
# Create structure
mkdir my_project && cd my_project
mkdir -p src boards scripts tests doc

# Copy core templates
cp ~/.claude/skills/Developer/ncs/project/templates/LICENSE .
cp ~/.claude/skills/Developer/ncs/project/templates/.gitignore .
cp ~/.claude/skills/Developer/ncs/project/templates/README_TEMPLATE.md README.md
```

**Wi-Fi Project** (add overlay):
```bash
# Copy Wi-Fi configuration
cp ~/.claude/skills/Developer/ncs/project/configs/wifi-sta.conf overlay-wifi-sta.conf
# Or: wifi-softap.conf, wifi-p2p.conf, wifi-raw.conf
```

**Build**:
```bash
# Basic build (use your board)
west build -p -b nrf7002dk/nrf5340/cpuapp
# Or: nrf54l15dk/nrf54l15/cpuapp
# Or: nrf54lm20dk/nrf54lm20/cpuapp

# With Wi-Fi overlay
west build -p -b nrf7002dk/nrf5340/cpuapp -- -DEXTRA_CONF_FILE=overlay-wifi-sta.conf
```

### Review Project

**Quick automated check** (~1 min):
```bash
~/.claude/skills/ProductManager/ncs/review/check_project.sh /path/to/project
```

**Standard review** (2-3 hours):
1. Use `ProductManager/ncs/review/CHECKLIST.md` as guide
2. Fill out `ProductManager/ncs/review/QA_TEMPLATE.md`
3. Generate recommendations

### Improve Templates

After reviews, update templates based on findings:
1. Document issues in `ProductManager/ncs/review/IMPROVEMENT_GUIDE.md`
2. Update relevant templates/configs
3. Test improvements
4. Deploy for next project

---

## 📦 What's Included

### Templates (Ready to Copy)
- `templates/LICENSE` - Nordic 5-Clause license
- `templates/.gitignore` - NCS-specific ignore patterns  
- `templates/README_TEMPLATE.md` - Comprehensive README structure
- `ProductManager/ncs/prd/PRD_TEMPLATE.md` - Product Requirements Document with feature selection

### Wi-Fi Mode Configurations
- `configs/wifi-sta.conf` - Station mode (connect to AP)
- `configs/wifi-softap.conf` - SoftAP mode (create AP)
- `configs/wifi-p2p.conf` - P2P/Wi-Fi Direct mode
- `configs/wifi-raw.conf` - Monitor/raw packet mode

### Feature Overlays (NEW! Modular feature selection)
- `ProductManager/ncs/features/overlays/overlay-wifi-shell.conf` - Interactive Wi-Fi shell
- `ProductManager/ncs/features/overlays/overlay-udp.conf` - UDP protocol
- `ProductManager/ncs/features/overlays/overlay-tcp.conf` - TCP protocol
- `ProductManager/ncs/features/overlays/overlay-mqtt.conf` - MQTT messaging
- `ProductManager/ncs/features/overlays/overlay-http-client.conf` - HTTP/HTTPS client
- `ProductManager/ncs/features/overlays/overlay-https-server.conf` - HTTPS server
- `ProductManager/ncs/features/overlays/overlay-coap.conf` - CoAP protocol
- `ProductManager/ncs/features/overlays/overlay-memfault.conf` - Memfault monitoring
- `ProductManager/ncs/features/overlays/overlay-ble-prov.conf` - BLE credential provisioning

### Architecture Pattern Overlays (NEW!)
- `ProductManager/ncs/features/overlays/overlay-smf-zbus.conf` - SMF+zbus modular architecture
- `ProductManager/ncs/features/overlays/overlay-multithreaded.conf` - Simple multi-threaded architecture

### Comprehensive Guides
- `guides/ARCHITECTURE_PATTERNS.md` - **NEW!** Multi-threaded vs SMF+zbus patterns
- `guides/WIFI_GUIDE.md` - Wi-Fi development patterns & best practices
- `guides/CONFIG_GUIDE.md` - Configuration management
- `guides/PROJECT_STRUCTURE.md` - File organization
- `ProductManager/ncs/features/FEATURE_SELECTION.md` - Complete feature selection guide

**STEP 0: Always start with PRD.md!**

```bash
# 1. Create project
mkdir my_iot_device && cd my_iot_device
mkdir -p src boards config docQuick reference checklist
- `ProductManager/ncs/review/IMPROVEMENT_GUIDE.md` - Template feedback process

### Examples
- `examples/` - Reference implementations

---

## 🚀 Common Workflows

### Starting a New Project with OpenSpec

```bash
# 1. Create project structure
mkdir my_iot_device && cd my_iot_device
mkdir -p src/modules boards openspec/specs openspec/changes

# 2. Copy base templates
cp ~/.claude/skills/Developer/ncs/project/templates/{LICENSE,.gitignore} .
cp ~/.claude/skills/Developer/ncs/project/templates/README_TEMPLATE.md README.md

# 3. Product Manager creates PRD.md (business requirements)
cp ~/.claude/skills/ProductManager/ncs/prd/PRD_TEMPLATE.md PRD.md
# Edit PRD.md: select features, define user stories, acceptance criteria

# 4. Initialize OpenSpec (technical specifications)
cat > openspec/config.yaml << 'EOF'
schema: spec-driven
context: |
  Tech stack: Zephyr RTOS, Nordic NCS v3.2.1
  Architecture: [Multi-threaded OR SMF+Zbus - choose based on PRD]
  Domain: [Your domain from PRD]
  
  Configuration strategy:
  - Base configs in prj.conf
  - Module configs co-located with module code
  - Merge module configs into prj.conf when enabled
  - Use overlays only for secrets/environment differences
EOF

# 5. Create technical specs based on PRD features
# Example: If PRD selects WiFi + MQTT features:
# - openspec/specs/architecture.md (system design)
# - openspec/specs/wifi-module.md (WiFi implementation)
# - openspec/specs/mqtt-client.md (MQTT implementation)

# 6. Create base prj.conf with core features
cat > prj.conf << 'EOF'
# Base configuration
CONFIG_LOG=y
CONFIG_SHELL=y
CONFIG_DK_LIBRARY=y

# Architecture (from PRD selection)
CONFIG_SMF=y
CONFIG_ZBUS=y

# Module enable flags
CONFIG_WIFI_MODULE_ENABLED=y
CONFIG_MQTT_MODULE_ENABLED=y
# CONFIG_MEMFAULT_MODULE_ENABLED=n
EOF

# 7. Create modular structure with config fragments
mkdir -p src/modules/{wifi,mqtt,button}

# 8. Copy module config templates from skill library
# WiFi module
cat ~/.claude/skills/ProductManager/ncs/features/overlays/overlay-wifi-shell.conf \
    > src/modules/wifi/wifi.conf.template

# MQTT module  
cat ~/.claude/skills/ProductManager/ncs/features/overlays/overlay-mqtt.conf \
    > src/modules/mqtt/mqtt.conf.template

# 9. Merge enabled module configs into prj.conf
echo "\n# WiFi Module Configuration" >> prj.conf
cat src/modules/wifi/wifi.conf.template >> prj.conf

echo "\n# MQTT Module Configuration" >> prj.conf
cat src/modules/mqtt/mqtt.conf.template >> prj.conf

# 10. Implement modules according to OpenSpec specs
# Create module files following openspec/specs/ documentation:
# - CMakeLists.txt
# - Kconfig (rsource module Kconfigs)
# - src/main.c
# - src/modules/wifi/{wifi.c, wifi.h, Kconfig.wifi, CMakeLists.txt}
# - src/modules/mqtt/{mqtt.c, mqtt.h, Kconfig.mqtt, CMakeLists.txt}

# 11. Build (no overlays needed for modules!)
west build -p -b nrf7002dk/nrf5340/cpuapp

# 12. Create credentials overlay (git-ignored) if needed
cat > overlay-credentials.conf << 'EOF'
CONFIG_MEMFAULT_NCS_PROJECT_KEY="your-key-here"
EOF
echo "overlay-credentials.conf" >> .gitignore

# 13. Build with secrets
west build -p -b nrf7002dk/nrf5340/cpuapp -- \
  -DEXTRA_CONF_FILE="overlay-credentials.conf"

# 14. Initialize git
git init && git add . && git commit -m "Initial commit with OpenSpec"
```

### Starting a New Wi-Fi P2P Project

```bash
# 1. Create project
mkdir wifi_p2p_app && cd wifi_p2p_app
mkdir -p src boards

# 2. Copy templates
cp ~/.claude/skills/Developer/ncs/project/templates/{LICENSE,.gitignore} .
cp ~/.claude/skills/Developer/ncs/project/templates/README_TEMPLATE.md README.md

# 3. Copy P2P configuration + desired features
cp ~/.claude/skills/Developer/ncs/project/configs/wifi-p2p.conf .
cp ~/.claude/skills/ProductManager/ncs/features/overlays/overlay-wifi-shell.conf .
cp ~/.claude/skills/ProductManager/ncs/features/overlays/overlay-tcp.conf .

# 4. Create core files (see guides/PROJECT_STRUCTURE.md)
# - CMakeLists.txt
# - Kconfig  
# - prj.conf
# - src/main.c

# 5. Build
# Use your board: nrf7002dk/nrf5340/cpuapp, nrf54l15dk/nrf54l15/cpuapp, or nrf54lm20dk/nrf54lm20/cpuapp
west build -p -b nrf7002dk/nrf5340/cpuapp -- \
  -DEXTRA_CONF_FILE="wifi-p2p.conf;overlay-wifi-shell.conf;overlay-tcp.conf"

# 6. Initialize git
git init && git add . && git commit -m "Initial commit"
```

### Before Release Review

```bash
# 1. Verify documentation exists and is up-to-date
cat PRD.md              # Business requirements current?
ls openspec/specs/      # Technical specs complete?

# 2. Quick automated check
~/.claude/skills/ProductManager/ncs/review/check_project.sh .

# 3. Fix critical issues immediately

# 4. Request formal review
# Reviewer validates:
# - PRD.md: Business requirements met?
# - OpenSpec specs: Implementation matches technical specs?
# - Code: Follows architecture patterns from specs?

# 5. Address findings
# Update code, PRD.md (business changes), or specs (technical changes)

# 6. Re-review if needed
```

### Post-Review Improvement

```bash
# 1. Document common issues found
# Add to ProductManager/ncs/review/IMPROVEMENT_GUIDE.md

# 2. Update templates
# Fix gaps in templates/ or configs/

# 3. Update guides
# Enhance guides/ with new patterns

# 4. Next project benefits!
```
**PRD.md** (Product Requirements Document - BUSINESS REQUIREMENTS)
- ✅ Product vision and features ("what" to build)
- ✅ User stories and acceptance criteria
- ✅ Success metrics and target users
- ✅ High-level architecture only

**openspec/** (Technical Specifications - IMPLEMENTATION GUIDE)
- ✅ openspec/config.yaml - Project context and conventions
- ✅ openspec/specs/architecture.md - System design and patterns
- ✅ openspec/specs/[module].md - Module implementations ("how" to build)
- ✅ openspec/changes/ - Technical proposals and change tracking
- ✅ 
---

## 📊 Critical Requirements by Project Type

### All NCS Projects (MUST HAVE)
- ✅ CMakeLists.txt (min version 3.20.0)
- ✅ Kconfig (source "Kconfig.zephyr")
- ✅ prj.conf (base configuration)
- ✅ src/main.c (application entry)
- ✅ LICENSE (Nordic 5-Clause)
- ✅ README.md (complete documentation)
- ✅ .gitignore (NCS patterns)

### Wi-Fi Projects (ADDITIONAL REQUIREMENTS)
- ✅ `CONFIG_WIFI=y` and `CONFIG_WIFI_NRF70=y`
- ✅ `CONFIG_WIFI_READY_LIB=y` (safe initialization)
- ✅ `CONFIG_HEAP_MEM_POOL_SIZE≥80000` (critical!)
- ✅ Network stack configured
- ✅ Proper Wi-Fi mode overlay (sta/softap/p2p/raw)
- ✅ Event handling implemented
- ⚠️ NO hardcoded credentials

### Wi-Fi P2P Specific
- ✅ `CONFIG_NRF70_P2P_MODE=y`
- ✅ `CONFIG_NRF_WIFI_LOW_POWER=n` (disable for P2P)
- ✅ Both DHCP server and client enabled
- ✅ Timing delays configured (4-way handshake, DHCP start)
- ✅ GO intent properly set (0-15)

---

## 🔍 Review Scoring Guide

**100-Point Scale**:
- **PRD.md Quality**: 15 points (completeness, accuracy, up-to-date)
- Project Structure: 10 points
- Core Files Quality: 15 points
- Configuration: 15 points
- Code Quality: 15 points
- Documentation: 10 points
- Wi-Fi/Features: 10 points
- Security: 10 points
- Build & Testing: 10 points

**Grades**:
- 90-100: ✅ Excellent - Ready for release
- 80-89: ✅ Good - Minor improvements needed
- 70-79: ⚠️ Satisfactory - Notable issues to fix
- 60-69: 🔧 Needs work - Significant problems
- <60: ❌ Fail - Major rework required

---

## 🔒 Security Checklist

**Before Every Commit**:
- [ ] No hardcoded credentials in code
- [ ] No passwords in configuration files
- [ ] Credential overlays in .gitignore
- [ ] Track only `overlay-*-credentials.conf.template` files – real credential overlays stay local
- [ ] No private keys committed
- [ ] No API tokens in code

**Wi-Fi Security**:
- [ ] WPA2-PSK minimum (prefer WPA3)
- [ ] TLS enabled for network communications
- [ ] Certificate validation implemented
- [ ] Input validation on network data

---

## 🎓 Best Practices

### For Developers
1. **Read PRD.md first** - Understand business requirements and features
2. **Create OpenSpec specs** - Document technical implementation before coding
   - architecture.md - System design
   - [module].md - Module specs (state machines, APIs, dependencies)
3. **Create modular structure** - Each module in `src/modules/[name]/`
   - Module code: `.c`, `.h`
   - Module Kconfig: `Kconfig.[name]`
   - Module config template: `[name].conf.template` (copy from skill library)
4. **Merge module configs** - When enabling, merge `.conf.template` into `prj.conf`
5. **Follow Wi-Fi memory requirements** - Heap ≥80KB critical
6. **Handle all network events** - Connect/disconnect/IP assigned
7. **Keep docs in sync** - Update specs when implementation changes
8. **Test against specs** - Verify behavior matches state machines/sequences
9. **Use overlays for secrets only** - Credentials in git-ignored overlay files
10. **Copyright year** - Use **current year** (e.g. 2026) in new/modified files

## 🧪 Lessons from SoftAP Webserver QA

- **Document the actual SoftAP network** – Standardize on the `192.168.7.0/24` subnet with the gateway at `192.168.7.1`, and mirror those values across README Quick Start tables, PRD test cases, and troubleshooting guides so users never see stale `192.168.1.x` examples again.
- **Ship credential overlays, not secrets** – Check in `overlay-wifi-credentials.conf.template` (or equivalent) and `.gitignore` the developer-specific overlay. Every README needs a note that developers must copy the template locally and keep passwords out of logs, configs, and commits.
- **Describe per-board capabilities** – When a design supports multiple DKs, PRD.md and README.md must include button/LED tables that match each board’s silkscreen so QA can validate acceptance criteria without guessing.
- **Keep automation green** – Treat `ProductManager/ncs/review/check_project.sh` as a release gate. Fix the script as soon as it drifts (missing tools, path errors, etc.) before starting manual QA work.
- **Plan for Wi-Fi recovery** – SoftAP enable can fail after power events. Build a retry/back-off path into SMF/zbus states (and document it in requirements) so the system self-recovers instead of leaving QA to cycle power manually.

### For Reviewers
1. **Verify documentation** - PRD.md (business) and openspec/ (technical) exist
2. **Compare against specs**:
   - PRD.md: Business requirements met?
   - openspec/specs/: Implementation matches technical specs?
3. **Run automated check first** - check_project.sh
4. **Be systematic** - Follow checklist completely
5. **Provide examples** - Show specific issues in code vs specs
6. **Suggest solutions** - Not just problems
7. **Update templates** - Feed learnings back to templates and openspec/config.yaml

---

## 📁 File Organization Reference

```
ncs-project/
├── SKILL.md                    # This file (quick reference)
│
├── templates/                  # Project templates (copy to new projects)
│   ├── LICENSE                 # Nordic 5-Clause license
│   ├── .gitignore             # NCS ignore patterns
│   ├── README_TEMPLATE.md     # Complete README structure
│   └── PRD_TEMPLATE.md        # Product Requirements Document with feature selection
│
├── configs/                    # Wi-Fi mode configurations
│   ├── wifi-sta.conf          # Wi-Fi Station mode
│   ├── wifi-softap.conf       # Wi-Fi SoftAP mode
│   ├── wifi-p2p.conf          # Wi-Fi P2P mode
│   └── wifi-raw.conf          # Monitor/raw packet mode
│
├── features/                   # Modular feature overlays (NEW!)
│   ├── overlay-wifi-shell.conf    # Wi-Fi shell commands
│   ├── overlay-udp.conf           # UDP protocol
│   ├── overlay-tcp.conf           # TCP protocol
│   ├── overlay-mqtt.conf          # MQTT messaging
│   ├── overlay-http-client.conf   # HTTP client
│   ├── overlay-https-server.conf  # HTTPS server
│   ├── overlay-coap.conf          # CoAP protocol
│   ├── overlay-ble-prov.conf      # BLE provisioning
│   ├── overlay-smf-zbus.conf      # SMF+zbus modular architecture (NEW!)
│   └── overlay-multithreaded.conf # Simple multi-threaded architecture (NEW!)ing
│   └── overlay-ble-prov.conf      # BLE provisioning
│
├── guides/                     # Detailed documentation
│   ├── WIFI_GUIDE.md          # Complete Wi-Fi development guide
│   ├── CONFIG_GUIDE.md        # Configuration management
│   ├── PROJECT_STRUCTURE.md   # File organization guide
│   ├── FEATURE_SELECTION.md   # Feature selection guide (NEW!)
│   └── ARCHITECTURE_PATTERNS.md # Multi-threaded vs SMF+zbus patterns (NEW!)
│
├── (See ProductManager/ncs/review/) # Quality assurance resources now centralized under Product Manager
│
└── examples/                   # Reference implementations
    ├── basic_app/             # Minimal NCS application
    ├── wifi_sta/              # Wi-Fi Station example
    └── wifi_p2p/              # Wi-Fi P2P example
```

---

## 🔄 Continuous Improvement Cycle (OpenSpec-Driven)

```
┌─────────────────┐
│ PRD.md          │ ← Product Manager: Business requirements
│ (Business)      │    Features, user stories, success metrics
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ OpenSpec Specs  │ ← Developer: Technical specifications
│ (Technical)     │    architecture.md, module specs, APIs
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  IMPLEMENT      │ ← Follow OpenSpec specs
│                 │    Create modules, state machines, tests
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│   REVIEW        │ ← Validate against PRD + OpenSpec
│                 │    Business requirements + Technical specs
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  QA REPORT      │ ← Findings from both perspectives
└────────┬────────┘
         │
         ├───────────────────┐
         ▼                   ▼
┌─────────────────┐  ┌─────────────────┐
│  FIX ISSUES     │  │   IMPROVE       │
│                 │  │ - PRD template  │ ← Business lessons
│                 │  │ - OpenSpec cfg  │ ← Technical lessons
└─────────────────┘  └────────┬────────┘
                              │
                              ▼
                    (Better next project!)
```

---

## 🆘 Common Issues & Quick Fixes

### Build Fails
- **Missing Zephyr**: Run `west update`
- **Kconfig error**: Check prj.conf syntax
- **Linker error**: Increase stack/heap sizes

### Wi-Fi Connection Fails
- **Wrong credentials**: Verify SSID/password via shell
- **Out of memory**: Increase `CONFIG_HEAP_MEM_POOL_SIZE≥80000`
- **DHCP timeout**: Check network, increase timeout values

### P2P Connection Fails
- **GO negotiation timeout**: Check both devices have P2P enabled
- **Role assignment wrong**: Adjust GO intent (0-15)
- **DHCP fails**: Add timing delays (4-way handshake, DHCP start)

### Review Fails
- **Missing PRD.md**: Copy from templates/PRD_TEMPLATE.md (Product Manager)
- **Missing OpenSpec**: Create openspec/ directory and specs (Developer)
- **PRD out of date**: Update business requirements as needed
- **Specs out of date**: Update technical specs to match implementation
- **Missing files**: Copy from templates/
- **Hardcoded credentials**: Move to .gitignore overlay
- **Poor documentation**: Use templates/README_TEMPLATE.md
- **Copyright year outdated**: Use **current year** (e.g. 2026) in new files

---

## 📞 Getting Help

**Start here (Product Manager)**:
- Copy `templates/PRD_TEMPLATE.md` to your project
- Complete feature selection checklist
- Define business requirements and acceptance criteria

**Start here (Developer)**:
- Read PRD.md to understand requirements
- Create `openspec/` directory structure
- Write technical specs for each module in `openspec/specs/`
- Create modular structure: `src/modules/[module-name]/`
- Each module gets: code + Kconfig + config template
- Follow specs during implementation

**Module configuration workflow**:
1. Create module with config template:
   ```
   src/modules/mqtt/
   ├── mqtt.c, mqtt.h          # Implementation
   ├── Kconfig.mqtt            # Module Kconfig
   ├── CMakeLists.txt          # Build rules
   └── mqtt.conf.template      # Config fragment (copy from skill library)
   ```
2. Enable module in `prj.conf`: `CONFIG_MQTT_MODULE_ENABLED=y`
3. Merge module config: `cat src/modules/mqtt/mqtt.conf.template >> prj.conf`
4. Build: `west build -p -b <board>` (no overlay needed)

**Skill library overlays are templates**:
- Copy content into your module's `.conf.template` file
- Don't use with `-DEXTRA_CONF_FILE` (except for secrets)

**For project generation**:
- PM: Define features in PRD.md
- Dev: Create OpenSpec specs + modular structure
- See `guides/PROJECT_STRUCTURE.md` for module organization
- See `guides/CONFIG_GUIDE.md` for configuration strategy
- Check `guides/WIFI_GUIDE.md` for Wi-Fi
- Review `examples/` for reference

**For reviews**:
- Verify PRD.md (business) and openspec/ (technical) exist
- Use `ProductManager/ncs/review/CHECKLIST.md`
- Run `ProductManager/ncs/review/check_project.sh`
- Compare: PRD requirements + OpenSpec specs vs implementation

**For template improvements**:
- Document in `ProductManager/ncs/review/IMPROVEMENT_GUIDE.md`
- Update affected templates (especially PRD_TEMPLATE.md)
- Test with new project

**External resources**:
- [NCS Documentation](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/index.html)
- [Nordic DevZone](https://devzone.nordicsemi.com/)
- [Zephyr Docs](https://docs.zephyrproject.org/latest/)

---

## 🎯 Summary

This skill provides everything needed for professional NCS project development:

✅ **Dual Documentation**: PRD.md (business) + OpenSpec (technical)  
✅ **Clear Separation**: "What to build" vs "How to build"  
✅ **Generate**: Proven templates and configurations  
✅ **Review**: Validation against both PRD and specs  
✅ **Improve**: Continuous enhancement of both layers  
✅ **Wi-Fi**: Complete guide for all modes  
✅ **Features**: Modular overlay system for 12+ features  
✅ **Security**: Best practices and checks  
✅ **Quality**: Automated + manual validation  

### 🆕 Feature Selection System

**12 Core Features Available**:

**Wi-Fi Features:**
- Wi-Fi Shell - Interactive commands
- Wi-Fi STA - Station mode
- Wi-Fi SoftAP - Access point mode
- Wi-Fi P2P - Wi-Fi Direct

**Network Protocols:**
- UDP - Fast datagram protocol
- TCP - Reliable stream protocol
- MQTT - IoT messaging
- HTTP Client - REST API calls
- HTTPS Server - Web interface
- CoAP - Constrained protocol

**Advanced Features:**
- Memfault - Cloud monitoring & OTA
- BLE Provisioning - Credential setup
Architecture Patterns (NEW!):**
- Simple Multi-Threaded - Traditional approach (queues, semaphores)
- SMF + zbus Modular - Nordic's recommended pattern for complex systems

**
**Build with any combination**:
```bash
west build -p -b nrf7002dk/nrf5340/cpuapp -- \
  -DEXTRA_CONF_FILE="wifi-sta.conf;overlay-mqtt.conf;overlay-memfault.conf"
```

**Complete documentation** in `guides/FEATURE_SELECTION.md`:
- Detailed config requirements
- Memory requirements
- Code examples
- Dependencies
- Common combinations

**Start building better NCS projects today!** 🚀
