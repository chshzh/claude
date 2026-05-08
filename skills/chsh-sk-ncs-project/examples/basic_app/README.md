# Basic NCS Application Example

Simple Nordic nRF Connect SDK application demonstrating:
- LED control with DK library
- Button handling
- Logging subsystem
- Shell commands
- Kconfig options
- Proper project structure

## Features

- **LED Blinking**: Toggles LED1 at configurable interval
- **Button Control**: 
  - Button 1: Pause/resume application
  - Button 2: Toggle between fast (250ms) and slow (1000ms) blinking
- **Shell Commands**: Control via UART
- **Configuration**: Kconfig-based settings

## Building

```bash
cd examples/basic_app
# Choose your board
west build -p -b nrf7002dk/nrf5340/cpuapp  # nRF7002 DK
# or
west build -p -b nrf54l15dk/nrf54l15/cpuapp  # nRF54L15 DK
# or
west build -p -b nrf54lm20dk/nrf54lm20/cpuapp  # nRF54LM20 DK
# or
west build -p -b nrf5340dk/nrf5340/cpuapp  # nRF5340 DK
# or
west build -p -b nrf52840dk/nrf52840  # nRF52840 DK

west flash
```

## Supported Boards

**WiFi-capable boards** (Nordic nRF70 Series):
- nRF7002 DK (nrf7002dk/nrf5340/cpuapp)
- nRF54L15 DK + nRF7002EB (nrf54l15dk/nrf54l15/cpuapp)
- nRF54LM20 DK + nRF7002EB (nrf54lm20dk/nrf54lm20/cpuapp)

**General purpose boards**:
- nRF5340 DK (nrf5340dk/nrf5340/cpuapp)
- nRF52840 DK (nrf52840dk/nrf52840)
- Any board with DK_LIBRARY support

## Shell Commands

Connect to UART (115200 8N1):

```
uart:~$ app start       # Start LED blinking
uart:~$ app stop        # Stop LED blinking
uart:~$ app interval 500  # Set 500ms interval
```

## Configuration Options

In `prj.conf` or Kconfig:

- `CONFIG_BASIC_APP_LED_BLINK_INTERVAL_MS`: Default blink interval (100-10000ms)
- `CONFIG_BASIC_APP_ENABLE_SHELL`: Enable shell commands

## Project Structure

```
basic_app/
├── CMakeLists.txt      # Build configuration
├── Kconfig             # Configuration options
├── prj.conf            # Default configuration
├── README.md           # This file
└── src/
    └── main.c          # Application code
```

## Code Overview

**main.c**:
- `button_handler()`: Handles button presses
- `cmd_*()`: Shell command implementations
- `main()`: Initialization and main loop

**Key patterns**:
- DK library for buttons/LEDs (dk_buttons_and_leds.h)
- Logging with module registration
- Shell commands with SHELL_CMD_REGISTER
- Kconfig integration for runtime options
- Proper copyright headers

## Extending

**Add new feature**:
1. Add Kconfig option in `Kconfig`
2. Add configuration in `prj.conf`
3. Implement in `src/main.c`
4. Add shell commands if needed

**Add new module**:
1. Create `src/my_module.c` and `src/my_module.h`
2. Add to `CMakeLists.txt`: `target_sources(app PRIVATE src/my_module.c)`
3. Register logging: `LOG_MODULE_REGISTER(my_module, LOG_LEVEL_INF);`

## License

Copyright (c) 2026 Nordic Semiconductor ASA  
SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
