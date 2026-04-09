# System Architecture Specification — <Project Name>

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
| YYYY-MM-DD-HH-MM | Initial architecture design |

---

## Overview

<Project Name> uses a **<SMF+Zbus modular | multi-threaded> architecture**.
Each feature lives in its own module under `src/modules/`.
<All inter-module communication is exclusively through Zbus channels. | Modules communicate via queues/semaphores.>
Modules initialize through `SYS_INIT` at priority-ordered boot time.

<Brief description of what this version adds or changes vs. prior version.>

---

## Module Map

```
src/
├── main.c                        ← startup banner, SYS_INIT trigger
└── modules/
    ├── messages.h                ← all Zbus message structs (shared)
    ├── <module_a>/               ← <brief role>
    ├── <module_b>/               ← <brief role>
    └── <module_c>/               ← <brief role>
```

---

## Zbus Channels

| Channel | Message Type | Publisher | Subscribers | Direction |
|---------|-------------|-----------|-------------|-----------|
| `<CHANNEL_NAME>` | `struct <msg_type>` | `<module>` | `<module>` | boot-time / runtime |

### Message Definitions (`src/modules/messages.h`)

```c
/* <Group name> */
enum <type_name> {
    <ENUM_A> = 0,
    <ENUM_B> = 1,
};

struct <msg_type> {
    enum <type_name> type;
    /* add fields */
};
```

---

## Boot Sequence

| Priority | Module | SYS_INIT call | UART marker |
|----------|--------|---------------|-------------|
| 0 | `<module_a>` | `<module_a>_init` | `[<module_a>] initialized` |
| 1 | `<module_b>` | `<module_b>_init` | `[<module_b>] initialized` |

---

## Memory Budget

| Module | Flash (KB) | RAM (KB) | Notes |
|--------|-----------|---------|-------|
| `<module_a>` | ~X | ~Y | |
| `<module_b>` | ~X | ~Y | |
| **Total used** | **~X** | **~Y** | |
| **Headroom** | **>X** | **>Y** | target: >100 KB Flash, >50 KB RAM |

---

## Test Points

| Stage | UART log expected | Pass condition |
|-------|-------------------|----------------|
| Boot | `[main] <project> vX.X starting` | Always |
| Module init | `[<module>] initialized` | Per module |
| Feature ready | `[<module>] <ready state>` | Feature-specific |

---

*(Changelog is maintained at the top of this document.)*
