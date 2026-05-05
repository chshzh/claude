---
title: Memfault Version Requirements
created: 2026-05-05
updated: 2026-05-05
type: concept
tags: [memfault, versioning, constraint]
sources: [https://docs.memfault.com/docs/platform/software-version-hardware-version]
confidence: high
---

# Memfault Version Requirements

## Software Version Rules

| Rule | Detail |
|------|--------|
| Max length | 128 characters |
| Allowed characters | `a-z A-Z 0-9 - _ . + : ( ) [ ] / ,` and spaces |
| `v` prefix | Technically allowed but **not recommended** — use bare numbers |
| Ordering algorithm | [natsort](https://pypi.org/project/natsort/) with `.` → `~` tweak |

## Ordering Algorithm

Memfault uses natsort with a custom tweak:

```python
natsorted(items, key=lambda x: x.replace(".", "~") + "z")
```

This correctly handles:
- Numeric padding: `1.0.9` < `1.0.10` < `1.0.12` ✓
- Pre-release suffixes: `1.0.0-rc.1` < `1.0.0` ✓
- Four-segment versions: `3.3.0.4` < `3.3.0.10` ✓
- Cross-NCS-version: `3.3.0.4` < `3.3.1.0` ✓

## `v` Prefix Warning

The `v` character is in the allowed set, but Memfault never uses it in its own
examples and documentation. The natsort algorithm treats `v3.3.0.1` and `3.3.0.1`
as different strings — the `v` prefixed versions sort lexically before the bare
ones, which can cause unexpected OTA behavior.

**Convention:** strip `v` from the firmware string; keep `v` on git tags only.
See [[ncs-app-versioning]] for the full convention.

## OTA Implications

Memfault uses the ordering to determine if a device needs an update:
- `current < target` → offer update
- `current >= target` → no update

Four-segment versions sort correctly, so `3.3.0.4 < 3.3.1.0` will correctly
prompt devices on `3.3.0.x` to update to a `3.3.1.0` release.

## Hardware Version Rules

Same character set and length limit as Software Version. The first Hardware
Version reported by a device **cannot be changed automatically** — it sticks.
Update via the Memfault UI if needed.

## Recommended Scheme (Memfault's own)

Memfault recommends SemVer 2.0 with exactly three groups for releases:
`1.0.0`, `1.0.0-rc.1`, `1.0.0-nightly.20220123`.

Four-segment (`3.3.0.1`) deviates from strict SemVer but is fully supported
by natsort and is the chosen convention for NCS projects here. See [[ncs-app-versioning]].

## Testing Version Order Locally

```bash
pip install natsort
python3 - <<'EOF'
from natsort import natsorted
versions = ["3.3.0.1", "3.3.0.9", "3.3.0.10", "3.3.1.0", "3.3.0.4"]
ordered = natsorted(versions, key=lambda x: x.replace(".", "~") + "z")
print(ordered)
# ['3.3.0.1', '3.3.0.4', '3.3.0.9', '3.3.0.10', '3.3.1.0']
EOF
```
