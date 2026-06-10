---
name: chsh-sk-skill-create
description: >-
  Load when creating, writing, or authoring a new Agent Skill, or when asked
  about SKILL.md structure, frontmatter, or skill best practices.
---
# Creating Agent Skills

This skill guides you through creating effective Agent Skills. Skills are markdown files that teach the agent how to perform specific tasks: reviewing PRs using team standards, generating commit messages in a preferred format, querying database schemas, or any specialized workflow.

## Vault Orientation (mandatory when adding to an existing vault)

Before creating a new skill, orient yourself:

```bash
VAULT=~/.claude/skills
read_file "$VAULT/SCHEMA.md"     # naming rules, quality thresholds, category taxonomy
read_file "$VAULT/index.md"      # what skills already exist — avoid duplicates
```

Check the category taxonomy in SCHEMA.md — you'll need it to place the skill in `index.md`.

---

## Before You Begin: Gather Requirements

Before creating a skill, gather essential information from the user about:

1. **Purpose and scope**: What specific task or workflow should this skill help with?
2. **Target location**: Should this be a personal skill (`~/.claude/skills/`) or project skill (`.claude/skills/`)?
3. **Trigger scenarios**: When should the agent automatically apply this skill?
4. **Key domain knowledge**: What specialized information does the agent need that it wouldn't already know?
5. **Output format preferences**: Are there specific templates, formats, or styles required?
6. **Existing patterns**: Are there existing examples or conventions to follow?

### Inferring from Context

If you have previous conversation context, infer the skill from what was discussed. You can create skills based on workflows, patterns, or domain knowledge that emerged in the conversation.

### Gathering Additional Information

Always use `AskQuestion` — never ask conversationally. Fire these in sequence, stopping when the answer is already clear from context:

```
AskQuestion:
  prompt: "Where should this skill be stored?"
  options:
    - "Personal (~/.claude/skills/) — available across all projects"
    - "Project (.claude/skills/) — shared with repo collaborators"
```

```
AskQuestion:
  prompt: "Should this skill include executable utility scripts?"
  options:
    - "Yes — include scripts/ with helper scripts"
    - "No — instructions only"
```

Skip any question whose answer is already unambiguous from the conversation.

---

## Skill File Structure

### Directory Layout

Skills are stored as directories containing a `SKILL.md` file:

```
skill-name/
├── SKILL.md              # Required - main instructions
├── reference.md          # Optional - detailed documentation
├── examples.md           # Optional - usage examples
└── scripts/              # Optional - utility scripts
    ├── validate.py
    └── helper.sh
```

### Storage Locations

| Type | Path | Scope |
|------|------|-------|
| Personal | `~/.claude/skills/skill-name/` | Available across all your projects |
| Project | `.claude/skills/skill-name/` | Shared with anyone using the repository |

### SKILL.md Structure

Every skill requires a `SKILL.md` file with YAML frontmatter and markdown body:

```markdown
---
name: your-skill-name
description: "Load when [user intent in their own words]. ≤50 words."
---

# Your Skill Name

## Decision Flow          ← add this for workflow skills (see below)
...

## Instructions
Clear, step-by-step guidance for the agent.

## Examples
Concrete examples of using this skill.

## Gotchas
- [Known failure — add one entry per real agent failure observed]
```

### When to Add a Decision Flow

Add a `## Decision Flow` section **whenever the skill has multiple modes or a sequential branching flow** — i.e., the agent's first job is to detect context and pick a path.

Omit it for single-path reference skills (a style guide, a lookup table, a formatter).

Place it **after the intro paragraph and before the first `---`** so it is the first thing the agent reads after orientation.

Use a plain ASCII tree in a code fence — not Mermaid (agents read raw text; Mermaid adds tokens with no rendering benefit):

```markdown
## Decision Flow

\`\`\`
User request
  │
  ▼
Step 0: Detect context
  ├─ Condition A ──────────────→ [A] Mode A        (A1–A5)
  ├─ Condition B ──────────────→ [B] Mode B        (B1–B3)
  └─ Condition C ──────────────→ [C] Mode C
                          │
              (A and B)
                          │
                          ▼
                 Handoff AskQuestion
\`\`\`
```

Rules:
- One branch per condition — keep it scannable.
- Label the handoff (`AskQuestion`) only if it exists for some modes but not others.
- Match branch labels exactly to the mode headings that follow (`## Mode A`, `## Mode B`, etc.).

### Required Metadata Fields

