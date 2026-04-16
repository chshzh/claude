# Nordic nRF Connect SDK (NCS) Debugging Reference

## Key Concepts

**Debugging Methods:**
- **GDB/LLDB**: Interactive debugging with breakpoints and stepping
- **RTT (Real-Time Transfer)**: Low-overhead logging via SEGGER RTT
- **UART Logging**: Traditional serial console output
- **Core Dumps**: Post-mortem analysis of crashes

## Critical Rules

1. **Always Collect Environmental Baseline**: Request Device HW, NCS Version, Firmware/Software Versions, Router Brand/Version before starting.
2. **Always ensure NCS environment is set up** before debugging (use ncs-env-setup skill)
3. **Verify debugger connection** to the target device before starting debug session
4. **Check logging backend configuration** in prj.conf

## Workflow for Debugging
0: Information Gathering (Mandatory)

Before investigating, explicitly request and document:
- **Device HW**: Board name/version (e.g., nRF7002DK PCA10143 v1.0.1)
- **NCS Version**: SDK version tag (e.g., v2.9.0, v3.2.1)
- **Firmware Version**: Application version or git commit hash
- **Router Details**: Brand, Model, Firmware Version (critical for Wi-Fi issues)
- **Logs**: Relevant logs (boot sequence, error messages, crash dumps)

### Step 
### Step 1: Verify Build Configuration

Check that the project is built with debug symbols:
```sh
west build -b <board> -- -DCMAKE_BUILD_TYPE=Debug
```

### Step 2: Choose Debugging Method

**For Interactive Debugging (GDB/LLDB):**
1. Connect debugger (J-Link, etc.)
2. Start debug server
3. Attach GDB/LLDB

**For RTT Logging:**
1. Enable RTT in prj.conf:
   ```
   CONFIG_USE_SEGGER_RTT=y
   CONFIG_RTT_CONSOLE=y
   CONFIG_LOG_BACKEND_RTT=y
   ```
2. Use J-Link RTT Viewer or nRF Connect

**For UART Logging:**
1. Configure UART in prj.conf
2. Connect serial terminal (minicom, screen, etc.)

### Step 3: Common Debug Commands

**Start GDB debugging session:**
```sh
west debug
```

**Flash and attach debugger:**
```sh
west flash && west attach
```

**View RTT output:**
```sh
# Using J-Link RTT Client
JLinkRTTClient
```

## Common Debug Configurations

### Enable Debug Logging

Add to `prj.conf`:
```
CONFIG_LOG=y
CONFIG_LOG_DEFAULT_LEVEL=4
CONFIG_DEBUG=y
CONFIG_ASSERT=y
```

### Enable Thread Analysis

```
CONFIG_THREAD_ANALYZER=y
CONFIG_THREAD_ANALYZER_AUTO=y
CONFIG_THREAD_ANALYZER_AUTO_INTERVAL=5
```

### Enable Stack Sentinel

```
CONFIG_STACK_SENTINEL=y
```

## Troubleshooting

### Debugger Connection Issues

Check J-Link connection:
```sh
JLinkExe
```

Reset target device and retry.

### RTT Not Working

1. Verify RTT buffer configuration
2. Check J-Link firmware version
3. Ensure correct target device selected

### Missing Debug Symbols

Rebuild with debug configuration:
```sh
west build -b <board> -p -- -DCMAKE_BUILD_TYPE=Debug
```

## Example Workflows

### Debug Build and Flash
```sh
# Set up environment (see ncs-env-setup)
# Build with debug symbols
west build -b nrf52840dk_nrf52840 -p -- -DCMAKE_BUILD_TYPE=Debug
# Flash and start debugging
west debug
```

### View RTT Logs
```sh
# In one terminal: flash the app
west flash
# In another terminal: view RTT output
JLinkRTTClient
```

## Notes

- Debug builds are larger and slower than release builds
- RTT is preferred over UART for performance-critical debugging
- Use `west build -t menuconfig` to explore available debug options
- Check Nordic DevZone for device-specific debugging tips

## Self-Update Policy

At the **end of each conversation**, review what was discovered and check whether any facts in this skill are new, corrected, or outdated (e.g. new GDB tricks, new RTT patterns, new crash analysis techniques).

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
