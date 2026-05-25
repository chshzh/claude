# Product Requirements Document

## Document Information

| Field | Value |
|---|---|
| Product Name | |
| Version | YYYY-MM-DD-HH-MM |
| Previous Version | — |
| Status | Draft |
| Product Manager | |
| NCS Version | e.g. v3.2.4 |
| Target Board(s) | e.g. nRF7002DK, nRF54LM20DK |

---

## Changelog

| Version | Summary of changes |
|---|---|
| YYYY-MM-DD-HH-MM | Initial version |

---

## 1. Executive Summary

### 1.1 Product Overview

*One to two paragraphs describing what this product is and what it does.*

### 1.2 Problem Statement

*What problem does this product solve? What pain point exists today without it?*

### 1.3 Target Users

| User type | Description |
|---|---|
| Primary | e.g. Embedded developers evaluating nRF70 Wi-Fi |
| Secondary | e.g. Nordic field engineers running demos |
| Tertiary | e.g. IoT hobbyists learning Zephyr |

### 1.4 Success Metrics

*How will we know the product is successful?*

| Metric | Target | How to measure |
|---|---|---|
| e.g. Dashboard loads | < 2 seconds | Manual test with stopwatch |
| e.g. Wi-Fi connects | < 30 seconds from power-on | UART log timestamp |
| e.g. Works without restart | 24 hours | Leave running overnight |

### 1.5 Assumptions

*State the assumptions this PRD is built on. If any of these turn out to be wrong, requirements may need to change.*

| # | Assumption | Risk if wrong |
|---|---|---|
| A1 | e.g. Target users have a phone/laptop with Wi-Fi | High — core connection flow breaks |
| A2 | e.g. Device is used on a private network | Medium — security requirements may increase |

---

## 2. Device Capabilities

*Describe what the device can do in plain language. No code or configuration details — those are for the engineer.*

### 2.1 Wi-Fi Connectivity

Describe which Wi-Fi modes the device should support:

- [ ] **Connect to an existing Wi-Fi network (STA mode)** — device joins a home or office network like a laptop would
- [ ] **Create its own Wi-Fi hotspot (SoftAP mode)** — device acts as a small access point that other devices connect to
- [ ] **Connect directly to a phone without a router (P2P / Wi-Fi Direct)** — direct device-to-phone link, no infrastructure needed

*Notes (e.g. should all three modes be selectable at runtime? Which is the default?):*

### 2.2 Communication & Protocols

Check what the device needs to communicate over the network:

- [ ] **Web interface** — device serves a webpage that a user can open in a browser
- [ ] **REST API** — device exposes HTTP endpoints that apps can call
- [ ] **MQTT messaging** — device publishes/subscribes to an MQTT broker in the cloud
- [ ] **CoAP** — lightweight messaging for constrained networks
- [ ] **Reachable by name** — device can be found at a hostname like `mydevice.local` without knowing its IP address

### 2.3 Storage & Memory

- [ ] **Remember settings after power-off** — e.g. Wi-Fi mode selection, user preferences
- [ ] **Remember Wi-Fi credentials** — user enters credentials once; device reconnects automatically

### 2.4 Buttons & LEDs

*Describe what each physical button and LED should do in user-facing terms.*

| Hardware | nRF7002DK | nRF54LM20DK |
|---|---|---|
| Buttons available | 2 (Button 1, Button 2) | 3 (BUTTON 0, BUTTON 1, BUTTON 2) |
| LEDs available | 2 | 4 |

*How should buttons behave?*

| Button | Behavior | Example |
|---|---|---|
| Button 1 | e.g. Hold 3 seconds at boot → show mode selection menu | |
| Button 2 | e.g. Short press → toggle LED 1 | |

*How should LEDs behave?*

| LED | Meaning | Example |
|---|---|---|
| LED 1 | e.g. On = Wi-Fi connected | |
| LED 2 | e.g. Blinking = connecting… | |

### 2.5 Cloud & Monitoring

- [ ] **Memfault** — remote crash reporting, OTA firmware updates, device metrics visible in Memfault dashboard
- [ ] **BLE credential provisioning** — user sets Wi-Fi credentials via a phone app over Bluetooth

### 2.6 Developer & Debug Features

- [ ] **Serial shell** — developer can type commands over USB/UART to inspect state, run scans, etc.
- [ ] **Verbose startup log** — device prints board name, MAC address, firmware version, IP address at boot

---

## 3. Functional Requirements

*One row per user-facing behaviour. Link each to the engineering spec that will implement it.*

### P0 — Must Have