| Field | Requirements | Purpose |
|-------|--------------|---------|
| `name` | Max 64 chars, lowercase letters/numbers/hyphens only | Unique identifier for the skill |
| `description` | Max 1024 chars, non-empty | Helps agent decide when to apply the skill |

---

## Writing the Description

The description is a **routing trigger**, not documentation. It tells the agent *when to load the skill*, not what the skill does. This is the hardest line to write and the most important one to get right.

**Rule**: Start with **"Use when"** or **"Load when"** and describe the user's intent in their own words. Target 50 words or fewer. Do not summarize the workflow.

- Use **"Use when"** for workflow skills (always-active, action-oriented triggers).
- Use **"Load when"** for reference/knowledge skills (passive lookup, loaded on demand).

| ✅ Good | ❌ Bad |
|--------|--------|
| "Load when the user mentions PDF, form extraction, or document merging." | "This skill processes PDF files and generates reports." |
| "Load when asked to write, review, or fix a git commit message." | "Helps with git. Use when working with commits." |

**Negative examples matter as much as positive ones.** For each valid trigger, name one adjacent case that should NOT load this skill — these become the initial Gotchas entry for routing.

**The silent failure**: a description that describes what the skill does (not when to load it) fires on wrong queries and silently degrades every other skill in context. You won't see the error; you'll just see worse behavior across the board.

---

## Core Authoring Principles

### 1. Concise is Key

The context window is shared with conversation history, other skills, and requests. Every token competes for space.

**Default assumption**: The agent is already very smart. Only add context it doesn't already have.

Apply the **Pascal test** to every sentence: *"Would the agent get this wrong without this instruction?"* If no, delete it. If you're generating the skill in one pass, it is almost certainly too long — a short skill is hard to write, and that difficulty is the actual work.

**Gotcha — LLM-generated skills**: Research shows self-generated skills provide no benefit on average. The model cannot reliably author the procedural knowledge it benefits from consuming. Every line must encode *your* expertise: your gotchas, your taste, your real edge cases. Generic best-practice instructions the model already knows waste context and degrade other skills.

**Good (concise)**:
```markdown
## Extract PDF text

Use pdfplumber for text extraction:

\`\`\`python
import pdfplumber

with pdfplumber.open("file.pdf") as pdf:
    text = pdf.pages[0].extract_text()
\`\`\`
```

**Bad (verbose)**:
```markdown
## Extract PDF text

PDF (Portable Document Format) files are a common file format that contains
text, images, and other content. To extract text from a PDF, you'll need to
use a library. There are many libraries available for PDF processing, but we
recommend pdfplumber because it's easy to use and handles most cases well...
```

### 2. Keep SKILL.md Under 500 Lines

For optimal performance, the main SKILL.md file should be concise. Use progressive disclosure for detailed content.

### 3. Progressive Disclosure

Put essential information in SKILL.md; detailed reference material in separate files that the agent reads only when needed.

```markdown
# PDF Processing

## Quick start
[Essential instructions here]

## Additional resources
- For complete API details, see [reference.md](reference.md)
- For usage examples, see [examples.md](examples.md)
```

**Keep references one level deep** - link directly from SKILL.md to reference files. Deeply nested references may result in partial reads.

### 4. Set Appropriate Degrees of Freedom

Match specificity to the task's fragility:

| Freedom Level | When to Use | Example |
|---------------|-------------|---------|
| **High** (text instructions) | Multiple valid approaches, context-dependent | Code review guidelines |
| **Medium** (pseudocode/templates) | Preferred pattern with acceptable variation | Report generation |
| **Low** (specific scripts) | Fragile operations, consistency critical | Database migrations |

---

## Utility Scripts

Pre-made scripts offer advantages over generated code:
- More reliable than generated code
- Save tokens (no code in context)
- Save time (no code generation)
- Ensure consistency across uses

```markdown
## Utility scripts

**analyze_form.py**: Extract all form fields from PDF
\`\`\`bash
python scripts/analyze_form.py input.pdf > fields.json
\`\`\`

**validate.py**: Check for errors
\`\`\`bash
python scripts/validate.py fields.json
# Returns: "OK" or lists conflicts
\`\`\`
```

Make clear whether the agent should **execute** the script (most common) or **read** it as reference.

---

## Anti-Patterns to Avoid

### 1. Windows-Style Paths
- ✅ Use: `scripts/helper.py`
- ❌ Avoid: `scripts\helper.py`

