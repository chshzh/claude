# NCS Project Configuration Guide

## Configuration Strategy

### Module Co-Located Configs (Recommended)

**Each module has its config fragment alongside its code:**

```
src/modules/mqtt/
├── mqtt.c
├── mqtt.h
├── Kconfig.mqtt
├── CMakeLists.txt
└── mqtt.conf.template       ← Config fragment
```

**Workflow:**
1. **Enable module** in `prj.conf`: `CONFIG_MQTT_MODULE_ENABLED=y`
2. **Merge module config** into `prj.conf`:
   ```bash
   echo "\n# MQTT Module" >> prj.conf
   cat src/modules/mqtt/mqtt.conf.template >> prj.conf
   ```
3. **Build normally**: `west build -p -b <board>` (no overlay needed)

### Use Overlays Only For:

- 🔐 **Secrets**: API keys, credentials (in .gitignore)
- 🔧 **Environment**: Dev vs production differences
- 🧪 **Testing**: Temporary one-off experiments

**Example:**
```properties
# overlay-credentials.conf (git-ignored)
CONFIG_MEMFAULT_NCS_PROJECT_KEY="secret-key"
CONFIG_AWS_IOT_ENDPOINT="my-endpoint.iot.region.amazonaws.com"
```

### Skill Library Overlays are TEMPLATES

Overlays in `~/.claude/skills/ProductManager/ncs/features/overlays/` are:
- ✅ Reference templates to copy from
- ❌ NOT meant for `-DEXTRA_CONF_FILE` usage

**Correct usage:**
```bash
# Copy template content into your module's config
cat ~/.claude/skills/ProductManager/ncs/features/overlays/overlay-mqtt.conf \
    > src/modules/mqtt/mqtt.conf.template

# Then merge into prj.conf when enabling module
cat src/modules/mqtt/mqtt.conf.template >> prj.conf
```

## Base Configuration Templates

### 1. Basic NCS Application (prj.conf)

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
```

---

### 2. Wi-Fi Station Mode (overlay-wifi-sta.conf)

```properties
#
# Wi-Fi Station Mode Configuration
#

# Wi-Fi driver
CONFIG_WIFI=y
CONFIG_WIFI_NRF70=y

# Wi-Fi ready library
CONFIG_WIFI_READY_LIB=y

# Networking
CONFIG_NETWORKING=y
CONFIG_NET_SOCKETS=y
CONFIG_NET_LOG=y
CONFIG_NET_IPV4=y
CONFIG_NET_UDP=y
CONFIG_NET_TCP=y

# Network buffers
CONFIG_NET_PKT_RX_COUNT=16
CONFIG_NET_PKT_TX_COUNT=16
CONFIG_NET_BUF_RX_COUNT=32
CONFIG_NET_BUF_TX_COUNT=32
CONFIG_NRF70_RX_NUM_BUFS=16

# Wi-Fi memory configuration
CONFIG_HEAP_MEM_POOL_SIZE=80000
CONFIG_HEAP_MEM_POOL_IGNORE_MIN=y
CONFIG_NRF_WIFI_CTRL_HEAP_SIZE=40000
CONFIG_NRF_WIFI_DATA_HEAP_SIZE=100000

# Network interface configuration
CONFIG_NET_IF_UNICAST_IPV4_ADDR_COUNT=2
CONFIG_NET_MAX_CONTEXTS=10
CONFIG_NET_CONTEXT_SYNC_RECV=y

# Ethernet L2
CONFIG_NET_L2_ETHERNET=y

# Zephyr NET Connection Manager
CONFIG_NET_CONNECTION_MANAGER=y
CONFIG_L2_WIFI_CONNECTIVITY=y
CONFIG_L2_WIFI_CONNECTIVITY_AUTO_CONNECT=y
CONFIG_L2_WIFI_CONNECTIVITY_AUTO_DOWN=n

