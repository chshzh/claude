---
name: chsh-sk-txt-review
description: Use when drafting, polishing, or reviewing emails, messages, or responses to colleagues and customers — especially to remove AI-sounding phrasing and make text sound human.
---

# Professional Text Review

## Core Rules

1. **Active voice.** Every sentence needs a human subject doing something. No passive constructions. No false agency — inanimate objects don't perform human actions ("the complaint becomes a fix"). Name the person.
2. **Cut filler.** Remove throat-clearing openers, emphasis crutches, and all adverbs. See [references/phrases.md](references/phrases.md).
3. **Break formulaic structures.** No binary contrasts ("not X, it's Y" — just say Y), no negative listings, no dramatic fragmentation. See [references/structures.md](references/structures.md).
4. **Be specific.** No vague declaratives ("The stakes are high"). Name the specific thing.
5. **Trust readers.** State facts directly. Skip softening, justification, hand-holding.
6. **Vary rhythm.** Mix sentence lengths. Two items beat three. End paragraphs differently. No em dashes.

## Business Jargon → Plain Language

| Avoid | Use instead |
|-------|-------------|
| Navigate (challenges) | Handle, address |
| Moving forward | Next |
| Circle back | Return to, revisit |
| On the same page | Aligned, agreed |
| Deep dive | Analysis, examination |
| Lean into | Accept, embrace |
| Game-changer | Significant |
| Double down | Commit |
| As per my last email | As I mentioned |
| ASAP | Specific timeframe |
| Obviously / Clearly | Remove — condescending |
| Just (minimizing) | Remove |

## Pre-Delivery Checks

Before delivering text:
- Any adverbs (-ly words, "really," "just," "honestly")? Kill them.
- Any passive voice? Find the actor, make them the subject.
- Any throat-clearing ("Here's the thing:", "Let me be clear", "I'll be honest")? Cut to the point.
- Any "not X, it's Y" contrasts? State Y directly.
- Any vague declaratives ("The situation is complex")? Name the specific thing.
- Three consecutive same-length sentences? Break one.
- Em dash anywhere? Replace with comma or period.

## Scoring

Rate 1–10 on each before delivering:

| Dimension | Question |
|-----------|----------|
| Directness | Statements or announcements? |
| Rhythm | Varied or metronomic? |
| Trust | Respects reader intelligence? |
| Authenticity | Sounds human? |
| Density | Anything cuttable? |

Below 35/50: revise.

## Reference Files

- [references/phrases.md](references/phrases.md) — banned phrases by category (throat-clearing, jargon, adverbs, meta-commentary)
- [references/structures.md](references/structures.md) — structural anti-patterns with fixes (binary contrasts, false agency, passive voice, rhythm)

## Gotchas

- **Structure without voice**: Correctly structured text can still be full of AI tells ("Moving forward, let's circle back and navigate this together"). Run pre-delivery checks even when structure looks right.
- **False agency creep**: "The fix was deployed", "the decision emerged", "the update will address" — name who deployed, decided, or will address it.
- **Adverb hedging as sincerity**: "I genuinely believe", "I simply wanted to", "I truly appreciate" — adverb-driven filler that sounds like AI performing emotion.
- **"Power words" are themselves a slop pattern**: Coaching to use "recommend" over "think" or "ensure" over "try" is a tell. Use the most natural word.
- **Passive voice in apologies**: "Mistakes were made" / "This was overlooked" — name who made the mistake or overlooked it.

## End-of-Conversation Checks

Before closing, run these two steps:

1. **Duplicate section check** — scan for any repeated `##` headings in this file. Keep the richer instance; remove stubs (empty TODO entries or generic boilerplate).
2. **Reference updates** — if new phrases or structural anti-patterns were observed, add them to `references/phrases.md` or `references/structures.md` with real examples, not hypotheticals.

## Self-Update Policy

At the **end of each conversation**, review what was discovered and check
whether any phrases, structural anti-patterns, or skill rules are new, corrected,
or outdated.

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
