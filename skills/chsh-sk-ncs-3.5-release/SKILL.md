---
name: chsh-sk-ncs-3.5-release
description: Use when tagging a release, watching CI, or publishing firmware to GitHub for NCS projects. Covers git tag → CI → GitHub Release artifact. After publishing, triggers chsh-sk-ncs-4.2-validation for hardware validation with pre-built firmware.
---

# chsh-sk-ncs-3.5-release — Release Tagging & Publishing

Covers the full release cycle for NCS firmware projects: git tag → GitHub Actions CI → release artifact → flash → verify.

> **Prerequisites**: A passing build and a GitHub repo with a CI workflow that publishes `.hex` artifacts.
> If CI is broken, fix it first with **chsh-sk-ncs-3.2-debug** Mode G before tagging.

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

## Step 4 — Handoff to Validation

Release published with pre-built firmware artifact available. Hand off to Phase 4.2 for hardware validation.

```AskQuestion:
  prompt: "Release published. Run Phase 4.2 Validation with the pre-built firmware?"
  options:
    - "Yes — load chsh-sk-ncs-4.2-validation (will download and flash pre-built artifact)"
    - "No — release is done"
```

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
edit → commit (chsh-sk-git) → push → gh run watch
  → (fail) → gh run view --log-failed → fix → repeat
  → (pass) → AskQuestion (Step 4) → load chsh-sk-ncs-4.2-validation → done
```

---

## Related Skills

| Task | Skill |
|------|-------|
| Commit before pushing | `chsh-sk-ncs-3.4-git-commit` |
| Hardware validation with pre-built firmware | `chsh-sk-ncs-4.2-validation` |
| Debug CI failures, UART capture | `chsh-sk-ncs-3.2-debug` (Mode G) |
| Build commands, toolchain | `chsh-sk-ncs-env` |

---

## Gotchas
- TODO: add one entry per real observed failure or routing false-positive

## Self-Update Policy

At the **end of each conversation**, review what was discovered and check
whether any facts in this skill are new, corrected, or outdated (e.g. new
CI workflow names, GitHub API changes, firmware artifact paths, toolchain
hash updates).

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
