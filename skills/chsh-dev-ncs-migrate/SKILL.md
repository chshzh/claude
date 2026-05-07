---
name: chsh-dev-ncs-migrate
description: >-
  Migrate a Nordic nRF Connect SDK application from an older NCS version to a
  newer one — handles single-step and multi-step jumps. Establishes a baseline
  build + functional test on the source version, walks the official release
  notes and migration guides version-by-version, applies required code/Kconfig
  changes, drives the build to clean, flashes to hardware, and verifies
  functionality. Use when the user asks to upgrade, migrate, or port an NCS
  project to a newer SDK version, or when CI breaks after a west.yml SDK
  revision bump.
---

# chsh-dev-ncs-migrate — NCS Version Migration

Migrate an existing NCS application from `v<old>` to `v<new>`. Handles single
hops (`v3.2 → v3.3`) and multi-hop jumps (`v2.9 → v3.0 → v3.1 → v3.2 → v3.3`).

**Core rule**: every step must end with a successful build AND a functional
smoke test on hardware. Never claim a migration is done without flashing.

> **Prerequisite**: A working baseline of the application on its current NCS
> version. If the baseline does not build/run, **fix it first** — don't
> migrate broken code. Use `chsh-dev-ncs-debug` to triage.

---

## Authoritative Knowledge Sources

Always consult the official Nordic docs first — release notes and migration
guides are the source of truth, not memory.

| Source | URL pattern |
|--------|-------------|
| Release notes (current) | https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/releases_and_maturity/release_notes.html |
| Migration guides index | https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/releases_and_maturity/migration_guides.html |
| Per-version release notes | `bundle/ncs-<version>/page/nrf/releases_and_maturity/release_notes/rn_<version>.html` |
| Per-version migration guide | `bundle/ncs-latest/page/nrf/releases_and_maturity/migration/migration_<version>.html` |

For each hop, read **both** release notes AND migration guide for the target
version. Migration guides describe the changes to apply; release notes flag
new features, deprecated APIs, and known issues.

---

## Step 0 — Baseline (mandatory)

Establish what currently works before changing anything. Skipping this step
makes every later failure ambiguous.

### 0a. Identify versions

```bash
# Source version (current)
grep -E "revision: v[0-9]" <app>/west.yml | head -3
cat <app>/VERSION 2>/dev/null

# Target version (user-specified, or latest)
# Ask the user: "Migrate to which NCS version?"
```

### 0b. Read project context

```bash
cat <app>/README.md
cat <app>/west.yml
cat <app>/prj.conf | head -40
ls <app>/src/modules/  2>/dev/null
ls <app>/boards/       2>/dev/null
git -C <app> log --oneline -10
```

Note the **board target**, **shield**, and **all overlays** the project uses
— they all influence which migration items apply.

### 0c. Build and verify on the source version

Use **chsh-dev-ncs-env** to launch the source toolchain (e.g. `--ncs-version=v3.2.0`):

```bash
nrfutil sdk-manager toolchain launch --ncs-version=v<source> -- \
  west build -b <board> -p -d <app>/build <app> [-DSHIELD=<shield>] \
  [-DEXTRA_CONF_FILE="<overlays>"]
```

Flash and run a functional smoke test (see **chsh-qa-ncs-test** Part A
shortcut: boot → shell prompt → one feature command). Save the UART log as
the baseline reference. **Do not proceed if the baseline fails.**

### 0d. Lock the baseline

```bash
git -C <app> tag pre-migrate-from-v<source>
```

This is the rollback point.

---

## Step 1 — Plan the Migration Path

Ask the user to confirm the target version. Then determine intermediate hops.

### 1a. Choose hop strategy

| Source → Target | Strategy |
|-----------------|----------|
| One minor apart (v3.2 → v3.3) | **Mode A — Single hop** |
| Two or more minor apart (v2.9 → v3.3) | **Mode B — Multi-hop** (recommended): visit each intermediate release |
| User explicitly accepts risk | **Mode C — Direct jump** (skip intermediates) |

