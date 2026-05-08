---
name: chsh-sk-ncs-env
description: Set up Nordic nRF Connect SDK (NCS) environment and run west commands. Use when building, flashing, or running any west command for nRF projects, or when working with Zephyr-based Nordic applications.
---

# Nordic nRF Connect SDK (NCS) Environment

## Key Concepts

**SDK version** and **Toolchain version** are separate:
- **SDK (nRF Connect SDK)**: The source code (e.g., v3.2.1, main branch, custom branch)
- **Toolchain**: Build tools like west, cmake, compilers (tied to bundle IDs)

These can differ! Example: NCS main branch + v3.2.1 toolchain.

## Critical Rule

**ALWAYS check if the NCS environment is set up before running any `west` command.**

## nrfutil Command Wrapper (Preferred)

The canonical way to run any `west`, `cmake`, or `ninja` command within the correct toolchain:

```sh
nrfutil sdk-manager toolchain launch --ncs-version=<version> -- <command>
```

Example — build with the v3.2.4 toolchain:
```sh
nrfutil sdk-manager toolchain launch --ncs-version=v3.2.4 -- \
  west build -b nrf7002dk/nrf5340/cpuapp -d /path/to/app/build /path/to/app
```

> **Note:** `nrfutil` itself does **not** need this wrapper — only `west`/`cmake`/`ninja` do.

Legacy alternative (if `sdk-manager` is unavailable):
```sh
nrfutil toolchain-manager launch --ncs-version=<version> -- <command>
```

The **direct PATH approach** (existing workflow below) is equivalent and useful for
multi-command terminal sessions where you want to keep the environment active.

## Board Target Reference

Always use the **fully qualified** board target (`<board>/<soc>[/<variant>]`).

| DK | Board target |
|---|---|
| nRF52840 DK | `nrf52840dk/nrf52840` |
| nRF5340 DK | `nrf5340dk/nrf5340/cpuapp` |
| nRF54L15 DK | `nrf54l15dk/nrf54l15/cpuapp` |
| nRF54L15 DK (TF-M / non-secure) | `nrf54l15dk/nrf54l15/cpuapp/ns` |
| nRF54L10 DK | `nrf54l15dk/nrf54l10/cpuapp` |
| nRF7002 DK | `nrf7002dk/nrf5340/cpuapp` |
| nRF9151 DK (TF-M / non-secure) | `nrf9151dk/nrf9151/ns` |
| Thingy:53 | `thingy53/nrf5340/cpuapp` |

For devicetree overlay filenames, replace slashes with underscores:
`nrf54l15dk/nrf54l15/cpuapp` → `nrf54l15dk_nrf54l15_cpuapp.overlay`

List all valid targets: `west boards` (inside the toolchain environment).

## Build Directory Convention

- **Always pass `-d <path-to-app>/build`** to keep build artifacts inside the app folder.
- Pass `-d` explicitly when running outside the app directory to avoid creating `build/` in cwd.
- Use `-p` (pristine) whenever the board, sysbuild config, overlays, or toolchain changed.

## Workflow for West Commands

When the user requests any west command (`west build`, `west flash`, `west update`, etc.):

### Step 1: Check Terminal Status

Check if there's an active terminal with NCS configured. Look for version in prompt (e.g., `(v3.2.1)`).

### Step 2: Determine SDK and Toolchain Versions

**If NO active terminal OR terminal doesn't show NCS version:**

Ask the user TWO things:
1. **Which toolchain version?** (e.g., "v3.2.1" → determines bundle ID for build tools)
2. **Which SDK path/version?** (e.g., `/opt/nordic/ncs/v3.2.1` or `/opt/nordic/ncs/main`)

Example question: "Which toolchain version would you like to use? (e.g., v3.2.1) And which SDK path? (e.g., /opt/nordic/ncs/v3.2.1 or a custom path)"

**Common scenarios:**
- Same version: Toolchain v3.2.1 + SDK at `/opt/nordic/ncs/v3.2.1`
- Different: Toolchain v3.2.1 + SDK at `/opt/nordic/ncs/main`

**If terminal already shows NCS version in prompt:**
- Proceed directly with the west command

### Step 3: Set Up Environment (if needed)

1. Look up the toolchain bundle ID using the **toolchain version**:
   ```sh
   jq -r '.toolchains[] | select(.ncs_versions[]=="<TOOLCHAIN_VERSION>").identifier.bundle_id' /opt/nordic/ncs/toolchains/toolchains.json
   ```

