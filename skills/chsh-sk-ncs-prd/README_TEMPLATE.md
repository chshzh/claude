# Customer-Facing README Template (PM-owned)

This template is owned by Product/PM workflow and intended for evaluator and customer-facing documentation.

## Required audience split

- Evaluator path: pre-built firmware path, clear project access constraints, minimal setup steps.
- Developer path: source build path with full setup and credential ownership.

## Recommended structure

1. Project Overview
2. Supported hardware
3. Features
4. Target Users (Evaluator, Developer)
5. Evaluator Quick Start
6. Buttons and LEDs
7. Developer Info
8. Documentation
9. Methodology
10. License

## Evaluator access policy section (required when cloud key is project-specific)

Include this table in Step 1:

| Supported project | Artifact prefix | Status |
|-------------------|-----------------|--------|
| <project-a> | `<project-a>_*` | Supported |
| <project-b> | `<project-b>_*` | Supported |

And include this policy text:

If the evaluator project is not listed, it is not supported for pre-built access.
Contact the project owner and sales/support team, or follow the Developer path.

## Quick-start step naming guidance

- Step 1: Flash firmware
- Step 2: Provision Wi-Fi via BLE (or equivalent onboarding flow)
- Step 3: Verify (cloud dashboard + UART evidence)

## UART evidence table template

| Board | Port | Baud |
|-------|------|------|
| <Board A> | <VCOM path> | 115200 |
| <Board B> + <Shield> | <VCOM path> | 115200 |

## Buttons table template

| Board | Buttons | Function |
|-------|---------|----------|
| <Board A> | <Button 1, Button 2> | <High-level behavior mapping> |
| <Board B> + <Shield> | <BUTTON0, BUTTON1, BUTTON2> | <Same mapping; BUTTON3 unavailable due to shield conflict> |

## Required docs links

- docs/pm-prd/PRD.md
- docs/dev-specs/overview.md
- docs/dev-specs/architecture.md
- docs/dev-specs/flash-memory-layout.md (if partition migration is relevant)
