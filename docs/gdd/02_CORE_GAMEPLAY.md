# Chapter 2 — Core gameplay

**Status:** LOCKED 2026-11-02 by the owner. The CHOICE notes below are kept to show the options considered; the decisions are in 2.13.
**Built on:** [Chapter 1](01_VISION_AND_PILLARS.md) (locked) and decisions D-070 to D-077.
**What exists today** is noted in each section, so it's clear what's new work and what's already built in `experiments/hearthgrove_emaceart`.

## 2.1 The loop at three scales

| Scale | Loop |
|---|---|
| **A minute** | Spot a Signari → fight it → wear it down → tune in to recruit it, or win and move on. |
| **An hour** | Explore a region, fill the field book, recruit new Signari, grow the team, find secrets, push the story to the region's guardian. |
| **The whole game** | Six regions, each opened by beating the previous guardian and upgrading the Resonance Compass, until the mystery of the silent signal is solved. |

## 2.2 Exploration

**Exists:** walking (WASD + mouse) with a raised camera, sneaking (Shift), interacting (E), companions following in a trail, resting at cottages, the field book (B), saving (F5).

**Recommended:**
- **Visible wild Signari only.** No random encounters in grass. Every Signari is in the world, with a behaviour the player can read:
  - *Skittish* — flees when you get close; sneak up on it.
  - *Curious* — wanders toward you and watches.
  - *Territorial* — guards a spot and attacks if you come too close.
  - *Roaming* — travels a route, sometimes in small groups.
- **The Resonance Compass (Q)** sends a pulse. Nearby signals show as rings: Signari (including hidden ones underground or in water), secrets and story clues. It's the open-world "radar" and keeps the HUD clean.
- **Points of interest** in every region: shrines that restore the team, viewpoints, hidden items, small puzzles, rare Signari with special conditions (night only, rain only, after a story event).
- **Field abilities from the team** (*CHOICE C1*): a Tide Signari in the team lets you wade across shallow water, a Stone one moves boulders, a Lumen one lights dark caves. This gives team choices a purpose outside fights and gates parts of the open world without locked doors.

## 2.3 Starting a fight

**Exists:** pressing F near a wild Signari starts a fight right where you stand, inside a 9.5 m ring; stepping out of the ring ends it.

**Recommended:**
- F starts a fight with a calm or skittish Signari; territorial and aggressive ones start it themselves when you get too close.
- The fight happens **in place**, with no screen transition: the ring appears on the ground, your active Signari steps in.
- Leaving the ring = escaping. No penalty, but the wild Signari stays wary for a while.

## 2.4 Fighting

**Exists, and stays:**
- One Signari fights at a time; **Tab** swaps to another team member (with a short cooldown).
- **Your Signari also acts on its own** when you don't give a command.

**Changed by the owner (2026-11-02) — a MOBA-style kit.** Every Signari has exactly:

| Slot | What it is |
|---|---|
| **Basic attack** | Free, quick, always available; shaped by its affinity (Leaf Flick, Splash, Spark…) |
| **Skill 1, 2, 3** | Its three signature skills, each on its own cooldown. A kit mixes roles: damage, control (slow, knock back) and support (guard, heal, speed) |
| **Ultimate** | One big, flashy move unique to the species, charged by the Resonance meter (2.7) |
| **Dodge** | Not a skill and not a slot: an innate move every Signari can always do (short cooldown), like the Wayfinder's roll |

This replaces "4 learned moves". **CHOICE C6 — energy:** today moves also cost energy (0–8). Recommendation: **remove energy and use cooldowns only**, as MOBAs do. It's easier to read in a real-time fight, and the ultimate already has its own meter.
- **The Wayfinder** moves, rolls (Space) and stays out of the way of attacks; getting hit lowers your **composure**. At zero composure you lose the fight. You never attack directly (D-074).
- Statuses: Guard, Regen, Burn, Slow, Focus. Affinity matchups (Strong / Resisted).

**Recommended additions:**
- **Readable telegraphs on every attack:** a ground marker fills up during the wind-up, so dodging is about reading, not reflexes alone. *(Already partly built.)*
- **Losing a fight:** you wake at the last rest spot with the team healed. Nothing is taken from you. Pillar 4 — kind at heart.
- **Fainted team members** stay fainted until you rest or use a healing item.

## 2.5 Recruiting — the Resonance Compass

**Exists:** Reach Out (R) works when the wild Signari is worn down to 40% health or lower, calm (Spirit ≤ 30) and within 2.6 m. You hold still for about 1.2 s; a hit breaks it; knocking it out ends the chance.

