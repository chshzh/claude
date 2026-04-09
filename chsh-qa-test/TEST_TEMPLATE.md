# Functional Test Report — <Project Name>

## Document Information

| Field | Value |
|-------|-------|
| Project | |
| Version | YYYY-MM-DD-HH-MM |
| PRD Version | YYYY-MM-DD-HH-MM |
| Specs Version | YYYY-MM-DD-HH-MM |
| Tester | |
| Board / Shield | e.g. nRF7002DK + nRF7002-EK |
| NCS Version | e.g. v3.2.4 |
| Firmware built | YYYY-MM-DD-HH-MM |
| Status | Draft / Pass / Fail |

---

## Changelog

| Version | Summary of changes |
|---|---|
| YYYY-MM-DD-HH-MM | Initial test run |

---

## Test Environment

```bash
# Board connected:
# Flash command used:
west flash --build-dir build/

# Serial monitor:
# (port, baud rate)
```

---

## Summary

| Total TCs | Passed | Failed | Blocked | Not Run |
|-----------|--------|--------|---------|---------|
| | | | | |

**Overall Result:**
- [ ] ✅ PASS — all P0 acceptance criteria met
- [ ] ⚠️ PASS WITH ISSUES — P1/P2 failures only
- [ ] ❌ FAIL — one or more P0 criteria failed

**Verdict**: _one sentence summary_

---

## Test Results

> Each test case (TC) maps directly to a PRD acceptance criterion.
> Copy the TC IDs and acceptance text from `docs/PRD.md`.

### FR-001 — <Feature Title>

| TC | Acceptance Criterion (from PRD) | Result | UART / Evidence |
|----|--------------------------------|--------|-----------------|
| TC-001-01 | <criterion text> | ✅ Pass / ❌ Fail / ⚠️ Partial / ⬜ Not Run | |
| TC-001-02 | <criterion text> | | |

**Notes / Observations:**

---

### FR-002 — <Feature Title>

| TC | Acceptance Criterion (from PRD) | Result | UART / Evidence |
|----|--------------------------------|--------|-----------------|
| TC-002-01 | <criterion text> | | |

**Notes / Observations:**

---

### NFR — Non-Functional Requirements

| TC | Requirement (from PRD) | Metric | Measured | Result |
|----|----------------------|--------|----------|--------|
| TC-NFR-01 | Connection time < 30 s | 30 s | ___ s | ✅ / ❌ |
| TC-NFR-02 | Page load < 2 s | 2 s | ___ s | ✅ / ❌ |
| TC-NFR-03 | Memory headroom > 50 KB RAM | 50 KB | ___ KB | ✅ / ❌ |

---

## Failed Tests Detail

> Fill in only for ❌ Fail or ⚠️ Partial results.

### TC-XXX-YY — <criterion text>

**Expected**: <what PRD says should happen>

**Actual**: <what actually happened>

**UART log**:
```
<paste relevant log lines>
```

**Root cause (if known)**:

**Suggested fix / routing**:
- [ ] Code fix → Phase 3
- [ ] Spec gap → Phase 2
- [ ] PRD change → Phase 1

---

## UART Boot Log

> Paste the full boot sequence log here as evidence.

```
<paste UART output from power-on to ready state>
```

---

## Appendix — Performance Metrics

| Metric | Target | Measured | Pass? |
|--------|--------|----------|-------|
| Boot to ready | < ___ ms | ___ ms | |
| Wi-Fi connect time | < ___ s | ___ s | |
| Page load time | < ___ s | ___ s | |
| Heap free (steady state) | > ___ KB | ___ KB | |
| RAM free (steady state) | > ___ KB | ___ KB | |
