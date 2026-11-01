# WILDSIGNAL — Gameplay direction v0.2: fights first, connected regions

**Date:** 2026-11-02 | **Status:** the four direction choices below are CONFIRMED by the owner; every number and mechanic detail marked *proposed* still needs owner approval before it becomes production logic.
**Supersedes:** befriending-first as the core loop (D-027) and the "first complete encounter" milestone in `DEVELOPMENT_BACKLOG.md`. Earlier documents stay for history.
**Recorded in:** `DECISION_REGISTER_v0.1.md` §10 (D-070 to D-074).

## Why the change

The owner played the pilot and judged the watch / help / invite loop too slow to carry the game. Players come to creature games for the fights (Pokémon, Palworld, Cassette Beasts), so the fights move to the centre and the gentle acts become optional. The world also opens up: the player roams freely, region by region.

## Owner decisions (2026-11-02)

| # | Decision | Effect |
|---|---|---|
| 1 | **Keep real-time fights** (D-006). | No turn-based mode. The existing `fight.cpp` rules, move kits, affinities, swapping and dodging are the base. |
| 2 | **Fight, then recruit.** | You earn a wild Signari by fighting it well, then reaching out at the end. Watching and helping no longer gate recruitment. |
| 3 | **Connected regions.** | The world is several regions joined by short transitions. Only the current region loads, and inside it only the chunks near the player. |
| 4 | **Keep the player's current fight role.** | The Wayfinder moves, rolls, dodges and commands; the player does not attack directly. |

## Core loop

1. **Roam** a region. Wild Signari are visible in the world, each in its own habitat and behaviour.
2. **Engage**: walk up and press F, or get caught by an aggressive one.
3. **Fight in real time**: command your active Signari (slot 1 basic attack plus four learned moves), swap team members, roll and dodge attacks aimed at you.
4. **Recruit**: when the wild Signari is worn down and settled, hold Reach Out while staying unhurt. Success means it joins you. Knocking it out ends the chance for that fight.
5. **Grow the team**: levels and new moves (*system still to design*), type coverage, a carried team of 3 with the rest in reserve.
6. **Progress**: each region ends in a guardian fight that opens the way to the next region.

## Fight-then-recruit (proposed rules)

These rules build on what already exists, so the first version is small.

- **Already built:** Reach Out needs the wild Signari's Spirit at or below `CalmSpirit` (30), the player within `ReachRange` (2.6 m), and an uninterrupted channel of `ReachTicks` (72 ticks); a hit breaks it; a knockout ends the chance. Gentle moves lower Spirit more than strong ones.
- **Proposed change:** add a *worn down* requirement. Reach Out opens when the wild Signari's health is at or below 40% of its full health **and** its Spirit is calm. Damage and calming both matter, and pushing too hard knocks it out and loses it. This is the skill tension of the loop.
- **No dice roll.** Like the rest of the fight rules, recruitment is deterministic: if the conditions hold and the channel finishes, it joins.
- **The HUD must show it:** a recruit marker on the wild Signari's health bar at 40%, a glow once Reach Out is open, and the existing "[R] reach out" prompt.
- **Gentle acts become optional bonuses** (*proposed*): having watched a species earlier shortens its Reach Out channel; having helped it raises the threshold to 50%. Nothing requires them. Some rare Signari could refuse to fight at all and be met only through these acts.

## Connected regions with chunk streaming (proposed design)

- **Regions:** each is a self-contained map (village, forest, coast, caves…) with its own spawn tables, guardian and art set. Moving between regions goes through a gate (path, door, boat) with a short load.
- **Chunks:** a region is a grid of square chunks (*proposed* 32 m × 32 m). Each chunk lists its instances, collision boxes, ground and spawn points.
- **Streaming:** keep the player's chunk and its neighbours loaded (a 3 × 3 window), prefetch the next ring in the direction of travel on a worker thread, and unload what falls behind. Shared models and textures load once per region and are reference-counted across chunks.
- **Budget:** the i3 10th-gen / 4 GB PC and the Galaxy S10+ stay the minimum targets with ~30 FPS. The streaming window is sized by measured memory, not chosen up front.
- **World state:** saved per region as small changes (recruited, defeated guardians, opened gates, collected items), not as whole chunks. Wild spawns are re-created from tables when a chunk loads.
- **First proof:** split the existing Hearthgrove corner into chunks and stream it, before authoring any new region. The visual result must match the current corner.

## What stays

- The real-time fight rules, move kits, affinities and their tests.
- Mara Venn's field book becomes the region log: Signari seen, fought and recruited, plus habitat notes.
- Owner-approved art, the raised camera, the leaf, highlight and focus work, WS_HOUSE_001.
- Offline story first; co-op and cosmetics remain later phases (D-020 to D-024).

## Proposed next milestones

Each is a separate, testable task on its own branch.

1. **FIGHT-RECRUIT-001:** the worn-down + calm Reach Out rule, its HUD marker and tests, in the existing corner.
2. **REGION-001:** region and chunk data format and a streaming loader, proven by chunking the current corner, with memory and frame-time measurements.
3. **SPAWN-001:** wild Signari placed from per-chunk spawn tables, roaming, and starting fights on contact or F.
4. **REGION-002:** the first real region blockout (larger than the corner) with one gate.
5. **TEAM-001:** levels, experience and new moves — design first, then build.

## Open questions for the owner

- Levelling and growth: experience from fights only? Evolutions, or another growth hook?
- Wildsignal's signature twist (for example a "resonance" combo between two Signari), comparable to fusion in Cassette Beasts.
- How many species the first region holds, and which ones.
- Whether the type chart grows beyond today's Flora → Tide → Ember triangle (Wild, Stone and Lumen are currently neutral to everything).