| ID | As a… | I want to… | So that… | Acceptance Criteria | Engineering Spec |
|---|---|---|---|---|---|
| FR-001 | user | power on the device and see it connect to Wi-Fi | I can start using it without manual steps | - LED shows "connecting" within 5 s of boot<br>- LED shows "connected" within 30 s<br>- IP address logged over UART | |
| FR-002 | | | | | |

### P1 — Should Have

| ID | As a… | I want to… | So that… | Acceptance Criteria | Engineering Spec |
|---|---|---|---|---|---|
| FR-101 | | | | | |

### P2 — Nice to Have

| ID | As a… | I want to… | So that… | Acceptance Criteria | Engineering Spec |
|---|---|---|---|---|---|
| FR-201 | | | | | |

---

## 4. Non-Functional Requirements

### 4.1 Performance

*Describe speed and responsiveness expectations in user terms.*

| Behaviour | Target |
|---|---|
| Page load time in browser | < 2 seconds |
| Button press → LED response | < 200 ms |
| Wi-Fi reconnects after brief outage | < 60 seconds, automatically |

### 4.2 Reliability

| Expectation | Target |
|---|---|
| Continuous operation without restart | 24 hours |
| Automatic recovery after Wi-Fi drops | Yes — no user action needed |
| Automatic recovery after power cycle | Yes — boots into last saved mode |

### 4.3 Security

| Expectation | Requirement |
|---|---|
| Wi-Fi credentials | Never stored in plain text in code or logs |
| Device accessible over network | Only on intended network; no unintended exposure |

---

## 5. Hardware

### 5.1 Target Development Kits

| Board | Wi-Fi chip | Buttons | LEDs | Supported modes |
|---|---|---|---|---|
| nRF7002DK | nRF7002 (built in) | 2 | 2 | SoftAP, STA |
| nRF54LM20DK + nRF7002EBII shield | nRF7002 (shield) | 3 | 4 | SoftAP, STA, P2P |

*Note: BUTTON 3 on nRF54LM20DK is unavailable when nRF7002EBII shield is attached.*

### 5.2 Board-specific notes

*Document any differences in behaviour between boards here.*

---

## 6. User Experience

### 6.1 First-time Setup

*Describe what a new user does to get the device working, step by step.*

1. Flash firmware onto device.
2. …

### 6.2 Normal Operation

*Describe the typical daily use flow.*

### 6.3 Mode Selection (if applicable)

*If the device supports runtime mode selection (e.g. SoftAP vs STA), describe the UX.*

Example:
> 1. Hold Button 1 for 3 seconds while powering on.
> 2. A menu appears on the serial console.
> 3. Type `1` for SoftAP, `2` for STA, `3` for P2P.
> 4. Device saves the choice and reboots into the selected mode.

### 6.4 Troubleshooting (known scenarios)

*Describe what a user should do if common problems occur.*

| Symptom | What the user should do |
|---|---|
| Can't find the device's Wi-Fi hotspot | Wait 30 s; check LED is solid; try re-flashing |
| Browser shows "can't connect to site" | Confirm you are connected to the device's hotspot |

---

## 7. Release Criteria

*All P0 requirements below must pass before release.*

- [ ] All FR-0xx acceptance criteria pass on nRF7002DK
- [ ] All FR-0xx acceptance criteria pass on nRF54LM20DK (if supported)
- [ ] Device runs for 24 hours without restart
- [ ] Wi-Fi reconnects automatically after outage
- [ ] No credentials visible in UART logs
- [ ] README Quick Start guide tested by someone who was not the developer

---

## 8. Out of Scope

*Explicitly list what this product will NOT do in this release. Being clear here prevents scope creep and avoids engineer confusion.*

| Not building | Why / when might we revisit |
|---|---|
| e.g. Cloud dashboard | Out of scope for v1 — local-only is sufficient |
| e.g. iOS/Android app | Not needed; browser interface covers all users |
| e.g. Multiple simultaneous Wi-Fi modes | Too complex for initial release |

---

## 9. Open Questions

*Things that are not yet decided.*

| # | Question | Owner | Due |
|---|---|---|---|
| 1 | | | |

---

## 10. Engineering Spec References

*Filled in by the Developer Engineer after receiving this PRD.*

| Spec file | Covers |
|---|---|
| [architecture.md](../engineering/specs/architecture.md) | System design, module map, boot sequence |
| [wifi-module.md](../engineering/specs/wifi-module.md) | Wi-Fi STA / SoftAP / P2P |
| *(add more as created)* | |
