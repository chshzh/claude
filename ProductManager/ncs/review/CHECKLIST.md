# NCS Project Review Checklist

Quick reference checklist for conducting NCS project reviews.

---

## Pre-Review Setup

- [ ] Clone/access project repository
- [ ] Note NCS version used
- [ ] Identify target board/platform
- [ ] Review project documentation
- [ ] Run automated check script: `./check_project.sh`
- [ ] Check for PRD.md (business requirements)
- [ ] Check for openspec/ directory (technical specs)

---

## Quick Review (30 minutes)

### Essential Files
- [ ] CMakeLists.txt present and valid
- [ ] Kconfig present with options
- [ ] prj.conf with basic configs
- [ ] PRD.md present (business requirements)
- [ ] openspec/ directory present (technical specs)
- [ ] src/main.c exists
- [ ] README.md complete
- [ ] LICENSE file present (Nordic 5-Clause)
- [ ] .gitignore appropriate

### Build Test
- [ ] Clean build succeeds: `west build -p -b <board>`
- [ ] No build warnings
- [ ] Binary size reasonable

### Critical Security
- [ ] No hardcoded credentials
- [ ] No sensitive data in repo
- [ ] Credentials not committed

---

## Standard Review (2-3 hours)

### Project Structure (15 min)
- [ ] All required files present
- [ ] Optional files appropriate
- [ ] Directory structure organized
- [ ] Copyright headers on all files (**current year** for new/modified files, e.g. 2026)
- [ ] SPDX identifiers correct

### Core Files (30 min)
- [ ] CMakeLists.txt follows template
- [ ] Kconfig options documented
- [ ] prj.conf well organized
- [ ] main.c properly structured
- [ ] Error handling implemented

### Configuration (30 min)
- [ ] Wi-Fi configs appropriate (if applicable)
- [ ] Memory settings sufficient
  - [ ] Stack sizes optimized using Thread Analyzer
  - [ ] Heap size adequate for subsystems (≥64 KB for Wi-Fi apps)
  - [ ] Heap monitoring enabled during development
  - [ ] Net buffer pools sized appropriately
- [ ] Network stack configured
- [ ] Overlay files proper
- [ ] Board configs complete

### Code Quality (45 min)
- [ ] Coding style compliance
- [ ] Error handling comprehensive
- [ ] Memory management sound
- [ ] Thread safety considered
- [ ] Logging appropriate

### Documentation (30 min)
- [ ] README complete with all sections
- [ ] README communicates value proposition in user-friendly, marketing-oriented language
- [ ] Code comments adequate
- [ ] Build instructions accurate
- [ ] Configuration documented
- [ ] PDR documents present

---

## Comprehensive Review (1 day)

### All Standard Review Items Plus:

### Memory Optimization (REQUIRED)
- [ ] Thread stacks sized from Thread Analyzer data
  - [ ] CONFIG_THREAD_ANALYZER enabled during development
  - [ ] High-water marks documented
  - [ ] Appropriate margins applied (1.2-2.0x based on thread type)
  - [ ] Stack configurations include sizing rationale in comments
- [ ] Heap properly configured
  - [ ] Wi-Fi apps: CONFIG_HEAP_MEM_POOL_SIZE ≥ 64 KB (80 KB recommended)
  - [ ] Heap monitoring module integrated (development builds)
  - [ ] Peak heap usage tested under load
  - [ ] Heap size justified and documented
- [ ] Memory architecture documented
  - [ ] System heap vs dedicated pools explained
  - [ ] Net buffer slab sizing rationale provided
  - [ ] Wi-Fi driver heap strategy documented (global vs dedicated)
- [ ] Memory debugging enabled in development
  - [ ] CONFIG_STACK_SENTINEL=y
  - [ ] CONFIG_SYS_HEAP_RUNTIME_STATS=y
  - [ ] Heap/stack monitors active

### Wi-Fi Implementation (if applicable)
- [ ] Mode-specific implementation correct
- [ ] Event handling complete
- [ ] Connection retry logic
- [ ] Performance optimizations
- [ ] Low power configured
- [ ] Memory requirements met (see Memory Optimization above)

