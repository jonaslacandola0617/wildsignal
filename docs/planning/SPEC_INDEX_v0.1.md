# WILDSIGNAL — Pre-production Specification Index v0.1

**Status:** Authoritative document index; four owner decisions APPROVED FOR PLANNING 2026-10-20, detailed assets and code NOT yet production-locked.
**Branch:** `docs/world-preproduction-v0.1` (open draft PR #2).
**Companion branch:** `docs/creature-bible-v0.1` (open draft PR #1). Creature Bible is NOT yet on this branch or main; check PR review/merge order. **Do not imply all docs are merged to main.**
**Implementation status:** planning materials only, no game engine, gameplay or real performance tests claimed.

## 1. First things to read

| Rank | File | Authority |
|---|---|---|
| 1 | `docs/planning/DECISION_REGISTER_v0.1.md` + `DECISION_REGISTER_v0.1.json` | matching stable decision IDs; D-020/021/023/024 now confirmed planning directions |
| 2 | `docs/planning/PRODUCT_VISION_RELEASE_SCOPE_v0.1.md` | proposed product promise, milestones, pricing alternatives |
| 3 | `docs/planning/MASTER_PREPRODUCTION_v0.1.md` | original phase plan / inter-system dependencies |
| 4 | `docs/planning/PRODUCTION_READINESS_AUDIT_v0.1.md` | what is still missing/untested |
| 5 | `docs/architecture/ENGINE_ARCHITECTURE_v0.1.md` | C++/SDL3 platform-neutral engine proposal |
| 6 | `docs/architecture/RENDER_BENCHMARK_PLAN_v0.1.md` | hardware-renderer acceptance and decision evidence |

For planning crosschecks also read `docs/architecture/CODEX_R_AND_D_TASKS_v0.1.md` (future tasks, not assigned) and `docs/planning/NEXT_MILESTONE_ASSET_ENGINE_READINESS_v0.1.md` (next **planning-only** milestone).

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

**Approved FOR PLANNING 2026-10-20:** free-to-play with optional cosmetic-only purchases (D-020); aspirational 6 zones and 18–24 species/3–5h 1.0 subject to feasibility (D-021); 2-player Android↔Windows co-op at commercial launch (D-023); separate offline-local and online server-verified progression plus feasible visual-only appearance of familiar offline companion in casual co-op (D-024). **Still proposed:** alternate four-zone contingency, exact first nine creature names/stats/affinities/encounter mechanics, SDL3/renderer selection, avatar bodies, screens, map geometry, prices and SKU details.

**Unknown until measured:** actual low-end FPS/RAM/thermals, arches/height/pitch/occlusion fidelity, device OS/API support and final resource budgets.

## 4. Canonical conflict-handling rules

- Stable `WS-###`, `Z##`, `Q###` IDs in versioned schemas/data, not positional row indices in prose. Original sprite sheet ID ownership is established; proposed names/habitats may change.
- Latest v0.2 prototype blockout replaces the historical 15×11 map's **coordinates**; still does not become final art geometry.
- For Battle, `BATTLE_BALANCE_BIBLE_v0.1.md` + typed `FIRST_NINE_CREATURES_v0.1.json` represent current **proposal**. `GAMEPLAY_SYSTEMS_v0.1.md` is an earlier overview; any mismatch must be surfaced, not silently overwritten.
- Core story must be playable with no network. Proposed paid cosmetics/social extras may not become offline prerequisites.
- For any source disagreement between prose, schemas, quests or UI behavior, BLOCK implementation task and create a design review issue. Neither file silently wins while both are drafts.
- Promoting specs to `APPROVED` requires owner signoff and explicit change log; absence of feedback is not approval.

## 5. Near-term approval and development progression

**A. Owner baseline approved:** D-020, D-021, D-023, D-024 confirmed for planning. Resolve remaining detailed economy, co-op instance/visual projection UX, and feasibility-gated scope. Do not treat approving the target as approval of a production schedule.

**B. Engine proof approval:** approve ENGINE-001/RENDER-001/CI-001 **only as experiments**, not full game. Measure required 2D pixel quality against reference screenshots, FPS/thermals/RAM and practical development process.

**C. Creative finalization:** once renderer feasibility known, freeze 4 first-zone art-ready maps, 3 starter source images and animations, Q001–Q005 quest/test assertions and combat mechanics.

**D. Playable slice:** Codex builds offline 15–20m with saving and input, rebalances from playtest.

**E. Online proof → candidate release:** author secure co-op trust, store commerce, larger content after tests and budget.

**Keep draft PRs open until reviewed.** Main remains the source of released/approved artifacts only after deliberate merges.


## 6. 2026-10-20 consistency fix

Earlier `DECISION_REGISTER_v0.1.json` used a conflicting sequence of `D-###` IDs compared with the Markdown register. This was corrected in the current branch: JSON is now a **canonical mirror** of Markdown using the same 42 decision IDs; previous JSON content remains accessible through Git history but its old ID assignments are DEPRECATED. Tools and Codex must use only the current paired register revisions, never mix legacy JSON row meanings.

The user explicitly requested no implementation yet; current engine research tasks remain documentation only. The next milestone is specification/source/test readiness, not starting C++ code.


## 7. Current device and art-source reality (2026-10-20)

Read `docs/planning/HARDWARE_AND_SPRITE_RECOVERY_INTAKE_v0.1.md` before assigning Codex's test environment or requiring layered creature sources. Owner has an Intel i3 10th-gen / 4 GB RAM PC and Samsung Galaxy S10+, but **no confirmed 2 GB test device**, and the original editable Aseprite creature sheet is **unavailable** (broken old desktop). The surviving flattened user-owned 10×10 pixel sheet is acceptable source for carefully reviewed per-creature reconstruction; historical editable layers cannot be assumed recoverable. No program, device benchmark or sprite extraction has occurred as a result of this planning update.


## 8. Windows 7–11 compatibility (D-012; owner 2026-10-20)

Owner's Windows development PC runs **Windows 11**, i3 10th gen/4GB. Game must aim for **Windows 7, 8, 8.1, 10, 11**. **64-bit Windows OS and x86_64 executable only** are now owner-approved; **32-bit Windows and 32-bit executables are excluded**. Windows 7 SP1 x64 remains the proposed oldest test floor. Do not mistake SDL3 source-level compatibility for a tested WILDSIGNAL binary on those OSes. Steam dropped 7/8/8.1 client support in 2024; **legacy standalone distribution is separate from the modern Steam storefront** and not yet established. Full platform/test/distribution specification: `docs/architecture/WINDOWS_7_11_COMPATIBILITY_v0.1.md`. Source of approved target: `docs/planning/DECISION_REGISTER_v0.1.md` D-012, mirrored in JSON. Legacy co-op security and exact OS minimum remain test-gated. This extension contains planning only.


## 9. Closed compatibility question (2026-10-20)

The owner chose **64-bit only**. Do not request further Windows x86 compatibility decisions or create an x86 CI path. Verify Win7 SP1 x64 through Win11 x64 runtime behavior before advertising old-OS support. D-012 and D-042 in the synchronized Markdown/JSON decision registers define the final scope and outstanding proof.


## 10. Current asset/engine-readiness document package (2026-10-20)

| Area | Newly available planning file | Status |
|---|---|---|
| Original artwork indexing | `docs/design/assets/ORIGINAL_SPRITE_SHEET_MANIFEST_v0.1.json` | Source pixels inspected, 100 candidate crop records; no PNG exports |
| Four pilot sprite restoration | `docs/design/assets/PILOT_SPRITE_RECOVERY_SPEC_v0.1.md` | Method/QA planned, not executed |
| Three benchmark hero scenes | `docs/design/HERO_SCENE_TARGETS_v0.1.md` | Visual composition brief, not rendered or approved |
| Devices/legacy Win x64 | `docs/architecture/DEVICE_VALIDATION_MATRIX_v0.1.md` | Requirements listed; ALL TESTS NOT RUN |
| Co-op and cosmetic projection | `docs/planning/COOP_COSMETIC_HANDOFF_v0.1.md` | Product/UX paper flow, no networking/commerce |
| Q001–Q005 references and blockers | `docs/planning/FIRST_SLICE_CONSISTENCY_GATE_v0.1.md` | Some prose contradictions fixed; quest-gate/collision tests still outstanding |

**Owner artwork policy:** The original source PNG bytes are in the user's file Library and are **not published to the public GitHub repo**. Git contains only text manifests/specs and a SHA-256 fingerprint so source provenance can later be checked. The original historical Aseprite editable master is unavailable. Reconstructed editable sprite sources may be created in a later, separately authorized asset-editing milestone.

**Production gate:** No game code, Windows/Android build, transparency export, co-op server or purchase integration has been created by this package. Future ENGINE-001 and sprite cleanup tasks require their own explicit approval.
