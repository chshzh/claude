# Memory Optimization Configs — Reference

## Flash Reduction

### Top configurations

```
CONFIG_SIZE_OPTIMIZATIONS=y      # Optimize for size (-Os)
CONFIG_LTO=y                     # Link-time optimization
CONFIG_ASSERT=n                  # Disable assertions
CONFIG_LOG=n                     # Disable logging (or use levels below)
CONFIG_LOG_DEFAULT_LEVEL=1       # Errors only (if LOG=y)
CONFIG_LOG_MODE_MINIMAL=y        # Minimal log mode
CONFIG_SHELL=n                   # Disable shell
CONFIG_PRINTK=n                  # Disable printk
CONFIG_EARLY_CONSOLE=n
```

Build in release mode:
```sh
west build -b <board> -- -DCMAKE_BUILD_TYPE=MinSizeRel
```

### Networking stack

```
CONFIG_NET_BUF_DATA_SIZE=128
CONFIG_NET_PKT_RX_COUNT=4
CONFIG_NET_PKT_TX_COUNT=4
CONFIG_NET_IPV6=n                # Disable if IPv4-only
```

### Bluetooth stack

```
CONFIG_BT_BUF_ACL_TX_COUNT=3
CONFIG_BT_BUF_ACL_TX_SIZE=27
CONFIG_BT_CTLR_DATA_LENGTH_MAX=27
CONFIG_BT_OBSERVER=n             # Disable if unused
CONFIG_BT_BROADCASTER=n
```

---

## RAM Reduction

### Thread stacks

Measure first with Thread Analyzer (see SKILL.md), then reduce:

```
CONFIG_MAIN_STACK_SIZE=2048
CONFIG_IDLE_STACK_SIZE=512
CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=1024
CONFIG_BT_RX_STACK_SIZE=1024
CONFIG_NET_RX_STACK_SIZE=2048
CONFIG_NET_TX_STACK_SIZE=2048
```

Margin guidelines by thread type:

| Thread type | Margin |
|---|---|
| Simple worker | 1.2–1.3× |
| Protocol / parser | 1.5× |
| Heavy logging | 1.5–2.0× |
| Touched by ISRs | ≥1.5× |

### Heap

```
# Wi-Fi apps — never go below 64 KB base
CONFIG_HEAP_MEM_POOL_SIZE=80000   # 80 KB base, ~125 KB total

# Non-Wi-Fi minimal
CONFIG_HEAP_MEM_POOL_SIZE=2048

# No dynamic allocation
CONFIG_HEAP_MEM_POOL_SIZE=0
```

### Log and shell buffers

```
CONFIG_LOG_BUFFER_SIZE=512
CONFIG_SHELL_BACKEND_SERIAL_TX_RING_BUFFER_SIZE=64
CONFIG_SHELL_BACKEND_SERIAL_RX_RING_BUFFER_SIZE=64
```

---

## Development vs Production Configs

### Development (full debugging)

```
CONFIG_ASSERT=y
CONFIG_LOG=y
CONFIG_LOG_DEFAULT_LEVEL=4
CONFIG_THREAD_ANALYZER=y
CONFIG_THREAD_NAME=y
CONFIG_STACK_SENTINEL=y
CONFIG_SYS_HEAP_VALIDATE=y
CONFIG_HEAPS_MONITOR=y
```

### Production (minimal footprint)

```
CONFIG_SIZE_OPTIMIZATIONS=y
CONFIG_LTO=y
CONFIG_ASSERT=n
CONFIG_LOG=n
CONFIG_PRINTK=n
CONFIG_SHELL=n
CONFIG_HEAPS_MONITOR=n
```

Build:
```sh
west build -b <board> -- -DCMAKE_BUILD_TYPE=MinSizeRel
```

---

## Stack Overflow Debugging

```
CONFIG_STACK_SENTINEL=y        # Detects overflow at runtime
CONFIG_MPU_STACK_GUARD=y       # MPU protection (ARM Cortex-M)
CONFIG_THREAD_STACK_INFO=y     # Stack bounds info
CONFIG_HW_STACK_PROTECTION=y
```

---

## Partition Manager (Multi-Image Builds)

```sh
cat build/partitions.yml                   # View current layout
west build -t partition_manager_report     # Check fit after changes
```

Custom partition sizes via `pm_static.yml` (or `pm_static_<board>.yml`):

```yaml
mcuboot:
  address: 0x0
  size: 0x10000
app:
  address: 0x10000
  size: 0x70000
```

---

## Profiling Tools

```sh
west build -t rom_report                          # Flash by symbol
west build -t ram_report                          # RAM by symbol
west build -t puncover                             # Interactive HTML
arm-none-eabi-nm --size-sort -S zephyr.elf | tail -20   # Largest symbols
arm-none-eabi-size -A zephyr.elf                  # Section sizes
```
