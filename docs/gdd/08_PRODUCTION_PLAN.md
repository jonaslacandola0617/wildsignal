# Chapter 8 — Production plan

**Status:** LOCKED 2026-11-02 by the owner (all choices accepted as recommended).
**Built on:** chapters 1–7. Team: the owner (direction, review, music) and Claude (code, models, assets, writing drafts), with a zero budget. No dates are promised; work moves milestone by milestone, each one small enough to test and review.

## 8.1 How a milestone runs

1. Start a **fresh session** with the milestone's name and the GDD chapters it needs (cheaper than one long conversation).
2. Claude builds it on a task branch from `develop`, with tests.
3. Claude delivers a playable Windows folder, screenshots and a short moving preview.
4. The owner reviews; fixes happen on the same branch.
5. On approval it's merged into `develop` and marked done here.

## 8.2 Phases and milestones

**Phase 0 — Foundations** (code, using placeholder art)

| ID | Milestone |
|---|---|
| FOUND-001 | `develop` branch; merge the approved work so far (leaf fix, affinity basics, highlights, F6, F8, house support, recruit rule) |
| FOUND-002 | Move the game into the new source tree (chapter 7.2), no behaviour change |
| FOUND-003 | Data files for species, kits and types (chapter 7.3) with validation |
| FIGHT-002 | New kit rules: basic + 3 skills + ultimate, innate dodge, no energy, Resonance meter |
| FIGHT-003 | Tune In: worn down opens it, calm makes it faster; the compass ring and "joined" moment |

**Phase 1 — Look development**

| ID | Milestone |
|---|---|
| LOOKDEV-001 | Three style directions in one test scene; owner picks; style guide written into chapter 6 |
| UI-001 | Code-drawn UI in the chosen style for the exploration and fight HUDs |

**Phase 2 — Region 1 vertical slice** (the first time it feels like the real game)

| ID | Milestone |
|---|---|
| REGION-001 | Region and chunk format plus streaming, proven on the existing corner |
| ART-R1 | Hearthgrove Vale kit in the chosen style: buildings, vegetation, props, terrain |
| SPAWN-001 | Wild Signari from spawn tables, behaviours, fights starting on F or contact |
| CREATURE-R1 | Rigs and animations for the region 1 Signari and starters (12 species) |
| STORY-R1 | Act 1 opening: arrival, Mara, Old Fen and the compass, starter choice, first quests, rival fight, Logwarden |
| ECON-001 | Bag, money, healing items, one shop |
| HOME-001 | The homestead at its Hearthgrove site: plots, one crop cycle, crafting bench, placing a few buildings, resident Signari helping |
| AUDIO-R1 | Region 1 music, fight music, core sound effects |
| GATE | **Playtest of region 1 end to end** on the i3 PC; fix list before moving on |

**Phase 3 — Creature production** (runs alongside phases 2 and 4)

| ID | Milestone |
|---|---|
| CREATURE-MODELS | The 30 new models in batches of 5 (chapter 4) |
| CREATURE-RIGS | Six body-plan skeletons and their 12 animations; fitting for all 50 |
| CREATURE-KITS | Kits for all 50, reviewed in batches |

**Phase 4 — Regions 2 to 6**, one milestone each, following the region 1 recipe: kit, station, spawns, story act, guardian, homestead site, music. A playtest gate after each.

**Phase 5 — Ship**

| ID | Milestone |
|---|---|
| PLATFORM-001 | Android port and S10+ profiling |
| POLISH | Balance pass, accessibility options, settings, controller, credits |
| RELEASE | Licence audit (no placeholders left), store pages, Windows and Android builds |

## 8.3 What comes first

The order above front-loads the decisions that unblock everything else: foundations first (cheap, no art needed), then look development (so no art is made twice), then one complete region before the rest. Region 1 is the proof that the whole game works; if it doesn't feel right, we change course while it's cheap.

## 8.4 Risks

| Risk | What we do |
|---|---|
| 50 Signari with rigs and animations is a lot of work | Shared body-plan skeletons; models before rigs; reviewed in batches; chapter 1 allows fewer at launch if quality would suffer |
| Usage cost for the owner | Fresh session per milestone, smaller models for simple steps, work batched |
| Style isn't liked after look development | Scripted assets can be restyled by changing helpers and rebuilding |
| Low-end performance | Performance check at every milestone, not at the end |
| Licences | Manifest for every asset, audit before release |

## 8.5 Owner choices