### Security Audit
- [ ] Credential management secure
- [ ] Network security proper (TLS, WPA2+)
- [ ] Input validation
- [ ] No debug backdoors
- [ ] Logging doesn't expose secrets

### Testing
- [ ] Unit tests present (if applicable)
- [ ] Integration tests defined
- [ ] Runtime testing performed
- [ ] Stability verified
- [ ] Performance benchmarked

### PDR Compliance
- [ ] Requirements specification complete
- [ ] Architecture documented
- [ ] API documentation exists
- [ ] Test plan with results
- [ ] User manual complete

---

## Scoring Guide

### Overall Score Calculation

| Category | Max Points | Weight |
|----------|-----------|--------|
| Project Structure | 15 | 15% |
| Core Files | 20 | 20% |
| Configuration | 15 | 15% |
| Code Quality | 20 | 20% |
| Documentation | 15 | 15% |
| Wi-Fi/Feature | 10 | 10% |
| Security | 10 | 10% |
| Build & Test | 10 | 10% |
| **Total** | **100** | **100%** |

### Rating Thresholds

| Score | Grade | Status |
|-------|-------|--------|
| 90-100 | A | ✅ Excellent - Ready for release |
| 80-89 | B | ✅ Good - Minor improvements |
| 70-79 | C | ⚠️  Satisfactory - Notable issues |
| 60-69 | D | 🔧 Needs work - Significant issues |
| < 60 | F | ❌ Fail - Major rework required |

### Issue Severity

**Critical** (Must fix before release):
- Missing required files
- Build failures
- Security vulnerabilities
- Hardcoded credentials
- No error handling
- Memory leaks

**Warning** (Should fix):
- Incomplete documentation
- Style inconsistencies
- Suboptimal configurations
- Missing optional files
- Inadequate comments

**Improvement** (Nice to have):
- Additional test coverage
- Performance optimizations
- Enhanced documentation
- Better modularity
- CI/CD automation

---

## Common Issues Reference

### Project Structure
- ❌ Missing LICENSE file → Add Nordic 5-Clause
- ❌ No .gitignore → Copy from template
- ⚠️  Build artifacts committed → Add to .gitignore
- ⚠️  No board directory → Create if board-specific configs needed
- ⚠️  Copyright year outdated → Use **current year** (e.g. 2026) in new or substantially modified files; keep original year for unmodified upstream files

### CMakeLists.txt
- ❌ No minimum version → Add `cmake_minimum_required(VERSION 3.20.0)`
- ❌ Hardcoded paths → Use relative paths
- ⚠️  All sources not listed → Add missing files

### Kconfig
- ❌ Zephyr not sourced → Add `source "Kconfig.zephyr"`
- ⚠️  No help text → Add documentation for each option
- ⚠️  No constraints → Add ranges for numeric options

### prj.conf
- ❌ Insufficient heap for Wi-Fi → Increase to ≥64 KB (80 KB recommended)
- ❌ Thread stacks not sized from analyzer → Use Thread Analyzer data
- ❌ No heap monitoring → Add heap_monitor module (use ncs-mem skill)
- ⚠️  Debug in production → Use overlay for debug configs
- ⚠️  Missing dependencies → Enable required subsystems
- ⚠️  CONFIG_HEAP_MEM_POOL_IGNORE_MIN=y on Wi-Fi apps → Remove, use auto-minimum

### Code
- ❌ No error handling → Check all return values
- ❌ Memory leaks → Ensure cleanup on all paths
- ⚠️  Style violations → Follow Zephyr coding style
- ⚠️  Magic numbers → Use defines/config options

### Documentation
- ❌ README incomplete → Add all required sections
- ⚠️  README too technical → Rewrite key sections to highlight user benefits and marketing messaging
- ⚠️  No hardware requirements → List all required hardware
- ⚠️  Build commands wrong → Test and update

