# NCS Project Quality Assurance Report — <Project Name>

## Document Information

| Field | Value |
|-------|-------|
| Project | |
| Version | YYYY-MM-DD-HH-MM |
| PRD Version | YYYY-MM-DD-HH-MM |
| Specs Version | YYYY-MM-DD-HH-MM |
| Reviewer | |
| NCS Version | e.g. v3.2.4 |
| Board / Platform | e.g. nRF7002DK |
| Status | Draft / Pass / Fail |

---

## Changelog

| Version | Summary of changes |
|---|---|
| YYYY-MM-DD-HH-MM | Initial QA review |

---

## Executive Summary

**Overall Score**: _____ / 100

**Status**: 
- [ ] ✅ **PASS** - Ready for release
- [ ] ⚠️ **PASS WITH ISSUES** - Needs minor improvements
- [ ] 🔧 **REWORK REQUIRED** - Significant issues to address
- [ ] ❌ **FAIL** - Major problems, not ready

**Key Findings**:
_Brief 2-3 sentence summary of the review outcome_

**Recommendation**:
_Release / Hold / Rework - with justification_

---

## Score Breakdown

| Category | Score | Weight | Weighted Score |
|----------|-------|--------|----------------|
| 1. Project Structure | ___/15 | 15% | ___ |
| 2. Core Files Quality | ___/20 | 20% | ___ |
| 3. Configuration | ___/15 | 15% | ___ |
| 4. Code Quality | ___/20 | 20% | ___ |
| 5. Documentation | ___/15 | 15% | ___ |
| 6. Wi-Fi Implementation* | ___/10 | 10% | ___ |
| 7. Security | ___/10 | 10% | ___ |
| 8. Build & Testing | ___/10 | 10% | ___ |
| **Total** | | **100%** | **___** |

*_If not Wi-Fi project, redistribute 10% across other categories_

---

## 1. Project Structure Review [___/15]

### 1.1 Required Files (8 points)

| File | Present | Valid | Issues | Score |
|------|---------|-------|--------|-------|
| CMakeLists.txt | ☐ Yes ☐ No | ☐ | | /1 |
| Kconfig | ☐ Yes ☐ No | ☐ | | /1 |
| prj.conf | ☐ Yes ☐ No | ☐ | | /1 |
| src/main.c | ☐ Yes ☐ No | ☐ | | /1 |
| LICENSE | ☐ Yes ☐ No | ☐ | | /1 |
| README.md | ☐ Yes ☐ No | ☐ | | /1 |
| .gitignore | ☐ Yes ☐ No | ☐ | | /1 |
| Copyright headers (current year for new/modified) | ☐ All ☐ Most ☐ Some ☐ None | | | /1 |

**Subtotal**: ___/8

### 1.2 Optional Files (3 points)

| File | Present | Appropriate | Notes |
|------|---------|-------------|-------|
| west.yml | ☐ | ☐ | |
| sysbuild.conf | ☐ | ☐ | |
| overlay-*.conf | ☐ | ☐ | |
| boards/ | ☐ | ☐ | |
| VERSION | ☐ | ☐ | |

**Score (0-3)**: ___  
_3=All appropriate files present, 2=Most present, 1=Some present, 0=None_

**Subtotal**: ___/3

### 1.3 Directory Organization (4 points)

- [ ] Source files in src/ [1 pt]
- [ ] Headers properly organized [1 pt]
- [ ] No build artifacts committed [1 pt]
- [ ] Structure matches complexity [1 pt]

**Subtotal**: ___/4

### Issues Found

**Critical**:
- 

**Warnings**:
- 

**Recommendations**:
- 

---

## 2. Core Files Quality [___/20]

### 2.1 CMakeLists.txt (5 points)

- [ ] Minimum version ≥3.20.0 [1]
- [ ] Zephyr package found [1]
- [ ] All sources listed [1]
- [ ] No hardcoded paths [1]
- [ ] Copyright header with current year [1]

**Score**: ___/5

**Issues**:


### 2.2 Kconfig (5 points)

- [ ] Zephyr sourced [1]
- [ ] Options documented [1]
- [ ] Defaults sensible [1]
- [ ] Range constraints [1]
- [ ] Good naming [1]

**Score**: ___/5

**Issues**:


### 2.3 prj.conf (5 points)

- [ ] Well organized [1]
- [ ] Commented sections [1]
- [ ] No redundant options [1]
- [ ] Appropriate stack/heap [1]
- [ ] No security issues [1]

**Score**: ___/5

**Issues**:


### 2.4 src/main.c (5 points)

- [ ] Copyright header with current year [1]
- [ ] Proper includes [1]
- [ ] Error handling [1]
- [ ] Coding style [1]
- [ ] No magic numbers [1]

**Score**: ___/5

**Issues**:


---

## 3. Configuration Review [___/15]

