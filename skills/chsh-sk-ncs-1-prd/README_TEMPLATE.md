# Customer-Facing README Template (PM-owned)

This template guides authoring of a customer-facing project README for NCS/Zephyr IoT projects. Each section below corresponds to a section in the output README.

## Badges (optional)

Place immediately after the `# <Project Title>` heading. Both badges are optional; include them once a GitHub Actions workflow and a releases page exist:

```markdown
[![Build](https://github.com/<org>/<repo>/actions/workflows/build.yml/badge.svg)](https://github.com/<org>/<repo>/actions/workflows/build.yml)
[![Latest Release](https://img.shields.io/github/v/release/<org>/<repo>?label=Release&color=skyblue)](https://github.com/<org>/<repo>/releases/latest)
```

---

## Recommended README structure

1. Project Overview (`### Introduction`, `### Supported hardware`, `### Features`, `### Target Users`)
2. Evaluator Quick Start (Step 1: Flash, Step 2: Verify — includes UART log, Buttons & LEDs UX, Application logic)
3. Developer Guide (`### Project Structure`, `### Workspace Setup`, `### Build`, `### Flash`, `### Developer Notes`)
4. Documentation
5. Methodology
6. License

---

## Project Overview

### Introduction

Brief (2–4 sentences), user-facing. Do not include implementation details (Kconfig symbols, memory numbers, internal module wiring).

- Sentence 1: what the project is.
- Sentence 2: what problem/use case it covers and for whom.
- Sentences 3–4 (optional): key runtime capabilities or operating modes.

### Supported hardware

Include only boards the project supports. Use this fixed table format — add a `Comments` column only if boards have meaningful differences (default mode, missing features):

```markdown
| Board | Build target |
|-------|--------------|
| nRF54LM20DK + nRF7002EB2 | `nrf54lm20dk/nrf54lm20a/cpuapp` + `-DSHIELD=nrf7002eb2` |
| nRF7002DK | `nrf7002dk/nrf5340/cpuapp` |
| nRF5340 Audio DK + nRF7002EK | `nrf5340_audio_dk/nrf5340/cpuapp` + `-DSHIELD=nrf7002ek` |
```

### Features

Concise bullet list. Every bullet must include a short explanation:

- `<feature capability> — <what it does for users / why it matters>`
- Prefer 5–10 bullets; keep wording user-facing; avoid internal implementation details.
- If commands, endpoints, or modes are user-visible, include them in the explanation.

Examples:
- Runtime mode switching with `app_wifi_mode` — users can change network behavior without reflashing.
- REST API for `/api/system`, `/api/buttons`, `/api/leds`, `/api/led` — external tools can query status and control LEDs.

### Target Users

Copy verbatim, updating `<release-url>` with the project's GitHub Releases URL:

```markdown
- **Evaluator** — grab a pre-built `.hex` from the [Releases](<release-url>) page, flash it, and follow the [Evaluator Quick Start](#evaluator-quick-start) guide to reach the dashboard in under 5 minutes.
- **Developer** — clone the workspace, build from source, and customise the firmware; see [Developer Guide](#developer-guide) for build setup and [Documentation](#documentation) for product requirements, architecture, and per-module specs.
```

---

## Evaluator Quick Start

Use exactly **2 steps**.

### Step 1 — Flash the firmware

Direct the evaluator to the project's GitHub **Releases** page to download the pre-built `.hex`. Include a per-board table:

```markdown
| Board | Release page |
|-------|--------------|
| nRF54LM20DK + nRF7002EB2 | [Latest release](<release-url>) |
| nRF7002DK | [Latest release](<release-url>) |
```

Describe how to flash: nRF Connect for Desktop Programmer (Erase & Write) or `nrfutil device program --firmware <*.hex> --verify`.

If the project requires onboarding before use (e.g. BLE Wi-Fi provisioning, entering credentials), include it as a clearly labeled sub-step or note at the end of Step 1 — **not** as a separate numbered step.

### Step 2 — Verify

Cover all three areas:

**1. UART log** — open a serial terminal at 115200 baud. Include the per-board port table (copy verbatim):

```markdown
| Board | Port | Baud |
|-------|------|------|
| nRF54LM20DK + nRF7002EB2 | VCOM0 (`/dev/tty.usbmodem*1`) | 115200 |
| nRF7002DK | VCOM1 (`/dev/tty.usbmodem*3`) | 115200 |
```

Describe what the evaluator should see: boot banner, module list, and Wi-Fi connect/network-ready log lines.

If the project supports multiple Wi-Fi modes or connection methods, include mode-specific connection instructions here — one bullet per mode. The firmware prints connection guidance at boot (SSID, PIN, IP address, shell commands), so describe what to follow for each mode. Example pattern:

```markdown
- **<mode-name>** (default on fresh flash): <what boots automatically> — <what to do on phone/client>; open `<url>`
- **<mode-name>**: run `<shell-cmd>`, reboot, then <what to do>; open the IP shown in the terminal
```