| # | Question | Decision (owner, 2026-11-02) |
|---|---|---|
| P1 | The milestone flow (8.1)? | Yes |
| P2 | The phase order (8.2–8.3)? | Yes |
| P3 | Region 1 as the vertical slice and playtest gate (8.2)? | Yes |
| P4 | Start with FOUND-001 (the `develop` branch and merging approved work)? | Yes |

## Change log

- 2026-11-02 — First draft.
- 2026-11-02 — Owner accepted all choices. Locked.
- 2026-11-02 — FOUND-001 done: `develop` created (field-kit UI, leaf fix, house, direction docs, recruit rule, docs/gdd); 18/18 tests pass; PR #71 closed as superseded.
- 2026-11-02 — FOUND-002 (#72), FOUND-003 (#73), FIGHT-002 (#74) and FIGHT-003 (#75) merged into `develop`; 22/22 tests reported. Phase 0 is done.
- 2026-11-03 — LOOKDEV-001 (#76, draft) showed looks A–D in a test plot; LOOKDEV-002 to LOOKDEV-005 followed on branches without PRs.
- 2026-11-04 — Look development decided: the owner chose the LOOKDEV-005 "dream" look (D-089, D-090). The chapter 6 style guide is still to be written from D-089.
- 2026-11-04 — REGION-001 plan approved (D-091). Step 1 (build 015, the village from region and chunk files) followed.
- 2026-11-07 — REGION-001 builds 015 and 016 (chunk streaming, no model cap) merged into `develop` (#80, #81) after the owner's play test; 25/25 tests pass.
- 2026-11-07 — Build 017 measured before building: on the owner's laptop (Intel UHD, mains) shadows cost about 3 ms of a 22 ms GPU frame, and building the shadow maps only about 0.5 ms of that; per-chunk still shadows were put to the owner before any work.
- 2026-11-08 — Build 017 measured on the i3 / 4 GB / Intel UHD PC with a new opt-in GPU pass timer: shadows cost about 3–5 ms of a 27–28 ms GPU frame (shadow pass ~2.5–3 ms, shadow reads in the world ~1–2.5 ms), the same as on the owner's laptop. Most of the frame is the world pass (~65–75%): the ground (ground tiles plus the big lawn pieces, ~half of it) in the village and streaming test, trees at the forest edge. This PC's GPU also slows to about half speed after a few seconds of steady load. No shadow rewrite was made; build 017 plays the same as develop (village pictures byte-identical) and the streaming walk shows the same hitches as develop. Both builds also froze for 18–24 s inside the graphics driver once or twice per walk while the PC had little free memory; not new, to be looked at on its own. Next step proposed: cheaper ground shading, after the owner's OK.
- 2026-11-08 — Build 017B (owner OK): the bottom ground layer is drawn last, so ground hidden under lawn, paths and plants is no longer shaded. Pictures byte-identical to develop in the village (144), forest edge, height test and fights; GPU about 1–1.5 ms (4–6%) less per frame on Intel UHD. Batching the tuft rings was tested and dropped (about 0.5 ms for a large shader change).
- 2026-11-08 — Freeze recorder: frames over 2 s are written with memory numbers (freezes.txt). Build 018 (for the owner's look): trees use the approved R9 bare trunks with dream-look leaves placed from the Pixel Crawler leafy sprites (private build tools in the asset folder); outlines match, inner clumps/tiers do not show at the approved leaf size; forest edge about 2 ms slower.
- 2026-11-08 — Build 018B after the owner's notes on 018 (noisy leaves, loose clumps, branches sticking out, small trees, leaves in the way in first person): only the two biggest trees per family, scaled to 8.5–12 m, crowns from 3 m up, a few big clumps each, no protruding branches; the edge ring's trees are spaced wider (fewer, bigger trees). Forest edge about 4 ms slower than 017B. The owner proposed the Farever concept art (ArtStation) as a reference for world and regions; recorded as a proposal pending the owner's decision-register OK (ideas only, no copying).
- 2026-11-08 — Build 018C, brighter ground (owner picked variant C of three from running-game screenshots; stays in the D-089 dream look): no drifting cloud shade (`cloud` 0.9 → 0), lighter shade greens and soil (grass ramp 0–2, soil ramp 0–1), calmer dirt texture (`amps` 0.30/0.40 → 0.18/0.18), lighter paths (soil ramp lifted one step, new top `#dcc398`), slightly lighter ambient light, softer and narrower edge blur. New optional dream-file key `focus` {start, end, radius} (defaults .10/.42/3 = old blur); old settings give byte-identical pictures. GPU time unchanged (village 21.9 → 21.6 ms, forest edge 27.7 → 26.3 ms). The settings file is private (play folder `assets/pilot/pilot_dream.json`).
