---
name: chsh-sk-skill-review
description: >-
  Load when running the Hermes daily skill audit, or when asked to review,
  audit, or health-check existing SKILL.md files for quality, duplication,
  dead links, or structural compliance.
---

# chsh-sk-skill-review — Daily Skill Health Audit

Systematic review of all Agent Skills. Finds structural violations, duplication,
stale content, missing cross-references, and security issues (credential leaks,
sensitive config files not in .gitignore). Produces a prioritized fix list.

> **Rule authoring**: This skill uses the rules defined in `chsh-sk-skill-create`
> as the quality standard. Read that skill and its
> [`principles.md`](../chsh-sk-skill-create/principles.md)
> before evaluating any SKILL.md.

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

## Step 0b — Registry Integrity

Check that `~/.claude/skills/chsh-sk-skill-review/REGISTRY.md` exists and is coherent. If missing, flag **P1** — create it via `chsh-sk-skill-create` Phase 5 and continue with per-skill checks.

```bash
REGISTRY=~/.claude/skills/chsh-sk-skill-review/REGISTRY.md

# Every name in the registry must resolve on disk with a matching name: field
grep -oP '`\K[a-z0-9-]+(?=`)' "$REGISTRY" | while read name; do
  dir=~/.claude/skills/$name
  [ -d "$dir" ]         || { echo "MISSING DIR: $name";     continue; }
  [ -f "$dir/SKILL.md" ] || { echo "MISSING SKILL.md: $name"; continue; }
  declared=$(grep "^name:" "$dir/SKILL.md" | sed 's/^name:[[:space:]]*//')
  [ "$declared" = "$name" ] || echo "NAME MISMATCH: registry=$name  SKILL.md=$declared"
done

# Every skill on disk must have a registry entry
find ~/.claude/skills -mindepth 2 -maxdepth 2 -name "SKILL.md" | while read f; do
  name=$(basename "$(dirname "$f")")
  grep -q "\`$name\`" "$REGISTRY" || echo "NOT REGISTERED: $name"
done
```

Flag each finding as **P1**. Apply registry fixes in Step 9.

---

## Step 1 — Structure Check (per skill)

For each personal skill, verify against `chsh-sk-skill-create` rules:

| Check | Pass condition | Severity |
|-------|---------------|----------|
| YAML frontmatter present | First 3 lines start with `---` | P0 |
| `name` field present | `name:` key exists | P0 |
| `name` ≤ 64 chars, lowercase + hyphens | Regex `^[a-z0-9-]{1,64}$` | P1 |
| `description` ≤ 1024 chars | Character count | P1 |
| Description uses routing trigger | Starts with "Load when" or equivalent routing phrase; does NOT describe what the skill does | P1 |
| Description ≤ 50 words | Word count | P1 |
| Skill has Gotchas section | `## Gotchas` heading present in body | P1 |
| No duplicate section headings | Each `##` heading appears exactly once | P1 |
| `Self-Update Policy` follows canonical 3-step form | Contains "end of each conversation", 3 numbered steps ending in "Apply approved updates", and "Do not modify this skill mid-conversation" | P1 |
| SKILL.md body ≤ 500 lines | `wc -l` | P2 |
| No Windows-style paths | No `\` path separators | P2 |
| No time-sensitive info | No phrases like "before August 202X" | P2 |
| File references one level deep | Linked files are direct children, not deeply nested | P2 |
| Pascal test (manual) | No section consisting of generic best practices the model already knows — flag and propose cutting | P2 |
| No hardcoded `/Users/` paths | `grep -r "/Users/" SKILL.md` returns empty | P0 |
| No repeated `~/.claude/skills/<name>/` in multi-command blocks | Multi-command blocks use a `SKILL=` variable; single-command lines may keep the full `~` path | P1 |
| Own-file text refs are relative | Markdown text mentioning a file in the same skill uses relative path, not absolute | P1 |

Detect duplicate headings:

```bash
# Flag any ## heading that appears more than once in the same SKILL.md
find ~/.claude/skills -name "SKILL.md" | while read f; do
  dupes=$(grep -E '^## ' "$f" | sort | uniq -d)
  [ -n "$dupes" ] && echo "DUPLICATE HEADINGS in $f: $dupes"
done
```

For each duplicate found: keep the richer instance, remove the stub. A stub is a section with only a TODO comment or generic boilerplate.

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

Check every relative markdown link in every SKILL.md resolves to an existing file.

### Quick Scan (per skill, bash)

```bash
# From the skill's directory, check that each linked file exists
grep -oP '\[.*?\]\(\K[^)]+' SKILL.md | while read -r f; do
  [ -f "$f" ] || echo "DEAD: $f"
done
```

### Comprehensive Scan (all skills, Python)

Run the reusable script at `scripts/check-deadlinks.py` from the skill directory:

```bash
python3 scripts/check-deadlinks.py
```

The script extracts every relative link, resolves it against the SKILL.md's own directory, and reports broken targets. It also marks links inside code blocks as likely-intentional examples.

### Pitfalls

- **Doubled-path links**: When a SKILL.md lives in a subdirectory (e.g. `protocols/SKILL.md`), a link like `(protocols/webserver/SKILL.md)` resolves to `protocols/protocols/webserver/SKILL.md` — the author mistakenly wrote the path from the repo root instead of relative to the file's own directory. The correct link is `(webserver/SKILL.md)`.
- **Code-block false positives**: Links inside triple-backtick code blocks or `inline code` are illustrative examples, not real references. The scan should flag them but note they are likely intentional.
- **Cross-repo wiki links**: Links pointing to `../../wiki/...` may reference wiki pages that were never created or were deleted. These should be treated as P0 since the wiki and skills repos may drift independently.
- **Anchor-only links** (`#section`) are valid and should not be flagged.

### Verification Pass

After fixing dead links, re-run the comprehensive scan and confirm zero broken links remain before declaring the fix complete.

---

## Step 5 — Cross-Reference Gap Check

When skill A mentions a topic that skill B owns, A should reference B.

Patterns to detect:
- A skill mentions "debug" or "UART" without referencing `chsh-sk-ncs-3.2-debug`
- A skill mentions "git commit" or "push" without referencing `chsh-sk-git-commit`
- A skill mentions "release" or "tag" without referencing `chsh-sk-git-release`
- A skill mentions "PRD" or "requirements" without referencing `chsh-sk-ncs-1-prd`
- A skill mentions "specs" without referencing `chsh-sk-ncs-2-spec`
- A skill talks about "QA" or "test report" without referencing `chsh-sk-ncs-4.1-verification`

For each gap, suggest a one-line addition to the Related Skills table.

Also verify every backtick-referenced skill name in every SKILL.md resolves on disk:

```bash
# Extract all `chsh-sk-*` names from every SKILL.md and check they exist
find ~/.claude/skills -name "SKILL.md" | while read f; do
  grep -oP '`\Kchsh-sk-[a-z0-9.-]+(?=`)' "$f" | while read ref; do
    [ -d ~/.claude/skills/$ref ] || echo "DEAD REF in $f: $ref"
  done
done
```

Flag unresolvable references as **P1**.

---

## Step 6 — Freshness Check

Flag potentially stale content:

- Hard-coded NCS version strings (e.g. `v3.2.0`) — check if the workspace uses a newer version
- References to old board names (e.g. `nrf54l15dk` instead of `nrf54lm20dk`)
- Deprecated commands (e.g. `nrfjprog` instead of `nrfutil device`)
- `Self-Update Policy` section missing — this signals a skill that was never updated after creation

---

## Step 7 — Security Scan

Run `chsh-sk-security-scan` across all skill files:

```bash
SKILL=~/.claude/skills/chsh-sk-security-scan
python3 $SKILL/scripts/scan.py dir ~/.claude/skills/
```

| Exit code | Verdict | Severity in report |
|-----------|---------|-------------------|
| 0 | `CLEAN` | — |
| 1 | `BLOCK` | **P0** — must fix before next commit |
| 2 | `WARN` | **P1** — review required; may be intentional lab docs |

See `chsh-sk-security-scan` for the full pattern list and known false positives.

### Config file audit

Skill directories may contain `config.json` (first-run user setup: API keys, server addresses). These are high-risk for accidental commits.

```bash
find ~/.claude/skills -name "config.json" | while read f; do
  echo "=== $f ==="; cat "$f"
done
```

For each `config.json` found: verify it is listed in `~/.claude/.gitignore`. If not, flag **P0**.

### Gitignore coverage check

Verify `~/.claude/.gitignore` contains at minimum:
- `skills/**/config.json` — all first-run user setup files at any nesting level
- `skills/chsh-sk-skill-review/report-*.md` — audit reports (may expose skill internals)
- Any skill-specific known-sensitive files not already matched by `skills/**/config.json`

---

## Step 8 — Generate the Review Report

Produce a Markdown report at `report-YYYY-MM-DD.md` in this skill's directory:

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

## Security Issues
| Skill / File | Pattern matched | Action required |
|-------------|----------------|----------------|
| ... | ... | ... |
```

---

## Step 9 — Apply Fixes (with approval)

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

### Fix Templates

**P1-A — description doesn't start with routing trigger:**
Rewrite as two sentences: trigger first, then what the skill does.
```
OLD: "Processes PDF files and extracts form fields. Use when the user mentions PDF."
NEW: "Use when the user mentions PDF, form extraction, or document merging. Extracts form fields from PDF files using pdfplumber."
```
Rule: move the "Use when"/"Load when" clause to position 1. Preserve all original content; do not shorten or summarize.

**P1-B — missing `## Gotchas` section:**
Insert before `## Self-Update Policy` (or before `## Related Skills` if no Self-Update exists):
```markdown
## Gotchas
- TODO: add one entry per real observed failure or routing false-positive
```

**P1-C — `Self-Update Policy` section is missing, abbreviated, or non-canonical:**
Replace with the canonical template from `chsh-sk-skill-create` § Canonical Templates. Keep the domain-specific first paragraph; the 3 numbered steps and "Do not modify" line must be verbatim:
```markdown
## Self-Update Policy

At the **end of each conversation**, review what was discovered and check
whether any [domain-specific description].

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
```

**P2 — SKILL.md over 500 lines:**
1. Identify the largest self-contained section (workflow mode, reference table, long example).
2. Create `references/<topic>.md` in the same skill directory.
3. Move the section content there verbatim.
4. Replace the section in SKILL.md with a 3-5 line stub:
   ```markdown
   ## Mode X — <Name>
   See [`references/<topic>.md`](references/<topic>.md) for <summary of what moved>.
   > **Quick tip**: <one-liner most commonly needed>
   ```
5. Verify `wc -l SKILL.md` is now under 500.

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
| Commit reviewed skill fixes | `chsh-sk-git-commit` |
| Push to claude repo | `chsh-sk-git-commit` (Step 5 — push) |

---

## Gotchas

- **False positives in security scan**: Credential patterns inside illustrative code blocks (e.g. `password = "your-password-here"`) are documentation, not leaks. Always read the surrounding context before flagging.
- **Intentional placeholder links**: `path.md`, `reference.md`, `examples.md` in `chsh-sk-skill-create` and `chsh-sk-llm-wiki-review` are illustrative template placeholders, not dead links.
- **P1-A rewrites must preserve content**: When fixing a description to start with the routing trigger, keep all original information — do not shorten, summarize, or drop details.
- **P2 requires extraction, not deletion**: A 600-line skill is not fixed by cutting content; the content must move to `references/` so agents can still load it when needed.
- **Private IPs in hermes-setup are intentional**: `192.168.75.30` is a lab VM address documented in `chsh-sk-hermes-setup`. Flag it in the report but do not auto-remove.

---

## Self-Update Policy

At the **end of each conversation**, review what was discovered and check
whether any check criteria, severity levels, fix templates, or false-positive
patterns need updating based on what was found in this audit.

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