### 2. Too Many Options
```markdown
# Bad - confusing
"You can use pypdf, or pdfplumber, or PyMuPDF, or..."

# Good - provide a default with escape hatch
"Use pdfplumber for text extraction.
For scanned PDFs requiring OCR, use pdf2image with pytesseract instead."
```

### 3. Time-Sensitive Information
```markdown
# Bad - will become outdated (conditional instruction)
"If you're doing this before August 2025, use the old API."

# Good - use an "old patterns" section
## Current method
Use the v2 API endpoint.

## Old patterns (deprecated)
<details>
<summary>Legacy v1 API</summary>
...
</details>
```

**Exempt**: historical labels in data tables describing *when something existed*
(e.g. `period: Oct 2025 – Mar 2026` in an import-sources table) are fine — they
describe facts, not conditional instructions that expire.

### 4. Absolute Paths

- ✅ **Own files (text)**: use relative — `scripts/run.py`, `configs/wifi-sta.conf`
- ✅ **Own files (shell, multiple cmds)**: define `SKILL=~/.claude/skills/<skill-name>` once, then `$SKILL/path`
- ❌ **Never**: hardcode `/Users/<name>/...` — breaks for any other user
- ❌ **Avoid**: repeating `~/.claude/skills/<skill-name>/` on every line of a multi-command block
- ❌ **Private IPs in Quick run examples**: use `<server-ip>` placeholder instead of
  a real `192.168.x.x` address. Document the actual address in a Gotcha bullet so it
  is available as reference without appearing in an executable example.

```bash
# Good — SKILL variable for multi-command blocks
SKILL=~/.claude/skills/my-skill
cp $SKILL/templates/* src/
cp $SKILL/overlay.conf .

# Bad — repeated absolute path
cp ~/.claude/skills/my-skill/templates/* src/
cp ~/.claude/skills/my-skill/overlay.conf .
```

### 5. Frontmatter `name` Must Match Directory Name

The SKILL.md frontmatter `name:` field and the skill's directory name **must be identical**. Hermes discovers skills by scanning directories and loading `SKILL.md` — if the directory name differs from `name:`, the skill may not surface correctly.

**Pitfall when renaming via file manager or WebUI:**
Renaming a skill directory through the WebUI workspace or a file manager only changes the **directory name**. The `name:` field inside `SKILL.md` frontmatter stays as the old value. After renaming via WebUI, you MUST also edit SKILL.md to update the `name:` field:

```yaml
# directory: chsh-sk-new-name/
# SKILL.md frontmatter:
---
name: chsh-sk-new-name     # ← Must match directory name
---
```

To rename safely, use `skill_manage` action=patch to update the `name:` field after a directory rename.

### 5. Inconsistent Terminology
Choose one term and use it throughout:
- ✅ Always "API endpoint" (not mixing "URL", "route", "path")
- ✅ Always "field" (not mixing "box", "element", "control")

### 5. Vague Skill Names
- ✅ Good: `processing-pdfs`, `analyzing-spreadsheets`
- ❌ Avoid: `helper`, `utils`, `tools`

---

## Skill Creation Workflow

When helping a user create a skill, follow this process:

### Phase 0: Write Evals First

Before writing a single line of SKILL.md:
1. Write 2–3 **hero queries**: real user phrases that should load this skill
2. Write 2 **negative examples**: adjacent phrases that should NOT load it
3. Identify at least one **known failure** that motivated this skill's creation

These seed the initial Gotchas section and serve as the routing test baseline. If you can't name a known failure or a negative example, the skill may not be needed yet.

### Phase 1: Discovery

Gather information about:
1. The skill's purpose and primary use case
2. Storage location (personal vs project)
3. Trigger scenarios
4. Any specific requirements or constraints
5. Existing examples or patterns to follow

Always use `AskQuestion` for structured gathering — never ask conversationally. Skip questions whose answers are already clear from context.

### Phase 2: Design

1. Draft the skill name (lowercase, hyphens, max 64 chars)
2. Write a specific, third-person description
3. Outline the main sections needed
4. Identify if supporting files or scripts are needed

### Phase 3: Implementation

1. Create the directory structure
2. Write the SKILL.md file with frontmatter
3. Create any supporting reference files
4. Create any utility scripts if needed

### Phase 4: Verification

1. Verify the SKILL.md is under 500 lines
2. Check that the description is specific and includes trigger terms
3. Ensure consistent terminology throughout
4. Verify all file references are one level deep

### Phase 5: Register the Skill

Confirm `name:` in the SKILL.md frontmatter **exactly matches** the directory name.

**A. Add to `index.md`** (`~/.claude/skills/index.md`):

