# NCS Project Structure Guide

Complete guide for organizing Nordic nRF Connect SDK projects.

## Standard Project Structure

```
my_ncs_project/
├── CMakeLists.txt          # REQUIRED: Build configuration
├── Kconfig                 # REQUIRED: Configuration options
├── prj.conf               # REQUIRED: Base + enabled module configs
├── LICENSE                # REQUIRED: Nordic 5-Clause license
├── README.md              # REQUIRED: Project documentation
├── .gitignore             # REQUIRED: Ignore patterns
├── west.yml               # OPTIONAL: Standalone project manifest
├── sysbuild.conf          # OPTIONAL: Multi-image build
├── VERSION                # OPTIONAL: Version tracking
├── overlay-*.conf         # OPTIONAL: Secrets/environment-specific only
│
├── src/                   # REQUIRED: Source code
│   ├── main.c            # Application entry point
│   ├── CMakeLists.txt    # Source-level CMake (optional)
│   └── modules/          # RECOMMENDED: Modular architecture
│       ├── button/
│       │   ├── button.c
│       │   ├── button.h
│       │   ├── Kconfig.button
│       │   └── button.conf.template  # Config fragment for this module
│       ├── wifi/
│       │   ├── wifi.c
│       │   ├── wifi.h
│       │   ├── Kconfig.wifi
│       │   └── wifi.conf.template
│       └── mqtt/
│           ├── mqtt.c
│           ├── mqtt.h
│           ├── Kconfig.mqtt
│           └── mqtt.conf.template
│
├── include/               # OPTIONAL: Public headers
│   └── *.h
│
├── boards/                # OPTIONAL: Board-specific configs
│   ├── board_name.conf
│   └── board_name.overlay
│
├── lib/                   # OPTIONAL: Custom libraries
│   ├── CMakeLists.txt
│   └── my_lib/
│
├── drivers/               # OPTIONAL: Custom drivers
│   └── my_driver/
│
├── dts/                   # OPTIONAL: Device tree
│   └── bindings/
│
├── scripts/               # OPTIONAL: Helper scripts
│   └── *.py/*.sh
│
├── tests/                 # OPTIONAL: Test cases
│   └── */
│
├── doc/                   # OPTIONAL: Documentation
│   ├── PRD.md
│   └── images/
│
└── .github/              # OPTIONAL: CI/CD
    └── workflows/
        └── build.yml
```

## Module Configuration Strategy

### Recommended Approach: Module-Specific Config Fragments

**Each module should have a config template co-located with its code:**

```
src/modules/mqtt/
├── mqtt.c                 # Implementation
├── mqtt.h                 # Header
├── Kconfig.mqtt          # Kconfig options for this module
├── CMakeLists.txt        # Module build rules
└── mqtt.conf.template    # Config fragment for this module
```

**When enabling a module:**
1. Set the module's Kconfig enable flag (e.g., `CONFIG_MQTT_MODULE_ENABLED=y`)
2. Copy the module's config fragment into `prj.conf`
3. Adjust configs as needed for your project

**Example workflow:**
```bash
# Enable MQTT module
# 1. Edit prj.conf: CONFIG_MQTT_MODULE_ENABLED=y
# 2. Copy module configs into prj.conf:
cat src/modules/mqtt/mqtt.conf.template >> prj.conf
# 3. Build normally (no overlay needed):
west build -p -b nrf7002dk/nrf5340/cpuapp
```

### Why Module Co-Located Configs?

**✅ Advantages:**
- Code and config together (better maintainability)
- Clear what configs belong to which module
- Easy to see all enabled features in prj.conf
- No complex overlay file juggling at build time
- Follows OpenSpec pattern (specs describe module configs)

**❌ Avoid:**
- Separate `overlays/` directory for module configs
- Using `-DEXTRA_CONF_FILE` for module features
- Scattering module configs across multiple files

### When to Use Overlay Files

**Only use overlay files for:**
- 🔐 **Secrets**: Credentials, API keys (git-ignored)
- 🔧 **Environment-specific**: Dev vs production differences  
- 🧪 **Temporary testing**: One-off experiments

**Example: Credentials overlay (git-ignored)**
```properties
# overlay-credentials.conf (in .gitignore)
CONFIG_MEMFAULT_NCS_PROJECT_KEY="your-secret-key"
CONFIG_WIFI_SSID="YourNetwork"
CONFIG_WIFI_PASSWORD="YouPassword"
```