**Recommended:** recruiting becomes **Tune In**. When the wild Signari is worn down, you raise the Resonance Compass and lock onto its signal. A ring closes around it while you hold steady; when the ring closes, its signal is recorded and it joins your team. Same rules as today, new story and look. No thrown object, no trap.

**CHOICE C2 — what opens Tune In:**

| Option | Rule | Effect |
|---|---|---|
| A | Worn down **and** calm (today's rule) | Rewards gentle play; strong-move-only players can never recruit |
| **B (recommended)** | Worn down opens it; **calm makes it faster** (calm ≈ 1.2 s, agitated ≈ 3 s) | Every play style can recruit; gentle play is rewarded, not required |
| C | Worn down only | Simplest; calming no longer matters |

**Also recommended:**
- **One of each species.** Your collection is the 50 species. *(The save system already enforces this.)*
- **Team of 3, the rest in reserve.** Change the team at any rest spot. *(Already built.)*
- **Earlier research helps:** having watched a species before (the old befriending acts) makes its Tune In faster. Optional, never required.
- **Special Signari** (region guardians, rare ones) can't be recruited the normal way; each has its own condition, set in chapter 4.

## 2.6 Growth

**Exists:** Signari have a level (1–30) and stats from their species; there's no experience or move learning yet.

**Recommended:**
- **Levels 1–50 with experience.** Every team member who took part in a fight earns experience; the reserve earns a small share so nobody falls far behind.
- **No grinding required** (pillar): the story's difficulty follows the levels a normal playthrough reaches.

**CHOICE C7 — how kits grow with levels:**

| Option | What it means |
|---|---|
| **A (recommended)** | **Fixed kit, skills rank up.** Each species always has the same basic, 3 skills and ultimate, like a MOBA hero. Levels raise stats and rank skills up (more damage, shorter cooldown, extra effect). The ultimate unlocks at a set level. Every species keeps a clear identity, and it's far less work for 50 species. |
| B | **Fixed kit plus one branch choice.** As A, but at two levels you pick one of two upgrades for a skill (for example wider area or longer slow). More depth, more balancing. |
| C | **Learn and swap skills** from a pool, Pokémon style. Most flexible, but weakens each species' identity and multiplies balancing. |

**CHOICE C3 — evolutions:**

| Option | What it means |
|---|---|
| **A** | Signari evolve at certain levels into a new form. Needs evolution lines among the owner's 100 designs. |
| B | No evolutions. Each Signari changes look slightly at high level (colour accent, bigger aura) instead. |
| C | Decide in chapter 4, after looking at the 100 designs together. |

Recommendation: **C**. It depends on whether the hand-drawn designs already form families.

## 2.7 Wildsignal's signature twist — Resonance

Every great creature game has one hook (fusion in Cassette Beasts, base-building in Palworld).

**Resonance meter — follows from the MOBA kit (2.4).** The ultimate is charged by **your** play: clean dodges, rolls through attacks, skills that land and well-timed commands fill the Resonance meter. When it's full, the active Signari's ultimate is ready. The player's skill powers the Signari, which fits your role on the field.

**CHOICE C4 — the swap combo:** when the meter is full, swapping in a different Signari could trigger a two-Signari combo instead of the ultimate. Recommendation: **yes, but after the first region is playable**: first prove ultimates feel great on their own.

(Rejected alternatives, kept for the record: signal fusion — very costly in art and close to Cassette Beasts; habitat bonding — too quiet to be the hook.)

## 2.8 Progression and the open world

- **Regions open through the story:** beat a region's guardian, and Old Fen upgrades the Resonance Compass to hear the next region's signal. The upgrade unlocks the path.
- **Within a region you go anywhere.** Wild Signari levels rise toward the edges and the guardian, so the challenge is visible.
- **Compass upgrades** also add new pulse abilities (hear underground signals, see Signari through walls, sense rare ones).
- **Field abilities** (if C1 is chosen) open optional areas inside regions.

## 2.9 Resting, healing, items and saving

- **Rest spots** (cottages, camps, shrines) heal the team, let you change the team, and autosave.
- **Save anywhere** with F5, plus the autosave at rest spots. *(Already built.)*
- **Money and items — decided by the owner (2026-11-02):**
  - **Money** is earned from fights, selling found materials and quests. Spent at **shops** in villages (healing items, compass parts, seeds, decorations) and on crafting.
  - **Healing items are also found** in the world: berries, herbs and tonics to forage.
  - Never sold for real money (chapter 1: no pay-to-win).

## 2.10 Crafting, farming and building — under discussion

The owner wants these on the table for variety. They can make Wildsignal much richer, and they're also among the most expensive features in any game, so the shape matters more than the yes or no. One idea ties all three together and gives the reserve team a reason to exist:

**The Wayfinder's Homestead.** In the first village, the player gets a small plot of their own.
- **Farming:** grow berries, herbs and flowers in plots. Harvests become healing items, crafting materials and money.
- **Crafting:** a workbench turns found and farmed materials into healing items, compass parts (pulse range, faster Tune In) and decorations.
- **Building:** place and arrange buildings and furniture on the homestead: a barn, a greenhouse, a pond, paths, fences, decorations. Cozy, and a natural home for optional cosmetics later.
- **Signari help out:** reserve Signari live at the homestead and help by affinity. Flora ones make crops grow faster, Tide ones water, Ember ones run the kiln, Stone ones gather ore. Your collection keeps mattering after the fights.

**CHOICE C8 — how far to take it for 1.0:**

| Option | Scope | Cost |
|---|---|---|
| A | None in 1.0; add after launch | Lowest; 1.0 is purely adventure |
| **B (recommended)** | **The Homestead:** farming plots, a crafting bench and placing buildings and decorations **on the homestead only**, with reserve Signari helping | Medium; one contained area, so it's testable and fits the low-end budget |
| C | Build anywhere in the open world (camps, bases), Palworld style | Very high: placement anywhere, saving it across streamed chunks, Signari jobs and AI everywhere; at odds with chunk streaming on a 4 GB PC |

The recommendation keeps the adventure as the heart of the game (pillars 1 and 2) and makes the homestead the cozy home you come back to between regions.

**Decided (owner, 2026-11-02): B, with relocation.** The homestead can be **moved, but only to designated homestead sites**, never anywhere:
- Each region has one or more fixed **homestead sites** (a cleared meadow, a cove, a ruin courtyard), found by exploring.
- Moving packs the whole homestead up and rebuilds it at the chosen site with the same layout: plots, buildings, decorations and resident Signari. The old site goes back to empty.
- Every site has the same plot size and grid, so any layout fits any site. Sites can differ in look (ground, surroundings) and in a small bonus suited to their region (for example faster herb growth in the forest).
- Only one homestead exists at a time. Building stays inside the homestead plot.
- Technically, the homestead is saved as one layout relative to its site, so moving it is a change of anchor, and it streams with the chunk its site sits in.

## 2.11 Difficulty and accessibility

- **Assist options:** slower fights, longer telegraphs, auto-dodge for the player. Nobody misses the story because of reflexes.
- No hard difficulty in 1.0; a challenge mode can come later.

## 2.12 What the old design becomes

- **Watch / help / sit together** → optional research in the field book that speeds up Tune In.
- **Mara Venn's field book** → the Signari log: seen, fought, recruited, habitat notes, plus Mara's questions as optional goals.
- **Friendly fight (F)** → the normal way to start a fight.

## 2.13 Owner choices

All decided by the owner on 2026-11-02.

| # | Question | Decision |
|---|---|---|
| C1 | Field abilities from team Signari? | **Yes** — e.g. Tide wades shallow water, Stone moves boulders, Lumen lights caves |
| C2 | What opens Tune In? | **Worn down opens it; calm makes it faster** (≈1.2 s calm, ≈3 s agitated) |
| C3 | Evolutions? | **Decide in chapter 4**, after reviewing the 100 designs |
| C4 | Swap combo at a full Resonance meter? | **Yes, after the first region is playable** |
| C5 | Money and shops? | **Money and village shops, plus healing items found in the world** |
| C6 | Energy costs on skills? | **Removed; cooldowns only** |
| C7 | How do kits grow? | **Fixed kit per species; skills rank up with levels; the ultimate unlocks at a set level** |
| C8 | Crafting, farming, building? | **The Homestead**, relocatable to designated sites only, with reserve Signari helping |
| — | Kit shape | **Basic attack, 3 skills, 1 ultimate; dodge is innate, not a skill** |

## Change log

- 2026-11-02 — First draft.
- 2026-11-02 — Owner: MOBA-style kit (basic, 3 skills, ultimate, innate dodge); money and shops plus found healing items; crafting, farming and building opened for discussion. Added C6–C8; the Resonance meter now charges the ultimate.
- 2026-11-02 — Owner accepted all recommendations (C1–C4, C6–C8) and added homestead relocation to designated sites. Locked.
