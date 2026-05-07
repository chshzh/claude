---
name: chsh-dev-task-plan
description: Plan, track, and execute multi-step NCS project tasks from a rolling task log file. Use when the user has a "prepare for release" or similar task-list file with mixed project + meta tasks across multiple sessions. Produces a structured todo list, executes in priority order, and updates the task file on completion.
---

# chsh-dev-task-plan — Multi-Step Task Planning and Execution

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
3. A skill name on its own line (e.g., `claude`, `chsh-dev-ncs-project`) signals a meta-task block for tooling/wiki improvements
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
Use `chsh-dev-git-commit` skill for consistent commit messages.
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
2. Commit and push (use `chsh-dev-git-commit`)
3. Watch CI: `gh run watch <run-id>` or `gh run list --limit 3`
4. If CI fails: read failure output, diagnose, fix, repeat from step 1
5. When CI passes: flash pre-built firmware and verify boot + expected shell output
6. Mark complete

See [wiki: github-actions-ncs-ci](../../wiki/concepts/github-actions-ncs-ci.md) for the full CI debugging loop patterns.

### "Improve after learning from X" pattern
1. Read X completely (XML, text file, or URL)
2. Extract the key technical insight
3. Apply it concretely: update code, docs, or wiki
4. Do NOT just summarize — always produce a concrete artifact

### "Give proper name" pattern
When renaming artifacts (e.g., `merged.hex` → `nordic-wifi-shell-sqspi-nrf54lm20dk-nrf7002ebii-ncs3.3.0.hex`):
- Update the source (build.yml collect step)
- Update the release `files:` field
- Update any README references to the old filename

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
| Build / flash / west commands | `chsh-dev-ncs-env` |
| Debug CI failures, pre-built firmware test | `chsh-dev-ncs-debug` (Mode G) |
| Git commits with Conventional Commits style | `chsh-dev-git-commit` |
| Generate or update specs | `chsh-dev-ncs-spec` |
| README template reference | `/Users/chsh/.claude/skills/chsh-dev-ncs-project/templates/README_TEMPLATE.md` |
| CI debugging loop patterns | wiki: `concepts/github-actions-ncs-ci.md` |
