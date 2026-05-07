---
name: chsh-dev-git-commit
description: Plans, groups, and executes git commits. Inspects the worktree, proposes a commit plan grouped by logical change, waits for approval, then stages and commits. Supports Conventional Commits (user app repos) and Zephyr style (NCS/Zephyr repos). Use when the user asks to prepare commits, commit changes, or split work into multiple git commits.
---

# Git Commit Workflow

## Critical Rules

1. **Inspect before committing**: Always run `git diff .` to review the full worktree before proposing or creating any commits.
2. **Plan first, commit second**: Present a commit plan (grouped files, rationale, suggested messages) and wait for explicit user approval before running `git commit`.
3. **Split by logical change**: Separate commits by concern — do not mix unrelated fixes, refactors, docs, formatting-only changes, or generated files unless the user explicitly requests it.
4. **No interactive commands**: Use only non-interactive Git commands.
5. **Never assume permission**: Treat any request to "commit changes" as permission to inspect and plan, not as permission to execute `git commit`.

## Workflow

### Step 1 — Inspect the worktree

```bash
git diff .
git status --short
git diff --cached   # only if staged changes exist
```

### Step 2 — Propose a commit plan

Present a table or list showing:
- **Group**: the set of files for each proposed commit
- **Rationale**: why these files belong together
- **Suggested commit message**: use the correct style for the repo (see **Commit Message Formats** below)

### Step 3 — Wait for approval

After presenting the plan, use the `AskQuestion` tool to collect approval. Always offer these options:

```
Question: "Proceed with this commit plan?"
Options:
  - "Approve — commit as planned"
  - "Edit commit messages first"
  - "Re-group the commits"
  - "Cancel"
```

Do not run `git commit` until the user selects **Approve**. Treat any other selection as a request to revise the plan.

### Step 4 — Execute approved commits

For each approved commit, stage only the relevant files and commit:

```bash
git add <file1> <file2> ...
git commit -s -m "type(scope): description"
```

Use a HEREDOC for multi-line messages when needed:

```bash
git commit -s -m "$(cat <<'EOF'
type(scope): short summary

- detail line 1
- detail line 2
EOF
)"
```

### Step 5 — Push and verify CI (for repos with GitHub Actions)

After pushing, always check that CI passes before declaring the task done:

```bash
git push origin <branch>

# Get the run ID of the new run (may take a few seconds to appear)
gh run list --repo <owner>/<repo> --limit 3

# Watch until completion
gh run watch <run-id> --repo <owner>/<repo> --interval 30

# If CI fails, get logs of failing step:
gh run view <run-id> --repo <owner>/<repo> --log-failed
```

For the "loop until succeeded" pattern (user says "fix, commit, check CI, repeat"):
1. Fix the issue
2. Commit and push
3. Watch CI with `gh run watch`
4. On failure: read `--log-failed`, diagnose, fix, repeat from step 1
5. On success: proceed to flash pre-built firmware and verify (see `chsh-dev-ncs-debug` Mode G)

> **Rule**: CI green is not enough for firmware repos — always flash the pre-built artifact
> and verify `uart:~$` appears on the correct VCOM port before marking done.

## Commit Message Formats

### Detect repo type first

| Repo | Style |
|---|---|
| User app repo (own project) | Conventional Commits |
| `zephyr/`, `nrf/`, `nrfxlib/`, `modules/` | Zephyr style |

### User App Repos — Conventional Commits

```
type(scope): short summary
```

Common types: `feat`, `fix`, `refactor`, `docs`, `chore`, `test`, `style`, `build`. Title ≤ 72 chars. Body optional but helpful for non-trivial changes.

### NCS / Zephyr Repos — Zephyr Style

```
[nrf tag] area: subarea: short summary

Body explaining what, why, assumptions, and how it was verified.

Upstream PR #: NNNNN          (if porting from upstream)
Ref: NCSDK-XXXXX              (if tracking an internal ticket)
Assisted-by: Claude:claude-sonnet-4.6   (if AI-assisted)
Signed-off-by: Full Name <email>
```

- Title ≤ 72 chars; **body is required** (non-empty) for any upstream contribution
- Body lines ≤ 75 chars
- `Signed-off-by:` required (DCO) — use `git commit -s`
- NRF prefix tags: `[nrf fromlist]` (to upstream), `[nrf fromtree]` (from upstream), `[nrf noup]` (NCS-only), `[nrf ncsdk]` (SDK-only)
- Add `Assisted-by:` when AI wrote substantial code

