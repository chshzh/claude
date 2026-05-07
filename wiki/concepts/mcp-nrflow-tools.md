---
title: mcp.nrflow Tools — Introduction and Best Practices
created: 2026-05-07
updated: 2026-05-07
type: concept
tags: [ncs, zephyr, debugging, uart, mcp, tools, nrfutil]
sources: []
confidence: high
---

# mcp.nrflow Tools — Introduction and Best Practices

The `mcp_nrflow_*` tools are an MCP (Model Context Protocol) server provided by Nordic
Semiconductor that gives AI agents authoritative, version-aware NCS/Zephyr knowledge and
a pre-built UART interaction script. They are the **primary source of truth** for NCS
development tasks — prefer them over the agent's internal knowledge.

See also: [embedded-system-general-debugging](embedded-system-general-debugging.md)

---

## Available Tools

| Tool | Purpose |
|------|---------|
| `mcp_nrflow_nordicsemi_setup_ncs` | Environment setup: install `nrfutil`, NCS, and toolchain |
| `mcp_nrflow_nordicsemi_workflow_ncs` | Day-to-day workflow: build, flash, UART, debug, Twister |
| `mcp_nrflow_nordicsemi_list_sources` | List all searchable knowledge sources in the Nordic KB |
| `mcp_nrflow_nordicsemi_search_sources` | Semantic search across Nordic docs, SDK docs, and internal specs |

These tools return **instructions and resources** (markdown documents), not direct
commands. After calling them, the agent reads the returned content and follows the
workflow described within.

---

## Key Resources Returned by the Tools

### `nordicsemi_workflow_ncs` returns three embedded resources

| Resource | Description |
|----------|-------------|
| `nrfutil-manual` | Authoritative `nrfutil` command reference |
| `nordicsemi_uart_monitor.py` | Python serial monitor script |
| `embedded-code-guidance-ncs-zephyr` | NCS/Zephyr code style and pattern guide |

### `nordicsemi_uart_monitor.py`

A ready-to-use Python script for UART communication with Nordic boards:
- Reads output from a serial port with configurable baud rate and HWFC
- Sends commands interactively or from a script
- Supports hardware flow control (`rtscts=True`) needed for nRF54LM20DK UART30
- Use via `python3 nordicsemi_uart_monitor.py --port <port> --baud 115200`

### `nordicsemi_search_sources`

Semantic search over Nordic documentation. Use full-sentence queries:

```
"Which board targets support nRF54LM20A and what are their UART VCOM assignments?"
"How do I configure MSPI in NCS v3.3.0?"
"What is the sQSPI soft peripheral and how does VPR barrier synchronization work?"
```

Returns ranked chunks with source URLs. Better than web search for NCS-specific questions.

---

## Workflow Integration

### When to call which tool

| Task | Tool |
|------|------|
| First-time NCS setup, toolchain install | `nordicsemi_setup_ncs` |
| Build command syntax, overlay patterns | `nordicsemi_workflow_ncs` |
| Board name, PCA number, target resolution | `nordicsemi_search_sources` |
| UART monitoring, log capture | `nordicsemi_workflow_ncs` → uart monitor script |
| Code patterns, Kconfig best practices | `nordicsemi_workflow_ncs` → embedded code guidance |
| Unknown API, driver, Kconfig option | `nordicsemi_search_sources` |

### Source-of-truth order (from the workflow resource)

1. `nordicsemi_search_sources` for any Nordic/Zephyr guidance
2. `embedded-code-guidance-ncs-zephyr` resource for code style
3. `nrfutil-manual` resource for nrfutil syntax
4. `nordicsemi_uart_monitor.py` for serial communication
5. Local project files (`west.yml`, `prj.conf`, overlays)

Never rely on the agent's internal knowledge for board targets, PCA numbers, or UART
VCOM assignments — they drift between NCS versions.

---

## Comparison: mcp.nrflow vs Manual Scripting

The session that produced the sQSPI implementation used **manual Python scripting** for
UART and reset because the mcp.nrflow tools were not consulted. Here is a comparison:

### UART Log Capture

| | Manual (session approach) | mcp.nrflow |
|--|--------------------------|-----------|
| Script | Custom `loop_test.py` with `pyserial` | `nordicsemi_uart_monitor.py` (pre-built) |
| HWFC setup | Manual (`rtscts=True`) | Pre-configured in script |
| Drain/read patterns | Hand-written `read_until()` | Built-in |
| Development time | ~30 min to write + debug | 0 (already exists) |
| **Verdict** | Flexible but slow to write | **Preferred** — use the pre-built script |

### Board Reset

| | Manual | mcp.nrflow |
|--|--------|-----------|
| Command | `nrfutil device reset --serial-number <SN>` | Same — from `nrfutil-manual` resource |
| Discovery | Hardcoded serial number | `nrfutil device list` guided by workflow |
| Multiple device handling | Manual disambiguation | Workflow warns about ambiguity |
| **Verdict** | Fine once the serial number is known | **Preferred** for initial setup and multi-device |

### Build

| | Manual | mcp.nrflow |
|--|--------|-----------|
| Command | Hardcoded `nrfutil sdk-manager toolchain launch ...` | Same — from workflow resource |
| Board target | Looked up manually | `nordicsemi_search_sources` resolves it |
| Shield syntax | Trial and error | Workflow documents the `-DSHIELD=` pattern |
| **Verdict** | Fine for repeat tasks | **Preferred** for unfamiliar boards |

### Debugging with Multiple Devices

Both approaches support multi-device testing, but mcp.nrflow handles it more cleanly:
- The workflow resource explicitly warns: when multiple devices are attached, ask the
  user to disambiguate before flashing.
- The `nrfutil device list` step is mandatory in the workflow, preventing wrong-device
  flashes.
- The session used hardcoded serial numbers (`0010518513281` vs `1051869687`) to
  distinguish Board A (SPI reference) from Board B (sQSPI target) — same idea but
  fragile to typos.

**Best practice for multi-device testing:**

```bash
nrfutil device list  # identify both boards by serial number
# Board A (reference): --dev-id 0010518513281
# Board B (test):      --dev-id 1051869687
# Run on B, compare output with A
```

---

## Best Practices

1. **Always call `nordicsemi_workflow_ncs` at the start of any NCS build/flash/debug
   session** — it loads the uart monitor script and nrfutil manual into context.

2. **Use `nordicsemi_search_sources` before hardcoding any board name, VCOM number, or
   Kconfig symbol.** These change across NCS versions.

3. **Use the pre-built `nordicsemi_uart_monitor.py` script** instead of writing
   `pyserial` loops from scratch. The session wasted time debugging HWFC and drain
   patterns that the pre-built script already handles.

4. **For multi-device testing:** list all devices, assign them explicit roles (reference
   vs test), and always pass `--dev-id` to avoid flashing the wrong board.

5. **Call `nordicsemi_search_sources` for architecture questions before writing driver
   code.** In the session, the VPR barrier timing issue could have been understood
   earlier by searching for "sQSPI VPR barrier synchronization" and "nRF54LM20 FLPR
   event handling" in the Nordic KB.

6. **Gate the setup tool and workflow tool separately:** `nordicsemi_setup_ncs` is a
   one-time environment setup; `nordicsemi_workflow_ncs` is for every work session. Do
   not conflate them.
