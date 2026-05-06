---
name: chsh-dev-spec
description: Translate an approved PRD into engineering specs under docs/dev-specs/. Produces architecture.md, per-module specs (app modules and library wrapper modules). Architecture pattern applies to application code only; library wrappers document the external lib API boundary. Use when starting a new project design, updating specs after a PRD change, or documenting an existing codebase's design.
---

# chsh-dev-spec — Technical Design Workflow

Turns the product requirements in `docs/pm-prd/PRD.md` into the engineering specifications
that drive implementation. Specs live in `docs/dev-specs/` and are the contract
between design (this skill) and implementation (`chsh-dev-ncs-project`).

---

## Step 0 — Detect Context

Check what exists before choosing a mode:

```bash
cat docs/pm-prd/PRD.md                    # product requirements
ls docs/dev-specs/ 2>/dev/null         # existing specs
git log --oneline -5                             # recent commits
```

| Condition | Mode |
|-----------|------|
| No specs exist | **A — New Design** |
| Specs exist, PRD has newer revision (check Revision History table) | **B — Update Specs** |
| Code exists, no specs → document what is built | **C — Reverse Design** |
| User asks to review spec-vs-PRD alignment only | **D — Alignment Check** |

Ask the user to confirm the mode before proceeding.

---

## Mode A — New Design

### A1. Read the PRD

Load `docs/pm-prd/PRD.md`. Extract:

- Project name, NCS version, target board(s)
- Selected feature list (FR entries)
- Architecture pattern — SMF+Zbus or multi-threaded (ask the user if not stated)
  > **Scope**: the architecture pattern applies to **application code only**.
  > External libraries (Memfault SDK, Wi-Fi driver, BLE stack) run their own internal
  > threads regardless of the chosen pattern. Wrapper modules (`app_<lib>/`) bridge them.
- External libraries used (Memfault, BLE provisioning, etc.) — identify which need a wrapper module
- Hardware matrix: buttons, LEDs per board
- Non-functional requirements: memory headroom, latency targets, security notes

If no PRD exists, stop:
> "No PRD found at `docs/pm-prd/PRD.md`. Please run **chsh-pm-ncs-prd** first."

### A2. Plan the spec set

Derive the required spec files from PRD features. Always required:

| File | Content |
|------|---------|
| `docs/dev-specs/overview.md` | Entry point — spec index, PRD-to-spec mapping, design decisions |
| `docs/dev-specs/architecture.md` | System overview, module map, Zbus channels, boot sequence, memory budget |

Per module (one file per significant feature):

| Module type | Feature / Library | Spec file |
|-------------|-------------------|-----------|
| App module | Wi-Fi (SoftAP / STA / P2P) | `wifi-module.md` |
| App module | HTTP web server / REST API | `webserver-module.md` |
| App module | Button / GPIO | `button-module.md` |
| App module | LED output | `led-module.md` |
| App module | Mode selection / persistence | `mode-selector.md` |
| App module | MQTT client | `mqtt-client.md` |
| **Lib wrapper** | **Memfault SDK** | **`app-memfault-module.md`** |
| **Lib wrapper** | **BLE provisioning** | **`app-wifi-prov-ble-module.md`** |
| *(app module)* | *(any significant feature)* | `<name>-module.md` |
| *(lib wrapper)* | *(any external library)* | `app-<lib>-module.md` |

Present the plan to the user and confirm before generating.

### A3. Generate `docs/dev-specs/overview.md`

Use `OVERVIEW_TEMPLATE.md` as the base. Fill in:

- **PRD Version**: read the latest entry from `docs/pm-prd/PRD.md`'s Changelog table and copy its timestamp into the `PRD Version` field of Document Information. Do the same for every spec file generated.
- **Spec Index**: list every spec file to be generated, with a one-line description and the PRD sections it covers
- **Architecture Summary**: pattern choice (SMF+Zbus vs multi-threaded) and the top 3–5 design decisions with rationale
- **PRD-to-Spec Mapping**: table mapping each FR/NFR to the spec file that implements it
- **Module Dependency Map**: ASCII diagram of Zbus message flows between modules
- **Open Issues**: any unresolved design questions before work starts