# Wi-Fi credentials storage
CONFIG_WIFI_CREDENTIALS=y
CONFIG_WIFI_CREDENTIALS_CONNECT_STORED=y
CONFIG_FLASH=y
CONFIG_FLASH_PAGE_LAYOUT=y
CONFIG_FLASH_MAP=y
CONFIG_NVS=y
CONFIG_SETTINGS=y
CONFIG_SETTINGS_NVS=y

# DHCP client
CONFIG_NET_DHCPV4=y

# Network shell
CONFIG_NET_SHELL=y

# Disable auto network init (handle manually)
CONFIG_NET_CONFIG_SETTINGS=n

# Stack sizes
CONFIG_NET_TX_STACK_SIZE=4096
CONFIG_NET_RX_STACK_SIZE=4096
```

---

### 3. Wi-Fi SoftAP Mode (overlay-wifi-softap.conf)

```properties
#
# Wi-Fi SoftAP Mode Configuration
#

# Include base Wi-Fi config (you can also include overlay-wifi-sta.conf)
# Or duplicate the necessary Wi-Fi configs here

# SoftAP mode
CONFIG_NRF70_AP_MODE=y
CONFIG_WIFI_NM_WPA_SUPPLICANT=y
CONFIG_WIFI_NM_WPA_SUPPLICANT_AP=y

# DHCP Server for SoftAP
CONFIG_NET_DHCPV4_SERVER=y

# SoftAP credentials
CONFIG_SOFTAP_SSID="my-softap"
CONFIG_SOFTAP_PASSWORD="mypassword123"

# Static IP for SoftAP
# (Typically configured in code: 192.168.1.1)
```

---

### 4. Wi-Fi P2P Mode (overlay-wifi-p2p.conf)

```properties
#
# Wi-Fi Direct (P2P) Mode Configuration
#

# Wi-Fi P2P support
CONFIG_NRF70_P2P_MODE=y
CONFIG_NRF70_AP_MODE=y
CONFIG_WIFI_NM_WPA_SUPPLICANT=y
CONFIG_WIFI_NM_WPA_SUPPLICANT_P2P=y
CONFIG_WIFI_NM_WPA_SUPPLICANT_AP=y
CONFIG_WIFI_NM_WPA_SUPPLICANT_WPS=y
CONFIG_WIFI_NM_WPA_SUPPLICANT_EAPOL=y
CONFIG_WIFI_NM_WPA_SUPPLICANT_INF_MON=y

# Disable low power for stable P2P operation
CONFIG_NRF_WIFI_LOW_POWER=n

# DHCP Server for GO role
CONFIG_NET_DHCPV4_SERVER=y

# DHCP Client for CLI role
CONFIG_NET_DHCPV4=y

# Increased memory for P2P
CONFIG_HEAP_MEM_POOL_SIZE=100000
CONFIG_NRF_WIFI_CTRL_HEAP_SIZE=50000
CONFIG_NRF_WIFI_DATA_HEAP_SIZE=120000

# P2P specific
CONFIG_WIFI_P2P_MAX_PEERS=5

# LTO optimization
CONFIG_LTO=y
CONFIG_ISR_TABLES_LOCAL_DECLARATION=y
```

---

### 5. Wi-Fi Raw Packet Mode (overlay-wifi-raw.conf)

```properties
#
# Wi-Fi Raw Packet / Monitor Mode Configuration
#

# Monitor mode support
CONFIG_WIFI_NM_WPA_SUPPLICANT_INF_MON=y

# Increased MTU for raw packets
CONFIG_NRF_WIFI_IFACE_MTU=1500

# Raw packet injection (if supported)
# CONFIG_RAW_TX_ENABLE=y

# Disable low power for monitoring
CONFIG_NRF_WIFI_LOW_POWER=n
```

---

### 6. Debug Configuration (overlay-debug.conf)

```properties
#
# Debug Configuration
#

# Enhanced logging
CONFIG_LOG=y
CONFIG_LOG_MODE_IMMEDIATE=y
CONFIG_LOG_BUFFER_SIZE=8192
CONFIG_LOG_PRINTK=y

# Wi-Fi debug logging
CONFIG_WIFI_NM_WPA_SUPPLICANT_LOG_LEVEL_DBG=y
CONFIG_NET_LOG=y
CONFIG_NET_SHELL=y

