---
name: chsh-sk-skill-create
description: >-
  Load when creating, writing, or authoring a new Agent Skill, or when asked
  about SKILL.md structure, frontmatter, or skill best practices.
---
# Creating Agent Skills

This skill guides you through creating effective Agent Skills. Skills are markdown files that teach the agent how to perform specific tasks: reviewing PRs using team standards, generating commit messages in a preferred format, querying database schemas, or any specialized workflow.

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

If you need clarification, use the AskQuestion tool when available:

```
Example AskQuestion usage:
- "Where should this skill be stored?" with options like ["Personal (~/.claude/skills/)", "Project (.claude/skills/)"]
- "Should this skill include executable scripts?" with options like ["Yes", "No"]
```

If the AskQuestion tool is not available, ask these questions conversationally.

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

## Instructions
Clear, step-by-step guidance for the agent.

## Examples
Concrete examples of using this skill.

## Gotchas
- [Known failure — add one entry per real agent failure observed]
```

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
# Bad - will become outdated
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

### 4. Absolute Paths

- ✅ **Own files (text)**: use relative — `scripts/run.py`, `configs/wifi-sta.conf`
- ✅ **Own files (shell, multiple cmds)**: define `SKILL=~/.claude/skills/<skill-name>` once, then `$SKILL/path`
- ❌ **Never**: hardcode `/Users/<name>/...` — breaks for any other user
- ❌ **Avoid**: repeating `~/.claude/skills/<skill-name>/` on every line of a multi-command block

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

If you have access to the AskQuestion tool, use it for efficient structured gathering. Otherwise, ask conversationally.

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

Add the new skill to `~/.claude/skills/chsh-sk-skill-review/REGISTRY.md`:

```
| `<skill-name>` | <when to invoke, ≤10 words> | <primary output> |
```

If `REGISTRY.md` does not exist yet, create it first at `~/.claude/skills/chsh-sk-skill-review/REGISTRY.md`:

```markdown
# Skill Registry

| Skill | Invoke when | Output |
|-------|-------------|--------|
```

---

## Complete Example

Here's a complete example of a well-structured skill:

**Directory structure:**
```
code-review/
├── SKILL.md
├── STANDARDS.md
└── examples.md
```

**SKILL.md:**
```markdown
---
name: code-review
description: Review code for quality, security, and maintainability following team standards. Use when reviewing pull requests, examining code changes, or when the user asks for a code review.
---

# Code Review

## Quick Start

When reviewing code:

1. Check for correctness and potential bugs
2. Verify security best practices
3. Assess code readability and maintainability
4. Ensure tests are adequate

## Review Checklist

- [ ] Logic is correct and handles edge cases
- [ ] No security vulnerabilities (SQL injection, XSS, etc.)
- [ ] Code follows project style conventions
- [ ] Functions are appropriately sized and focused
- [ ] Error handling is comprehensive
- [ ] Tests cover the changes

## Providing Feedback

Format feedback as:
- 🔴 **Critical**: Must fix before merge
- 🟡 **Suggestion**: Consider improving
- 🟢 **Nice to have**: Optional enhancement

## Additional Resources

- For detailed coding standards, see [STANDARDS.md](STANDARDS.md)
- For example reviews, see [examples.md](examples.md)
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
- [ ] No LLM-generated boilerplate — every line encodes real domain expertise
- [ ] `name:` frontmatter matches directory name exactly
- [ ] Entry added to `~/.claude/skills/chsh-sk-skill-review/REGISTRY.md`

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
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
