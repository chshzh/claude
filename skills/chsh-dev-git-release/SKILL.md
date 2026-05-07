---
name: chsh-dev-git-release
description: Tag a git release, create a GitHub release with firmware artifacts, download pre-built firmware, and flash + verify on hardware. Use when cutting a release, publishing firmware, or doing a full release validation loop (tag → CI → download → flash → test).
---

# chsh-dev-git-release — Release Tagging, Publishing & Firmware Validation

Covers the full release cycle for NCS firmware projects: git tag → GitHub Actions CI → release artifact → flash → verify.

> **Prerequisites**: A passing build and a GitHub repo with a CI workflow that publishes `.hex` artifacts.
> If CI is broken, fix it first with **chsh-dev-ncs-debug** Mode G before tagging.

---

## Step 0 — Decide Release Type

| Situation | Action |
|-----------|--------|
| New feature set ready | Bump `MAJOR.MINOR` — create annotated tag |
| Bug fix only | Bump `PATCH` — create annotated tag |
| Rolling/nightly release | Push to `main`/`master` — CI overwrites `latest` release |
| Pre-release / RC | Tag as `v1.2.0-rc1`, mark GitHub release as pre-release |

Read the current version:

```bash
git -C <app> describe --tags --abbrev=0   # latest tag
git -C <app> log --oneline -5             # recent commits
```

---

## Step 1 — Prepare and Tag

### 1a. Verify the build is clean

```bash
nrfutil sdk-manager toolchain launch --ncs-version=v3.3.0 -- \
  west build -b <board> -d <app>/build <app>
```

Do not tag a broken build.

### 1b. Create annotated tag

```bash
git -C <app> tag -a v<MAJOR>.<MINOR>.<PATCH> -m "Release v<MAJOR>.<MINOR>.<PATCH>

- <summary of key changes>
- <bug fixes>"

git -C <app> push origin v<MAJOR>.<MINOR>.<PATCH>
```

> For rolling/nightly releases that use GitHub Actions to overwrite `latest`, just push to main:
> ```bash
> git -C <app> push origin main
> ```

---

## Step 2 — Watch CI

After pushing, monitor the GitHub Actions run:

```bash
# List recent runs
gh run list --repo <owner>/<repo> --limit 3

# Watch live (updates every 30 s)
gh run watch <run-id> --repo <owner>/<repo> --interval 30

# On failure — get failing step logs
gh run view <run-id> --repo <owner>/<repo> --log-failed
```

**Expected CI outcome**: a new GitHub Release is created (or updated) with a `.hex` artifact named like:
```
<project>-<board>-<shield>-ncs<version>.hex
```

If CI fails, see **Common CI failures** below. Fix, push, watch again (do not re-tag unless the previous tag is wrong).

---

## Step 3 — Create / Update GitHub Release (manual, if CI doesn't auto-release)

```bash
# Create a new release for a version tag
gh release create v<MAJOR>.<MINOR>.<PATCH> \
  --repo <owner>/<repo> \
  --title "v<MAJOR>.<MINOR>.<PATCH>" \
  --notes "See README for setup instructions." \
  <app>/build/merged.hex#<descriptive-filename>.hex

# Or upload additional artifacts to an existing release
gh release upload v<MAJOR>.<MINOR>.<PATCH> \
  <path/to/firmware.hex> \
  --repo <owner>/<repo> --clobber

# Delete a stale artifact (e.g. renamed from merged.hex)
gh release delete-asset v<MAJOR>.<MINOR>.<PATCH> merged.hex \
  --repo <owner>/<repo> --yes 2>/dev/null || true
```

**Release notes policy**: keep them short.
- State what the build contains and the commit SHA.
- Link to the README Evaluator Quick Start for setup instructions.
- Do NOT repeat hardware modification steps or version requirements — these duplicate the README.

---

## Step 4 — Download Pre-Built Firmware

