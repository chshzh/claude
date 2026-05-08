---
name: chsh-sk-ncs-formatter
description: Format C/C++ source files using the clang-format binary from the active NCS toolchain. Use when the user asks to format code, run clang-format, fix formatting, or clean up a file or folder in an NCS/Zephyr project.
---

# NCS Clang-Format Skill

## Determine the Toolchain

Follow the same toolchain resolution logic as `chsh-sk-ncs-env`:

1. **Check active terminal** — look for an NCS version in the prompt (e.g., `(v3.2.1)`).
   If found, use that version.

2. **If no active terminal**, ask:
   > "Which toolchain version should I use for formatting? (e.g., v3.2.1)"

3. **Look up the bundle ID** from the toolchain version:
   ```sh
   jq -r '.toolchains[] | select(.ncs_versions[]=="<VERSION>").identifier.bundle_id' \
     /opt/nordic/ncs/toolchains/toolchains.json
   ```

4. The clang-format binary is at:
   ```
   /opt/nordic/ncs/toolchains/<BUNDLE_ID>/bin/clang-format
   ```

## Determine the Style File

Priority order:
1. `.clang-format` in the **project root** (if it exists) — clang-format picks it up automatically.
2. Zephyr's style from the SDK:
   ```
   /opt/nordic/ncs/<SDK_VERSION>/zephyr/.clang-format
   ```
   Pass it explicitly: `--style=file:<path>/.clang-format`

If neither is found, ask the user before proceeding.

## Determine the Target

- **Current project in conversation** — use the project root or `src/` directory.
- **User-specified folder** — use exactly the path the user provides.
- **Single file** — format just that file.

If ambiguous, ask: "Format the whole project, a specific folder, or just one file?"

## Run the Formatter

### Format all C/C++ files recursively in a folder
```sh
find <TARGET_DIR> -name "*.[ch]" -o -name "*.cpp" -o -name "*.hpp" | \
  xargs /opt/nordic/ncs/toolchains/<BUNDLE_ID>/bin/clang-format -i
```

### Format a single file
```sh
/opt/nordic/ncs/toolchains/<BUNDLE_ID>/bin/clang-format -i <FILE>
```

### Dry-run (show diff without modifying)
```sh
find <TARGET_DIR> -name "*.[ch]" | \
  xargs /opt/nordic/ncs/toolchains/<BUNDLE_ID>/bin/clang-format --dry-run --Werror 2>&1 | head -60
```

## Common Bundle IDs

| Toolchain Version | Bundle ID   |
|-------------------|-------------|
| v3.2.4            | 185bb0e3b6  |
| v3.2.0, v3.2.1    | 322ac893fe  |
| v3.1.1            | 561dce9adf  |
| v2.8.0            | 15b490767d  |
| v2.6.0            | 580e4ef81c  |

For any other version, always look up `toolchains.json`.

## Notes

- clang-format edits files **in-place** with `-i`. No backup is created — ensure git is clean first if needed.
- If the project has no `.clang-format`, recommend copying from `<SDK>/zephyr/.clang-format` and setting `ColumnLimit: 80` (Zephyr checkpatch enforces 80, not 100).
- Zephyr macros like `HTTP_RESOURCE_DEFINE` or `ZBUS_CHAN_DEFINE` may need `/* clang-format off/on */` guards if the formatter fights checkpatch style.

## Self-Update Policy

At the **end of each conversation**, review what was discovered and check whether any facts in this skill are new, corrected, or outdated (e.g. new bundle IDs, new clang-format quirks, macro guard patterns).

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
