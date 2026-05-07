---
title: NCS Application Versioning Scheme
created: 2026-05-05
updated: 2026-05-05
type: concept
tags: [versioning, convention, decision, ncs, zephyr]
sources: []
confidence: high
---

# NCS Application Versioning Scheme

## Decision

Use **`MAJOR.MINOR.PATCH.APP`** (four numeric segments, no `v` prefix in firmware,
`v` prefix on git tags) for all NCS workspace application projects.

Example: firmware reports `3.3.0.1`, git tag is `v3.3.0.1`.

The first three segments **mirror the NCS SDK version** the app is built against.
The fourth segment is an **independent app patch counter** that resets to `0`
when the NCS version bumps.

## Rationale

### Why not `v3.3.x`?

`v3.3.x` is ambiguous — `v3.3.1` could mean "app patch 1 on NCS v3.3.0"
or "app now targets NCS v3.3.1". Since NCS itself releases `v3.3.1` as a patch
release, the namespaces collide and create confusion when triaging field issues.

### Alignment with Zephyr

Zephyr's own `VERSION` file uses exactly four fields:

```
VERSION_MAJOR = 4
VERSION_MINOR = 3
PATCHLEVEL    = 99
VERSION_TWEAK = 0    ← per-build/per-app increment
```

`VERSION_TWEAK` is the idiomatic slot for app-level patches on top of a fixed
upstream release. The `MAJOR.MINOR.PATCH.APP` scheme maps directly to this model.

### Existing precedent

`nordic-wifi-audio` already uses this scheme: `3.2.1.1` → `3.2.1.2` → `3.2.1.3` → `3.2.1.4`.
It reads cleanly and makes the SDK baseline immediately obvious.

## Convention Rules

| Situation | Action |
|-----------|--------|
| First release on NCS `v3.3.0` | Tag `v3.3.0.1`, firmware `3.3.0.1` |
| Bug fix / feature, same NCS | Increment last digit: `v3.3.0.2` |
| Rebase to NCS `v3.3.1` | Reset last digit: `v3.3.1.0` |
| Pre-release / RC | `3.3.0.1-rc.1` (SemVer pre-release suffix) |
| Local dev builds | CMake injects `3.3.0-dev` or `v3.3.0-dev` automatically |

## Git Tags vs. Firmware String

```
Git tag:        v3.3.0.1   ← humans and CI use this
Firmware string: 3.3.0.1   ← what Memfault receives (no leading v)
```

The `v` is stripped in the firmware because Memfault's sorting algorithm treats
it lexically and Memfault's own documentation never uses a `v` prefix in examples.
See [memfault-version-requirements](memfault-version-requirements.md).

## CMake Injection Pattern (webdash example)

```cmake
# Release CI:    pass -DAPP_VERSION_STRING=3.3.0.1
# Local build:   auto-generates "v3.3.0-dev"
if(DEFINED APP_VERSION_STRING)
    target_compile_definitions(app PRIVATE APP_VERSION_STRING="${APP_VERSION_STRING}")
else()
    target_compile_definitions(app PRIVATE APP_VERSION_STRING="v${NCS_VERSION}-dev")
endif()
```

## Projects Using This Scheme

| Project | Current series | Notes |
|---------|---------------|-------|
| `nordic-wifi-audio` | `3.2.1.x` | Origin project for this convention |
| `nordic-wifi-webdash` | `3.3.0.x` | Migrating from ad-hoc tags (`v3.3`, `v3.3.0-rc2-1`) |
| `nordic-wifi-memfault` | `3.3.0.x` | Migrating from independent semver (`3.1.0` … `3.2.0`) |

## Open Questions

- Should the APP counter start at `0` or `1` for the first release on a new NCS version?
  `nordic-wifi-audio` used `1` (`3.2.1.1`). Recommendation: start at `1` so `x.y.z.0`
  can be reserved to signal "baseline port, no app changes yet".

## Related

- [memfault-version-requirements](memfault-version-requirements.md) — platform constraints that shape this decision
