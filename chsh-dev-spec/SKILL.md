---
name: chsh-dev-spec
description: Translate an approved PRD into engineering specs under docs/engineering/. Produces architecture.md, per-module specs, and config.yaml. Use when starting a new project design, updating specs after a PRD change, or documenting an existing codebase's design.
---

# chsh-dev-spec — Technical Design Workflow

Turns the product requirements in `docs/product/PRD.md` into the engineering specifications
that drive implementation. Specs live in `docs/engineering/specs/` and are the contract
between design (this skill) and implementation (`chsh-dev-project`).

---

## Step 0 — Detect Context

Check what exists before choosing a mode:

```bash
cat docs/product/PRD.md                          # product requirements
ls docs/engineering/specs/ 2>/dev/null           # existing specs
cat docs/engineering/config.yaml 2>/dev/null     # existing project context
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

Load `docs/product/PRD.md`. Extract:

- Project name, NCS version, target board(s)
- Selected feature list (FR entries)
- Architecture pattern — SMF+Zbus or multi-threaded (ask the user if not stated)
- Hardware matrix: buttons, LEDs per board
- Non-functional requirements: memory headroom, latency targets, security notes

If no PRD exists, stop:
> "No PRD found at `docs/product/PRD.md`. Please run **chsh-pm-prd** first."

### A2. Plan the spec set

Derive the required spec files from PRD features. Always required:

| File | Content |
|------|---------|
| `docs/engineering/specs/overview.md` | Entry point — spec index, PRD-to-spec mapping, design decisions |
| `docs/engineering/specs/architecture.md` | System overview, module map, Zbus channels, boot sequence, memory budget |
| `docs/engineering/config.yaml` | Project context fed to `chsh-dev-project` |

Per module (one file per significant feature):

| Feature in PRD | Spec file |
|----------------|-----------|
| Wi-Fi (SoftAP / STA / P2P) | `wifi-module.md` |
| HTTP web server / REST API | `webserver-module.md` |
| Button / GPIO | `button-module.md` |
| LED output | `led-module.md` |
| Mode selection / persistence | `mode-selector.md` |
| MQTT client | `mqtt-client.md` |
| *(any significant module)* | `<name>-module.md` |

Present the plan to the user and confirm before generating.

### A3. Generate `docs/engineering/config.yaml`

```yaml
schema: spec-driven
context: |
  Project: <name>
  Tech stack: Zephyr RTOS, NCS <version>
  Architecture: SMF+Zbus        # or Multi-threaded
  Target boards: <boards>

  Configuration strategy:
  - Base configs in prj.conf
  - Module Kconfig in src/modules/<name>/Kconfig.<name>
  - CMakeLists.txt per module via add_subdirectory

modules:
  - name: <module_name>
    spec: docs/engineering/specs/<module_name>-module.md
    enabled: true
```

### A4. Generate `docs/engineering/specs/overview.md`

Use `OVERVIEW_TEMPLATE.md` as the base. Fill in:

- **PRD Version**: read the latest entry from `docs/product/PRD.md`'s Changelog table and copy its timestamp into the `PRD Version` field of Document Information. Do the same for every spec file generated.
- **Spec Index**: list every spec file to be generated, with a one-line description and the PRD sections it covers
- **Architecture Summary**: pattern choice (SMF+Zbus vs multi-threaded) and the top 3–5 design decisions with rationale
- **PRD-to-Spec Mapping**: table mapping each FR/NFR to the spec file that implements it
- **Module Dependency Map**: ASCII diagram of Zbus message flows between modules
- **Open Issues**: any unresolved design questions before work starts

Add the changelog entry (initial version, today's date).

### A6. Generate `docs/engineering/specs/architecture.md`

Use `ARCH_TEMPLATE.md` as the base. Fill in:

- **Overview**: one paragraph — architecture pattern, key design decisions
- **Module Map**: directory tree showing all `src/modules/<name>/` directories
- **Zbus Channels table**: channel name, message type, publisher, subscriber(s), direction
- **Message Definitions**: `messages.h` struct/enum definitions
- **Boot Sequence**: numbered list with `SYS_INIT` priorities
- **Memory Budget**: table of Flash/RAM allocation per module plus total headroom
- **Test Points**: UART log markers expected at each stage

Add the revision history entry (version 1.0, today's date).

### A7. Generate per-module specs

For each module from A2, use `MODULE_TEMPLATE.md` as base. Fill in:

- **Overview**: role of this module, version notes
- **File location**: `src/modules/<name>/` — list expected files
- **Zbus Integration**: channels subscribed to and published on, with struct definitions
- **State Machine**: Mermaid `stateDiagram-v2` diagram (SMF pattern); omit for simple modules
- **Kconfig Flags**: table of config symbol → description → default value
- **API / Public Interface**: function signatures with brief descriptions
- **Error Handling**: list of error conditions and how each is handled
- **Memory Estimate**: Flash KB / RAM KB (static + stack)
- **Test Points**: UART log strings expected when module works correctly
- **Open Issues / TBD**: anything not yet decided

Add the revision history entry (version 1.0, today's date).

### A8. Confirm and handoff

After all specs are generated:

1. Present a summary table of all files created.
2. Ask the user to review and approve the specs.
3. Remind the user:
   > "Specs are ready. Run **chsh-dev-project** to scaffold the project and implement the code."

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

> "Specs updated. Review the changes, then run **chsh-dev-project** to update the implementation."

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