Also describe any key application-specific log evidence the evaluator should confirm (e.g. Memfault heartbeat triggered, OTA check complete, REST API ready).

**2. Buttons & LEDs** — describe the UX interaction: what to press and what to observe. Include the full Buttons and LEDs reference tables here so the evaluator has everything in one place. Map each button to its visible effect (LED change, log line, dashboard update, cloud event). Call out board differences (e.g. BUTTON3 unavailable on nRF54LM20DK due to shield conflict).

Buttons table skeleton:

```markdown
### Buttons

| Board | Buttons | Function |
|-------|---------|----------|
| nRF54LM20DK + nRF7002EB2 | BUTTON0 (idx 0), BUTTON1 (idx 1), BUTTON2 (idx 2) | <Same mapping; BUTTON3 unavailable due to shield conflict> |
| nRF7002DK | Button 1 (idx 0), Button 2 (idx 1) | <High-level behavior mapping> |
| nRF5340 Audio DK + nRF7002EK | VOL- (idx 0), VOL+ (idx 1), PLAY/PAUSE (idx 2), BTN4 (idx 3), BTN5 (idx 4) | <High-level behavior mapping> |
```

LEDs table skeleton:

```markdown
### LEDs

| Board | LEDs | Control |
|-------|------|---------|
| nRF54LM20DK + nRF7002EB2 | LED0 (idx 0), LED1 (idx 1), LED2 (idx 2), LED3 (idx 3) | <Same mapping or board-specific differences> |
| nRF7002DK | LED1 (idx 0), LED2 (idx 1) | <User-visible control mapping> |
| nRF5340 Audio DK + nRF7002EK | RGB1 R/G/B (idx 0–2), RGB2 R/G/B (idx 3–5), LED1 (idx 6), LED2 (idx 7), LED3 (idx 8) | <User-visible control mapping; 9 channels total> |
```

**3. Application logic** — describe the primary user-visible behavior: open the dashboard URL, trigger an OTA check, observe a Memfault cloud event, confirm a REST API response, etc. Tailor this to the main value of the application.

Optional: include a screenshot or device timeline image to orient the evaluator.

---

## Developer Guide

### Project Structure

Use a fenced `text` code block showing the repo directory tree:

- Top-level files (`CMakeLists.txt`, `Kconfig`, `prj.conf`, `west.yml`) each annotated with a `←` comment describing their role.
- `boards/` — annotate with what the per-board fragments control (button count, LED count, overlays).
- `docs/` — list each spec file under `dev-specs/` with a `←` comment; include `pm-prd/PRD.md` and `qa-test/`.
- `src/` — show `main.c` and each `modules/` subdirectory annotated.
- External zego modules — show as `../zego/<module>/` entries at the bottom, annotated with their Zbus channel and purpose.
- Only list files and directories that actually exist.

### Workspace Setup

Copy this block verbatim, replacing only `<project-repo-url>` and `<project-dir-name>`:

```markdown
### Workspace Setup

West workspace is driven by [west.yml](west.yml), which contains the NCS version this application is based on. For example, the following entry means NCS v3.3.0:

```sh
    - name: sdk-nrf
      path: nrf
      revision: v3.3.0
      import: true
      remote: ncs
```

Release versions follow the NCS version with a build counter suffix: `v<ncs-version>.<build>` (e.g. `v3.3.0.1`, `v3.3.0.2`). The major/minor/patch components always match the NCS version the firmware is based on, making it easy to identify which SDK a given release targets.

Use nRF Connect for VS Code or a shell initialized with the NCS toolchain.

#### Method 1 (Preferred) — Add to an existing NCS installation

If you already have a matching NCS version installed, reuse it directly — no re-downloading required.

```sh
cd /opt/nordic/ncs/<ncs-version>   # your existing NCS workspace root

git clone <project-repo-url>

# Switch the workspace manifest to <project-dir-name> (one-time change)
west config manifest.path <project-dir-name>

# Sync — NCS repos already present, only new project repos are cloned
west update
```

#### Method 2 — Fresh installation as a Workspace Application

##### Option A: nRF Connect for VS Code

Follow the [custom repository guide](https://docs.nordicsemi.com/bundle/nrf-connect-vscode/page/guides/extension_custom_repo.html).

##### Option B: CLI

```sh
west init -m <project-repo-url> --mr main <workspace-dir>
cd <workspace-dir>
west update
```

See the Nordic guide on [Workspace Application Setup](https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/dev_model_and_contributions/adding_code.html#workflow_4_workspace_application_repository_recommended) for details.
```

### Build

Dual-board build skeleton:

```bash
# nRF54LM20DK + nRF7002EB2
west build -p -b nrf54lm20dk/nrf54lm20a/cpuapp -d <build_dir_54lm20dk> -- <project_specific_args> -DSHIELD=nrf7002eb2

# nRF7002DK
west build -p -b nrf7002dk/nrf5340/cpuapp -d <build_dir_7002dk> -- <project_specific_args>
```

