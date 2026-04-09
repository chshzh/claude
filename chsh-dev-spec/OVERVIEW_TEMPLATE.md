# Engineering Specs Overview — <Project Name>

## Document Information

| Field | Value |
|-------|-------|
| Project | |
| Version | YYYY-MM-DD-HH-MM |
| PRD Version | YYYY-MM-DD-HH-MM |
| Author | |
| NCS Version | e.g. v3.2.4 |
| Target Board(s) | e.g. nRF7002DK |
| Status | Draft |

---

## Changelog

| Version | Summary of changes |
|---|---|
| YYYY-MM-DD-HH-MM | Initial overview |

---

## 1. Purpose

This document is the entry point for the engineering specs of `<project name>`.
It maps product requirements to spec files and captures top-level design decisions.

For the product requirements that drive this design, see `docs/product/PRD.md`.

---

## 2. Spec Index

| Spec file | Covers | PRD sections |
|-----------|--------|--------------|
| [architecture.md](architecture.md) | System overview, module map, Zbus channels, boot sequence, memory budget | All |
| [<module-a>-module.md](<module-a>-module.md) | <brief description> | FR-00x, FR-00y |
| [<module-b>-module.md](<module-b>-module.md) | <brief description> | FR-00z |

---

## 3. Architecture Summary

**Pattern**: SMF + Zbus modular  *(or: Multi-threaded)*

**Key design decisions:**

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Architecture pattern | SMF + Zbus | Decoupled modules; Zbus is the only inter-module channel |
| Configuration | prj.conf + per-module Kconfig | Each module owns its config symbols |
| Credentials | git-ignored overlay | Never in source control |
| <other decision> | <choice> | <reason> |

---

## 4. PRD-to-Spec Mapping

| PRD requirement | Spec file | Status |
|----------------|-----------|--------|
| FR-001 <title> | <module>.md | Specified / TBD |
| FR-002 <title> | <module>.md | Specified / TBD |
| NFR-001 <title> | architecture.md | Specified / TBD |

---

## 5. Module Dependency Map

```
<module-a>  ──Zbus──▶  <module-b>
<module-a>  ──Zbus──▶  <module-c>
<module-d>  ──Zbus──▶  <module-b>
```

> For the full Zbus channel table, see [architecture.md](architecture.md).

---

## 6. Open Issues

| # | Description | Owner | Target |
|---|-------------|-------|--------|
| 1 | <unresolved design question> | <name> | <date> |

*(Changelog is maintained at the top of this document.)*
