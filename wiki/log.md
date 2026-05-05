# Wiki Log

> Chronological record of all wiki actions. Append-only.
> Format: `## [YYYY-MM-DD] action | subject`
> Actions: ingest, update, query, lint, create, archive, delete
> When this file exceeds 500 entries, rotate: rename to log-YYYY.md, start fresh.

## [2026-05-05] create | Wiki initialized
- Domain: Engineering decisions for Nordic NCS IoT application projects
- Structure created: SCHEMA.md, index.md, log.md, concepts/, comparisons/, entities/, queries/, raw/

## [2026-05-05] create | concepts/ncs-app-versioning.md
- Sourced from: live conversation about version numbering for nordic-wifi-webdash/memfault/audio
- Covers: four-field scheme, rationale vs v3.3.x, NCS counter reset rule, Zephyr VERSION file alignment

## [2026-05-05] create | concepts/memfault-version-requirements.md
- Sourced from: Memfault docs (https://docs.memfault.com/docs/platform/software-version-hardware-version)
- Covers: allowed characters, natsort ordering, v-prefix caveat, OTA ordering across NCS bumps
