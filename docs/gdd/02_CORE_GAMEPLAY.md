# Chapter 2 — Core gameplay

**Status:** DRAFT 2026-11-02. Sections marked **CHOICE** need the owner's answer; everything else is the recommendation and becomes final when the chapter is locked.
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
- Each Signari has **6 commands**: its affinity's basic attack (free), 4 learned moves (costing energy 0–8, each with a cooldown), and Dodge (Q).
- **Your Signari also acts on its own** when you don't give a command.
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
- **Moves learned by level.** When a Signari learns a 5th move, you choose which one to keep. Forgotten moves can be relearned at a rest spot.
- **No grinding required** (pillar): the story's difficulty follows the levels a normal playthrough reaches.

**CHOICE C3 — evolutions:**

| Option | What it means |
|---|---|
| **A** | Signari evolve at certain levels into a new form. Needs evolution lines among the owner's 100 designs. |
| B | No evolutions. Each Signari changes look slightly at high level (colour accent, bigger aura) instead. |
| C | Decide in chapter 4, after looking at the 100 designs together. |

Recommendation: **C**. It depends on whether the hand-drawn designs already form families.

## 2.7 Wildsignal's signature twist — Resonance

Every great creature game has one hook (fusion in Cassette Beasts, base-building in Palworld). **CHOICE C4:**

| Option | Hook |
|---|---|
| **A (recommended)** | **Resonance meter.** Clean dodges, rolls through attacks and well-timed commands fill a meter. When full, press **E** to unleash the active Signari's **Resonance move**: a unique, flashy signature attack per species. Swapping in a new Signari at a full meter triggers a two-Signari combo. The player's skill powers the Signari, which fits your role on the field. |
| B | **Signal fusion.** Two Signari combine into a temporary fused form. Strong, but very costly in art (models for every pair) and close to Cassette Beasts. |
| C | **Habitat bonding.** Signari get stronger in their home habitat. Simple, but not exciting enough to be the hook. |

## 2.8 Progression and the open world

- **Regions open through the story:** beat a region's guardian, and Old Fen upgrades the Resonance Compass to hear the next region's signal. The upgrade unlocks the path.
- **Within a region you go anywhere.** Wild Signari levels rise toward the edges and the guardian, so the challenge is visible.
- **Compass upgrades** also add new pulse abilities (hear underground signals, see Signari through walls, sense rare ones).
- **Field abilities** (if C1 is chosen) open optional areas inside regions.

## 2.9 Resting, healing, items and saving

- **Rest spots** (cottages, camps, shrines) heal the team, let you change the team, and autosave.
- **Save anywhere** with F5, plus the autosave at rest spots. *(Already built.)*
- **CHOICE C5 — items and money:**

| Option | What it means |
|---|---|
| **A (recommended)** | **No money in 1.0.** Healing items (berries, tonics) are found and foraged in the world; Old Fen gives tools as story rewards. Keeps 1.0 focused. |
| B | Money from fights and a shop in each village town. More classic, more balancing work. |

## 2.10 Difficulty and accessibility

- **Assist options:** slower fights, longer telegraphs, auto-dodge for the player. Nobody misses the story because of reflexes.
- No hard difficulty in 1.0; a challenge mode can come later.

## 2.11 What the old design becomes

- **Watch / help / sit together** → optional research in the field book that speeds up Tune In.
- **Mara Venn's field book** → the Signari log: seen, fought, recruited, habitat notes, plus Mara's questions as optional goals.
- **Friendly fight (F)** → the normal way to start a fight.

## 2.12 Owner choices needed

| # | Question | Recommendation |
|---|---|---|
| C1 | Field abilities from team Signari (cross water, move boulders, light caves)? | Yes |
| C2 | What opens Tune In? | B — worn down opens it, calm makes it faster |
| C3 | Evolutions? | C — decide in chapter 4 |
| C4 | The signature twist? | A — Resonance meter and Resonance moves |
| C5 | Money and shops in 1.0? | A — no money, found items |

## Change log

- 2026-11-02 — First draft.
