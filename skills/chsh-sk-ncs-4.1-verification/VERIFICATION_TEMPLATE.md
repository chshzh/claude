# QA Report — <Project Name>

## Document Information

| Field | Value |
|-------|-------|
| Project | |
| Version | YYYY-MM-DD-HH-MM |
| PRD Version | YYYY-MM-DD-HH-MM |
| Specs Version | YYYY-MM-DD-HH-MM |
| Reviewer | |
| NCS Version | e.g. v3.3.0 |
| Status | Draft / Pass / Fail |

---

## Changelog

| Version | Summary |
|---------|---------|
| YYYY-MM-DD-HH-MM | Initial QA review |

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

## 3. Code Quality

| Category | Score (0–10) | Notes |
|----------|-------------|-------|
| Project structure | | |
| Core files (CMakeLists, Kconfig, prj.conf) | | |
| Error handling | | |
| Thread safety | | |
| Memory | | |
| Wi-Fi implementation | | |
| Documentation / README | | |
| Build (zero warnings) | | |
| Security (from §1) | | |
| **Normalized total** | **/100** | |

Score formula: (sum / 90) × 100

**Quality verdict**: ✅ Excellent / ✅ Good / ⚠️ Satisfactory / ⚠️ Needs Work / ❌ Fail

---

## 4. PRD Satisfaction Check (Code Reading)

| FR | Acceptance Criterion | Code Evidence | Verdict |
|----|---------------------|---------------|---------|
| FR-001 | | | ✅ / ⚠️ / ❓ / ❌ |

`❓ Not visible` = cannot confirm without hardware — add as priority TC in **chsh-sk-ncs-4.2-validation**
`❌ Mismatch` = P0, route to Phase 3

**PRD result**: ___ implemented, ___ partial, ___ not visible, ___ mismatch

---

## Summary

**Overall verdict**: PASS / PASS WITH ISSUES / FAIL

| Priority | Finding | Route |
|----------|---------|-------|
| P0 | | Phase 3 |
| P1 | | Phase 3 |
