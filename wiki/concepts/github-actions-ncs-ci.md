---
title: GitHub Actions CI Monitoring and Pre-Built Firmware Testing
created: 2026-05-07
updated: 2026-05-07
type: concept
tags: [ci, github-actions, firmware, testing, ncs, embedded]
sources: []
confidence: high
---

# GitHub Actions CI Monitoring and Pre-Built Firmware Testing

Lessons from operating a rolling CI/CD pipeline for NCS firmware projects.
Covers: monitoring run status, common failure patterns, pre-built firmware flash-and-test loop.

See also: [embedded-system-general-debugging](embedded-system-general-debugging.md), [mcp-nrflow-tools](mcp-nrflow-tools.md)

---

## 1. GitHub Actions Run Lifecycle

After a `git push`, the workflow goes through these states:

| State | `gh` symbol | Meaning |
|-------|------------|---------|
| queued | `*` (pending) | Runner not yet assigned |
| in_progress | `*` (spinning) | Build running |
| completed/success | `✓` | All steps passed |
| completed/failure | `X` | One or more steps failed |
| completed/cancelled | `-` | Manually cancelled or `cancel-in-progress` triggered |

---

## 2. Monitoring with `gh` CLI (Best Practice)

Always use the `gh` CLI to check run status. Do not rely on polling GitHub web UI.

### List recent runs

```bash
gh run list --repo <owner>/<repo> --limit 5
```

### Watch a specific run until completion

```bash
gh run watch <run-id> --repo <owner>/<repo> --interval 30
```

### Get logs of a failed step

```bash
gh run view <run-id> --repo <owner>/<repo> --log-failed
```

### Re-run failed jobs only

```bash
gh run rerun <run-id> --repo <owner>/<repo> --failed
```

---

## 3. Common NCS CI Failure Patterns

| Failure | Root cause | Fix |
|---------|-----------|-----|
| `curl: (22) 404` when installing nrfutil | nrfutil download URL changed | Use `ghcr.io/nrfconnect/sdk-nrf-toolchain:<ncs-version>` Docker container instead |
| `west: command not found` | Missing toolchain in PATH | Container approach puts everything in PATH automatically |
| `west init` fails with "already initialized" | Cache hit restores repos but not `.west/` | Re-run `west init -l` with `|| true` guard after cache restore |
| `git am` fails with "no identity" | Container root has no git config | Add `git config --global user.email "ci@github-actions"` + `user.name` before `git am` |
| `git am` silently skipped patch | `--check` fails on shallow clone | Remove `--check`; run `git am` directly, detect "already applied" by checking commit log subject line |
| `git am` fails with "already applied" | Patch cached with repos | Abort `git am`, check log for subject line; if found, skip — otherwise exit 1 |
| `merged.hex not found` | No MCUboot in sysbuild | Fall back to `zephyr/zephyr.hex`; check sysbuild.conf |
| `permission denied` on git operations in container | UID mismatch | Add `git config --global --add safe.directory '*'` |
| Release creation fails | Missing `permissions: contents: write` | Add to workflow top-level |
| `No shield 'nrf7002eb2_mspi'` | Explicit `zephyr:` entry in west.yml overrides nrf's import allowlist | Remove explicit `zephyr` and `nrfxlib` entries from west.yml; let `nrf: import: true` handle them |
| Many repos fail to update | `west update --narrow` skips transitive imports | Use full `west update -o=--depth=1 -n` without `--narrow`; use `ci-skip` group filter for optional repos |
| `expected one argument` for group filter | `--group-filter -ci-skip` parsed as new flag | Use `west config manifest.group-filter -- -ci-skip,-benchmark` before `west update` |
| `invalid version 0.1` in west.yml | YAML parses `0.10` as float `0.1` | Quote the version: `version: "0.10"` |

---

## 4. Docker Container Approach (Recommended for NCS)

Replace manual nrfutil install + toolchain download with the pre-built image:

```yaml
jobs:
  build:
    runs-on: ubuntu-24.04
    container:
      image: ghcr.io/nrfconnect/sdk-nrf-toolchain:v3.3.0
```

Benefits:
- All tools (west, cmake, ninja, arm toolchain) pre-installed — no 404 risk
- Build starts immediately, no 10-minute toolchain download
- Reproducible environment identical to Nordic's own CI
- `west build` works directly without `nrfutil sdk-manager toolchain launch` wrapper

---

## 5. Rolling "Latest" Release Pattern

For reference/demo repos that don't need semantic versioning:

```yaml
- name: Update rolling latest release
  if: github.ref == 'refs/heads/main'
  uses: softprops/action-gh-release@v2
  with:
    tag_name: latest
    name: "Latest Build"
    make_latest: true
    files: artifacts/merged.hex
    body: |
      Auto-updated. Commit: ${{ github.sha }}
  env:
    GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
```

The action creates a `latest` tag if absent, or updates it (and the attached release) to the new commit. No manual tagging required.

**Key**: `permissions: contents: write` must be declared at the job or workflow level.

---

## 6. Pre-Built Firmware Flash and Test Loop

After CI passes and the release is updated, test the pre-built firmware as an evaluator would:

### Step 1 — Download the pre-built hex

```bash
gh release download latest --repo <owner>/<repo> --pattern "*.hex" -D /tmp/fw/
```

Or manually from the Releases page.

### Step 2 — Flash with west

```bash
# Filename uses the descriptive naming convention (see Section 10)
west flash --hex-file /tmp/fw/<project>-<board>-<shield>-ncs<version>.hex --recover --dev-id <BOARD_SERIAL>
```

Or use **nRF Connect for Desktop → Programmer** (no local toolchain needed).

### Step 3 — Connect to UART and verify boot

