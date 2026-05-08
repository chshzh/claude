---
name: chsh-sk-skill-review
description: >-
  Daily skill health audit. Reviews all SKILL.md files for structural
  compliance, duplication, size, dead links, and cross-reference gaps.
  Produces a prioritized improvement report and applies approved fixes.
  Use when running the Hermes daily review job, or when asked to audit skills.
---

# chsh-sk-skill-review — Daily Skill Health Audit

Systematic review of all Agent Skills. Finds structural violations, duplication,
stale content, and missing cross-references. Produces a prioritized fix list.

> **Rule authoring**: This skill uses the rules defined in `chsh-sk-skill-create`
> as the quality standard. Read that skill before evaluating any SKILL.md.

---

## Step 0 — Discover All Skills

```bash
# Personal skills (primary target)
find ~/.claude/skills -name "SKILL.md" | sort

# Cursor built-in skills (read-only — for deduplication check only)
find ~/.cursor/skills-cursor -name "SKILL.md" | sort
```

Build a flat list of `(skill_name, path, line_count)` tuples.

---

## Step 1 — Structure Check (per skill)

For each personal skill, verify against `chsh-sk-skill-create` rules:

| Check | Pass condition | Severity |
|-------|---------------|----------|
| YAML frontmatter present | First 3 lines start with `---` | P0 |
| `name` field present | `name:` key exists | P0 |
| `name` ≤ 64 chars, lowercase + hyphens | Regex `^[a-z0-9-]{1,64}$` | P1 |
| `description` ≤ 1024 chars | Character count | P1 |
| Description in third person | Does NOT start with "I " or "You " | P1 |
| Description includes WHAT and WHEN | Contains "Use when" or similar | P1 |
| SKILL.md body ≤ 500 lines | `wc -l` | P2 |
| No Windows-style paths | No `\` path separators | P2 |
| No time-sensitive info | No phrases like "before August 202X" | P2 |
| File references one level deep | Linked files are direct children, not deeply nested | P2 |

---

## Step 2 — Size Check

Skills over 500 lines are candidates for splitting.

For each oversized skill:
1. Read the skill body
2. Identify natural split points: separate workflow modes, distinct use cases, long reference tables
3. Propose a split plan: which sections → which sub-skills
4. Check if the skill already uses progressive disclosure (links to sub-files) — if yes, the main SKILL.md may still be fine even if total lines are high

**Split threshold**: 500 lines in SKILL.md. Sub-skill files (e.g. `wifi/SKILL.md`) are excluded from this count.

---

## Step 3 — Deduplication Check

Identify skills that overlap in scope:

1. For each skill, extract its purpose from the description + first 20 lines
2. Compare against:
   - All other personal skills
   - Cursor built-in skills in `~/.cursor/skills-cursor/`
   - Active MCP servers (check `~/.cursor/mcp.json` or workspace MCP config)

Flag when:
- Two skills cover the same task with no clear division (e.g. two "git commit" skills)
- A skill duplicates capability already in an MCP tool (e.g. a skill for GitHub operations when `user-github` MCP is enabled)
- A skill covers functionality owned by another skill without referencing it

For each duplicate pair, suggest:
- Which skill should be the canonical owner
- What cross-reference to add to the non-owner
- Whether one can be deleted

---

## Step 4 — Dead Link Check

For each skill with file references (links in the form `[text](path.md)`):

```bash
# From the skill's directory, check that each linked file exists
grep -oP '\[.*?\]\(\K[^)]+' SKILL.md | while read -r f; do
  [ -f "$f" ] || echo "DEAD: $f"
