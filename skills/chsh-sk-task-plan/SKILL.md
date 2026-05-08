---
name: chsh-sk-task-plan
description: Plan, track, and execute multi-step NCS project tasks from a rolling task log file. Use when the user has a "prepare for release" or similar task-list file with mixed project + meta tasks across multiple sessions. Produces a structured todo list, executes in priority order, and updates the task file on completion.
---

# chsh-sk-task-plan — Multi-Step Task Planning and Execution

Structured approach for breaking down, tracking, and executing multi-session NCS project tasks.
Designed for the "prepare for release" style: a single rolling file accumulating task blocks over many sessions.

---

## Step 0 — Read the Task File

The task log is typically at `<project>/prepare for release` (no extension, plain text).

```bash
cat "<project>/prepare for release"
```

**File structure**:
- Blocks separated by blank lines or horizontal rules
- Each block begins with either `Do the following job automatically with all my permissions` (project tasks) or a bare skill name (meta/tooling tasks)
- Numbered lists inside each block = individual tasks
- Unnumbered bullets = sub-tasks or constraints
- Tasks may reference specific files, skills, or output of previous tasks
- The file grows over sessions — older blocks at top, newest at bottom

**Key parsing rules**:
1. Read the entire file — context from older blocks informs newer ones
2. `Do the following job automatically with all my permissions` = full autonomy, no confirmation needed
3. A skill name on its own line (e.g., `claude`, `chsh-sk-ncs-project`) signals a meta-task block for tooling/wiki improvements
4. The last block is the current session's task set
5. References like `refer /path/to/file` or `learn from /path/to/file` mean: read those files before executing

---

## Step 1 — Classify and Prioritize Tasks

Split all tasks from the current block into two groups:

### Group A — Project tasks
Changes to the project repo (code, README, CI, firmware, tests).
- These can be done autonomously with the permissions granted
- Dependencies: code tasks before CI tasks, CI before release

### Group B — Meta tasks (Claude improvements)
Updates to wiki, skills, and commit logs.
- These should be done after project tasks are complete
- Typically: wiki page updates, skill rewrites, skill creation, push to claude repo

### Priority order within each group:
1. Tasks that block other tasks (e.g., read reference files before writing)
2. Code/config changes
3. Verification (build, flash, test)
4. Documentation
5. Commit and push

---

## Step 2 — Build the Todo List

Convert each task into a `manage_todo_list` entry with this format:
- Title: 3-7 words, action-oriented
- Status: `not-started`
- ID: sequential integer

**Example task decomposition** (from "prepare for release" session):
```
Input task: "Improve the project after learning from NRF7X-391.xml and VOIDD_3.3"
→ Todo 1: "Read NRF7X-391.xml and VOIDD_3.3"

Input task: "In Hardware Modification chapter... mention Board Configurator"
→ Todo 2: "Add Board Configurator note to README"

Input task: "README quality is not good... improve"
→ Todo 3: "Rewrite README to match template"
(requires reading README_TEMPLATE.md and webdash/README.md first → add as prerequisite todos)
```

**Rule**: If a task says "refer to X" or "learn from X", create an explicit read/analyze todo for X before the task that uses it.

---

## Step 3 — Execute in Order

For each todo:

1. Mark as **in-progress** in `manage_todo_list`
2. Gather required context (read files, check existing code)
3. Execute (edit, create, run commands)
4. Verify result (check output, grep for expected content)
5. Mark as **completed** immediately

**Key patterns**:

### Reference file reads
Always read in parallel when independent:
```python
# Good: read both at once
read_file(NRF7X-391.xml), read_file(README_TEMPLATE.md)
```

### README rewrites
- Read template + reference README + current README first
- Write the full replacement — do not edit section by section
- Confirm line count and first few lines after writing

### Build/CI changes
- Read full build.yml before editing
- Use `multi_replace_string_in_file` for multiple related edits
- Verify with `grep -n <pattern>` after edits

### Commit and push
Use `chsh-sk-git` skill for consistent commit messages.
Always push project repo before claude repo (project changes are the source of meta improvements).

---

## Step 4 — Handle Dependencies Between Blocks

Tasks in newer blocks often build on artifacts from older blocks:
- A "build.yml CI fix" from a previous block may be referenced in the current block's "release" task
- A "README rewrite" may reference a shield overlay that was created in a previous block's code task
- Always scan older blocks to understand what was already done

**Idempotency rule**: Check if a task is already done before re-executing.
```bash
# Check if patch already applied
git -C <repo> log --oneline | grep -q "<subject>"
# Check if file already has expected content
grep -q "<keyword>" <file>
```

---

## Step 5 — Update the Task File (Optional)