Add the changelog entry (initial version, today's date).

### A6. Generate `docs/dev-specs/architecture.md`

Use `ARCH_TEMPLATE.md` as the base. Fill in:

- **Overview**: one paragraph — architecture pattern, key design decisions
- **Module Map**: directory tree showing all `src/modules/<name>/` directories
- **Zbus Channels table**: channel name, message type, publisher, subscriber(s), direction
- **Message Definitions**: `messages.h` struct/enum definitions
- **Boot Sequence**: numbered list with `SYS_INIT` priorities
- **Memory Budget**: table of Flash/RAM allocation per module plus total headroom
- **Thread Budget**: table of every application thread with name, stack size, priority, and purpose (justify each one — see Threads ≠ Modules below)
- **Test Points**: UART log markers expected at each stage

#### Threads ≠ Modules

A thread is an **execution context** (priority, stack, timing). A module is **logic** (state machine or callback-driven). They are independent:

- SMF needs **no dedicated thread** when driven from hardware callbacks (ISR/`dk_buttons_init`), `net_mgmt` callbacks, or zbus listeners. The state machine runs inside the publisher's or peripheral's context.
- Only add a thread when a module must **block-wait on events** (e.g. `zbus_sub_wait`) or perform **sustained/CPU-intensive processing**.
- A thread that only loops with `k_sleep()` and does zero blocking serves no purpose — remove it.

#### SYS_INIT Safety Rules

- **Never use `K_FOREVER`** in a `SYS_INIT` callback. If the awaited resource never arrives (hardware fault, wrong Kconfig), boot hangs permanently with no error.
- Use a bounded timeout: `k_sem_take(&sem, K_SECONDS(30))`; on failure log and `return -ETIMEDOUT`.
- The entire `SYS_INIT` sequence runs sequentially on the main thread; an infinite block prevents all subsequent init functions from executing.

#### zbus Observer Type Selection

Choose the observer type that matches the work performed by each module:

| Observer type | Execution context | When to use |
|---|---|---|
| `ZBUS_LISTENER_DEFINE` | Publisher's thread (VDED) — **synchronous** | Fastest reaction. **Must complete in microseconds.** No `k_sleep`, no blocking I/O, no mutex with non-zero timeout, no `zbus_chan_pub` with non-zero timeout. Treat like an ISR. |
| `ZBUS_ASYNC_LISTENER_DEFINE` | System work queue — **deferred** | Light deferred work (< ~20 ms). Gets message copy. Safe to submit further `k_work`. |
| `ZBUS_MSG_SUBSCRIBER_DEFINE` | Dedicated thread blocks on `zbus_sub_wait_msg` | Module needs to block-wait on events with **zero data loss** (gets full message copy). |
| `ZBUS_SUBSCRIBER_DEFINE` | Dedicated thread blocks on `zbus_sub_wait` | Module blocks-waits; occasional loss OK (notification only — must re-read channel). |

> **Anti-pattern**: calling `k_sleep()` or `http_server_start()` inside a `ZBUS_LISTENER` callback blocks the publisher's thread (e.g. the `net_mgmt` thread) for the entire duration. Use `k_work_schedule(&work, K_MSEC(delay))` from the listener instead.

Add the revision history entry (version 1.0, today's date).

### A7. Generate per-module specs

For each module from A2, use `MODULE_TEMPLATE.md` as base. Select the module type first:

**For application modules** (SMF+Zbus or multi-threaded):
- **Overview**: role of this module, version notes
- **File location**: `src/modules/<name>/` — list expected files
- **Zbus Integration**: channels subscribed to and published on, with struct definitions
- **State Machine**: Mermaid `stateDiagram-v2` diagram (SMF pattern); omit for simple modules
- **Kconfig Flags**: table of config symbol → description → default value
- **API / Public Interface**: function signatures with brief descriptions
- **Error Handling**: list of error conditions and how each is handled
- **Memory Estimate**: Flash KB / RAM KB (static + stack)
- **Test Points**: UART log strings expected when module works correctly

**For library wrapper modules** (`app_<lib>/`) — additional required sections:
- **External Library Interface** section (do not skip):
  - Library name and NCS Kconfig symbol (e.g. `CONFIG_MEMFAULT=y`)
  - Library internal threads (e.g. the Memfault SDK starts an HTTP upload thread)
  - **APIs called** by the wrapper: list the key `<lib_function>()` calls with purpose
  - **Callbacks implemented**: list every callback signature the library expects the app to provide
  - **Zbus integration**: table showing how library callbacks translate into Zbus messages published to the rest of the app
- Simplify or omit the State Machine section (lib wrapper typically has no SMF state machine)
- Note any thread-safety constraints (e.g. callbacks arriving in lib's thread context)

Add the changelog entry (initial version, today's date).

### A8. Confirm and handoff

After all specs are generated:

1. Present a summary table of all files created.
2. Ask the user to review and approve the specs.
3. Remind the user:
   > "Specs are ready. Run **chsh-dev-ncs-project** to scaffold the project and implement the code."

---

## Mode B — Update Specs

Use when the PRD has a newer revision entry than the latest spec revision.

### B1. Identify the delta

1. Read the PRD's **Revision History** table — note the new entries since the last spec update.
2. Read each spec's **Revision History** table — note its current version date.
3. List which PRD sections changed and which specs are affected.

Present the impact analysis to the user before making changes.

### B2. Update affected specs

For each impacted spec file:

- Update the `PRD Version` field in Document Information to the new PRD Changelog timestamp.
- Apply the PRD change to the relevant section (state machine, Kconfig, API, etc.).
- Add a new row to the spec's **Changelog** table:
  `| YYYY-MM-DD-HH-MM | Updated to PRD v<timestamp>: <summary of change> |`
- If a module is newly added, generate its spec from `MODULE_TEMPLATE.md`.
- If a module is removed, mark it `[DEPRECATED]` in the header; do not delete.

### B3. Update `architecture.md` and `overview.md`

If the module map, Zbus channels, or memory budget changed:
- Update those sections in `architecture.md` and add a Changelog entry.

Always update `overview.md`:
- Add/remove rows from the Spec Index and PRD-to-Spec Mapping tables.
- Update the Module Dependency Map if inter-module message flows changed.
- Add a Changelog entry to `overview.md`.

### B4. Handoff

> "Specs updated. Review the changes, then run **chsh-dev-ncs-project** to update the implementation."

---

## Mode C — Reverse Design

Use when code exists but no specs have been written.

1. Scan `src/modules/` to discover existing modules.
2. For each module, read `*.c`, `*.h`, `Kconfig.*` to extract:
   - Purpose and dependencies
   - Zbus channels used
   - State machine (if SMF)
   - Kconfig symbols
   - Public functions
3. Generate specs as in Mode A4–A5 using the code as the source of truth.
4. Generate `architecture.md` from the discovered structure.
5. Note any undocumented behaviours or gaps as **Open Issues**.

---

## Mode D — Alignment Check

Verify that existing specs match the PRD without generating new files.

1. For each FR in the PRD, identify the spec that covers it.
2. Check that the spec's acceptance criteria match the PRD's acceptance criteria.
3. Report mismatches as a table:

| PRD section | Spec file | Status | Gap |
|-------------|-----------|--------|-----|
| FR-001 Wi-Fi STA | wifi-module.md | OK | — |
| FR-004 P2P | wifi-module.md | Gap | P2P error states not specified |

Output the report to the user. If gaps exist, offer to fix them (Mode B).

---

## Changelog Convention

Every document produced by this skill must include a **Changelog** table near the top
(after Document Information), matching the format used in `PRD.md`:

```markdown
## Changelog

| Version          | Summary of changes          |
|------------------|-----------------------------|
| YYYY-MM-DD-HH-MM | Initial design              |
| YYYY-MM-DD-HH-MM | Updated wifi state machine  |
```

Rules:
- Version is a timestamp `YYYY-MM-DD-HH-MM` (e.g. `2026-04-09-14-30`) — includes time so multiple edits on the same day are distinguishable.
- Never delete rows; the table is append-only.
- Keep the summary short — one line describing what changed. When the change is driven by a PRD update, include the PRD version: `Updated to PRD v2026-04-09-10-00: added P2P mode`.
- The `PRD Version` field in Document Information always reflects the PRD Changelog timestamp this spec was written against.
- Git tracks the actual diff; the Changelog is the human-readable log.

## Self-Update Policy

At the **end of each conversation**, review what was discovered and check whether any facts in this skill are new, corrected, or outdated (e.g. new spec templates, changelog conventions, architecture guidance).

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.

---

## Spec Quality Checklist

Use this checklist before handing off specs to `chsh-dev-ncs-project`.

> **Reminder**: The spec covers HOW. The PRD covers WHAT. A good spec answers every question a developer needs to write code — without them having to re-read the PRD.

### Completeness
- [ ] Every FR in the PRD maps to at least one spec section (check PRD-to-Spec table in `overview.md`)
- [ ] Every module has a spec file with Overview, Zbus Integration, Error Handling, and Memory Estimate
- [ ] Architecture.md has a Thread Budget (justified) and Memory Budget (with headroom)
- [ ] All Zbus channels are listed with publisher, subscriber(s), and message struct

### Implementability (the "no-ask" test)
- [ ] A developer could write all source files using only the specs — no PRD re-reading needed
- [ ] All public function signatures are documented with parameters and return types
- [ ] State machine diagrams show all states and all transitions, including error paths
- [ ] Each Kconfig symbol has a default value and a one-line description
- [ ] SYS_INIT priorities are assigned and justified

### Architecture safety
- [ ] No `K_FOREVER` in SYS_INIT callbacks — bounded timeouts only
- [ ] Each thread has a documented justification (blocking I/O or sustained processing)
- [ ] ZBUS_LISTENER callbacks do not call `k_sleep()` or blocking I/O
- [ ] Library wrapper modules document all callbacks the library will call into app code

### Size guidelines (not hard limits — use judgment)
| Document | Target length |
|----------|--------------|
| `overview.md` | 200–400 words + tables |
| `architecture.md` | 400–800 words + diagrams |
| Per-module spec | 250–600 words + state diagram (if applicable) |

Specs longer than these targets often contain PRD content that belongs in `docs/pm-prd/PRD.md` instead.