done
```

Report dead links with the skill name and broken path.

---

## Step 5 — Cross-Reference Gap Check

When skill A mentions a topic that skill B owns, A should reference B.

Patterns to detect:
- A skill mentions "debug" or "UART" without referencing `chsh-sk-ncs-debug`
- A skill mentions "git commit" or "push" without referencing `chsh-sk-git`
- A skill mentions "release" or "tag" without referencing `chsh-sk-git-release`
- A skill mentions "PRD" or "requirements" without referencing `chsh-sk-ncs-prd`
- A skill mentions "specs" without referencing `chsh-sk-ncs-spec`
- A skill talks about "QA" or "test report" without referencing `chsh-sk-ncs-test`

For each gap, suggest a one-line addition to the Related Skills table.

---

## Step 6 — Freshness Check

Flag potentially stale content:

- Hard-coded NCS version strings (e.g. `v3.2.0`) — check if the workspace uses a newer version
- References to old board names (e.g. `nrf54l15dk` instead of `nrf54lm20dk`)
- Deprecated commands (e.g. `nrfjprog` instead of `nrfutil device`)
- `Self-Update Policy` section missing — this signals a skill that was never updated after creation

---

## Step 7 — Generate the Review Report

Produce a Markdown report at `~/.claude/skills/chsh-sk-skill-review/report-YYYY-MM-DD.md`:

```markdown
# Skill Review — YYYY-MM-DD

## Summary
- Skills reviewed: N
- Issues found: N (P0: N, P1: N, P2: N)
- Recommended actions: N

## P0 Issues (must fix)
| Skill | Issue | Fix |
|-------|-------|-----|
| ... | ... | ... |

## P1 Issues (should fix)
...

## P2 Issues (nice to fix)
...

## Split Candidates
| Skill | Lines | Proposed sub-skills |
|-------|-------|---------------------|
| ... | ... | ... |

## Deduplication Candidates
| Skill A | Skill B | Overlap | Recommendation |
|---------|---------|---------|----------------|
| ... | ... | ... | ... |

## Cross-Reference Gaps
| Skill | Missing reference | Add to section |
|-------|------------------|----------------|
| ... | ... | ... |

## Dead Links
| Skill | Broken path |
|-------|-------------|
| ... | ... |
```

---

## Step 8 — Apply Fixes (with approval)

Present the report summary. Use `AskQuestion` to ask:

```
Question: "Apply fixes from the review?"
Options:
  - "Apply P0 fixes automatically"
  - "Apply P0 + P1 fixes automatically"
  - "Show me each fix for approval"
  - "Report only, no changes"
```

For each approved fix:
1. Apply using StrReplace (prefer) or Write
2. Log what was changed in the report under `## Applied Fixes`
3. Mark the issue resolved

Do NOT apply split or delete operations without explicit per-item approval.

---

## Brainstorm: What Else Can This Skill Do

The following are ideas for future expansion — not yet implemented:

### Transcript Mining
Scan `~/.cursor/projects/*/agent-transcripts/*.jsonl` for patterns:
- Tasks the agent struggled with → candidate for a new skill
- Repeated manual steps the agent improvised → candidate for documenting in an existing skill
- Corrections the user made frequently → candidate for a rule update

### New Skill Proposals
Based on transcript analysis, propose new skills with:
- Title, description, trigger conditions
- Which existing skills or MCPs it would complement

### Skill Usage Frequency
Track which skills appear in recent transcripts — skills never referenced may be stale or poorly described.

### MCP Overlap Audit
Cross-check all enabled MCP tools with skill descriptions to find redundant skills that should be retired in favor of the MCP.

### Terminology Consistency Report
Extract key terms from all skills and flag inconsistencies (e.g. "VCOM1" vs "VCOM port 1", "prj.conf" vs "project config").

### Changelog Tracking
Check if skills with `Self-Update Policy` sections were last updated recently (via git blame). Flag skills that haven't been touched in >90 days but cover active workflows.

---

## Related Skills

| Task | Skill |
|------|-------|
| Author a new skill | `chsh-sk-skill-create` |
| Commit reviewed skill fixes | `chsh-sk-git` |
| Push to claude repo | `chsh-sk-git` (Step 5 — push) |
