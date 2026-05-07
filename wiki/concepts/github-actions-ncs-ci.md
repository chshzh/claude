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
| `git am` fails with "already applied" | Patch cached with repos | Add idempotent check before `git am` |
| `merged.hex not found` | No MCUboot in sysbuild | Fall back to `zephyr/zephyr.hex`; check sysbuild.conf |
| `permission denied` on git operations in container | UID mismatch | Add `git config --global --add safe.directory '*'` |
| Release creation fails | Missing `permissions: contents: write` | Add to workflow top-level |

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
west flash --hex-file /tmp/fw/merged.hex --recover --dev-id <BOARD_SERIAL>
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
  version: 0.10
  projects:
    - name: nrf
      url: https://github.com/nrfconnect/sdk-nrf
      revision: v3.3.0  # pin NCS version
      import: true       # imports all NCS transitive deps
  self:
    path: <your-app-folder-name>
```

This makes the app a self-contained west manifest project. CI and local developers can
both use `west init -l <app>` to set up a complete workspace from scratch.
