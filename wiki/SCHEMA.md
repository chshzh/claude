# Wiki Schema

## Domain
Homelab, networking, WireGuard, OpenWrt, and self-hosted infrastructure; Nordic NCS/Zephyr embedded development.

## Conventions
- File names: lowercase, hyphens, no spaces (e.g., `ncs-app-versioning.md`, `wireguard-guide.md`)
- Every wiki page starts with YAML frontmatter (see below)
- Use standard markdown links `[page-name](relative-path.md)` for cross-page links (minimum 2 outbound links per page). This works on both GitHub and local renderers. From `concepts/`, relative links are `[target](target.md)`; from `index.md` use `[target](concepts/target.md)`.
- When updating a page, always bump the `updated` date
- Every new page must be added to `index.md` under the correct section
- Every action must be appended to `log.md`
- **Provenance markers:** On pages that synthesize 3+ sources, append `^[raw/articles/source-file.md]`
  at the end of paragraphs whose claims come from a specific source.

## Frontmatter
```yaml
---
title: Page Title
created: YYYY-MM-DD
updated: YYYY-MM-DD
type: entity | concept | comparison | guide | query | summary
tags: [from taxonomy below]
sources: [raw/articles/source-name.md]
# Optional quality signals:
confidence: high | medium | low
contested: true
contradictions: [other-page-slug]
---
```

### raw/ Frontmatter
```yaml
---
source_url: https://example.com/article
ingested: YYYY-MM-DD
sha256: <hex digest of the body>
---
```

## Tag Taxonomy
See log.md for the evolving tag list.
