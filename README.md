# Charlie Skills Repository

Personal collection of specialized skills for development workflows with Claude.

## 📦 Skills Overview

### Workflow Orchestrator

#### `chsh-ncs-workflow` — Full Lifecycle Orchestrator
Single entry point for any NCS project. Scans project state, shows a status dashboard,
and guides through each phase — invoking the right skill at each step.
- Entry: [SKILL.md](chsh-ncs-workflow/SKILL.md)

### Developer Skills

#### `chsh-dev-commit` — Git Commit Workflow
Plan, group, and execute git commits following Conventional Commits style.
- Entry: [SKILL.md](chsh-dev-commit/SKILL.md)

#### `chsh-dev-mem-opt` — NCS Memory Optimization
Strategies for footprint reduction and heap profiling in NCS/Zephyr projects.
- Entry: [SKILL.md](chsh-dev-mem-opt/SKILL.md)

#### `chsh-dev-spec` — Technical Design (Phase 2)
Translates an approved PRD into engineering specs: architecture overview, per-module specs
(state machines, Kconfig, APIs, memory.
- Entry: [SKILL.md](chsh-dev-spec/SKILL.md)
- Templates:
	- [OVERVIEW_TEMPLATE.md](chsh-dev-spec/OVERVIEW_TEMPLATE.md) — top-level spec index + PRD-to-spec mapping
	- [ARCH_TEMPLATE.md](chsh-dev-spec/ARCH_TEMPLATE.md) — architecture spec template
	- [MODULE_TEMPLATE.md](chsh-dev-spec/MODULE_TEMPLATE.md) — module spec template

#### `chsh-dev-project` — Code Implementation (Phase 3)
Implements NCS project code from engineering specs. Scaffolds new projects, implements
modules, and updates code when specs change.
- Entry: [SKILL.md](chsh-dev-project/SKILL.md)
- Quick start:
	- `cp ~/.claude/skills/chsh-dev-project/templates/.gitignore ./`
	- `cp ~/.claude/skills/chsh-dev-project/templates/LICENSE ./`
	- `cp ~/.claude/skills/chsh-dev-project/templates/README_TEMPLATE.md README.md`
	- `cp ~/.claude/skills/chsh-dev-project/overlays/overlay-wifi-sta.conf .`
- Sub-skills:
	- [debug/SKILL.md](chsh-dev-project/debug/SKILL.md) — debugging, RTT, GDB
	- [env-setup/SKILL.md](chsh-dev-project/env-setup/SKILL.md) — toolchain & west setup
	- [architecture/SKILL.md](chsh-dev-project/architecture/SKILL.md) — SMF+zbus vs multi-threaded patterns
	- [protocols/SKILL.md](chsh-dev-project/protocols/SKILL.md) — MQTT, CoAP, HTTP, TCP/UDP
	- [protocols/webserver/SKILL.md](chsh-dev-project/protocols/webserver/SKILL.md) — static HTTP server
	- [wifi/SKILL.md](chsh-dev-project/wifi/SKILL.md) — STA, SoftAP, P2P modes

### Product Manager Skills

#### `chsh-pm-prd` — Interactive PRD Authoring
Guides the Product Manager through creating and maintaining `docs/PRD.md`.
Changes are tracked via a Revision History table inside the document (no dated filenames).
- Entry: [SKILL.md](chsh-pm-prd/SKILL.md)
- Template: [PRD_TEMPLATE.md](chsh-pm-prd/PRD_TEMPLATE.md)

#### `chsh-qa-test` — QA & Functional Test (Phase 4)
Test Report (always, hardware required) and QA Report (release/demo only, no hardware).
- Entry: [SKILL.md](chsh-qa-test/SKILL.md)
- Templates:
	- [TEST_TEMPLATE.md](chsh-qa-test/TEST_TEMPLATE.md) — PRD acceptance criteria testing
	- [QA_TEMPLATE.md](chsh-qa-test/QA_TEMPLATE.md) — code quality review (0–100)

### Writing Skills

#### `chsh-txt-review` — Text / Message Reviewer
Polish replies to customers or colleagues: friendly, professional, clear.
- Entry: [SKILL.md](chsh-txt-review/SKILL.md)

---

## 🚀 Usage

Skills are loaded during development conversations. Each skill provides templates,
guides, automation scripts, and best practices.

### Invocation Pattern

Mention the skill name in your request, for example:
- *"Use `chsh-dev-project` to generate a new Wi-Fi STA project."*
- *"Run `chsh-qa-test` on the nordic-wifi-webdash project."*
- *"Commit with `chsh-dev-commit`."*

## 📁 Structure

```
skills/
├── chsh-ncs-workflow/     Full lifecycle orchestrator (entry point)
├── chsh-dev-commit/       Git commit workflow
├── chsh-dev-mem-opt/      Memory optimization
├── chsh-dev-spec/         Technical design — specs from PRD
│   ├── OVERVIEW_TEMPLATE.md
│   ├── ARCH_TEMPLATE.md
│   └── MODULE_TEMPLATE.md
├── chsh-dev-project/      Code implementation from specs
│   ├── debug/
│   ├── env-setup/
│   ├── architecture/
│   ├── protocols/
│   │   └── webserver/
│   ├── wifi/
│   ├── overlays/
│   ├── templates/
│   └── guides/
├── chsh-pm-prd/           PRD authoring (SKILL.md + PRD_TEMPLATE.md)
├── chsh-qa-test/        QA report + functional test report
├── chsh-txt-review/       Text / message review
├── README.md
└── .gitignore
```

## 🔄 Workflow Integration

See [`chsh-ncs-workflow/SKILL.md`](chsh-ncs-workflow/SKILL.md) for the full four-phase lifecycle, document conventions, and ownership table:

```
chsh-pm-prd  →  PRD.md  →  chsh-dev-spec  →  docs/specs/  →  chsh-dev-project  →  code  →  chsh-qa-test
```

Living documents (`PRD.md`, specs) use a **Changelog table**. QA/Test reports use dated filenames (`TEST-YYYY-MM-DD-HH-MM.md`, `QA-YYYY-MM-DD-HH-MM.md`).

## 📊 Token Efficiency

- Core `SKILL.md` files: ~2,000 tokens (auto-loaded)
- Detailed guides: 5,000+ tokens (loaded on-demand)
- Templates and configs: accessed as needed

## 📝 License

Skills and templates include appropriate license headers for target projects.

Individual skills may reference or include:
- Nordic 5-Clause License (LicenseRef-Nordic-5-Clause)
- Apache License 2.0
- MIT License


## 📅 Last Updated

April 9, 2026
