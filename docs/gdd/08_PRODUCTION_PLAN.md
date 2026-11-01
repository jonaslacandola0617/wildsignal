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