**Examples:**

```
[nrf fromlist] drivers: nrf_wifi: add WPA3-AUTO security type

The wpa_supplicant requires a dedicated key_mgmt enum value for
WPA3-AUTO mode. Without this, stations advertising both WPA2 and
WPA3 fail key exchange.

Upstream PR #: 106674
Signed-off-by: Your Name <your.email@example.com>
```

```
[nrf noup] tests: my_module: add unit test for zero-length buffer

Tests the boundary condition where input buffer length equals zero,
which previously triggered an assertion in the parser.

Ref: NCSDK-12345
Signed-off-by: Your Name <your.email@example.com>
```

## Grouping Guidelines

| Change type | Separate commit? |
|---|---|
| New feature code | Yes |
| Bug fix | Yes |
| Refactor (no behaviour change) | Yes |
| Config / Kconfig changes | Yes (or with related feature) |
| Docs / README | Yes (unless tightly coupled to a feature) |
| Formatting / whitespace only | Yes |
| Generated / auto-updated files | Yes |
| Board overlay + matching prj.conf | Can group together |
| Sysbuild files | Group by logical purpose |

## NCS-Specific Notes

**File grouping:**
- `prj.conf`, `boards/*.conf`, `overlay-*.conf` — group with the feature they enable
- `CMakeLists.txt` changes — separate commit unless trivially part of a feature add
- `west.yml` changes — always a separate commit with a clear rationale
- `sysbuild/` files — group by logical purpose (e.g. mcuboot config changes together)
- Generated partition manager files (`pm/`, `*.map`) — separate `chore` commit or omit if not meaningful

**In NCS/Zephyr repos (`zephyr/`, `nrf/`, `nrfxlib/`):**
- Use Zephyr style (see above), not Conventional Commits
- Always include a non-empty body — CI will reject empty bodies for upstream PRs
- Run `git log -- <file>` to see how others formatted commits for the same subsystem

## Using Conversation Context

When making commits during or after a conversation where changes were made, **use the conversation to enrich commit messages** — not just the raw diff.

### What to pull from context

| Context signal | How to use it |
|---|---|
| Why a change was made (user explanation or decision) | Use in the commit body as rationale |
| What was tried and rejected | Mention in the body to document intent |
| Linked issue / ticket number mentioned in chat | Add as `Ref:` or body reference |
| AI-assisted implementation discussed in chat | Add `Assisted-by: Claude:claude-sonnet-4.6` |
| A specific user instruction (e.g. "remove X because Y") | Reflect Y in the commit message body |

### Example

If the conversation shows: *"remove .toolchain-version and update CI because Nordic publishes toolchain images for all releases including RCs"*, the commit body should say exactly that — don't just write "remove .toolchain-version".

### Step 1 addendum — Review conversation alongside diff

Before Step 2 (propose plan), scan the conversation for:
- Stated reasons for changes
- Decisions made (e.g. "use v3.3.0-rc2 instead of v3.3-branch")
- Any user-provided context that explains *why* files changed

Incorporate this into the **Rationale** column of the commit plan and the commit body.

## Splitting Changes Within a Single File

When one file contains two or more logically distinct changes that belong in separate commits (e.g. a bug fix + a new policy section), split them without interactive staging:

1. Temporarily strip the *later* section from the file (Python or StrReplace)
2. Stage and commit the *first* change
3. Restore the stripped section
4. Stage and commit the *second* change

```python
MARKER = "\n## Self-Update Policy\n"
text = path.read_text()
path.write_text(text[:text.find(MARKER)])  # strip later section
# → git add + commit "fix: ..."
path.write_text(text)                      # restore full content
# → git add + commit "feat: ..."
```

**Why**: `git log -p <file>` then shows each logical change in its own commit — easier to bisect, review, or revert independently.

**Pitfall**: Always verify the marker falls *before* the function/block closing lines. If the marker is the last statement in a function, the stripped file will be missing `return` and `}`, causing `expected declaration or statement at end of input` errors.

## Self-Update Policy

At the **end of each conversation**, review what was discovered and check whether any facts in this skill are new, corrected, or outdated (e.g. new grouping patterns, NCS-specific file conventions, commit message conventions).

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