### 3.1 Wi-Fi Configuration (10 points) *[Skip if not Wi-Fi project]*

**Basic Setup (4 points)**:
- [ ] WIFI=y enabled [1]
- [ ] WIFI_NRF70=y enabled [1]
- [ ] WIFI_READY_LIB=y [1]
- [ ] Networking stack configured [1]

**Memory (3 points)**:
- [ ] Heap ≥80KB for Wi-Fi [1]
- [ ] Buffer counts appropriate [1]
- [ ] Wi-Fi heaps configured [1]

**Mode-Specific (3 points)**:
- [ ] Station: DHCP client [1]
- [ ] SoftAP: DHCP server [1]
- [ ] P2P: Both + low power off [1]

**Score**: ___/10

**Issues**:


### 3.2 Build Configuration (5 points)

- [ ] Overlays properly used [2]
- [ ] Board configs present [1]
- [ ] Sysbuild if needed [1]
- [ ] Optimization appropriate [1]

**Score**: ___/5

**Issues**:


---

## 4. Code Quality Analysis [___/20]

### 4.1 Coding Standards (5 points)

**Zephyr Style Compliance**:
- [ ] Indentation (tabs) [1]
- [ ] Line length ≤100 [1]
- [ ] Naming conventions [1]
- [ ] Braces style [1]
- [ ] Comments present [1]

**Score**: ___/5

### 4.2 Error Handling (5 points)

- [ ] Return values checked [2]
- [ ] Proper error codes [1]
- [ ] Error logging [1]
- [ ] Resource cleanup [1]

**Score**: ___/5

**Issues**:


### 4.3 Memory Management (5 points)

- [ ] No memory leaks [2]
- [ ] Stack usage reasonable [1]
- [ ] Buffer overflow prevention [1]
- [ ] NULL checks [1]

**Score**: ___/5

**Issues**:


### 4.4 Thread Safety (5 points)

- [ ] Shared resources protected [2]
- [ ] Race conditions avoided [1]
- [ ] Atomic operations proper [1]
- [ ] No deadlocks [1]

**Score**: ___/5

**Issues**:


---

## 5. Documentation Assessment [___/15]

### 5.1 README.md (8 points)

| Section | Present | Complete | Quality | Score |
|---------|---------|----------|---------|-------|
| Title/Badges | ☐ | ☐ | ☐ Good ☐ Fair ☐ Poor | /1 |
| Overview | ☐ | ☐ | ☐ Good ☐ Fair ☐ Poor | /1 |
| Hardware Requirements | ☐ | ☐ | ☐ Good ☐ Fair ☐ Poor | /1 |
| Quick Start | ☐ | ☐ | ☐ Good ☐ Fair ☐ Poor | /1 |
| Build Instructions | ☐ | ☐ | ☐ Good ☐ Fair ☐ Poor | /1 |
| Configuration Guide | ☐ | ☐ | ☐ Good ☐ Fair ☐ Poor | /1 |
| Operation Guide | ☐ | ☐ | ☐ Good ☐ Fair ☐ Poor | /1 |
| Troubleshooting | ☐ | ☐ | ☐ Good ☐ Fair ☐ Poor | /1 |

**Score**: ___/8

### 5.2 Code Documentation (4 points)

- [ ] Function comments [1]
- [ ] Module documentation [1]
- [ ] Complex logic explained [1]
- [ ] Doxygen compatible [1]

**Score**: ___/4

### 5.3 PDR Compliance (3 points)

- [ ] Requirements spec exists [1]
- [ ] Architecture documented [1]
- [ ] Test plan present [1]

**Score**: ___/3

**Missing Documentation**:


---

## 6. Wi-Fi Implementation [___/10] *[Skip if not Wi-Fi]*

### 6.1 Mode Implementation (5 points)

**For Detected Mode**: ☐ Station ☐ SoftAP ☐ P2P ☐ Monitor ☐ Raw

- [ ] Mode-specific config correct [2]
- [ ] Initialization proper [1]
- [ ] Connection logic sound [1]
- [ ] Retry mechanism [1]

**Score**: ___/5

### 6.2 Event Handling (3 points)

- [ ] Connected event [1]
- [ ] Disconnected event [1]
- [ ] IP assigned event [1]

**Score**: ___/3

### 6.3 Performance (2 points)

- [ ] Low power appropriate [1]
- [ ] Buffers optimized [1]

**Score**: ___/2

**Issues**:


---

## 7. Security Audit [___/10]

### 7.1 Credential Management (4 points)

- [ ] No hardcoded credentials [2]
- [ ] Secure storage used [1]
- [ ] Not in version control [1]

**Score**: ___/4

**Critical Issues**:


### 7.2 Network Security (3 points)

- [ ] TLS enabled if needed [1]
- [ ] Strong encryption (WPA2+) [1]
- [ ] Input validation [1]

**Score**: ___/3