**Optional: `### Feature Overlay Builds`** — include this subsection when the project ships named overlay `.conf` files for alternate build configurations (STA-only, headless, cloud-key injection, etc.):
- List overlays in a table: **Overlay** | **Purpose**.
- Explain what each overlay enables/disables and when to use it.
- Show the full `west build` command with `-DEXTRA_CONF_FILE="<overlay>.conf"` for at least one board.
- Include measured Flash/RAM delta in a table if available.

```bash
# Example — nRF7002DK with <overlay-name>
west build -p -b nrf7002dk/nrf5340/cpuapp -d <build_dir> -- \
  -DEXTRA_CONF_FILE="<overlay-name>.conf"
```

### Flash

First-time flash (erases NVS — credentials must be re-provisioned):

```bash
# nRF54LM20DK
west flash -d <build_dir_54lm20dk> --recover

# nRF7002DK
west flash -d <build_dir_7002dk> --erase
```

Optional — subsequent flash (preserves credentials and settings):

```bash
# nRF54LM20DK
west flash -d <build_dir_54lm20dk>

# nRF7002DK
west flash -d <build_dir_7002dk>
```

### Developer Notes

Project-specific bullets a developer must know before modifying or debugging the firmware. Include what applies:

- **Board quirks** — hardware limitations or differences (shield pin conflicts, missing buttons, different LED numbering).
- **Default runtime state** — what mode/configuration the device starts in after fresh flash, and how to change it.
- **NVS and credential persistence** — what survives a reboot vs. a full erase; when re-provisioning is required.
- **Session or connection behavior** — intentional design choices affecting reconnect logic, mode switching, credential handling.
- **Log interpretation** — startup banner contents (firmware version, module list, SYS_INIT priorities) and any periodic log reminders.
- **Links to module specs** — inline links to `docs/dev-specs/<module>.md` or `zego/<module>` docs for non-obvious behavior.



---

## Documentation

Copy this block verbatim, then update the table rows for the project:

```markdown
## Documentation

The full design documentation lives under `docs/`. Start with [docs/dev-specs/overview.md](docs/dev-specs/overview.md), which maps every PRD requirement to the spec file that implements it and provides an architecture summary.

| Document | Description |
|---|---|
| [docs/pm-prd/PRD.md](docs/pm-prd/PRD.md) | Product Requirements — user perspective features, behavior, acceptance criteria, changelog |
| [docs/dev-specs/overview.md](docs/dev-specs/overview.md) | **Start here** — technical spec index, PRD-to-spec mapping, architecture summary, design decisions |
| [docs/dev-specs/architecture.md](docs/dev-specs/architecture.md) | System architecture — module map, Zbus channels, SYS_INIT boot sequence, memory budget |
| [docs/dev-specs/<module>.md](docs/dev-specs/<module>.md) | Module behavior and interfaces |
| [zego/<module> ↗](https://github.com/chshzh/zego/blob/main/modules/<module>/docs/<module>-spec.md) | zego module spec — add one row per module used (button, led, wifi, network) |
| [docs/qa-test/README.md](docs/qa-test/README.md) | QA snapshot folder conventions |
```

Notes:
- Keep at least PRD, overview, and architecture rows.
- Add one row per module spec file that exists in `dev-specs/`.
- Add `zego/<module> ↗` rows for every zego module the project uses.
- Include `flash-memory-layout.md` row when partition migration or storage layout is relevant.

---

## Methodology

Copy/paste verbatim:

```markdown
## Methodology

This project was developed using the [chsh-sk-ncs-0-workflow skill](https://github.com/chshzh/claude/blob/main/skills/chsh-sk-ncs-0-workflow/SKILL.md) — a four-phase lifecycle for NCS/Zephyr IoT projects where each phase has a dedicated AI skill:

| Phase | Focus | Skill | Output |
|-------|-------|-------|--------|
| 1 — Product Definition | What the device should do, for whom, and why | `chsh-sk-ncs-1-prd` | `docs/pm-prd/PRD.md` |
| 2 — Technical Design | Translate PRD into engineering specs | `chsh-sk-ncs-2-spec` | `docs/dev-specs/*.md` |
| 3 — Implementation | Implement, debug, and optimise code from approved specs | `chsh-sk-ncs-3.1-coding` · `chsh-sk-ncs-3.2-debug` · `chsh-sk-ncs-3.3-memopt` | `src/`, passing build |
| 4 — V&V | Verify code quality (no HW), then validate on hardware against PRD criteria | `chsh-sk-ncs-4.1-verification` · `chsh-sk-ncs-4.2-validation` | `docs/qa-test/VERIFICATION-*.md` + `docs/qa-test/VALIDATION-*.md` |

Each phase feeds the next: requirements drive specs, specs drive code, code drives tests. Issues loop back to the right phase — code bugs to Phase 3, spec gaps to Phase 2, new requirements to Phase 1.
```

---

## License

Copy/paste verbatim:

```markdown
## License

[SPDX-License-Identifier: LicenseRef-Nordic-5-Clause](LICENSE)
```