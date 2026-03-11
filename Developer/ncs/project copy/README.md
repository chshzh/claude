```
Developer/ncs/project/
├── SKILL.md                # Main skill reference (~2000 tokens)
├── INDEX.md                # Asset index and navigation
├── templates/              # Copy to new projects
│   ├── LICENSE
│   ├── .gitignore
│   └── README_TEMPLATE.md
├── configs/                # Wi-Fi mode configurations
│   ├── wifi-sta.conf
│   ├── wifi-softap.conf
│   ├── wifi-p2p.conf
│   └── wifi-raw.conf
├── architecture/           # Pattern-specific templates
│   ├── simple-multithreaded/
│   └── smf-zbus/
├── guides/                 # Project + configuration guides
│   ├── CONFIG_GUIDE.md
│   └── PROJECT_STRUCTURE.md
├── protocols/              # Protocol-specific helpers
│   ├── coap/
│   ├── http/
│   ├── mqtt/
│   ├── tcp/
│   ├── udp/
│   └── webserver/
├── wifi/                   # Wi-Fi configs and guides
│   ├── configs/
│   └── guides/
└── examples/               # Ready-to-run sample projects
  └── basic_app/

ProductManager/ncs/
├── features/               # Modular feature overlays + references
├── prd/                    # PRD templates and planning tools
└── review/                 # Review checklists, reports, automation
```

# Full review: Use ProductManager/ncs/review/CHECKLIST.md
```

## 📁 Structure

```
ncs-project/
├── SKILL.md                # Main skill reference (~2000 tokens)
│
├── templates/              # Copy to new projects
│   ├── LICENSE
│   ├── .gitignore
│   ├── README_TEMPLATE.md
│   └── PDR_TEMPLATE.md     # With feature selection checklist
│
├── configs/                # Wi-Fi mode configurations
│   ├── wifi-sta.conf
│   ├── wifi-softap.conf
│   ├── wifi-p2p.conf
│   └── wifi-raw.conf
│
├── features/               # Modular feature overlays (NEW!)
│   ├── overlay-wifi-shell.conf
│   ├── overlay-udp.conf
│   ├── overlay-tcp.conf
│   ├── overlay-mqtt.conf
│   ├── overlay-http-client.conf
│   ├── overlay-https-server.conf
│   ├── overlay-coap.conf
│   ├── overlay-memfault.conf
│   ├── overlay-ble-prov.conf
│   ├── overlay-smf-zbus.conf        # SMF+zbus architecture (NEW!)
│   └── overlay-multithreaded.conf   # Simple multi-threaded (NEW!)
│   ├── overlay-smf-zbus.conf        # SMF+zbus architecture
│   └── overlay-multithreaded.conf   # Simple multi-threaded
│
├── guides/                 # Detailed documentation
│   ├── WIFI_GUIDE.md
│   ├── FEATURE_SELECTION.md      # Complete feature guide (NEW!)
│   └── ARCHITECTURE_PATTERNS.md  # Multi-threaded vs SMF+zbus (NEW!)
│   ├── PROJECT_STRUCTURE.md
│   ├── FEATURE_SELECTION.md      # Complete feature guide
│   └── ARCHITECTURE_PATTERNS.md  # NEW! Multi-threaded vs SMF+zbus
│
├── review/                 # QA tools
  - **FEATURE_SELECTION.md**: NEW! Complete guide for all 12 features
  - **WIFI_GUIDE.md**: Wi-Fi development patterns
  - **CONFIG_GUIDE.md**: Configuration management
  - **PROJECT_STRUCTURE.md**: File organization
- **templates/**: Ready-to-use project templates
- **configs/**: Wi-Fi mode configurations
- **features/**: Modular feature overlay
│   ├── CHECKLIST.md
│   └── IMPROVEMENT_GUIDE.md
│
└── examples/               # Reference implementations
```

## 📖 Documentation

- **SKILL.md**: Quick reference guide (load this for overview)
- **guides/**: Comprehensive guides (reference when needed)
- **templates/**: Ready-to-use project templates
- **configs/**: Wi-Fi overlay configurations

## 🔄 Workflow

```
Generate → Develop → Review → QA Report → Fix → Improve Templates → Generate
```

## Token Efficiency

- **SKILL.md**: ~2,000 tokens (auto-loaded)
- **Guides**: 5,000+ tokens each (loaded on demand)
- **Templates**: Accessed as needed

**Totafeature selection**: See `guides/FEATURE_SELECTION.md` - complete details on all 12 features
**For l auto-load**: ~2,000 tokens vs previous ~35,000 tokens = **94% reduction!**

## 🆘 Getting Help

**For generation**: See `SKILL.md` and `guides/PROJECT_STRUCTURE.md`  
**For Wi-Fi**: See `guides/WIFI_GUIDE.md`  
**For review**: Use `ProductManager/ncs/review/CHECKLIST.md`  
**For configs**: Check `guides/CONFIG_GUIDE.md`

Start with `SKILL.md` for the complete quick reference!
