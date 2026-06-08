# Skills Schema

## Domain
Personal Agent Skills for Claude Code. Each skill encodes a specialized workflow,
domain procedure, or quality standard that the agent loads on demand.

## Conventions
- Directory names: lowercase, hyphens, max 64 chars (e.g., `chsh-sk-ncs-3.2-debug`)
- `name:` frontmatter must exactly match the directory name
- Every skill lives in its own directory with at minimum `SKILL.md`
- Supporting files go inside the skill directory: `references/`, `scripts/`
- Use standard markdown links `[text](path.md)` in index.md — not `[[wikilinks]]`
- Every new skill must be added to `index.md`
- Every create/update/delete/review action must be appended to `log.md`

## Frontmatter

```yaml
---
name: skill-directory-name    # must match directory name exactly; ≤64 chars
description: >-               # routing trigger, ≤50 words, starts with "Use when" or "Load when"
  Use when / Load when ...
---
```

## Quality Thresholds

| Rule | Threshold | Severity |
|------|-----------|----------|
| SKILL.md length | ≤ 500 lines | P2 |
| Description word count | ≤ 50 words | P1 |
| Description routing form | Starts with "Use when" or "Load when" | P1 |
| Required sections | `## Gotchas` and `## Self-Update Policy` | P1 |
| Cross-skill references | Every `` `chsh-sk-*` `` backtick name must resolve to an existing directory | P1 |
| Absolute paths | No `/Users/...` hardcoded paths | P0 |
| `name:` ↔ directory | Frontmatter `name:` matches directory name | P0 |

## Required Sections

Every SKILL.md must contain:
1. **YAML frontmatter** with `name` and `description`
2. **`## Gotchas`** — at least one real observed failure (not a TODO placeholder)
3. **`## Self-Update Policy`** — canonical 3-step form (see `chsh-sk-skill-create`)

## Category Taxonomy

Used as section headers in `index.md`. Assign each skill to exactly one category.

| Category | Scope |
|----------|-------|
| NCS / Firmware Workflow | nRF Connect SDK lifecycle: PRD, spec, code, debug, V&V |
| NCS Test Cases | Specific hardware test scenarios |
| Git & Release | Commit, push, tag, release workflows |
| Memfault | OTA, symbol upload, cohort deploy, crash trace |
| Infrastructure | Router control, security scan, environment setup |
| Memory / AI Tools | AgentMemory, wiki, knowledge management |
| Skill Meta | Skill creation, review, registry maintenance |
| Text & Docs | Text editing, doc review |

## Link Conventions

| Context | Format |
|---------|--------|
| Cross-skill reference in SKILL.md body | `` `chsh-sk-skill-name` `` (backtick, no path) |
| index.md entries | `[skill-name](skill-dir/SKILL.md)` |
| Related Skills tables | `` `chsh-sk-skill-name` `` in second column |
| Related Skills tables | `` `chsh-sk-skill-name` `` in second column |
