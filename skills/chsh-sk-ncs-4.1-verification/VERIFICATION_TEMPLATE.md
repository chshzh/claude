# QA Report — <Project Name>

## Document Information

| Field | Value |
|-------|-------|
| Project | |
| Version | YYYY-MM-DD-HH-MM |
| PRD Version | YYYY-MM-DD-HH-MM |
| Specs Version | YYYY-MM-DD-HH-MM |
| NCS Version | e.g. v3.3.0 |
| Status | Draft / Pass / Fail |

---

## Changelog

| Version | Summary |
|---------|---------|
| YYYY-MM-DD-HH-MM | Initial QA review |
---

## Build State

| Item | Value |
|------|-------|
| Uncommitted changes | Yes / No (list files if yes) |
| Build reflects | Last commit / Uncommitted on-disk state |
---

## 1. Security Risk

| ID | Risk Class | File / Location | Finding | Verdict |
|----|-----------|----------------|---------|---------|
| SEC-1 | Hardcoded credentials | | | ✅ Clean / ❌ Issue |
| SEC-2 | Debug features in release | | | ✅ Clean / ❌ Issue |
| SEC-3 | HTTP (unencrypted) endpoint | | | ✅ Clean / ❌ Issue |
| SEC-4 | Key / token in source | | | ✅ Clean / ❌ Issue |

**Security result**: ✅ No findings / ❌ _N_ findings — route to Phase 3 immediately

---

## 2. Code Format

| Verdict | Files with violations |
|---------|-----------------------|
| ✅ Clean / ⚠️ Issues | |

Violations:
```
<list clang-format --dry-run output here>
```

---

## 3. Build Results

| Board | Result | Compiler Warnings | FLASH | RAM |
|-------|--------|-------------------|-------|-----|
| | ✅ PASS / ❌ FAIL | 0 | xx% | xx% |

> Kconfig `Experimental symbol` notices are expected on nRF70 builds and are not counted as compiler warnings.

---

## 4. PRD Satisfaction Check (Code Reading)

| FR | Acceptance Criterion | Code Evidence | Verdict |
|----|---------------------|---------------|---------|
| FR-001 | | | ✅ / ⚠️ / ❓ / ❌ |

`❓ Not visible` = cannot confirm without hardware — add as priority TC in **chsh-sk-ncs-4.2-validation**
`❌ Mismatch` = P0, route to Phase 3

**PRD result**: ___ implemented, ___ partial, ___ not visible, ___ mismatch

---

## 5. Documentation Consistency Audit

| Step | Check | Result | Notes |
|------|-------|--------|-------|
| A | `overview.md` PRD Version field matches latest PRD Changelog timestamp | ✅ / ❌ | |
| A | All FR/NFR items in PRD traceable to spec | ✅ / ❌ | |
| B | `CONFIG_ZEGO_APP_SPECS_VERSION` in `prj.conf` matches `overview.md` latest Changelog | ✅ / ❌ | |
| B | `CONFIG_ZEGO_APP_PRD_VERSION` in `prj.conf` matches PRD latest Changelog | ✅ / ❌ | |
| B | Spec modules have `src/modules/<name>/` counterparts | ✅ / ❌ | |
| C | README features match current PRD FR list | ✅ / ❌ | |
| C | No stale features in README | ✅ / ❌ | |

---

## 6. Fixes Applied During This Verification

| File | Fix | Severity |
|------|-----|----------|
| | | P0/P1/P2 |

*(Delete this section if no fixes were applied.)*

---

## Summary

**Overall verdict**: PASS / PASS WITH ISSUES / FAIL

| Priority | Finding | Route |
|----------|---------|-------|
| P0 | | Phase 3 |
| P1 | | Phase 3 |
| P2 | | Next iteration |