2. Export PATH, GIT_EXEC_PATH, and source Zephyr environment from the **SDK path**:
   ```sh
   BUNDLE_ID="$(jq -r '.toolchains[] | select(.ncs_versions[]=="<TOOLCHAIN_VERSION>").identifier.bundle_id' /opt/nordic/ncs/toolchains/toolchains.json)"
   export PATH="/opt/nordic/ncs/toolchains/${BUNDLE_ID}/bin:$PATH"
   export GIT_EXEC_PATH=$(ls -d /opt/nordic/ncs/toolchains/${BUNDLE_ID}/Cellar/git/*/libexec/git-core)
   source <SDK_PATH>/zephyr/zephyr-env.sh
   ```

### Step 3.5: Update NCS Main Branch (if using main)

**When using the NCS main branch**, always update to the latest before building:

1. **Pull latest nrf repository changes**:
   ```sh
   cd /opt/nordic/ncs/main/nrf && git pull
   ```

2. **Update all west modules** (fetches dependencies like OpenThread, Matter, Memfault SDK, etc.):
   ```sh
   cd /opt/nordic/ncs/main && west update --narrow -o=--depth=1
   ```

The `--narrow -o=--depth=1` flags speed up the update by doing shallow clones.

**Why this is needed:**
- The main branch manifest changes frequently, adding/removing module dependencies
- Build errors like "Unmet or cyclic dependencies" indicate missing modules
- Running `west update` ensures all required modules (OpenThread, Matter, etc.) are present

### Step 4: Execute West Command

Combine setup and command in a single invocation:
```sh
export PATH="/opt/nordic/ncs/toolchains/<BUNDLE_ID>/bin:$PATH" && \
export GIT_EXEC_PATH=$(ls -d /opt/nordic/ncs/toolchains/<BUNDLE_ID>/Cellar/git/*/libexec/git-core) && \
source <SDK_PATH>/zephyr/zephyr-env.sh && \
cd /path/to/project && \
west <command>
```

Replace:
- `<BUNDLE_ID>`: From toolchain version lookup
- `<SDK_PATH>`: User-specified SDK location (e.g., `/opt/nordic/ncs/v3.2.1` or `/opt/nordic/ncs/main`)

### Sysbuild (NCS v2.7+)

Modern NCS enables **sysbuild** by default for many boards. Sysbuild produces multi-image outputs
(`mcuboot`, `merged.hex`, etc.). Control it explicitly when needed:

```sh
west build --sysbuild    # force sysbuild on
west build --no-sysbuild # force single-image build
```

Check for `sysbuild.conf` or `Kconfig.sysbuild` in the project — these indicate a sysbuild project.
Never force `--no-sysbuild` on a project that ships `sysbuild.conf` without checking with the user.

## Common Toolchain Bundle IDs

| Toolchain Version | Bundle ID   |
|-------------------|-------------|
| v3.2.4            | 185bb0e3b6  |
| v3.2.0, v3.2.1    | 322ac893fe  |
| v3.1.1            | 561dce9adf  |
| v2.8.0            | 15b490767d  |
| v2.6.0            | 580e4ef81c  |

For unlisted versions, always look up in `/opt/nordic/ncs/toolchains/toolchains.json`.

## Example Dialogs

### Same SDK and Toolchain Version
**User:** "Build my project at /opt/nordic/ncs/myApps/my_app"  
**Assistant:** "Which toolchain version would you like to use? (e.g., v3.2.1, v3.1.1)"  
**User:** "v3.2.1"  
**Assistant:** *Uses toolchain v3.2.1 + SDK at /opt/nordic/ncs/v3.2.1, then builds*

### Different SDK and Toolchain Versions
**User:** "Build my project using NCS main branch"  
**Assistant:** "Which toolchain version would you like to use with the main branch? (e.g., v3.2.1)"  
**User:** "v3.2.1 toolchain with SDK at /opt/nordic/ncs/main"  
**Assistant:** *Sets up toolchain v3.2.1, pulls latest nrf repo, runs west update, then builds*

### User Specifies Both Upfront
**User:** "Build with v3.2.1 toolchain and NCS main branch at /opt/nordic/ncs/main"  
**Assistant:** *Sets up environment, pulls latest, runs west update, then builds*

### Full Main Branch Build Sequence
```sh
# 1. Set up toolchain
export PATH="/opt/nordic/ncs/toolchains/<BUNDLE_ID>/bin:$PATH"
export GIT_EXEC_PATH=$(ls -d /opt/nordic/ncs/toolchains/<BUNDLE_ID>/Cellar/git/*/libexec/git-core)

# 2. Pull latest NCS main
cd /opt/nordic/ncs/main/nrf && git pull

# 3. Update west modules
cd /opt/nordic/ncs/main && west update --narrow -o=--depth=1

# 4. Source environment and build
source /opt/nordic/ncs/main/zephyr/zephyr-env.sh
cd /path/to/project && west build -b <board> -p
```

## Troubleshooting

### Permanent fix: toolchain git priority (VS Code extension terminals)

The nRF Connect extension sets `GIT_EXEC_PATH` correctly but **appends** the toolchain
bin to PATH, so `/usr/bin/git` wins over the toolchain git. Add to `~/.zshrc`:

```sh
if [[ -n "$NRF_CONNECT_VSCODE" && -n "$GIT_EXEC_PATH" ]]; then
  _nrf_root="${GIT_EXEC_PATH%%/Cellar/*}"
  [[ -d "$_nrf_root/bin" ]] && export PATH="$_nrf_root/bin:$PATH"
  unset _nrf_root
fi
```

This derives the toolchain root from the already-set `GIT_EXEC_PATH`, is version-agnostic,
and only activates in extension-opened terminals (`NRF_CONNECT_VSCODE=1`).

### `west update` Git errors (manual / one-shot fix)

If `west update` fails with "did not send all necessary objects" or "unknown option detach"
in a terminal where the permanent fix above is not yet in place:

```sh
export PATH="/opt/nordic/ncs/toolchains/185bb0e3b6/bin:$PATH"
hash -r
```

For another toolchain version, swap `185bb0e3b6` for the matching bundle id in `toolchains.json`.

### Git remote helper issues

After exporting PATH, verify git-remote-https resolves:
```sh
command -v git-remote-https || echo "Missing git-remote-https. Current GIT_EXEC_PATH: $GIT_EXEC_PATH"
```

### Avoid nrfutil sdk-manager --shell

Don't use `nrfutil sdk-manager … --shell` in automated terminals—it keeps the VS Code task busy after builds complete.

## Development Environment Setup

### Code Formatting with clang-format

Configure VS Code to automatically format code using Zephyr's clang-format rules.

**Add to VS Code settings** (`~/Library/Application Support/Code/User/settings.json` on macOS):

```json
{
    "editor.rulers": [75,100],
    "editor.formatOnSave": true,
    "editor.defaultFormatter": "ms-vscode.cpptools",
    "C_Cpp.clang_format_path": "/opt/nordic/ncs/toolchains/<BUNDLE_ID>/bin/clang-format",
    "C_Cpp.clang_format_style": "file:<SDK_PATH>/zephyr/.clang-format",
}
```

**Replace placeholders:**
- `<BUNDLE_ID>`: Your toolchain bundle ID (e.g., `322ac893fe` for v3.2.x)
- `<SDK_PATH>`: Your SDK path (e.g., `/opt/nordic/ncs/v3.2.1` or `/opt/nordic/ncs/main`)

**Example for v3.2.1:**
```json
{
    "C_Cpp.clang_format_path": "/opt/nordic/ncs/toolchains/322ac893fe/bin/clang-format",
    "C_Cpp.clang_format_style": "file:/opt/nordic/ncs/v3.2.1/zephyr/.clang-format",
}
```

**Important: Verify ColumnLimit in .clang-format**

After copying Zephyr's `.clang-format` to your project root, verify that `ColumnLimit` is set to `80` (not `100`). Checkpatch enforces an 80-column limit, and mismatches will cause CI failures.

```yaml
# In .clang-format, ensure this line reads:
ColumnLimit: 80
```

If you find `ColumnLimit: 100`, change it to `80` and commit the corrected `.clang-format` to your repository.

**When clang-format and checkpatch disagree:**

Zephyr’s `checkpatch.pl` is the authority. Some macros (for example `HTTP_RESOURCE_DEFINE`, `ZBUS_CHAN_DEFINE`) require braces on the same line, while the shared `.clang-format` prefers Linux-style line breaks. Wrap those regions so VS Code doesn’t reflow them:

```c
/* clang-format off */
HTTP_RESOURCE_DEFINE(button_api_resource, webserver_service, "/api/buttons",
	&button_api_detail);
/* clang-format on */
```

Inside the off/on block you can keep the `checkpatch`-approved layout (same-line braces, wrapped strings, etc.) without fighting the formatter.

### Speed Up Flashing: Disable Verification

**Problem:** Verification during flashing adds significant time, especially for large binaries.

**Solution:** Disable verification in the nrfutil runner.

> ⚠️ **Warning:** This modifies a file inside the SDK installation. The change will be
> overwritten when the SDK is updated or reinstalled. Re-apply it after every SDK update.

**Steps:**

1. Navigate to the runner script in your SDK:
   ```
   <SDK_PATH>/zephyr/scripts/west_commands/runners/nrf_common.py
   ```

2. Find the `_op_program` method (around line 460)

3. Change the verification option:
   ```python
   # Before:
   'options': {'chip_erase_mode': erase, 'verify': 'VERIFY_READ'}
   
   # After:
   'options': {'chip_erase_mode': erase, 'verify': 'VERIFY_NONE'}
   ```

**Example path for v3.2.1:**
```
/opt/nordic/ncs/v3.2.1/zephyr/scripts/west_commands/runners/nrf_common.py
```

### Git Pre-Push Hook for Code Quality

Automatically validate code style and commit messages before pushing to prevent CI failures.

**Setup:**

1. **Create the pre-push hook** in your project:
   ```bash
   nano .git/hooks/pre-push
   ```

2. **Add this content** (from [Zephyr docs](https://docs.zephyrproject.org/latest/contribute/style/index.html)):
   ```bash
   #!/bin/sh
   remote="$1"
   url="$2"
   
   z40=0000000000000000000000000000000000000000
   
   echo "Run push hook"
   
   while read local_ref local_sha remote_ref remote_sha
   do
       args="$remote $url $local_ref $local_sha $remote_ref $remote_sha"
       exec ${ZEPHYR_BASE}/scripts/series-push-hook.sh $args
   done
   
   exit 0
   ```

3. **Make it executable:**
   ```bash
   chmod +x .git/hooks/pre-push
   ```

**What it checks:**
- Code style validation using `checkpatch.pl`
- Commit message format
- Signed-off-by verification

**Environment requirement:**

Ensure `ZEPHYR_BASE` is set (automatically set when you source `zephyr-env.sh`):
```bash
export ZEPHYR_BASE=<SDK_PATH>/zephyr
```

**Testing the hook:**
```bash
# Make a test commit with sign-off
git commit --signoff -m "test: Validate pre-push hook"

# Push (hook will run automatically)
git push
```

The hook will display "Run push hook" and any validation errors before allowing the push.

## Device Discovery and Selection

Always start device operations with:
```sh
nrfutil device list
```

- **No devices found**: ask the user to connect/power the board, then retry.
- **One device**: use it automatically.
- **Multiple devices**: ask the user which to use (board name, SEGGER serial number, or explicit choice).
- Pass `--dev-id <segger_id>` to `west flash` / `west debug` when multiple boards are connected.

## VCOM Port Defaults (UART Logging)

Default UART settings: 115200 8N1, no flow control.

Nordic DKs expose multiple VCOM ports via the J-Link interface MCU. The `zephyr,console` chosen
node determines which UART carries application logs (default: `uart0` → Serial Port 0).

| DK | Serial Port 0 (first ttyACM/COM) | Serial Port 1 (second ttyACM/COM) |
|---|---|---|
| nRF52840 DK | Application logs (`uart0`) | Unused by default |
| nRF5340 DK | App core logs (`uart0`, VCOM0) | Network core / secondary (`uart1`) |
| nRF54L* DK | App logs (`uart1`, pins P1.04–P1.07) | Secondary (`uart0`, pins P0.00–P0.03) |

If no logs appear on the first port, try the second and reset the board.
Applications can reassign `zephyr,console` via an overlay — do not assume the default mapping.

### DTR Requirement

Nordic DK UART lines are tri-stated until the terminal asserts **DTR (Data Terminal Ready)**.
If the port opens but no data appears after a board reset, the terminal tool is not asserting DTR.

## Notes

- **Toolchain** provides build tools (west, cmake, compilers) via bundle ID
- **SDK path** provides source code and zephyr-env.sh
- Multiple terminals can have different PATH exports simultaneously
- Prompt may not change after PATH export; rely on the explicit bundle/version you set
- To switch versions, repeat setup with new bundle ID and/or SDK path (no terminal restart needed)
- When user says "NCS v3.2.1", clarify if they mean toolchain, SDK, or both

## Self-Update Policy

At the **end of each conversation**, review what was discovered and check whether any facts in this skill are new, corrected, or outdated (e.g. new bundle IDs, new toolchain versions, new git workarounds, environment tips).

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