If the user provides new tasks mid-session, append them to the task file in the established format:

```
Do the following job automatically with all my permissions
<project>
- <new task 1>
- <new task 2>

claude
- <meta task 1>
```

Do NOT delete completed tasks — the file is a historical log.

---

## Common Patterns from Real Sessions

### "Loop until succeeded" pattern
When the user says "change, commit, check github action result, loop until succeeded":
1. Implement the fix
2. Commit and push (use `chsh-sk-git`)
3. Watch CI: `gh run watch <run-id>` or `gh run list --limit 3`
4. If CI fails: read failure output (`gh run view <run-id> --log-failed`), diagnose, fix, repeat from step 1
5. When CI passes: download the pre-built firmware and flash it (see **Pre-firmware flash test** below)
6. Verify boot + expected shell output on the correct VCOM
7. Mark complete

See [wiki: github-actions-ncs-ci](../../wiki/concepts/github-actions-ncs-ci.md) for the full CI debugging loop patterns.

### Pre-firmware flash and VCOM test
After CI passes and a firmware artifact is released, always flash and verify:

```bash
# 1. Identify correct VCOM port
nrfutil device list
# Output shows both VCOM ports with their physical /dev/tty paths
# VCOM1 = UART20 (app console) on nRF54LM20DK with sQSPI shield

# 2. Download released firmware
gh release download latest --repo <owner>/<repo> --pattern "*.hex" -D /tmp/fw/
ls /tmp/fw/   # confirm the descriptive filename exists

# 3. Flash
cd <ncs-workspace> && nrfutil sdk-manager toolchain launch --ncs-version=v3.3.0 -- \
  west flash --hex-file /tmp/fw/<name>.hex --recover --dev-id <SN>

# 4. Connect to correct VCOM and verify shell prompt
python3 nordicsemi_uart_monitor.py --port /dev/tty.usbmodem<VCOM1> --baud 115200
# expect: uart:~$
```

**Key**: The VCOM port for the app console depends on the shield overlay:
- nRF54LM20DK + nRF7002eb2_mspi (sQSPI): **UART20 = VCOM1** (no HWFC, `rtscts=False`)
- nRF7002DK: **UART0 = single VCOM** (hw-flow-control, `rtscts=True`)

When uncertain, run `nrfutil device list` and try the second port first.

### "Improve after learning from X" pattern
1. Read X completely (XML, text file, or URL)
2. Extract the key technical insight
3. Apply it concretely: update code, docs, or wiki
4. Do NOT just summarize — always produce a concrete artifact

### "Give proper name" / rename artifact pattern
When renaming artifacts (e.g., `merged.hex` → `nordic-wifi-shell-sqspi-nrf54lm20dk-nrf7002ebii-ncs3.3.0.hex`):
- Update the source (build.yml collect step `cp "$HEX" artifacts/<descriptive-name>.hex`)
- Update the release `files:` field
- Update any README references to the old filename
- Add a `gh release delete-asset latest merged.hex --yes 2>/dev/null || true` step to remove stale assets from the rolling release

### "Review and improve README, do not change structure" pattern
When the user says to improve the README without changing structure:
1. Read the full README
2. Check for: stale firmware filenames, wrong UART/VCOM references, missing Serial Monitor section, missing target users, outdated commands
3. Use `multi_replace_string_in_file` for targeted fixes — do NOT rewrite sections
4. Add missing sections (e.g., "Serial Monitor") in the same style as existing sections
5. Verify with `grep` after edits

### Release body: keep it short
Release bodies should only:
1. State what the build contains and the commit SHA
2. Link to the README Evaluator Quick Start for setup instructions

Do NOT repeat hardware modification steps, Board Configurator steps, or version requirements in the release body — these duplicate the README and drift out of sync.

### Skill/wiki improvement after a session
After completing project tasks, always:
1. Identify what went wrong or what was learned
2. Check if an existing wiki page or skill covers it
3. If yes: add a new section or update an existing one
4. If no: create a new page/skill
5. Update `wiki/index.md` if a new wiki page was added
6. Append to `wiki/log.md`

---

## Related Skills and Resources

| Need | Use |
|------|-----|
| Build / flash / west commands | `chsh-sk-ncs-env` |
| Debug CI failures, pre-built firmware test | `chsh-sk-ncs-debug` (Mode G) |
| Git commits with Conventional Commits style | `chsh-sk-git` (includes CI watch step) |
| Generate or update specs | `chsh-sk-ncs-spec` |
| README template reference | `/Users/chsh/.claude/skills/chsh-sk-ncs-project/templates/README_TEMPLATE.md` |
| CI debugging loop patterns | wiki: `concepts/github-actions-ncs-ci.md` |