# Stack usage tracking
CONFIG_THREAD_STACK_INFO=y
CONFIG_THREAD_MONITOR=y
CONFIG_INIT_STACKS=y

# Assert and error checking
CONFIG_ASSERT=y
CONFIG_ASSERT_LEVEL=2

# Core dump
CONFIG_DEBUG_COREDUMP=y
CONFIG_DEBUG_COREDUMP_BACKEND_LOGGING=y

# Disable optimizations for better debugging
CONFIG_NO_OPTIMIZATIONS=y
CONFIG_DEBUG=y
```

---

### 7. Static Wi-Fi Credentials (overlay-wifi-static.conf)

```properties
#
# Static Wi-Fi Credentials Configuration
#

# Static credentials (hardcoded)
CONFIG_WIFI_CREDENTIALS_STATIC=y
CONFIG_WIFI_CREDENTIALS_STATIC_SSID="YourSSID"
CONFIG_WIFI_CREDENTIALS_STATIC_PASSWORD="YourPassword"
CONFIG_WIFI_CREDENTIALS_STATIC_TYPE_WIFI_PSK=y

# Auto-connect
CONFIG_L2_WIFI_CONNECTIVITY_AUTO_CONNECT=y
```

---

### 8. Sysbuild Configuration (sysbuild.conf)

```properties
#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# MCUboot bootloader
CONFIG_BOOTLOADER_MCUBOOT=y

# Partition manager
CONFIG_PARTITION_MANAGER=y

# Network core (for nRF5340 + nRF70)
CONFIG_WIFI_NRF70_SYSTEM_WITH_RAW_MODES=y

# For nRF7002 DK specifically
# CONFIG_WIFI_NRF70_SYSTEM_WITH_WIFI_CORE=y
```

---

## Build Command Examples

### Basic Build
```bash
west build -p -b nrf52840dk/nrf52840
```

### Wi-Fi Station Mode (nRF7002 DK)
```bash
west build -p -b nrf7002dk/nrf5340/cpuapp
```

### Wi-Fi with Shield (e.g., nRF5340 Audio DK + nRF7002 EK)
```bash
west build -p -b nrf5340_audio_dk/nrf5340/cpuapp -- -DSHIELD=nrf7002ek
```

### Wi-Fi Station with Static Credentials
```bash
west build -p -b nrf7002dk/nrf5340/cpuapp -- \
    -DEXTRA_CONF_FILE=overlay-wifi-static.conf
```

### Wi-Fi SoftAP Mode
```bash
west build -p -b nrf7002dk/nrf5340/cpuapp -- \
    -DEXTRA_CONF_FILE=overlay-wifi-softap.conf
```

### Wi-Fi P2P Mode
```bash
west build -p -b nrf7002dk/nrf5340/cpuapp -- \
    -DEXTRA_CONF_FILE=overlay-wifi-p2p.conf
```

### Multiple Overlays
```bash
west build -p -b nrf7002dk/nrf5340/cpuapp -- \
    -DEXTRA_CONF_FILE="overlay-wifi-sta.conf;overlay-debug.conf"
```

### With Sysbuild
```bash
west build -p -b nrf7002dk/nrf5340/cpuapp --sysbuild
```

---

## Notes

1. **Order Matters**: When using multiple overlays, later files override earlier ones
2. **Board Files**: Create board-specific `.conf` and `.overlay` files in `boards/` directory
3. **Memory**: Adjust heap and stack sizes based on your application needs
4. **Low Power**: Enable `CONFIG_NRF_WIFI_LOW_POWER` for production, disable for development
5. **Credentials**: Never commit static credentials to public repositories
6. **Shields**: Use `-DSHIELD=` for external Wi-Fi modules

## Best Practices

- Start with minimal `prj.conf` and add features via overlays
- Use separate overlays for different features/modes
- Document configuration options in Kconfig
- Test with debug overlay during development
- Use static credentials only for testing, prefer credential shell for production
