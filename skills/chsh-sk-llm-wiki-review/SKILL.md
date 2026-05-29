---
name: chsh-sk-llm-wiki-review
description: >-
  Load when running the daily Hermes wiki audit, or when asked to lint,
  audit, or health-check a Karpathy-style LLM Wiki for schema compliance,
  broken links, orphans, stale content, source drift, or contradiction markers.
---

# chsh-sk-llm-wiki-review — Daily LLM Wiki Health Audit

Systematic review of a Karpathy-style LLM Wiki. Finds schema violations,
broken links, orphan pages, index gaps, stale content, and source drift.
Reports findings in the session (no file written) and asks the user how to fix each issue group.

> **Schema source**: Each wiki defines its own rules in `SCHEMA.md`. Read the
> schema first — frontmatter fields, tag taxonomy, page thresholds, and
> conventions vary per wiki.

> **Authoring rules**: Companion to `chsh-sk-llm-wiki` (which builds the wiki).
> This skill audits what that skill produces.

---

## Step 0 — Orient (mandatory)

Before any check, read the wiki's own conventions:

```bash
WIKI="${WIKI_PATH:-$HOME/.claude/wiki}"
read_file "$WIKI/SCHEMA.md"
read_file "$WIKI/index.md"
read_file "$WIKI/log.md" offset=<last 30 lines>
```

Capture from the schema:
- Frontmatter required fields (`title`, `created`, `updated`, `type`, `tags`, `sources`)
- Link convention (`[[wikilinks]]` vs `[text](path.md)`)
- Tag taxonomy (closed list)
- Page thresholds (lines, sources)

The audit must respect the wiki's own conventions, not impose external defaults.

---

## Step 1 — Inventory

Build a flat list of all wiki pages:

```bash
find "$WIKI" -type f -name "*.md" \
  -not -path "*/raw/*" \
  -not -path "*/_archive/*" \
  | sort
```

Track for each page: relative path, line count, frontmatter fields,
inbound link count, outbound link count.

---

## Step 2 — Frontmatter Validation

For every wiki page (`entities/`, `concepts/`, `comparisons/`, `queries/`):

| Check | Severity |
|-------|----------|
| Frontmatter present (between two `---` lines) | P0 |
| Required fields present (`title`, `created`, `updated`, `type`, `tags`, `sources`) | P0 |
| `type` is in allowed set from SCHEMA.md | P1 |
| Every `tags:` entry is in SCHEMA.md taxonomy | P1 |
| `created` and `updated` are valid `YYYY-MM-DD` | P2 |
| `updated` >= `created` | P2 |
| `sources:` references actually exist in `raw/` | P1 |

For raw sources (`raw/`):

| Check | Severity |
|-------|----------|
| `source_url` and `ingested` present | P2 |
| `sha256` present and matches body | P2 |

---

## Step 3 — Link Health

### Convention check
The wiki may use either `[[wikilinks]]` (Obsidian style) or `[text](path.md)`
(GitHub-compatible). Check which convention `SCHEMA.md` mandates and flag pages
that mix conventions.

### Broken links
For each link in every page, resolve the target:
- `[[link]]` → search for `<link>.md` anywhere in `entities/`, `concepts/`, `comparisons/`, `queries/`
- `[text](path.md)` → resolve relative to the page's directory and verify file exists

Flag any unresolved target. P0 if used in body; P1 if only in frontmatter
(`contradictions:`, `sources:`).

### Orphan pages
A page is an orphan if zero other pages link to it.

```python
import re, pathlib, os
wiki = pathlib.Path(os.environ.get("WIKI_PATH", os.path.expanduser("~/.claude/wiki")))
inbound = {}
for md in wiki.rglob("*.md"):
    if "/raw/" in str(md) or "/_archive/" in str(md):
        continue
    text = md.read_text()
    for m in re.finditer(r'\[\[([^\]]+)\]\]|\[[^\]]+\]\(([^)]+\.md)\)', text):
        target = m.group(1) or m.group(2)
        inbound.setdefault(target, set()).add(md.name)
# Pages whose stem isn't in `inbound` are orphans
```

Flag with severity P1. Exception: `index.md`, `SCHEMA.md`, `log.md`.

---

## Step 4 — Index Completeness

Every page in `entities/`, `concepts/`, `comparisons/`, `queries/` must
appear in `index.md`. Compare filesystem against index entries.

Inversely, every link in `index.md` must point to an existing page.

| Issue | Severity |
|-------|----------|
| Page exists but missing from index | P1 |
| Index lists a page that doesn't exist | P1 |
| Index page count in header doesn't match actual count | P2 |
| Index "Last updated" date older than newest page | P2 |

---

## Step 5 — Staleness

For each page, compare `updated` against:
- The newest source in `sources:` — if a source was re-ingested after the page's
  `updated` date, the page may need a refresh
- Time elapsed (>180 days without update) — flag as `P2 stale candidate`

For pages with `confidence: low` or `contested: true`, surface them
unconditionally — these need user review.

---

## Step 6 — Source Drift (sha256 mismatch)

For each `raw/` file with a `sha256:` frontmatter field:

```python
import hashlib, re, pathlib, os
for raw in pathlib.Path(os.environ.get("WIKI_PATH", os.path.expanduser("~/.claude/wiki"))).joinpath("raw").rglob("*.md"):
    text = raw.read_text()
    m = re.search(r'^sha256:\s*([0-9a-f]+)\s*$', text, re.M)
    if not m:
        continue
    expected = m.group(1)
    body_start = text.find("\n---\n", 4) + len("\n---\n")
    body = text[body_start:]
    actual = hashlib.sha256(body.encode()).hexdigest()
    if actual != expected:
        print(f"DRIFT: {raw}  expected={expected[:8]} actual={actual[:8]}")
```

Drift means the raw file was edited (shouldn't happen — `raw/` is immutable)
or the source URL has changed since ingest. Severity P2.

---

## Step 7 — Page Size

Pages over 200 lines are split candidates per the standard schema. Flag
with severity P2 and propose natural split points (top-level `##` sections).

---

## Step 8 — Tag Audit

Extract all `tags:` from all pages. Compare against SCHEMA.md taxonomy.

| Issue | Severity |
|-------|----------|
| Tag used on a page but missing from taxonomy | P1 |
| Tag in taxonomy but never used (>30 days after addition) | P2 (cleanup) |

---

## Step 9 — Log Health

| Check | Severity |
|-------|----------|
| `log.md` > 500 entries → rotate to `log-YYYY.md` | P2 |
| `log.md` last entry > 30 days ago — wiki may be neglected | P2 (info) |
| Entries reference pages that don't exist | P2 |

---

## Step 10 — Contradiction Surface

List every page with:
- `contested: true` in frontmatter
- non-empty `contradictions:` field
- `confidence: low`

These need user attention to resolve or downgrade.

---

## Step 11 — Report in Session

Do NOT write a report file. Present findings directly in the chat session:

```
## Wiki Review — YYYY-MM-DD

**Summary**
- Pages reviewed: N (excluding raw/archive)
- Issues: P0=N, P1=N, P2=N
- Orphans: N | Broken links: N | Index gaps: N | Drift: N

**P0 Issues (must fix)**
| Page | Issue | Proposed fix |

**P1 Issues (should fix)**
| Page | Issue | Proposed fix |

**P2 Issues (nice to fix)**
| Page | Issue | Proposed fix |
```

Then ask the user how to proceed for each issue group (P0, P1, P2 separately).

---

## Step 12 — Ask User How to Fix

After showing the report, ask one issue group at a time:

```
"Found N P0 issues. How would you like to handle them?"
Options:
  - "Apply all P0 fixes automatically"
  - "Show me each P0 fix for approval"
  - "Skip for now"
```

Repeat for P1 and P2 groups. Never batch P0+P1+P2 into a single question.

Do NOT ask about groups with zero issues.

Auto-applicable fixes (with approval):
- Add page to `index.md` under correct section
- Bump `updated:` date when applying programmatic fixes
- Append review entry to `log.md`

Never auto-apply without approval:
- Tag changes (semantic)
- Page splits
- Archiving
- Rewording / merging

### Fix Templates

**P0 — page missing required frontmatter:**
Insert at the top of the file:
```markdown
---
title: <Page Title>
created: YYYY-MM-DD
updated: YYYY-MM-DD
type: entity | concept | comparison
tags: []
sources: []
---
```
Fill in `title` from the first `#` heading, `created`/`updated` from file mtime, `type` from the directory (`entities/` → entity, `concepts/` → concept, `comparisons/` → comparison).

**P1 — page missing from `index.md`:**
Add under the correct section header in `index.md`:
```
- [[entities/thing-name]] — one-line description
```
Use the page's `title:` frontmatter field for the description. Then bump `index.md`'s own `updated` date and increment its page count.

**P1 — orphan page (zero inbound links):**
Search existing pages for any mention of the topic; if found, add a `[[wikilink]]` to the orphan from the most relevant page. If no natural home exists, add a bare link in `index.md` or a related page.

**P2 — page over 200 lines:**
Identify the largest `##` section. If it is standalone content, create a new page for it (`concepts/subtopic.md`) and replace the section with a 2-line summary + `[[subtopic]]` link.

---

## Brainstorm: What Else Can This Skill Do

Ideas for future expansion — not yet implemented:

### Cross-wiki analysis
If the user has multiple wikis (work, personal, research), produce a
cross-wiki overlap report — same entities or concepts covered in different
domains, candidates for one canonical page with cross-references.

### Source-coverage map
For each tag in the taxonomy, list how many pages and how many distinct
raw sources back it. Flag tags with thin coverage (< 2 sources) as
"under-supported".

### Citation graph
Build a graph where nodes are pages and edges are citations through
`sources:` frontmatter. Surface pages that cite many others but are cited
by none (knowledge consumers) vs. pages cited heavily (knowledge anchors).

### Update prompts
For pages older than 90 days, generate a one-line "did anything change?"
prompt that the user can run against fresh sources to detect updates.

---

## Related Skills

| Task | Skill |
|------|-------|
| Build, ingest, query the wiki | `chsh-sk-llm-wiki` |
| Author a new skill | `chsh-sk-skill-create` |
| Audit skills | `chsh-sk-skill-review` |
| Commit reviewed wiki fixes | `chsh-sk-git-commit` |

---

## Gotchas

- **False positives in code blocks**: Links and credential patterns inside triple-backtick blocks are illustrative examples, not real references or leaks. Flag them with a note but do not mark P0.
- **Intentional placeholder links**: `path.md`, `reference.md`, `examples.md` in skills' own illustrative SKILL.md examples are template placeholders, not dead links. Do not flag them.
- **`index.md` / `SCHEMA.md` / `log.md` are exempt from orphan check**: These three root files are expected to have zero inbound links from other pages.
- **LOG RESTORE markers need a full buffer**: `log.md` entries that reference LOG RESTORE only appear in Memfault when the log buffer was full at disconnect. A partial run may look like a missing entry but is correct.
- **sha256 drift on local edits**: If `raw/` files were edited locally (notes added, formatting fixed), sha256 will mismatch. Decide whether to re-hash or revert the edit — do not silently re-hash without user confirmation.

---

## Self-Update Policy

At the **end of each conversation**, review what was discovered and check
whether any facts in this skill are new, corrected, or outdated (e.g. new
schema rules, audit criteria, linting patterns, or changes to the wiki
directory layout).

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