```bash
mkdir -p /tmp/fw/<project>

# Download latest release
gh release download latest \
  --repo <owner>/<repo> \
  --pattern "*.hex" \
  -D /tmp/fw/<project>/

ls /tmp/fw/<project>/   # confirm descriptive filename
```

---

## Step 5 — Flash and Verify

### 5a. Identify connected boards

```bash
nrfutil device list
```

Note the serial numbers and VCOM port assignments.

### 5b. Flash

```bash
# nRF54LM20DK (needs --recover on first flash or after erase)
nrfutil sdk-manager toolchain launch --ncs-version=v3.3.0 -- \
  west flash --hex-file /tmp/fw/<project>/<firmware>.hex \
  --recover --dev-id <SN>

# nRF7002DK
nrfutil sdk-manager toolchain launch --ncs-version=v3.3.0 -- \
  west flash --hex-file /tmp/fw/<project>/<firmware>.hex \
  --erase --dev-id <SN>
```

Or use **nRF Connect for Desktop → Programmer** (no local toolchain needed).

### 5c. Verify boot over UART

Call `mcp_nrflow_nordicsemi_workflow_ncs` to load `nordicsemi_uart_monitor.py`, then:

```bash
python3 nordicsemi_uart_monitor.py --port /dev/tty.usbmodem<VCOM> --baud 115200
```

VCOM mapping:
| Board + Shield | App console | HWFC |
|----------------|-------------|------|
| nRF54LM20DK + nRF7002EB-II sQSPI | VCOM1 (UART20) | No (`rtscts=False`) |
| nRF7002DK | VCOM0 (single port) | Yes (`rtscts=True`) |

Expected boot sequence:
1. `*** Booting nRF Connect SDK` — MCUboot + firmware running
2. Module init lines
3. `uart:~$` — shell ready

Quick functional smoke test:
```sh
uart:~$ wifi scan
uart:~$ wifi status
uart:~$ kernel threads
```

---

## Step 6 — Loop Until Stable (for releases)

Before marking a release **done**, run the loop test to confirm stability:

```bash
python3 <app>/scripts/loop_test.py 10    # 10 passes = acceptance gate
python3 <app>/scripts/loop_test.py 20    # 20 passes = release gate
```

If `loop_test.py` doesn't exist, copy the template from:
```
~/.claude/skills/chsh-dev-ncs-debug/scripts/loop_test.py
```

Record the pass rate in the QA report (see **chsh-qa-ncs-test**).

---

## Common CI Failures

| Failure | Root cause | Fix |
|---------|-----------|-----|
| `curl: (22) 404` on nrfutil | URL no longer valid | Switch to `ghcr.io/nrfconnect/sdk-nrf-toolchain:<ver>` container |
| `west: command not found` | Toolchain not in PATH | Use container image |
| `merged.hex not found` | No MCUboot sysbuild | Fallback to `zephyr/zephyr.hex`; check `sysbuild.conf` |
| `git am` fails "already applied" | Patch cached | Add idempotent check: `git am --check "$p" && git am "$p"` |
| Permission denied on git ops | UID mismatch in container | Add `git config --global --add safe.directory '*'` |
| Release step fails | Missing `permissions: contents: write` | Add at workflow top level |

---

## Fix-Push-Verify Loop

When CI is failing, iterate:

```
edit → commit (chsh-dev-git-commit) → push → gh run watch
  → (fail) → gh run view --log-failed → fix → repeat
  → (pass) → download latest → flash → verify uart:~$ → done
```

---

## Related Skills

| Task | Skill |
|------|-------|
| Commit before pushing | `chsh-dev-git-commit` |
| Functional test after flashing | `chsh-qa-ncs-test` |
| Debug CI failures, UART capture | `chsh-dev-ncs-debug` (Mode G) |
| Build commands, toolchain | `chsh-dev-ncs-env` |
| Loop test script template | `chsh-dev-ncs-debug/scripts/loop_test.py` |
