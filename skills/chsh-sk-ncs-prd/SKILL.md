---
name: chsh-sk-ncs-prd
description: Interactive PRD authoring for NCS IoT projects. Guides the Product Manager through creating, extending, or updating PRD.md under docs/. No coding knowledge required. Use when creating a new PRD, adding or changing a feature, or syncing the PRD after code changes.
---

# chsh-sk-ncs-prd — Interactive PRD Workflow

This skill is for the **Product Manager** role. It asks questions in plain language
and maintains a single `PRD.md` in `docs/` with a built-in revision history table.

No Kconfig, no Flash/RAM numbers, no architecture diagrams — those are for the engineer.
The PRD answers: **What should this device do, for whom, and how should it behave?**

Template: [`PRD_TEMPLATE.md`](PRD_TEMPLATE.md)

---

## Step 0 — Detect Mode

Check what exists in the project:

```bash
cat docs/pm-prd/PRD.md 2>/dev/null           # existing PRD (check Revision History)
git log --oneline -10 -- src/ prj.conf CMakeLists.txt    # recent code changes
```

Present the user with the right starting options:

| Situation | Offer |
|-----------|-------|
| No PRD exists yet | **New** |
| PRD exists, no code changes after last PRD revision date | **Add Feature** or **Change Feature** |
| PRD exists, code commits exist after its last revision date | **Update** (code moved ahead of PRD), **Add Feature**, or **Change Feature** |

Ask: *"Which would you like to do?"* — then follow the matching section below.

---

## Mode A — New PRD

> **Existing project with no PRD yet?** This is still Mode A. Before asking the questions
> below, scan the existing code (`src/`, `prj.conf`, `CMakeLists.txt`) to understand what
> features are already implemented, then use those findings to pre-fill answers and confirm
> them with the user rather than starting from a blank slate.

Walk through each section of the PRD template conversationally.
Ask one section at a time. Wait for answers before moving on.

### A1. Project Identity
- What is the name of the product?
- One sentence: what does it do?
- Which NCS version? (e.g. v3.2.4)
- Which development board(s) will it run on?

### A2. Problem & Users
- What problem does this device solve?
- Who will use it? (primary / secondary users)
- What would a user have to do today without this product?

### A3. Wi-Fi Connectivity
Ask which Wi-Fi modes are needed in plain terms:
- Should the device **join an existing Wi-Fi network** (like a laptop does)?
- Should the device **create its own Wi-Fi hotspot** for others to connect to?
- Should the device **connect directly to a phone** without any router?
- Can the user **switch between modes** at runtime, or is it fixed at build time?
- What is the **default mode** on first boot?

### A4. Network Communication
- Should the device serve a **webpage** that users open in a browser?
- Should it expose a **REST API** for other apps to call?
- Should it send data to the cloud via **MQTT**?
- Should it be reachable by a friendly **hostname** (like `mydevice.local`)?
- Any other communication needs?

### A5. Buttons & LEDs
For each board, ask:
- What should each **button** do? (short press, long press, hold at boot)
- What should each **LED** indicate? (connected, connecting, error, etc.)

### A6. Storage & Persistence
- Should the device **remember its Wi-Fi mode** after power-off?
- Should it **remember Wi-Fi credentials** so it reconnects automatically?
- Any other settings that should survive a power cycle?

### A7. Optional Capabilities
- Should the device support **remote monitoring and OTA updates** (Memfault)?
- Should **Wi-Fi credentials be set via a phone app over Bluetooth**?
- Should a developer be able to **type commands** over the serial console?

### A8. Success Metrics
Ask for 3–5 things that prove the product is working well.
Examples: "connects within 30 seconds", "page loads under 2 seconds", "runs for 24 hours without restart".

### A9. Release Criteria
Ask: which P0 requirements must all pass before the product can be released?

### A10. Out of Scope
Ask: *"Are there features or behaviours that you explicitly do NOT want in this release?"*

This is important — a clear "not building" list prevents engineer confusion and scope creep.
Examples: "no cloud connectivity in v1", "no iOS app", "no OTA updates yet".

---

## Mode B — Add Feature

1. Read `docs/pm-prd/PRD.md` and check the current **Revision History**.
2. Show the current feature list (Sections 2 and 3) as a brief summary.
3. Ask: *"What feature would you like to add?"*
4. For the new feature, ask:
   - Who needs it and why?
   - How should it behave from the user's point of view?
   - What are 2–3 acceptance criteria that prove it works?
   - Is it P0 / P1 / P2?