```bash
python3 nordicsemi_uart_monitor.py --port /dev/cu.usbmodem... --baud 115200
```

Expected sequence in logs:
1. `*** Booting nRF Connect SDK` — MCUboot started
2. `[0.xxx] <inf> ...` — application modules initializing
3. `uart:~$` — shell prompt ready

### Step 4 — Run a quick functional check

```sh
uart:~$ wifi scan        # expect scan results or "Scan request failed" if no radio
uart:~$ wifi status      # expect interface state
uart:~$ kernel threads   # check thread health
```

### Step 5 — Repeat the loop

```
push fix → gh run list (wait for green) → gh release download latest → west flash → verify
```

Minimum loop for each fix: one full boot + wifi scan confirms the firmware is functional.
For reliability issues, run the loop test (see [embedded-system-general-debugging](embedded-system-general-debugging.md#7-loop-testing-is-the-only-reliable-quality-gate)).

---

## 7. NCS Workspace Caching Strategy

Cache the cloned NCS repos to cut CI time from ~15 min → ~3 min on cache hits:

```yaml
- name: Cache NCS workspace
  uses: actions/cache@v4
  with:
    path: |
      workspace/nrf
      workspace/zephyr
      workspace/modules
      workspace/nrfxlib
      workspace/bootloader
    key: ncs-v3.3.0-${{ hashFiles('path/to/west.yml', 'path/to/patches/**') }}
```

**Cache key must include patches** — if a patch changes, invalidate the cache so the
repos are re-cloned at the base tag and the new patch is applied cleanly.

**After a cache hit**: west's `.west/` config directory is not cached. Re-run
`west init -l <app> 2>/dev/null || true` to restore it before calling `west build`.

---

## 8. `west.yml` Manifest for App Repos

If your app does not have a `west.yml`, add a minimal one so `west init -l` works:

```yaml
manifest:
  version: "0.10"   # MUST be quoted — bare 0.10 is a YAML float → parsed as 0.1
  projects:
    - name: nrf
      url: https://github.com/nrfconnect/sdk-nrf
      revision: v3.3.0  # pin NCS version
      import: true       # imports ALL transitive deps including zephyr, hal_nordic, etc.
  self:
    path: <your-app-folder-name>
```

**Critical**: Do NOT add explicit `zephyr` or `nrfxlib` entries alongside `nrf: import: true`.
nrf's import uses an allowlist that adds the correct `west-commands:` and provides `hal_nordic`.
Overriding it breaks `west build` and causes missing modules.

**For optional repos** (wfa-qt-control-app, coremark, etc.) that fail shallow fetches: assign them to a `ci-skip` group and filter it out:

```yaml
    - name: wfa-qt-control-app
      ...
      groups: [ci-skip]
```

```yaml
# In CI, before west update:
west config manifest.group-filter -- -ci-skip,-benchmark
west update -o=--depth=1 -n
```

The `--` separator is required so `-ci-skip` is not parsed as a new flag.
The `-n` flag tells west to skip SHA resolution for repos not in the manifest (speeds up shallow clones).

---

## 9. Idempotent `git am` Pattern for Cached Repos

When repos are cached, patches may already be applied. A naive `git am` will fail with "already applied".
Use this pattern for graceful handling:

```bash
apply_patch() {
  local repo="$1"
  local patch="$2"
  if git -C "$repo" am "$patch" 2>&1; then
    echo "Patch applied to $repo"
  else
    git -C "$repo" am --abort 2>/dev/null || true
    local subject
    subject=$(sed -n 's/^Subject: \[PATCH[^]]*\] //p' "$patch" | head -1)
    if git -C "$repo" log --oneline | grep -qF "$subject"; then
      echo "Already applied — skipping"
    else
      echo "ERROR: patch failed and is not already applied"
      exit 1
    fi
  fi
}
```

**Key**: Always configure git identity in the container before calling `git am`:
```bash
git config --global user.email "ci@github-actions"
git config --global user.name "GitHub Actions"
git config --global --add safe.directory '*'
```

Do NOT use `git am --check` to pre-flight on shallow clones — it may fail even for valid patches.

---

## 10. Pre-Built Firmware Artifact Naming

Use a descriptive, versioned filename instead of a generic `merged.hex`:

```
<project>-<board>-<shield>-ncs<version>.hex
# Example:
nordic-wifi-shell-sqspi-nrf54lm20dk-nrf7002ebii-ncs3.3.0.hex
```

In build.yml:
```yaml
- name: Collect artifacts
  run: |
    mkdir -p artifacts
    HEX="workspace/<app>/build/merged.hex"
    [ ! -f "$HEX" ] && HEX="workspace/<app>/build/<app>/zephyr/zephyr.hex"
    [ ! -f "$HEX" ] && HEX="workspace/<app>/build/zephyr/zephyr.hex"
    cp "$HEX" artifacts/<descriptive-name>.hex
```

The release body should link to the Evaluator Quick Start section, not repeat instructions inline:
```yaml
body: |
  For setup and flashing instructions, see the **[Evaluator Quick Start](https://github.com/<org>/<repo>#evaluator-quick-start)** in the README.
```

> Do not duplicate hardware modification or Board Configurator steps in the release body — they are already documented in the README Quick Start. Keeping the body short reduces drift.

### Remove stale release assets after renaming

When a hex file is renamed (e.g., `merged.hex` → descriptive name), old assets remain attached to the rolling release. Delete them with:

```yaml
- name: Remove stale merged.hex from release
  if: github.ref == 'refs/heads/main'
  run: |
    gh release delete-asset latest merged.hex --yes 2>/dev/null || true
  env:
    GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
    GH_REPO: ${{ github.repository }}
```

The `|| true` guard prevents the step from failing when the stale asset no longer exists.
