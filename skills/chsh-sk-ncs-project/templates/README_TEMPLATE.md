# <Project Name>

<!--
README QUALITY TARGETS
  Reading time:  3–5 min for Evaluator Quick Start path | 8–12 min for full document
  Character count:  3,000–7,000 characters (excluding code blocks)
  Principle: cognitive funnel — broad → narrow. Evaluator should reach
  a working device by the end of Evaluator Quick Start without scrolling past it.
-->

[![Build](https://github.com/<org>/<repo>/actions/workflows/build.yml/badge.svg)](https://github.com/<org>/<repo>/actions/workflows/build.yml)
[![License](https://img.shields.io/badge/License-LicenseRef--Nordic--5--Clause-blue.svg)](LICENSE)
[![NCS](https://img.shields.io/badge/NCS-v3.x.x-skyblue)](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/index.html)

<One sentence describing what the device does and for whom.>

---

## Project Overview

### Introduction

<Two to three sentences: what problem this solves, who the target user is, and what makes it distinctive.>

### Supported hardware

| Board | Build target |
|-------|--------------|
| &lt;Board A&gt; | `<target/cpuapp>` |
| &lt;Board B&gt; + &lt;Shield&gt; | `<target/cpuapp>` + `-DSHIELD=<shield>` |

### Features

- **&lt;Feature 1&gt;** — brief description
- **&lt;Feature 2&gt;** — brief description
- **&lt;Feature 3&gt;** — brief description
- **&lt;Feature 4&gt;** — brief description

### Target Users

- **Evaluator** — grab a pre-built `.hex` from the [Releases](<releases-url>) page, flash it, and follow the [Evaluator Quick Start](#evaluator-quick-start) guide to reach a working device in under 5 minutes.
- **Developer** — clone the workspace, build from source, and customise the firmware; see [Developer Info](#developer-info) for build setup and [Documentation](#documentation) for product requirements, architecture, and per-module specs.

---

## Evaluator Quick Start

> Evaluator path — no build environment needed. ~5 minutes.

### Step 1 — Flash the firmware

Download the pre-built `.hex` for your board from the [Releases](<releases-url>) page, then open **nRF Connect for Desktop → Programmer**, select your board, add the `.hex` file, and click **Erase & Write**.

| Board | Release page |
|-------|--------------|
| &lt;Board A&gt; | [Latest release](<releases-url>/latest) |
| &lt;Board B&gt; + &lt;Shield&gt; | [Latest release](<releases-url>/latest) |

### Step 2 — Connect and open the dashboard

Open a serial terminal at `115200` baud and follow the instructions printed by the firmware.

- **&lt;Mode 1&gt;** (default on fresh flash): &lt;describe auto-start behaviour and how to reach the UI&gt;
- **&lt;Mode 2&gt;**: run `<shell command>`, reboot, &lt;describe connection steps&gt;
- **&lt;Mode 3&gt;**: run `<shell command>`, reboot, &lt;describe connection steps&gt;

At any time, you can switch modes with `<shell command> [mode1|mode2|mode3]`. The choice is saved to NVS and survives reboot.

### Step 3 — Verify

&lt;What the user should see: browser dashboard URL, UART output. Include the screenshot here.&gt;

![&lt;Project name&gt; dashboard](docs/images/screenshot.png)

## Buttons & LEDs

### Buttons

| Board | Buttons | Function |
|-------|---------|----------|
| &lt;Board A&gt; | &lt;SW1, SW2&gt; | &lt;description, e.g. state and count shown in dashboard&gt; |
| &lt;Board B&gt; + &lt;Shield&gt; | &lt;BUTTON0–BUTTON2&gt; | &lt;Same; BUTTON3 unavailable — shield pin conflict&gt; |

### LEDs

| Board | LEDs | Control |
|-------|------|---------|
| &lt;Board A&gt; | &lt;LED1, LED2&gt; | &lt;description, e.g. controlled via dashboard or `/api/led`&gt; |
| &lt;Board B&gt; | &lt;LED0–LED3&gt; | &lt;Same&gt; |

---

## Developer Info

### Project Structure

```text
<project-name>/
├── CMakeLists.txt
├── Kconfig
├── prj.conf
├── west.yml
├── docs/
│   ├── PRD.md                   ← product requirements and acceptance criteria
│   └── specs/
│       ├── overview.md          ← spec index and architecture summary
│       ├── architecture.md      ← module map, Zbus channels, boot sequence
│       └── <module>-module.md   ← per-module specs
├── src/
│   ├── main.c
│   └── modules/
│       ├── <module-a>/
│       ├── <module-b>/
│       └── messages.h
```

### Workspace Setup

West workspace is driven by [west.yml](west.yml), which contains the NCS version this application is based on. For example, the following entry means NCS v3.3.0:

```sh
    - name: sdk-nrf
      path: nrf
      revision: v3.3.0
      import: true
      remote: ncs
```

Use nRF Connect for VS Code or a shell initialized with the NCS toolchain.

#### Method 1 (Preferred) — Add to an existing NCS installation

If you already have a matching NCS version installed, reuse it directly — no re-downloading required.

Under a terminal with the toolchain:

```sh
cd /opt/nordic/ncs/<ncs-version>   # your existing NCS workspace root

git clone https://github.com/<org>/<repo>.git

# Switch the workspace manifest to <repo> (one-time change)
west config manifest.path <repo>

# Sync — NCS repos already present, only new project repos are cloned
west update
```

#### Method 2 — Fresh installation as a Workspace Application

##### Option A: nRF Connect for VS Code

Follow the [custom repository guide](https://docs.nordicsemi.com/bundle/nrf-connect-vscode/page/guides/extension_custom_repo.html).

##### Option B: CLI

```sh
west init -m https://github.com/<org>/<repo> --mr main <workspace-dir>
cd <workspace-dir>
west update
```

See the Nordic guide on [Workspace Application Setup](https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/dev_model_and_contributions/adding_code.html#workflow_4_workspace_application_repository_recommended) for details.

### Build

```bash
# <Board A>
west build -p -b <board-target> -- <cmake-args>

# <Board B> + <Shield>
west build -p -b <board-target> -- <cmake-args> -DSHIELD=<shield>
```

### Flash

```bash
# <Board A>
west flash --erase

# <Board B>
west flash --recover
```

### Serial Monitor

Connect at **115200 baud**. The device prints its IP address, Wi-Fi mode, and connection instructions at boot.

### Developer Notes

- &lt;board-specific caveat, e.g. one button unavailable due to shield pin conflict&gt;
- &lt;intentional design decision worth flagging, e.g. session-based connections&gt;
- &lt;default mode or configuration on fresh flash&gt;
- &lt;startup banner or debug output notes&gt;

---

## Documentation

The full design documentation lives under `docs/`. Start with [docs/dev-specs/overview.md](docs/dev-specs/overview.md), which maps every PRD requirement to the spec file that implements it and provides an architecture summary.

| Document | Description |
|---|---|
| [docs/pm-prd/PRD.md](docs/pm-prd/PRD.md) | Product Requirements — user perspective features, behavior, acceptance criteria, changelog |
| [docs/dev-specs/overview.md](docs/dev-specs/overview.md) | **Start here** — technical spec index, PRD-to-spec mapping, architecture summary, design decisions |
| [docs/dev-specs/architecture.md](docs/dev-specs/architecture.md) | System architecture — module map, Zbus channels, SYS_INIT boot sequence, memory budget |
| [docs/dev-specs/&lt;module&gt;-module.md](docs/dev-specs/) | &lt;Module&gt; module — &lt;brief description&gt; |

## Methodology

This project was developed using the [chsh-ncs-workflow](https://github.com/chshzh/charlie-skills) — a four-phase lifecycle for NCS/Zephyr IoT projects where each phase has a dedicated AI skill:

| Phase | Focus | Skill | Output |
|-------|-------|-------|--------|
| 1 — Product Definition | What the device should do, for whom, and why | `chsh-pm-prd` | `docs/pm-prd/PRD.md` |
| 2 — Technical Design | Translate PRD into engineering specs | `chsh-dev-spec` | `docs/dev-specs/*.md` |
| 3 — Implementation | Implement code from approved specs | `chsh-dev-project` | `src/`, passing build |
| 4 — QA & Test | Validate the build against PRD criteria | `chsh-qa-test` | `docs/qa-test/QA-*.md` |

Each phase feeds the next: requirements drive specs, specs drive code, code drives tests. Issues loop back to the right phase — code bugs to Phase 3, spec gaps to Phase 2, new requirements to Phase 1.

---

## License

[SPDX-License-Identifier: LicenseRef-Nordic-5-Clause](LICENSE)

---

<!--
## Optional Sections

Add any of the following when appropriate for your project.
Delete this comment block before publishing.

### API Reference

Use when the device exposes an HTTP REST API or other machine-readable interface.

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/resource` | GET | Returns ... |
| `/api/resource` | POST | Controls ... |

### Troubleshooting

Use when common failure modes are known and documented.

- <Symptom>: <cause and resolution>
- <Symptom>: <cause and resolution>

### References

Use for key upstream documentation links.

- [nRF Connect SDK Documentation](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/index.html)
- [Zephyr RTOS Documentation](https://docs.zephyrproject.org/latest/)

### Contributing

This project follows Nordic Semiconductor coding standards and Zephyr contribution guidelines.
Contributions are welcome — open an issue or pull request.
-->