### Skill Library Overlays

**The overlays in `~/.claude/skills/ProductManager/ncs/features/overlays/` are TEMPLATES:**

❌ **Don't** use them at build time:
```bash
# Don't do this:
west build -- -DEXTRA_CONF_FILE="~/.claude/skills/.../overlay-mqtt.conf"
```

✅ **Do** copy their contents into your module's config or `prj.conf`:
```bash
# Do this:
cat ~/.claude/skills/ProductManager/ncs/features/overlays/overlay-mqtt.conf >> src/modules/mqtt/mqtt.conf.template
# Then merge mqtt.conf.template into prj.conf when enabling the module
```

## Core Files Details

### CMakeLists.txt (REQUIRED)

```cmake
#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(my_project_name)

# Source files
target_sources(app PRIVATE
    src/main.c
    src/module1.c
    src/module2.c
)

# Include directories (optional)
target_include_directories(app PRIVATE
    include
)

# Subdirectories (optional)
add_subdirectory(lib/my_lib)
```

### Kconfig (REQUIRED)

```kconfig
#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

source "Kconfig.zephyr"

menu "My Project Configuration"

config MY_PROJECT_FEATURE
    bool "Enable my feature"
    default y
    help
      Enable the main feature of this project.

config MY_PROJECT_TIMEOUT_MS
    int "Timeout in milliseconds"
    range 100 60000
    default 5000
    depends on MY_PROJECT_FEATURE
    help
      Timeout for operations (100-60000 ms).

config MY_PROJECT_BUFFER_SIZE
    int "Buffer size in bytes"
    range 64 2048
    default 256
    help
      Size of internal buffers.

endmenu
```

### prj.conf (REQUIRED)

```properties
#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Logging
CONFIG_LOG=y
CONFIG_LOG_BUFFER_SIZE=4096
CONFIG_PRINTK=y

# Shell (for debugging)
CONFIG_SHELL=y

# GPIO and DK library
CONFIG_GPIO=y
CONFIG_DK_LIBRARY=y

# Kernel options
CONFIG_MAIN_STACK_SIZE=4096
CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=2048
CONFIG_ENTROPY_GENERATOR=y
CONFIG_REBOOT=y

# Debugging
CONFIG_STACK_SENTINEL=y
CONFIG_DEBUG_COREDUMP=y
CONFIG_DEBUG_COREDUMP_BACKEND_LOGGING=y
CONFIG_DEBUG_COREDUMP_MEMORY_DUMP_MIN=y

# Timing
CONFIG_POSIX_TIMERS=y

# Project-specific options
CONFIG_MY_PROJECT_FEATURE=y
CONFIG_MY_PROJECT_TIMEOUT_MS=5000
```

### src/main.c (REQUIRED)

```c
/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <dk_buttons_and_leds.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* Application entry point */
int main(void)
{
    int ret;

    LOG_INF("My NCS Project Started");
    LOG_INF("Version: 1.0.0");

    /* Initialize hardware */
    ret = dk_buttons_init(NULL);
    if (ret) {
        LOG_ERR("Failed to initialize buttons: %d", ret);
        return ret;
    }

    ret = dk_leds_init();
    if (ret) {
        LOG_ERR("Failed to initialize LEDs: %d", ret);
        return ret;
    }

    LOG_INF("Initialization complete");

    /* Main application loop */
    while (1) {
        dk_set_led_on(DK_LED1);
        k_msleep(1000);
        dk_set_led_off(DK_LED1);
        k_msleep(1000);
    }

    return 0;
}
```

## Wi-Fi Project Structure

Wi-Fi projects have additional requirements:

```
wifi_project/
├── CMakeLists.txt         # Include Wi-Fi sources
├── Kconfig               # Wi-Fi options
├── prj.conf              # Base config
├── overlay-wifi-sta.conf  # Station mode config
├── LICENSE
├── README.md
├── .gitignore
│
└── src/
    ├── main.c             # Application entry
    ├── wifi_utils.c       # Wi-Fi management
    ├── wifi_utils.h
    ├── net_utils.c        # Network utilities
    ├── net_utils.h
    └── udp_utils.c        # Protocol handlers
```

### Wi-Fi CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wifi_project)

target_sources(app PRIVATE
    src/main.c
    src/wifi_utils.c
    src/net_utils.c
    src/udp_utils.c
)
```

### Wi-Fi Module Pattern

**wifi_utils.h**:
```c
#ifndef WIFI_UTILS_H
#define WIFI_UTILS_H

