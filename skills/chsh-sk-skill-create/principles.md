# Skill Design Principles

Core rationale behind the rules in `SKILL.md`. Read this when evaluating whether a skill is well-designed or deciding what to cut.

---

## The Token Tax

Every loaded skill is a tax paid on every session that touches it. Tiers:

| Tier | What loads | Cost | Paid when |
|------|-----------|------|-----------|
| Index | `name: description` for every skill | ~100 tokens per skill | Every session, always |
| Load | Full `SKILL.md` body | ~5,000 tokens | When skill is activated |
| Runtime | `scripts/`, `references/`, `assets/` | Unbounded | Only when the agent reads them |

The index cost is paid by every session before any task starts. A skill whose description is 200 words instead of 50 costs every user, every time, even when they'll never trigger it. A skill body that is 800 lines instead of 400 doubles the context consumed for every conversation that loads it — crowding out other skills and degrading the agent's ability to reason.

When multiple skills are active simultaneously (3–5 is typical), the compound cost is severe. Fluff in one skill makes all other skills worse.

---

## The Pascal Test

> *« Je n'ai fait celle-ci plus longue que parce que je n'ai pas eu le loisir de la faire plus courte. »*
> — Blaise Pascal, Lettres Provinciales, 1657
> ("I have only made this letter longer because I have not had the time to make it shorter.")

Apply to every sentence in a SKILL.md: **"Would the agent get this wrong without this instruction?"**

- If **no** → delete it. The model already knows it.
- If **maybe** → test it. Run the task without the sentence; if output is unchanged, delete.
- If **yes** → keep it, but make it as short as possible.

A skill that is easy to write is probably too long or shouldn't exist. The work of making it short is the real authoring effort.

---

## Routing vs. Documentation

The description field does two completely different jobs depending on how you frame it:

| Framing | Effect |
|---------|--------|
| "This skill does X" | Documents the skill for a human reader. Useless for routing. |
| "Load when the user says Y" | Tells the agent when to activate. This is the correct framing. |

The agent sees a list of `(name, description)` pairs and decides which skill to load. It is doing pattern matching against user intent, not reading a README. A description written as documentation will match the wrong queries, fire off-target, and silently degrade every other skill in context.

**Good description test**: Cover it up. Can you predict from the description alone — without reading the body — exactly which user messages will trigger this skill and which won't? If not, rewrite it.

---

## LLM-Generated Skills

Early research (2025–2026) consistently shows: **self-generated skills provide no benefit on average.** The model cannot reliably author the procedural knowledge it benefits from consuming.

What goes wrong:
- The LLM writes what it already knows → the skill recaps training data → zero new signal
- Generic best practices fill the body → the Pascal test fails on every sentence
- The description describes the skill, not the routing trigger → off-target activation

What works:
- The human provides real examples, real failures, real gotchas
- The LLM formats and structures the human's knowledge
- Every sentence in the final skill can be traced to a real observed failure or a real domain constraint

A skill built entirely by an LLM with no human expertise injected is documentation, not a skill.

---

## Gotchas Are the Highest-Value Content

Gotchas accumulate the most value over time because they encode real failures. They tell the model "this specific thing goes wrong; here's what to do instead."

A gotcha is not a general warning. It is a specific edge case the agent tripped on, written so precisely that it prevents the same failure from recurring.

**Lifecycle:**
- Skill ships with 1–2 seed gotchas (the failures that motivated the skill)
- Each production failure adds one more gotcha entry
- The description is changed only when routing fires incorrectly, and only with new evals to support the change
- Everything else in the skill is relatively stable; the gotchas section grows

Skills that have no gotchas are usually either brand-new (acceptable) or never been tested in production (a signal to add evals).

---

## Self-Update Policy

Every skill must include a `## Self-Update Policy` section at the end of its body.

### Why it matters

A skill frozen at creation time becomes stale. Production conversations surface
edge cases, corrected assumptions, and new domain facts the model does not yet
know. Without a mechanism to capture these, the skill decays relative to the
codebase it serves. The Self-Update Policy is the trigger that turns each
conversation into a compounding improvement — it feeds the Gotchas section and
corrects outdated instructions.

### Canonical snippet

Copy this block verbatim into every skill's `## Self-Update Policy` section:

```
## Self-Update Policy

At the **end of each conversation**, review what was discovered and check
whether any facts in this skill are new, corrected, or outdated.

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
```

### Enforcement

**`chsh-sk-skill-create`** — before finalising any new skill, verify the
`## Self-Update Policy` section is present. If missing, add the canonical
snippet above.

**`chsh-sk-skill-maintain`** — a missing `## Self-Update Policy` section is a
**P1 defect**, same severity as a missing Gotchas section. Flag it in the
Step 1 structure check and Step 6 freshness check.
