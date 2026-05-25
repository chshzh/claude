# EEDP Platform Reference

EEDP (Embodied Embedded Development Platform) is the AI-operable test
infrastructure for nRF hardware. When running on EEDP, the agent has direct
access to all hardware modules via REST API and MCP.

## Module Overview

| Module | Interface | What it enables |
|--------|-----------|----------------|
| **Hardware Control** | nRF7002DK GPIO Shell (UART0) | Button press/release (`gpio set/clear`), LED read (`gpio get`) |
| **Serial Terminal** | ttyd Web terminals (4 ports) | UART log + Zephyr shell for all target boards |
| **JLink / Debugger** | `jlink_mcp` MCP | Flash, reset, addr2line crash decode |
| **Router Control** | SSH to ASUS BE92U | AP restart, client block — simulate network failures |
| **PPK2** | `ppk2 measure` CLI | Current measurement for power budget tests |
| **Logic Analyzer** | Saleae MCP | SPI/QSPI/UART bus capture and protocol decode |
| **Wireshark** | Packet capture | 802.11 frame analysis |

## When Not on EEDP

Use `nordicsemi_uart_monitor.py` for UART capture
(load via `mcp_nrflow_nordicsemi_workflow_ncs`):

```bash
python3 nordicsemi_uart_monitor.py --port /dev/cu.usbmodem... --baud 115200
```

Perform flash, reset, and router operations manually.

## Related

- `chsh-sk-ncs-3.2-debug` Mode H — Saleae MCP and JLink MCP detailed setup
- [`eedp-gpio-shell-approach.md`](eedp-gpio-shell-approach.md) — GPIO Shell for button/LED test automation
- wiki: `eedp-platform` — architecture, BOM, MCP integration patterns