int wifi_init(void);
int wifi_connect(const char *ssid, const char *psk);
int wifi_disconnect(void);
bool wifi_is_connected(void);

#endif /* WIFI_UTILS_H */
```

**wifi_utils.c**:
```c
#include "wifi_utils.h"
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(wifi_utils, LOG_LEVEL_INF);

int wifi_init(void)
{
    /* Initialize Wi-Fi subsystem */
    LOG_INF("Initializing Wi-Fi");
    return 0;
}

int wifi_connect(const char *ssid, const char *psk)
{
    /* Connect to Wi-Fi network */
    LOG_INF("Connecting to %s", ssid);
    return 0;
}
```

## Common Patterns

### Modular Organization

Group related functionality:

```
src/
├── main.c              # Entry point + coordination
├── wifi/               # Wi-Fi subsystem
│   ├── wifi_mgmt.c
│   ├── wifi_events.c
│   └── wifi_creds.c
├── network/            # Network layer
│   ├── udp_handler.c
│   ├── tcp_handler.c
│   └── dhcp_mgmt.c
└── app/                # Application logic
    ├── data_processor.c
    └── state_machine.c
```

### Header Organization

```
include/
├── wifi/
│   └── wifi_mgmt.h    # Public Wi-Fi API
├── network/
│   └── protocols.h     # Public network API
└── app/
    └── app_api.h       # Public app API
```

## Build Variants

### Multiple Configurations

```
project/
├── prj.conf              # Base config
├── overlay-debug.conf     # Debug build
├── overlay-release.conf   # Release build
├── overlay-wifi-sta.conf  # Station mode
└── overlay-wifi-p2p.conf  # P2P mode
```

Build with overlays:
```bash
west build -b nrf7002dk/nrf5340/cpuapp -- \
    -DEXTRA_CONF_FILE="overlay-wifi-sta.conf;overlay-debug.conf"
```

### Board-Specific Configs

```
boards/
├── nrf7002dk_nrf5340_cpuapp.conf      # Board Kconfig
├── nrf7002dk_nrf5340_cpuapp.overlay   # Board DTS
├── nrf5340_audio_dk_nrf5340_cpuapp.conf
└── nrf5340_audio_dk_nrf5340_cpuapp.overlay
```

## Testing Structure

```
tests/
├── lib/
│   └── my_lib/
│       ├── CMakeLists.txt
│       ├── prj.conf
│       └── src/
│           └── main.c
└── integration/
    └── system_test/
        ├── CMakeLists.txt
        ├── prj.conf
        └── src/
            └── main.c
```

## Documentation Structure

```
doc/
├── PRD.md                    # Product Requirements Document
├── ARCHITECTURE.md           # System architecture
├── API.md                    # API documentation
├── CONFIGURATION.md          # Config options
├── images/                   # Diagrams and screenshots
│   ├── architecture.png
│   └── setup.jpg
└── Doxyfile                  # Doxygen configuration
```

## Best Practices

### 1. File Naming
- **C source**: lowercase with underscores (wifi_utils.c)
- **Headers**: match source name (wifi_utils.h)
- **Configs**: overlay-feature.conf
- **Board files**: board_name_variant.conf/.overlay

### 2. Copyright Headers
All source files must include a copyright header. Use the **current year** in the copyright line (e.g. 2026). For existing files from upstream, keep the year of first publication; for new or substantially modified files, use the current year.

```c
/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
```

### 3. Module Organization
- One module = one responsibility
- Clear interfaces (header files)
- Minimal coupling
- Maximum cohesion

### 4. Configuration Management
- `prj.conf` = common base
- Overlays = specific features
- Board files = hardware-specific
- Keep configs documented

### 5. Documentation
- README.md = user guide
- PRD.md = detailed requirements and design (product manager perspective)
- Code comments = non-obvious logic
- API docs = function contracts

## Quick Start Template

Copy this structure for new projects:

```bash
# Create structure
mkdir -p my_project/{src,boards,scripts,tests,doc}
cd my_project

# Copy templates
cp ~/.claude/skills/Developer/ncs/project/templates/* .

# Create minimal files
touch src/main.c
touch CMakeLists.txt
touch Kconfig
touch prj.conf

# Initialize git
git init
git add .
git commit -m "Initial project structure"
```

See `templates/` directory for complete file templates.
