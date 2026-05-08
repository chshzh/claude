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

**Application architecture**: <Project Name> application code uses a **<SMF+Zbus modular | multi-threaded> architecture**.
Each application feature lives in its own module under `src/modules/`.
<All inter-module communication is exclusively through Zbus channels. | Modules communicate via queues/semaphores.>
Application modules initialize through `SYS_INIT` at priority-ordered boot time.

> **Scope note**: The architecture pattern describes **application code only**.
> External libraries (Memfault SDK, Wi-Fi driver, BLE stack, etc.) run in their own
> internal threads and are not subject to this architecture. Application-level wrapper
> modules (`app_<lib>/`) provide the interface boundary — calling library APIs,
> implementing required callbacks, and integrating library events into the app's
> Zbus message flow.

<Brief description of what this version adds or changes vs. prior version.>

---

## Module Map

```
src/
├── main.c                        ← startup banner, SYS_INIT trigger
└── modules/
    ├── messages.h                ← all Zbus message structs (shared)
    │
    │   ── Application modules (SMF+Zbus / multi-threaded) ──
    ├── <module_a>/               ← <brief role>
    ├── <module_b>/               ← <brief role>
    │
    │   ── Library wrapper modules ──
    └── app_<lib>/                ← wraps <lib name>; calls lib API + implements callbacks
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

## External Libraries

Libraries whose internal threading is **not** controlled by application code.
Each must have a corresponding wrapper module in `src/modules/app_<lib>/`.

| Library | NCS Kconfig | Internal threads | App wrapper module |
|---------|-------------|------------------|--------------------|
| `<lib_name>` | `CONFIG_<LIB>=y` | <thread name(s) if known> | `app_<lib>/` |

> Remove this section if the project uses no external libraries.

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