### 7.3 Debug Features (3 points)

- [ ] No debug in release [1]
- [ ] Logging safe [1]
- [ ] No backdoors [1]

**Score**: ___/3

---

## 8. Build & Testing [___/10]

### 8.1 Build Verification (5 points)

- [ ] Clean build succeeds [2]
- [ ] No warnings [1]
- [ ] Overlays build [1]
- [ ] Binary size reasonable [1]

**Build Command Used**:
```bash

```

**Score**: ___/5

**Build Issues**:


### 8.2 Runtime Testing (5 points)

- [ ] Firmware boots [1]
- [ ] Main functionality works [2]
- [ ] Error paths tested [1]
- [ ] Stability verified [1]

**Score**: ___/5

**Runtime Issues**:


---

## Issues Summary

### Critical Issues (Must Fix Before Release)

| # | Category | Description | Impact | Recommendation |
|---|----------|-------------|--------|----------------|
| 1 | | | High | |
| 2 | | | High | |

### Warnings (Should Fix)

| # | Category | Description | Impact | Recommendation |
|---|----------|-------------|--------|----------------|
| 1 | | | Medium | |
| 2 | | | Medium | |

### Improvements (Nice to Have)

| # | Category | Description | Benefit | Priority |
|---|----------|-------------|---------|----------|
| 1 | | | | Low/Med/High |
| 2 | | | | Low/Med/High |

---

## Detailed Findings

### Code Examples

**Issue Location**: File: _________, Line: ___
```c
// Current code (problematic)


// Suggested fix

```

**Issue Location**: File: _________, Line: ___
```
Issue description
```

---

## Compliance Matrix

| Category | Requirement | Met | Partial | Not Met | N/A |
|----------|-------------|-----|---------|---------|-----|
| Structure | Required files present | ☐ | ☐ | ☐ | ☐ |
| Structure | Directory organization | ☐ | ☐ | ☐ | ☐ |
| Config | Wi-Fi properly configured | ☐ | ☐ | ☐ | ☐ |
| Code | Follows Zephyr style | ☐ | ☐ | ☐ | ☐ |
| Code | Error handling complete | ☐ | ☐ | ☐ | ☐ |
| Docs | README complete | ☐ | ☐ | ☐ | ☐ |
| Docs | Code documented | ☐ | ☐ | ☐ | ☐ |
| Docs | PDR documents present | ☐ | ☐ | ☐ | ☐ |
| Security | No credential leaks | ☐ | ☐ | ☐ | ☐ |
| Security | Network security proper | ☐ | ☐ | ☐ | ☐ |
| Build | Clean build success | ☐ | ☐ | ☐ | ☐ |
| Testing | Runtime verified | ☐ | ☐ | ☐ | ☐ |

---

## Recommendations for Project Team

### Immediate Actions (Before Next Build)
1. 
2. 
3. 

### Short-term Improvements (Next Sprint)
1. 
2. 
3. 

### Long-term Enhancements
1. 
2. 
3. 

---

## Recommendations for ncs-project-generate Skill

Based on this review, we recommend the following improvements to the ncs-project-generate templates:

### Template Updates
1. 
2. 
3. 

### Documentation Enhancements
1. 
2. 
3. 

### New Examples/Patterns
1. 
2. 
3. 

### Checklist Additions
1. 
2. 
3. 

---

## Follow-up Actions

### For Project Team

| Action | Owner | Due Date | Status |
|--------|-------|----------|--------|
| Fix critical issue #1 | | | ☐ |
| Fix critical issue #2 | | | ☐ |
| Address warning #1 | | | ☐ |

### For Reviewer

- [ ] Send report to project team
- [ ] Schedule follow-up review
- [ ] Update ncs-project-generate templates
- [ ] Share learnings with team

---

## Conclusion

**Overall Assessment**:


**Risk Level**: 
- [ ] 🟢 Low - Ready for release
- [ ] 🟡 Medium - Minor issues to address
- [ ] 🟠 High - Significant rework needed
- [ ] 🔴 Critical - Major problems

**Estimated Effort to Address Issues**:
- Critical: ___ hours
- Warnings: ___ hours
- Improvements: ___ hours
- **Total**: ___ hours

**Next Review**: _______________

**Sign-off**:

Reviewer: _________________________ Date: _________

Project Lead: _____________________ Date: _________

---

## Appendix A: Test Results

### Build Output
```


```

### Runtime Log
```


```

### Performance Metrics
- Boot time: ___ ms
- Memory usage: ___ KB
- Throughput: ___ Mbps
- Latency: ___ ms

---

## Appendix B: Tool Versions

- NCS Version: 
- Zephyr Version: 
- GCC Version: 
- West Version: 
- Board: 
- Shield: 

---

## Appendix C: References

- Project repository: 
- Documentation: 
- Related issues: 
- Previous reviews:
