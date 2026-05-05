# GitHub Actions CI/CD for NCS Projects

A guide for setting up and customising the `BUILD_WORKFLOW_TEMPLATE.yml` for any NCS/Zephyr project.

---

## What the workflow does

| Job | Purpose | Runs on |
|-----|---------|---------|
| `set-version` | Reads `revision:` from `west.yml` to pin the toolchain container | Every trigger |
| `build-and-test` | Builds all board configurations in a matrix; extracts ROM/RAM usage; uploads hex + logs as artifacts | Every trigger |
| `validate-docs` | Checks README section headings and required config files exist | Every trigger |
| `static-analysis` | Runs `zephyr/scripts/checkpatch.pl` on changed source files | Every trigger |
| `create-release` | Packages hex files and creates a GitHub Release | Tag push (`v*`) only |

---

## Setup checklist

1. **Copy the template**

   ```sh
   cp /path/to/BUILD_WORKFLOW_TEMPLATE.yml .github/workflows/build.yml
   ```

2. **Replace all `<PLACEHOLDER>` values** — search for `<` in the file and fill in every one:

   | Placeholder | What to put |
   |-------------|-------------|
   | `<REPO_NAME>` | Directory name of your repo (e.g. `nordic-wifi-webdash`) |
   | `<PROJECT_DISPLAY_NAME>` | Human-readable name for job titles and release names |
   | `<Board A display name>` | Label shown in the CI matrix (e.g. `nRF7002DK`) |
   | `build_<board_a>` | Directory suffix for build artifacts (e.g. `build_nrf7002dk`) |
   | `<board_a/cpuapp>` | West build target (e.g. `nrf7002dk/nrf5340/cpuapp`) |
   | `<shield>` | Shield name if applicable (e.g. `nrf7002eb2`), or remove the cmake_args entry |

3. **Verify `west.yml` has the NCS revision**

   ```yaml
   # west.yml
   manifest:
     projects:
       - name: nrf
         revision: v3.3.0   # <-- this line drives the toolchain container tag
   ```

4. **Grant write permission** (for releases)

   The workflow already sets `permissions: contents: write`. If your repo uses a custom token with limited scope, ensure it has `repo` or `contents: write` access.

---

## Customising the build matrix

Add one entry per board. For a project that only supports one board, remove the second entry:

```yaml
strategy:
  matrix:
    config:
      - name: "nRF7002DK"
        build_dir: "build_nrf7002dk"
        board: "nrf7002dk/nrf5340/cpuapp"
        cmake_args: "-DSNIPPET=wifi-p2p"
```

For extra CMake arguments (snippets, shields, Kconfig overrides), add them to `cmake_args`:

```yaml
cmake_args: "-DSNIPPET=wifi-p2p -DSHIELD=nrf7002eb2 -DCONFIG_FOO=y"
```

---

## ELF path and sysbuild

The size report step uses this path by default:

```
<build_dir>/<REPO_NAME>/zephyr/zephyr.elf
```

This matches the sysbuild output layout (Zephyr ≥ 3.x with `sysbuild.conf`). If your project does **not** use sysbuild, change the path to:

```
<build_dir>/zephyr/zephyr.elf
```

---

## Release artifacts

On a tag push (`v1.2.3`), the workflow:

1. Downloads all `build-*` artifacts from the matrix jobs.
2. Renames `merged.hex` files to `<REPO_NAME>-<board>-v1.2.3.hex`.
3. Zips them into `<REPO_NAME>-firmware-v1.2.3.zip`.
4. Creates a GitHub Release with the hex files and a changelog.

The `merged.hex` is automatically assembled from MCUboot + signed app. If MCUboot is not used, the plain `zephyr.hex` is used instead.

---

## README section validation

The `validate-docs` job greps for three required section headings. Update the grep patterns to match your project's README structure:

```yaml
grep -q "Project Overview"      README.md || (echo "Missing: Project Overview" && exit 1)
grep -q "Evaluator Quick Start" README.md || (echo "Missing: Evaluator Quick Start" && exit 1)
grep -q "Developer Info"        README.md || (echo "Missing: Developer Info" && exit 1)
```

---

## Concurrency

The `concurrency` block at the top cancels any running workflow for the same branch or PR when a new commit is pushed. This prevents queuing redundant builds and saves CI minutes:

```yaml
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true
```

To disable, remove the `concurrency` block.

---

## Reference: webdash project workflow

The live example is at [`nordic-wifi-webdash/.github/workflows/build.yml`](../../../../nordic-wifi-webdash/.github/workflows/build.yml).
It was the direct basis for this template.
