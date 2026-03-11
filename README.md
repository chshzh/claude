# Charlie Skills Repository

Personal collection of specialized skills for development workflows with Claude.

## 📦 Skills Overview

### Developer Skills

#### **NCS Env Setup**
Environment provisioning scripts and notes for Nordic SDK tooling.
- Location: `Developer/ncs/env-setup/`
- Entry: [SKILL.md](Developer/ncs/env-setup/SKILL.md)

#### **NCS Debug**
Debugging tips, CLI references, and troubleshooting playbooks.
- Location: `Developer/ncs/debug/`
- Entry: [SKILL.md](Developer/ncs/debug/SKILL.md)

#### **NCS Memory Optimization**
Strategies for footprint reduction and profiling.
- Location: `Developer/ncs/mem-opt/`
- Entry: [SKILL.md](Developer/ncs/mem-opt/SKILL.md)

#### **NCS Project Workflow**
Complete lifecycle management for NCS projects with integrated generate→review→improve workflow.
- Location: `Developer/ncs/project/`
- Quick start:
	- `cp ~/.claude/skills/Developer/ncs/project/templates/.gitignore ./`
	- `cp ~/.claude/skills/Developer/ncs/project/templates/LICENSE ./`
	- `cp ~/.claude/skills/Developer/ncs/project/templates/README_TEMPLATE.md README.md`
	- `cp ~/.claude/skills/Developer/ncs/project/configs/wifi-sta.conf overlay-wifi.conf`
- Documentation:
	- [SKILL.md](Developer/ncs/project/SKILL.md)
	- [PROJECT_STRUCTURE.md](Developer/ncs/project/guides/PROJECT_STRUCTURE.md)
	- [WIFI_GUIDE.md](Developer/ncs/project/guides/WIFI_GUIDE.md)

### Product Manager Skills

#### **NCS Feature Planning**
Feature selection matrices, overlays, and prioritization aids.
- Location: `ProductManager/ncs/features/`
- Entry: [SKILL.md](ProductManager/ncs/features/SKILL.md)

#### **PRD Toolkit**
Canonical PRD template and supporting material.
- Location: `ProductManager/ncs/prd/`
- Key asset: [PRD_TEMPLATE.md](ProductManager/ncs/prd/PRD_TEMPLATE.md)

#### **NCS Review Framework**
Checklists, report templates, and automation scripts for validating builds against the PRD.
- Location: `ProductManager/ncs/review/`
- Highlights:
	- [CHECKLIST.md](ProductManager/ncs/review/CHECKLIST.md)
	- [QA_TEMPLATE.md](ProductManager/ncs/review/QA_TEMPLATE.md)
	- [check_project.sh](ProductManager/ncs/review/check_project.sh)

## 🚀 Usage

Skills are designed to be referenced during development conversations. Each skill provides:
- Templates and configurations
- Documentation and guides
- Automation scripts
- Best practices

### Invocation Pattern

- Declare the role, domain, and desired skill, for example: “As Developer of NCS, I want to do env-setup with NCS 3.2.1.”
- The assistant maps the role to the matching directory (Developer, ProductManager, linguist) and loads the requested sub-skill (env-setup, project, features, review, etc.).
- Mention additional context (board, SDK version, target feature) so the assistant pulls the relevant files within that subdirectory.

## 📁 Structure

```
skills/
├── Developer/
│   └── ncs/
│       ├── env-setup/
│       ├── debug/
│       ├── mem-opt/
│       └── project/
├── ProductManager/
│   └── ncs/
│       ├── features/
│       ├── prd/
│       └── review/
├── linguist/
│   ├── message-reviewer/
│   └── norwegian-teacher/
├── README.md
└── .gitignore
```

## 🔄 Workflow Integration

Skills support iterative development workflows:

```
Generate → Develop → Review → QA Report → Fix → Improve → Generate
```

## 📊 Token Efficiency

Skills are optimized for token consumption:
- Core SKILL.md files: ~2,000 tokens (auto-loaded)
- Detailed guides: 5,000+ tokens (loaded on-demand)
- Templates and configs: Accessed as needed

## 📝 License

Skills and templates include appropriate license headers for target projects.

Individual skills may reference or include:
- Nordic 5-Clause License (LicenseRef-Nordic-5-Clause)
- Apache License 2.0
- MIT License

## 👤 Author

**Charlie Shao** ([@chshzh](https://github.com/chshzh))  
Nordic Semiconductor - Oslo, Norway

## 📅 Last Updated

February 3, 2026