5. Ask: *"Does this change any success metrics, hardware notes, or release criteria?"*
6. Proceed to **Generate Output**.

---

## Mode C — Change Feature

1. Read `docs/pm-prd/PRD.md` and check the current **Revision History**.
2. List the current functional requirements (FR-xxx) with one-line titles.
3. Ask: *"Which requirement would you like to change?"*
4. Show the current text, then ask what should change:
   - The user story?
   - The acceptance criteria?
   - The priority?
5. Collect the updated text.
6. Proceed to **Generate Output**.

---

## Mode D — Update (Code Changed, PRD Stale)

Use this when the developer has made code changes but the PRD has not been updated.

### D1. Find the gap

Read the **Revision History** table in `docs/pm-prd/PRD.md` to get the last PRD revision date.

```bash
PRD_DATE=<last revision date from table>
git log --oneline --since="$PRD_DATE" -- src/ prj.conf CMakeLists.txt Kconfig boards/
```

### D2. Summarise what changed

For each commit or changed area, describe in plain language what the code change means from a user perspective. For example:
- "P2P mode was added — users can now connect directly to a phone without a router."
- "A mode-selection menu was added — users can now hold Button 1 at boot to choose Wi-Fi mode."

Present a table to the user:

| Code change | User-visible effect | Update PRD? |
|-------------|---------------------|-------------|
| Added wifi-p2p support | New connectivity option for users | Yes / No |
| Fixed DHCP timeout | Faster connection (non-visible internally) | No |

### D3. Collect updates

For each agreed change, ask for:
- Updated or new user story
- Acceptance criteria
- Priority

Proceed to **Generate Output**.

---

## Generate Output

After any mode:

1. Update `docs/pm-prd/PRD.md` using `PRD_TEMPLATE.md` as the structure.
2. Add a new row to the **Revision History** table at the bottom of the document:
   ```markdown
   | YYYY-MM-DD-HH-MM | <one-line summary of changes> |
   ```
3. Confirm: *"PRD updated. New revision added to Revision History."*

---

## PRD Quality Checklist

Before handing off, verify the PRD meets these criteria.

### Completeness
- [ ] Every FR has at least 2 acceptance criteria that can be tested without reading the code
- [ ] Success metrics have numeric targets and a measurable method
- [ ] Assumptions are listed (even obvious ones)
- [ ] Out-of-scope items are explicitly named
- [ ] Release criteria (P0 gate) are listed

### Clarity (the "no-code" test)
- [ ] A non-engineer could read this and understand what the device does
- [ ] No Kconfig symbols, Flash/RAM numbers, or architecture terms in functional requirements
- [ ] User stories use "As a… I want to… so that…" format

### Living document
- [ ] Revision History has a new entry for every change
- [ ] PRD Version matches the latest Changelog entry date

### Anti-patterns to avoid
- Over-specifying implementation details (leave HOW to the engineer)
- Writing acceptance criteria that require source code to verify
- Treating the PRD as immutable — update it as the team learns
- Writing the PRD alone without developer/designer input

---

## Handoff Prompt

After saving the PRD, always ask:

> "The PRD is updated at `docs/pm-prd/PRD.md` (see Revision History for this change).
>
> Would you like to hand off to **chsh-sk-ncs-spec** to update the engineering specs,
> or to **chsh-sk-ncs-project** to implement or update the code?
>
> Reply **design**, **implement**, or **no** to stop here."

## Related Skills

| Task | Skill |
|------|-------|
| Translate PRD to engineering specs | `chsh-sk-ncs-spec` |
| Implement code from specs | `chsh-sk-ncs-project` |
| Generate test report from acceptance criteria | `chsh-sk-ncs-test` |
| Full project lifecycle orchestration | `chsh-sk-ncs-workflow` |

## Self-Update Policy

At the **end of each conversation**, review what was discovered and check whether any facts in this skill are new, corrected, or outdated (e.g. new PRD section patterns, handoff question wording, revision history conventions).

If updates are warranted:
1. Collect all proposed changes with a brief rationale for each.
2. Present a summary to the user and ask for approval using `AskQuestion`.
3. Apply approved updates to this file immediately.

Do **not** modify this skill mid-conversation unless the user explicitly asks.
