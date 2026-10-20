# WILDSIGNAL — Pre-production Specification Index v0.1

**Status:** Draft index; not approved release specification. Updated 2026-10-20.
**Branch:** `docs/world-preproduction-v0.1` (open draft PR #2).
**Companion branch:** `docs/creature-bible-v0.1` (open draft PR #1). Creature Bible is NOT yet on this branch or main; check PR review/merge order. **Do not imply all docs are merged to main.**
**Implementation status:** planning materials only, no game engine, gameplay or real performance tests claimed.

## 1. First things to read

| Rank | File | Authority |
|---|---|---|
| 1 | `docs/planning/DECISION_REGISTER_v0.1.md` | current CONFIRMED/OPEN/BLOCKED list, supersession rules |
| 2 | `docs/planning/PRODUCT_VISION_RELEASE_SCOPE_v0.1.md` | proposed product promise, milestones, pricing alternatives |
| 3 | `docs/planning/MASTER_PREPRODUCTION_v0.1.md` | original phase plan / inter-system dependencies |
| 4 | `docs/planning/PRODUCTION_READINESS_AUDIT_v0.1.md` | what is still missing/untested |
| 5 | `docs/architecture/ENGINE_ARCHITECTURE_v0.1.md` | C++/SDL3 platform-neutral engine proposal |
| 6 | `docs/architecture/RENDER_BENCHMARK_PLAN_v0.1.md` | hardware-renderer acceptance and decision evidence |

For planning crosschecks also read `docs/architecture/CODEX_R_AND_D_TASKS_v0.1.md` (future tasks, not assigned).

## 2. Creative and gameplay reference documents

| Area | Source of detailed proposal |
|---|---|
| Overall world tone/locations | `docs/design/WORLD_BIBLE_v0.1.md` |
| Pixel rendering/materials/visual palettes | `docs/design/VISUAL_BIBLE_v0.1.md` |
| First four connected scene topology | `docs/design/FIRST_SIGNAL_ZONE_BLUEPRINT_v0.2.md` |
| Actual prototype fixture positions/portals | `docs/design/maps/FIRST_SIGNAL_BLOCKOUT_v0.2.json` |
| Historical map: deprecated geometry | `docs/design/LEVEL_MAPS_FIRST_SIGNAL_v0.1.md` — **SUPERSEDED** |
| First chapter quests and NPC encounters | `docs/design/NARRATIVE_QUESTS_v0.1.md` |
| Wayfinder/NPC art direction | `docs/design/CHARACTER_NPC_BIBLE_v0.1.md` |
| NPC art metadata and bindings | `docs/design/CHARACTER_ASSET_SCHEMA_v0.1.json` |
| Owned sprite sheet 100-cell catalog | `docs/design/CREATURE_CATALOG_v0.1.json` |
| Sprite segmentation/asset rights | `docs/design/CREATURE_CATALOG_GUIDE_v0.1.md` |
| Proposed nine creature gameplay records | `docs/design/FIRST_NINE_CREATURES_v0.1.json` |
| Proposed encounter/bonding | `docs/design/BONDING_ENCOUNTERS_v0.1.md` |
| Proposed deterministic battle mechanics | `docs/design/BATTLE_BALANCE_BIBLE_v0.1.md` |
| Battle reference math fixtures | `docs/design/BATTLE_TEST_VECTORS_v0.1.json` |
| Gameplay input/events/save overview | `docs/design/GAMEPLAY_SYSTEMS_v0.1.md` |
| Main offline vs online trust model | `docs/architecture/OFFLINE_ONLINE_SYSTEMS_v0.1.md` |
| Screen navigation, control and accessibility | `docs/design/UI_UX_BIBLE_v0.1.md` |
| Normalized screen layouts / inputs | `docs/design/UI_SCREEN_CONTRACTS_v0.1.json` |
| Annotated wireframes (not completed art) | `docs/design/UI_WIREFRAMES_v0.1.md` |
| Unexecuted UI QA cases | `docs/design/UI_ACCEPTANCE_TESTS_v0.1.json` |
| Creature Bible initial project philosophy | `docs/design/CREATURE_BIBLE_v0.1.md` — **ONLY in draft PR #1** |

## 3. True decisions vs spec suggestions

**Actually user-confirmed:** WILDSIGNAL working title; owned original creature art; beautiful/mysterious first-person pixel-art 2.5D tone; C++ lightweight approach; Windows Steam + Android Play; offline single-player and optional online multiplayer; turn-based combat and following creatures; planned IAP and Codex as implementer.

**Proposed:** 6 zones and 18–24 species for candidate 1.0, versus fallback of 4 expanded zones/9 species; F2P cosmetic-only vs premium cosmetic; two trusted progression realms; exact first nine creature names/stats/affinities/encounter mechanics; SDL3/renderer selection; number of avatar bodies, all 26 UI screens and detailed zone layouts.

**Unknown until measured:** actual low-end FPS/RAM/thermals, arches/height/pitch/occlusion fidelity, device OS/API support and final resource budgets.

## 4. Canonical conflict-handling rules

- Stable `WS-###`, `Z##`, `Q###` IDs in versioned schemas/data, not positional row indices in prose. Original sprite sheet ID ownership is established; proposed names/habitats may change.
- Latest v0.2 prototype blockout replaces the historical 15×11 map's **coordinates**; still does not become final art geometry.
- For Battle, `BATTLE_BALANCE_BIBLE_v0.1.md` + typed `FIRST_NINE_CREATURES_v0.1.json` represent current **proposal**. `GAMEPLAY_SYSTEMS_v0.1.md` is an earlier overview; any mismatch must be surfaced, not silently overwritten.
- Core story must be playable with no network. Proposed paid cosmetics/social extras may not become offline prerequisites.
- For any source disagreement between prose, schemas, quests or UI behavior, BLOCK implementation task and create a design review issue. Neither file silently wins while both are drafts.
- Promoting specs to `APPROVED` requires owner signoff and explicit change log; absence of feedback is not approval.

## 5. Near-term approval and development progression

**A. Product lock:** choose monetization, 1.0 content target, friend-co-op features, offline companion online presentation. Update Decision Register D-020..D-024.

**B. Engine proof approval:** approve ENGINE-001/RENDER-001/CI-001 **only as experiments**, not full game. Measure required 2D pixel quality against reference screenshots, FPS/thermals/RAM and practical development process.

**C. Creative finalization:** once renderer feasibility known, freeze 4 first-zone art-ready maps, 3 starter source images and animations, Q001–Q005 quest/test assertions and combat mechanics.

**D. Playable slice:** Codex builds offline 15–20m with saving and input, rebalances from playtest.

**E. Online proof → candidate release:** author secure co-op trust, store commerce, larger content after tests and budget.

**Keep draft PRs open until reviewed.** Main remains the source of released/approved artifacts only after deliberate merges.
