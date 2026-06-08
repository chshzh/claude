Behavioral guidelines to reduce common LLM coding mistakes.
**Tradeoff:** These guidelines bias toward caution over speed. For trivial tasks, use judgment.

---

## 1. Think Before Coding

**Don't assume. Don't hide confusion. Surface tradeoffs.**

Before implementing:
- State your assumptions explicitly. If uncertain, ask with AskQuestion tool.
- If multiple interpretations exist, present them - don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.

## 2. Simplicity First

**Minimum code that solves the problem. Nothing speculative.**

- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If you write 200 lines and it could be 50, rewrite it.

Ask yourself: "Would a senior engineer say this is overcomplicated?" If yes, simplify.

## 3. Surgical Changes

**Touch only what you must. Clean up only your own mess.**

When editing existing code:
- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it - don't delete it.

When your changes create orphans:
- Remove imports/variables/functions that YOUR changes made unused.
- Don't remove pre-existing dead code unless asked.

The test: Every changed line should trace directly to the user's request.

## 4. Goal-Driven Execution

**Define success criteria. Loop until verified.**

Transform tasks into verifiable goals:
- "Add validation" → "Write tests for invalid inputs, then make them pass"
- "Fix the bug" → "Write a test that reproduces it, then make it pass"
- "Refactor X" → "Ensure tests pass before and after"

For multi-step tasks, state a brief plan:
```
1. [Step] → verify: [check]
2. [Step] → verify: [check]
3. [Step] → verify: [check]
```

Strong success criteria let you loop independently. Weak criteria ("make it work") require constant clarification.

---

## Wiki

This project has a persistent engineering knowledge base at `.claude/wiki/` (28 pages).

**At the start of every non-trivial session**, orient before acting:
1. Read `.claude/wiki/index.md` to see what experience pages exist
2. Read the relevant page(s) for the task:
   - Building / Kconfig / west? → `concepts/ncs-build-system`
   - WiFi failures / provisioner / P2P? → `concepts/wifi-debugging-patterns`
   - NCS version upgrade? → `concepts/ncs-version-migration`
   - Memfault upload / OTA deploy? → `concepts/memfault-workflow`
   - Which board to target? → `entities/nrf7002dk` or `entities/nrf54lm20dk-plus-nrf7002eb2`
   - Homelab / Hermes / Docker? → relevant concept page in `concepts/`

**After resolving a non-trivial problem**, update the wiki:
1. Edit or create the relevant page with what was learned
2. Append a one-line entry to `.claude/wiki/log.md`

Wiki path: `.claude/wiki/` (relative to workspace root)
Link format: `[page-name](relative-path.md)` — standard markdown, NOT `[[wikilinks]]`

---

## Skills

Agent skills are reusable procedure files (`SKILL.md`) the agent reads before executing a specialized workflow (commit, release, debug, OTA, etc.).

**Full skill index:** `.claude/skills/index.md` — categorized list of all skills with one-line triggers.

**When to use a skill:** When the user's request matches a skill trigger, load its `SKILL.md` with `read_file` before acting. Key skills for this workspace:

| Trigger | Skill |
|---------|-------|
| Build / flash / west command | `chsh-sk-ncs-env` |
| Debug WiFi / firmware crash | `chsh-sk-ncs-3.2-debug` |
| NCS version upgrade | `chsh-sk-ncs-migrate` |
| Memfault OTA release | `chsh-sk-memfault` |
| Git commit / push | `chsh-sk-git-commit` |
| New NCS feature → code | `chsh-sk-ncs-0-workflow` |
| Hardware validation tests | `chsh-sk-ncs-4.2-validation` |

Skills path: `.claude/skills/` (relative to workspace root)

---

<!-- CODEGRAPH_START -->
## CodeGraph

This project has a CodeGraph MCP server (`codegraph_*` tools) configured. CodeGraph is a tree-sitter-parsed knowledge graph of every symbol, edge, and file. Reads are sub-millisecond and return structural information grep cannot.

### When to prefer codegraph over native search

Use codegraph for **structural** questions — what calls what, what would break, where is X defined, what is X's signature. Use native grep/read only for **literal text** queries (string contents, comments, log messages) or after you already have a specific file open.

| Question | Tool |
|---|---|
| "Where is X defined?" / "Find symbol named X" | `codegraph_search` |
| "What calls function Y?" | `codegraph_callers` |
| "What does Y call?" | `codegraph_callees` |
| "What would break if I changed Z?" | `codegraph_impact` |
| "Show me Y's signature / source / docstring" | `codegraph_node` |
| "Give me focused context for a task/area" | `codegraph_context` |
| "See several related symbols' source at once" | `codegraph_explore` |
| "What files exist under path/" | `codegraph_files` |
| "Is the index healthy?" | `codegraph_status` |

### Rules of thumb

- **Answer directly — don't delegate exploration.** For "how does X work" / architecture / trace questions, answer with 2-3 codegraph calls: `codegraph_context` first, then ONE `codegraph_explore` for the source of the symbols it surfaces. Codegraph IS the pre-built index, so spawning a separate file-reading sub-task/agent — or running a grep + read loop — repeats work codegraph already did and costs more for the same answer.
- **Trust codegraph results.** They come from a full AST parse. Do NOT re-verify them with grep — that's slower, less accurate, and wastes context.
- **Don't grep first** when looking up a symbol by name. `codegraph_search` is faster and returns kind + location + signature in one call.
- **Don't chain `codegraph_search` + `codegraph_node`** when you just want context — `codegraph_context` is one call.
- **Don't loop `codegraph_node` over many symbols** — one `codegraph_explore` call returns several symbols' source grouped in a single capped call, while each separate node/Read call re-reads the whole context and costs far more.
- **Index lag**: the file watcher debounces ~500ms behind writes; don't re-query immediately after editing a file in the same turn.

### If `.codegraph/` doesn't exist

The MCP server returns "not initialized." Ask the user: *"I notice this project doesn't have CodeGraph initialized. Want me to run `codegraph init -i` to build the index?"*
<!-- CODEGRAPH_END -->