Default to **Mode B** unless the user opts in to Mode C. Multi-hop reduces
debugging surface — each intermediate gets isolated, focused fixes.

### 1b. List the hops

```
v3.0.0 → v3.1.0 → v3.2.0 → v3.3.0
```

Use stable releases only — skip `-rc`, `-dev`, `-tip` unless the user requests.

### 1c. Pre-fetch migration material

For each hop, fetch and read the migration guide AND release notes. Use the
WebFetch tool to retrieve the current pages (URLs above). Capture:
- Required code changes (Kconfig renames, header moves, API signatures)
- Deprecated APIs the project may use
- Build system changes (sysbuild, partition manager, board-name changes)
- Toolchain version requirement

Present the summary to the user before starting the first hop.

---

## Step 2 — Execute Each Hop

For each hop in order, repeat Steps 2a → 2g. **Do not batch hops.**

### 2a. Switch the toolchain

```bash
nrfutil sdk-manager toolchain install --ncs-version=v<target>
nrfutil sdk-manager toolchain list           # confirm installed
```

### 2b. Update `west.yml`

```yaml
manifest:
  remotes:
    - name: ncs
      url-base: https://github.com/nrfconnect
  projects:
    - name: sdk-nrf
      remote: ncs
      revision: v<target>            # ← bump this
      import: true
```

Run `west update` (under the new toolchain).

### 2c. Apply migration-guide changes mechanically

Work through the migration guide top-to-bottom. For each item, if it
applies to the project (check the current code first), apply the change.

Common change categories:
- **Kconfig renames**: `CONFIG_FOO_OLD` → `CONFIG_FOO_NEW` in `prj.conf`,
  `boards/*.conf`, `overlay-*.conf`
- **Header moves / API signature changes**: update `#include` and call sites
- **Devicetree property changes**: update `.overlay` files
- **Board-name changes**: e.g. `nrf54l15dk_nrf54l15_cpuapp` → `nrf54l15dk/nrf54l15/cpuapp`
- **Sysbuild-required**: convert from child_image to sysbuild config
- **Driver migration**: legacy → new driver subsystem

### 2d. Build to clean

```bash
nrfutil sdk-manager toolchain launch --ncs-version=v<target> -- \
  west build -b <board> -p -d <app>/build <app> [-DSHIELD=<shield>]
```

Iterate on warnings/errors. For each error:
1. Look up in the migration guide first
2. Look up in the release notes (often listed under "Known issues" or
   "Bug fixes")
3. If neither covers it: **STOP and ask the user** — do not guess at API
   semantics. See **Decision Gates** below.

### 2e. Flash and functional smoke test

```bash
nrfutil sdk-manager toolchain launch --ncs-version=v<target> -- \
  west flash -d <app>/build --erase --dev-id <SN>   # or --recover for nRF54LM20DK
```

Connect via UART (use `mcp_nrflow_nordicsemi_workflow_ncs` for the helper
script) and run the same smoke test from Step 0c. Compare against the
baseline log — first divergence identifies the regression.

### 2f. Commit the hop

Use **chsh-dev-git-commit**:

```bash
git -C <app> tag migrated-v<target>
```

Commit message convention:
```
chore(ncs): migrate to v<target>

- west.yml: revision v<source> → v<target>
- prj.conf: <list of Kconfig changes>
- src/<file>: <API changes>
- boards/<file>: <DT property changes>

Verified: clean build + UART smoke test passes (boot, shell, <feature>).
```

### 2g. Decide: continue or stop

If more hops remain → return to **Step 2a** with the next target version.
Otherwise → **Step 3**.

---

## Step 3 — Final Functional Verification

A smoke test per hop is not enough for a final delivery. After the last hop:

1. Run the loop test for stability (≥ 10 passes for acceptance, 20 for
   release) — see **chsh-dev-ncs-debug** Mode F