Find the correct category section (see SCHEMA.md taxonomy). Add an entry:

```markdown
- [chsh-sk-new-skill](chsh-sk-new-skill/SKILL.md) — one-line trigger summary
```

Update the "Total skills" count and "Last updated" date in the index header.

**B. Append to `log.md`** (`~/.claude/skills/log.md`):

```markdown
| YYYY-MM-DD | chsh-sk-new-skill | Created: <one-line description> |
```

---

## Complete Example

See [`references/complete-example.md`](references/complete-example.md) for a full worked example with directory structure and annotated SKILL.md.
> **Quick tip**: Description must start with "Use when"/"Load when"; file must end with `## Self-Update Policy`.

### Phase 6: Handoff

After registering the skill in `index.md` and `log.md`, call `AskQuestion`:

```
AskQuestion:
  prompt: "Skill created. What would you like to do next?"
  options:
    - "Test it now — trigger the new skill with a sample query"
    - "Run skill audit — check all skills for issues (chsh-sk-skill-maintain)"
    - "Done — nothing more needed"
```

---

## Summary Checklist

Before finalizing a skill, verify:

### Core Quality
- [ ] Description starts with "Load when..." (routing trigger, not documentation)
- [ ] Description is ≤50 words
- [ ] SKILL.md body is under 500 lines
- [ ] Consistent terminology throughout
- [ ] Pascal test applied: every sentence would cause failure if removed
- [ ] Skill has a Gotchas section (even if initially sparse)
- [ ] Skill has a Self-Update Policy section following the canonical 3-step template (see Canonical Templates below)
- [ ] No LLM-generated boilerplate — every line encodes real domain expertise
- [ ] `name:` frontmatter matches directory name exactly
- [ ] No real IP addresses in Quick run examples — use `<server-ip>` placeholder
- [ ] Entry added to `~/.claude/skills/index.md` under correct category
- [ ] Row appended to `~/.claude/skills/log.md`

### Structure
- [ ] File references are one level deep
- [ ] Progressive disclosure used for heavy reference content
- [ ] Workflows have clear steps
- [ ] No time-sensitive information

### If Including Scripts
- [ ] Scripts solve problems rather than punt
- [ ] Required packages are documented
- [ ] Error handling is explicit and helpful
- [ ] No Windows-style paths

---

## Canonical Templates

### Self-Update Policy

Every skill must end with this section. Customize the bracketed description for the skill's domain; keep the 3-step process and the "Do not modify" rule verbatim — they must not be shortened, removed, or reworded.

```markdown
## Self-Update Policy

At the **end of each conversation**, review what was discovered and check
whether any [domain-specific facts, patterns, or rules] are new, corrected, or outdated.

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Call `AskQuestion`:
   ```
   AskQuestion:
     prompt: "Apply these self-update changes to this skill?"
     options:
       - "Yes — apply all"
       - "Yes — apply selected (describe below)"
       - "No — skip for now"
   ```
3. Apply approved updates immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
```

---

## Additional Resources

- [principles.md](principles.md) — deeper rationale for the token tax, Pascal test, and routing-vs-docs distinction

---

## Gotchas

- **Placeholder Gotchas**: Authors put `[Known failure — add one entry per real agent failure observed]` and never update it. The Gotchas section only has value when it records actual observed failures.
- **Description drift**: Descriptions often describe *what the skill does* rather than *when to load it*. Check the first sentence: if it could be a README intro, rewrite it as a routing trigger.
- **Silent line-count creep**: Adding one subsection at a time often pushes SKILL.md past 500 lines without notice. Run `wc -l SKILL.md` as the last step before saving.
- **"Use when" vs "Load when" confusion**: Both are valid. Workflow skills (always action-oriented) use "Use when"; passive reference skills use "Load when". Don't force "Load when" on action skills.
- **Reference depth**: Progressive disclosure links must be one level deep (`references/file.md`). Linking to nested subdirs (e.g. `references/sub/file.md`) risks partial reads.

---

## Self-Update Policy

At the **end of each conversation**, review what was discovered and check
whether any facts in this skill are new, corrected, or outdated (e.g. new
anti-patterns observed, frontmatter schema changes, routing rule refinements,
or new skill authoring best practices).

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Call `AskQuestion`:
   ```
   AskQuestion:
     prompt: "Apply these self-update changes to chsh-sk-skill-create?"
     options:
       - "Yes — apply all"
       - "Yes — apply selected (describe below)"
       - "No — skip for now"
   ```
3. Apply approved updates immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
