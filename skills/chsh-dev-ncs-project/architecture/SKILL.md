---
name: chsh-dev-ncs-project-architecture
description: Reference for choosing between Simple Multi-Threaded and SMF+zbus architecture patterns in Nordic NCS projects. Includes ready-to-copy templates for both. Use when designing a new NCS project, deciding which architecture pattern fits the use case, or scaffolding modules from templates.
---

# Architecture Patterns Reference

Choose the right architecture for your Nordic NCS project.

## 🎯 Two Patterns Available

### 1. Simple Multi-Threaded

**Best for**: 1-3 threads, simple control flow, quick prototypes

```bash
cp ~/.claude/skills/chsh-dev-ncs-project/architecture/simple-multithreaded/templates/* src/
cp ~/.claude/skills/chsh-pm-ncs-prd/overlays/overlay-multithreaded.conf .
```

**Features**:
- K_MSGQ for inter-thread communication
- K_SEM for synchronization
- K_MUTEX for resource protection
- Atomic state management
- Blocking and non-blocking APIs

**Memory**: 8KB heap (lower overhead)

### 2. SMF + zbus Modular

**Best for**: 4+ modules, complex state machines, production systems

```bash
# Copy example modules (choose what you need)
cp -r ~/.claude/skills/chsh-dev-ncs-project/architecture/smf-zbus/modules/button_example src/modules/button
cp -r ~/.claude/skills/chsh-dev-ncs-project/architecture/smf-zbus/modules/sensor_example src/modules/sensor
cp -r ~/.claude/skills/chsh-dev-ncs-project/architecture/smf-zbus/modules/data_processor_example src/modules/data_processor

# Copy common message definitions
cp ~/.claude/skills/chsh-dev-ncs-project/architecture/smf-zbus/modules/common/messages.h src/modules/

# Copy configuration overlay
cp ~/.claude/skills/chsh-pm-ncs-prd/overlays/overlay-smf-zbus.conf .
```

**Features**:
- State Machine Framework (SMF)
- Message bus (zbus) for decoupled communication
- Task watchdog integration
- Run-to-completion model
- Follows Nordic Asset Tracker Template pattern
- **Professional module organization**: Each module is self-contained with CMakeLists.txt, Kconfig, source files

**Memory**: 16KB heap (higher for message bus)

## 📖 Comprehensive Guide

**[ARCHITECTURE_PATTERNS.md](guides/ARCHITECTURE_PATTERNS.md)** (~20,000 tokens)
- Deep comparison of both patterns
- Decision tree for choosing
- Full implementation examples
- Migration path simple → modular
- Best practices from Nordic

## 📊 Quick Comparison

| Aspect | Simple Multi-Threaded | SMF+zbus Modular |
|--------|----------------------|------------------|
| **Complexity** | Low | Medium |
| **Scalability** | Poor (3+ threads) | Excellent |
| **Coupling** | Tight | Loose |
| **Memory** | 8KB heap | 16KB heap |
| **Setup Time** | < 1 hour | 2-4 hours |
| **Maintenance** | Ha (Asset Tracker Template Pattern)

**Complete module examples** (copy into your `src/modules/` directory):

- **`modules/button_example/`** - Button input handler
  - `button.c`, `button.h` - State machine for button events
  - `CMakeLists.txt` - Module build configuration
  - `Kconfig.button` - Module-specific Kconfig options
  - Demonstrates: GPIO input, debouncing, zbus publishing

- **`modules/sensor_example/`** - Environmental sensor module
  - `sensor.c`, `sensor.h` - Periodic sensor reading with SMF
  - `CMakeLists.txt`, `Kconfig.sensor`
  - Demonstrates: Sensor drivers, periodic sampling, data publishing

- **`modules/data_processor_example/`** - Data processing module
  - `data_processor.c`, `data_processor.h` - Process incoming data
  - `CMakeLists.txt`, `Kconfig.data_processor`
  - Demonstrates: zbus subscription, data transformation, state management

- **`modules/common/messages.h`** - Shared message definitions for all modules

Each module is **production-ready** and follows Nordic's best practices from Asset Tracker Template.
- `module_template_simple.c` (~450 lines)
- `module_template_simple.h`
- `Kconfig.module_template_simple`

### SMF + zbus Modular
- `module_template_smf.c` (~400 lines)
- `module_template_smf.h`
- `Kconfig.module_template`
- `messages.h` (common message definitions)

## 🚀 Usage

### Simple Multi-Threaded
```bash
# Copy template and build
cp ~/.claude/skills/chsh-dev-ncs-project/architecture/simple-multithreaded/templates/* src/
west build -p -b nrf7002dk/nrf5340/cpuapp -- \
  -DEXTRA_CONF_FILE="overlay-multithreaded.conf"
```

### SMF + zbus Modular (Recommended)
```bash
# 1. Create modules directory structure
mkdir -p src/modules

# 2. Copy example modules (customize module names as needed)
cp -r ~/.claude/skills/chsh-dev-ncs-project/architecture/smf-zbus/modules/button_example src/modules/button
cp -r ~/.claude/skills/chsh-dev-ncs-project/architecture/smf-zbus/modules/sensor_example src/modules/sensor
cp ~/.claude/skills/chsh-dev-ncs-project/architecture/smf-zbus/modules/common/messages.h src/modules/

# 3. In your main CMakeLists.txt, add:
#    add_subdirectory(src/modules/button)
#    add_subdirectory(src/modules/sensor)

# 4. Build
west build -p -b nrf7002dk/nrf5340/cpuapp -- \
  -DEXTRA_CONF_FILE="wifi-sta.conf;overlay-smf-zbus.conf"
```

**Module Organization** (follows Asset Tracker Template):
```
your_project/
├── CMakeLists.txt
├── prj.conf
└── src/
    ├── main.c
    └── modules/
        ├── messages.h          # Common message definitions
        ├── button/             # Button module
        │   ├── button.c
        │   ├── button.h
        │   ├── CMakeLists.txt
        │   └── Kconfig.button
        └── sensor/             # Sensor module
            ├── sensor.c
            ├── sensor.h
            ├── CMakeLists.txt
            └── Kconfig.sensor
```

For complete details and decision guidance, see [ARCHITECTURE_PATTERNS.md](guides/ARCHITECTURE_PATTERNS.md)

````
## Self-Update Policy

At the **end of each conversation**, review what was discovered and check whether any facts in this skill are new, corrected, or outdated (e.g. new architecture patterns, directory layout conventions, module templates).

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
