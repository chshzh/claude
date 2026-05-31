# Crash and Hardfault Analysis

Detailed crash diagnosis for NCS/Zephyr applications on Nordic hardware.
Referenced from [chsh-sk-ncs-3.2-debug Mode C](../SKILL.md#mode-c--crash-and-hardfault-analysis).

## C1. Enable crash logging

```kconfig
CONFIG_FAULT_DUMP=2
CONFIG_EXTRA_EXCEPTION_INFO=y
CONFIG_DEBUG_COREDUMP=y
CONFIG_DEBUG_COREDUMP_BACKEND_LOGGING=y
```

## C2. Capture the crash dump

```
FATAL ERROR: BUS FAULT
   Executing thread (tid: 0x20003abc, name: "wifi_mgmt"):
   ...
   Faulting instruction address (r15/pc): 0x0005e2f4
```

Use the ELF to decode:
```bash
arm-zephyr-eabi-addr2line -e build/zephyr/zephyr.elf 0x0005e2f4
```

## C3. GDB attach

```bash
nrfutil sdk-manager toolchain launch --ncs-version=${NCS_VERSION:-v3.3.0} -- \
  west debug -d <app>/build
```

```gdb
(gdb) bt full            # backtrace with locals
(gdb) info thread        # all threads
(gdb) frame <n>          # switch frame
(gdb) p <variable>       # print variable
```

## C4. Stack overflow via "hidden fault dump" pattern

Zephyr's default `CONFIG_LOG_MODE_DEFERRED=y` queues log messages. When a USAGE FAULT
(stack overflow) triggers a software reset at full queue capacity, the fault dump is
dropped and the device reboots silently. Symptoms:

- Log shows `--- N messages dropped ---` immediately before the reboot line
- `RESETREAS: 0x00000040` — SREQ (software reset by `NVIC_SystemReset()` in fault handler)
- `Thread Analyzer` shows peak usage near or at the configured stack size

**Workaround:** switch to immediate mode to make the fault dump print synchronously:

```kconfig
CONFIG_LOG_MODE_DEFERRED=n
CONFIG_LOG_MODE_IMMEDIATE=y
# Immediate mode causes each log call to process in the calling thread, which
# significantly increases stack depth. Increase thread_analyzer auto stack:
CONFIG_THREAD_ANALYZER_AUTO_STACK_SIZE=4096
```

Rebuild and flash. The next fault will print the full register dump including PC, LR,
PSP, and RESETREAS before the reboot. Revert both changes after the fault is diagnosed.

## C5. Decode fault dump — addr2line and thread identification

**Always use the Zephyr SDK `addr2line` — NOT `arm-none-eabi-addr2line`:**

```bash
# NCS v3.3.0 macOS toolchain path
ADDR2LINE="/opt/nordic/ncs/toolchains/0c0f19d91c/opt/zephyr-sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-addr2line"
ELF="<app>/build_<board>/zephyr/zephyr.elf"

# Decode faulting instruction (PC) and calling function (LR)
$ADDR2LINE -e $ELF -f -p <PC_HEX>
$ADDR2LINE -e $ELF -f -p <LR_HEX>
```

**Identify crashing thread from PSP** using the map file:

```python
import re

MAP = "<app>/build_<board>/zephyr/zephyr.map"
PSP = 0x<psp_from_fault_dump>          # e.g. 0x20050570

stacks = []
with open(MAP) as f:
    for line in f:
        m = re.search(r'(0x[0-9a-f]+)\s+(0x[0-9a-f]+)\s+(.+)', line)
        if m:
            addr, size = int(m.group(1), 16), int(m.group(2), 16)
            if 0x20000000 <= addr <= 0x20100000 and size > 0:
                stacks.append((addr, size, m.group(3).strip()))

for addr, size, name in sorted(stacks):
    if addr <= PSP <= addr + size:
        print(f"MATCH: 0x{addr:08x}+0x{size:x}=0x{addr+size:08x}: {name}")
```

**Key register meanings:**

| Register | Meaning |
|----------|---------|
| `r15/pc` | Faulting instruction — decode with addr2line |
| `r14/lr` | Return address in caller — decode with addr2line |
| `psp` | Stack pointer at fault — use map script to identify thread |
| `EXC_RETURN=0xffffffed` | Thread mode, PSP, extended FP frame (~104 bytes stacked) |
| `RESETREAS=0x00000040` | SREQ — fault handler called `sys_reboot()` |

**Common Wi-Fi reconnect stack overflow (sysworkq):**

The reconnect path (`wpa_cli_cmd_remove_network`) runs on `sysworkq` and allocates
large on-stack buffers deep in the call chain:

| Frame | Stack allocation |
|-------|-----------------|
| `wpa_cli_cmd` (`wpa_cli_cmds.c`) | `buf[1024]` (CMD_BUF_LEN) |
| `_wpa_ctrl_command` (`wpa_cli_zephyr.c`) | `buf[512]` (CMD_BUF_LEN) |
| `zvfs_poll_internal` | `poll_events[CONFIG_ZVFS_POLL_MAX=20]` ≈ 400 B |
| Call overhead + saved regs | ~552 B |
| **Total peak (nRF54LM20DK)** | **~4488 B** |

Fix: `CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=6144` (formula: floor(4488/0.9)=4988 minimum).