2. Run the full Phase 4 test from **chsh-qa-ncs-test** Part A against the
   PRD acceptance criteria
3. If a release is expected: tag and publish via **chsh-dev-git-release**

---

## Decision Gates — When to Ask the User

The migration guides cover **public API** changes. They do not cover:

- Out-of-tree drivers / custom patches in `nrf/`, `zephyr/`, `nrfxlib/`
- Third-party modules in `west.yml` (Memfault, FreeRTOS shim, vendor SDKs)
- Custom partition layouts that conflict with new defaults
- Behavioural differences in unchanged APIs (timing, memory ordering, defaults)
- Test failures that the loop test surfaces but the build doesn't

**Stop and ask** whenever any of the following is true:

| Trigger | Question to ask |
|---------|----------------|
| API removed, no clear replacement in docs | "Function X was removed in v<target>; the migration guide doesn't list a replacement. Should I (a) keep using a workaround, (b) remove the feature, (c) try Y/Z?" |
| Custom patch in `nrf/` or `zephyr/` no longer applies cleanly | "The patch `<file>.patch` no longer applies on v<target>. Should I (a) drop it, (b) re-base it manually, (c) ask you to re-author it?" |
| Third-party module pinned to old SHA | "The west manifest pins module X to a SHA from v<source> era; v<target> needs ≥Y. Update to which version?" |
| Functional regression after build passes | "Build is clean but the boot log diverges at line N (`<line>`). Investigate now or continue and revisit?" |
| Spec/PRD requirement now hard to meet | "Acceptance criterion AC-3 is no longer achievable as written because <reason>. Update PRD or relax target?" |

Use the **AskQuestion** tool when available so options are explicit.

---

## Common Migration Patterns

These appear across multiple NCS hops. Verify against the actual migration
guide for the target version before applying.

### Board target syntax (v3.0+)
`<board>_<soc>_<core>` → `<board>/<soc>/<core>` (HWMv2). Update build
commands, GitHub Actions matrices, and any docs that quote the legacy form.

### Sysbuild as default (v2.7+)
Multi-image apps (MCUboot + app) must use `sysbuild.conf` and
`pm_static_<board>.yml` instead of child_image YAML.

### Wi-Fi driver names (v3.0+)
`nrf_wifi` consolidations: `CONFIG_WIFI_NRF700X` → `CONFIG_WIFI_NRF70`.
Many sub-Kconfigs renamed to drop the `X` suffix.

### Connection Manager / net_mgmt (v2.5+)
`net_if_up()` ergonomics changed. Some apps now need
`CONFIG_NET_CONNECTION_MANAGER` and event-driven readiness.

### Bluetooth Mesh / Audio refactors
Each minor release has touched these. Always re-read the migration guide.

---

## Source-Version Compatibility Notes

When the source version is older than v2.6, expect significant work:
- Build system changed (KConfigLib, CMake versions)
- Kconfig hierarchy reorganised
- Many subsystems were upstreamed/refactored

For very old sources (pre-v2.5), consider whether a **rewrite-on-target**
is faster than a migration. Surface this trade-off to the user up front.

---

## Self-Update Policy

At the **end of each conversation**, review what was discovered and check
whether any facts in this skill are new, corrected, or outdated (e.g. new
URL patterns, new common migration items, new questions worth asking).

If updates are warranted:
1. Collect proposed changes with a brief rationale for each.
2. Present a summary and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.

---

## Related Skills

| Task | Skill |
|------|-------|
| Toolchain install + west command wrapper | `chsh-dev-ncs-env` |
| Build/runtime debugging during migration | `chsh-dev-ncs-debug` |
| Functional test against PRD acceptance criteria | `chsh-qa-ncs-test` |
| Per-hop commit | `chsh-dev-git-commit` |
| Final release after migration | `chsh-dev-git-release` |
| Update specs if migration changes architecture | `chsh-dev-ncs-spec` |
| Optimize after migration (heap/stack defaults change) | `chsh-dev-ncs-memory` |