### Wi-Fi
- ❌ Low power not configured → Disable for dev, enable for production
- ❌ No event handling → Handle connect/disconnect/IP events
- ❌ Heap < 64 KB → Increase CONFIG_HEAP_MEM_POOL_SIZE to ≥80000
- ❌ No heap monitoring → Add heap_monitor module
- ⚠️  Net buffers undersized → Tune CONFIG_NET_BUF_RX/TX_COUNT
- ⚠️  No retry logic → Add connection retry
- ⚠️  Wi-Fi driver using global heap unnecessarily → Keep dedicated heap unless constrained

### Security
- ❌ Hardcoded credentials → Never commit credentials
- ❌ No encryption → Use WPA2/WPA3
- ⚠️  No TLS → Enable for sensitive data
- ⚠️  Debug enabled → Remove from release builds

### Memory Optimization
- ❌ Stacks not sized from analyzer → Run Thread Analyzer, apply margins
- ❌ Wi-Fi heap < 64 KB → Critical: increase to ≥80 KB for WPA supplicant
- ❌ No heap monitoring → Add heap_monitor module from ncs-mem skill
- ❌ BUILD_ASSERT missing for heap floor → Add build-time validation
- ⚠️  Heap too large → Right-size from peak usage + margin
- ⚠️  No memory architecture docs → Document heap strategy
- ⚠️  Net buffers in heap → Keep dedicated slab pools for determinism

---

## Review Workflow

### 1. Initial Assessment (10 min)
- Run automated check script
- Review README
- Check file structure
- Note project type (Wi-Fi, BLE, etc.)

### 2. Document Review (20 min)
- Read README thoroughly
- Check for PDR documents
- Review configuration docs
- Note documentation gaps

### 3. Code Review (1-2 hours)
- Check each module
- Review error handling
- Assess code quality
- Note issues with context

### 4. Build & Test (30 min)
- Perform clean build
- Test on hardware if possible
- Check binary size
- Verify basic functionality

### 5. Report Generation (30 min)
- Fill out QA report template
- Categorize issues by severity
- Provide specific recommendations
- Suggest ncs-project-generate improvements

### 6. Follow-up (Ongoing)
- Schedule review meeting
- Track issue resolution
- Update templates based on findings
- Share learnings with team

---

## Review Report Delivery

### Report Format
- Use QA_TEMPLATE.md
- Include all sections
- Provide specific examples
- Give actionable recommendations

### Presentation
- Executive summary first
- Highlight critical issues
- Show code examples
- Provide fix suggestions

### Follow-up
- Schedule review meeting
- Track issue resolution
- Plan re-review if needed
- Update knowledge base

---

## Tips for Effective Reviews

### Be Systematic
- Follow checklist order
- Don't skip sections
- Document everything
- Use templates

### Be Objective
- Use defined criteria
- Avoid personal preferences
- Focus on standards
- Provide evidence

### Be Constructive
- Explain the "why"
- Suggest solutions
- Provide examples
- Acknowledge good work

### Be Thorough
- Check details
- Test assumptions
- Verify claims
- Look for patterns

### Be Timely
- Complete review promptly
- Provide actionable feedback
- Set clear priorities
- Enable quick fixes

---

## Post-Review Actions

### For Project Team
- [ ] Review QA report
- [ ] Prioritize fixes
- [ ] Address critical issues
- [ ] Plan improvements
- [ ] Schedule re-review

### For Reviewer
- [ ] Update templates
- [ ] Share learnings
- [ ] Improve checklist
- [ ] Track metrics
- [ ] Archive report

### For Organization
- [ ] Update standards
- [ ] Enhance training
- [ ] Improve tools
- [ ] Collect metrics
- [ ] Share best practices

---

## Continuous Improvement

### Metrics to Track
- Average review score
- Common issues
- Time to resolution
- Review effectiveness
- Re-review rate

### Template Updates
- Add new check items
- Refine scoring
- Improve examples
- Update references
- Enhance automation

### Knowledge Sharing
- Document patterns
- Share examples
- Train reviewers
- Update guides
- Build expertise
