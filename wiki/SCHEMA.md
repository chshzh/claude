# Wiki Schema

## Domain

Engineering decisions, conventions, and constraints for Nordic NCS-based IoT
application projects — versioning, build system, toolchain, CI/CD, and platform
integrations (Memfault, Zephyr, west).

## Conventions

- File names: lowercase, hyphens, no spaces (`ncs-app-versioning.md`)
- Every wiki page starts with YAML frontmatter (see below)
- Use `[[wikilinks]]` to link between pages (minimum 2 outbound links per page)
- When updating a page, always bump the `updated` date
- Every new page must be added to `index.md` under the correct section
- Every action must be appended to `log.md`
- **Provenance markers:** On pages that synthesize 3+ sources, append
  `^[raw/articles/source-file.md]` at the end of paragraphs whose claims come
  from a specific source.

## Frontmatter

```yaml
---
title: Page Title
created: YYYY-MM-DD
updated: YYYY-MM-DD
type: entity | concept | comparison | query
tags: [from taxonomy below]
sources: [raw/articles/source-name.md]
confidence: high | medium | low
contested: true                        # set when page has unresolved contradictions
contradictions: [other-page-slug]
---
```

## Tag Taxonomy

### Versioning
- `versioning` — version number schemes and conventions
- `semver` — Semantic Versioning 2.0 specific
- `ncs-version` — NCS SDK version coupling

### Platform
- `ncs` — Nordic nRF Connect SDK
- `zephyr` — Zephyr RTOS
- `west` — west build tool / manifest
- `mcuboot` — bootloader / OTA
- `memfault` — Memfault cloud integration

### Build & CI
- `build-system` — CMake, Kconfig, ninja
- `ci` — GitHub Actions, CI/CD pipelines
- `git` — git conventions, tagging, branching

### Hardware
- `nrf7002` — nRF7002 Wi-Fi chip
- `nrf5340` — nRF5340 dual-core SoC
- `nrf54lm20` — nRF54LM20 single-core SoC

### Meta
- `convention` — agreed conventions for this project family
- `decision` — a decided design choice with rationale
- `constraint` — external constraint (e.g. Memfault character rules)
- `comparison` — side-by-side analysis of options

## Page Thresholds

- **Create a page** when a topic appears in 2+ conversations OR is a decided convention
- **Add to existing page** when a conversation refines or extends an existing decision
- **DON'T create a page** for passing mentions or hypotheticals that weren't acted on
- **Split a page** when it exceeds ~200 lines
- **Archive a page** when the decision is reversed or fully superseded

## Update Policy

When new information conflicts with existing content:
1. Check dates — newer decisions supersede older ones
2. If genuinely contradictory, note both positions with dates
3. Mark `contradictions: [page-slug]` in frontmatter
4. Flag for review in lint report
