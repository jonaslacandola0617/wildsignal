# WILDSIGNAL — Canonical Specification Index v0.1

**Date:** 2026-10-20 | **Status:** Design documents only, pending owner approval.  
**Purpose:** avoid Codex treating conflicting, stale or proposed text as implementation authority.

## Git layout and current version heads
- **main:** initial repository README, not yet production design canon.
- **PR #1, `docs/creature-bible-v0.1`:** `docs/design/CREATURE_BIBLE_v0.1.md`. Contains high-level creature philosophy. Original user-owned art provenance clarified on branch. [Review PR #1](https://github.com/jonaslacandola0617/wildsignal/pull/1).
- **PR #2, `docs/world-preproduction-v0.1`:** all other design docs listed below, including this index. [Review PR #2](https://github.com/jonaslacandola0617/wildsignal/pull/2).
- **No documents were merged into main by this planning task.** Review, reconcile, then merge in deliberate order. Before Codex implementation, create tagged/committed approved design baseline and task-specific commit SHA; don't reference unreviewed floating branch heads.

## Precedence rules
1. `docs/planning/DECISION_REGISTER_v0.1.json` indicates user-confirmed product directions vs proposals vs blocked decisions. **Explicit user decisions outrank drafts.**
2. For precise numeric mechanics/data, structured JSON fixtures + latest relevant detailed spec outrank generic prose, but **only after approved**. Existing drafts are reviewable candidates.
3. For level positions `docs/design/maps/FIRST_SIGNAL_BLOCKOUT_v0.2.json` plus `FIRST_SIGNAL_ZONE_BLUEPRINT_v0.2.md` supersede **all tile coordinates** in `LEVEL_MAPS_FIRST_SIGNAL_v0.1.md`. The older map remains for history only.
4. `FIRST_NINE_CREATURES_v0.1.json` is the proposal for combat stats and type/cost/effects; `BATTLE_BALANCE_BIBLE_v0.1.md` explains resolution semantics; `BATTLE_TEST_VECTORS_v0.1.json` are check fixtures. Any mismatch blocks implementation, requires review.
5. Creature source-art ID/provenance maps from `CREATURE_CATALOG_v0.1.json` (100 original art cells), not from a temporary model index.
6. The UI `UI_SCREEN_CONTRACTS_v0.1.json` controls screen IDs/visual draft, `UI_UX_BIBLE_v0.1.md` controls interaction model. UI must not override battle/local inventory truth.
7. Renderer / camera geometry / memory limits **remain BLOCKED until tests**, so visual draft maps cannot be used to infer a final rendering engine.
8. Business model and commercial content in `PRODUCT_VISION_RELEASE_SCOPE_v0.1.md` are owner-review **proposals**. They do not cancel the explicitly confirmed intent for multiplayer and IAP.
9. Do not commit original reference screenshots from third-party games as game textures/assets.

## Design source index

| Source | Authority / content | Status |
|---|---|---|
| `docs/planning/DECISION_REGISTER_v0.1.json` | Confirmation status of every major decision | Confirmed items + proposed items |
| `docs/planning/PRODUCT_VISION_RELEASE_SCOPE_v0.1.md` | Pitch, launch scope, free/premium options, future roadmap | Proposed |
| `docs/planning/MASTER_PREPRODUCTION_v0.1.md` | Milestones, blocked work, QA gates | Working draft |
| `docs/planning/PRODUCTION_READINESS_AUDIT_v0.1.md` | Remaining gaps and release readiness | Working draft |
| `docs/architecture/ENGINE_ARCHITECTURE_ADR_v0.1.md` | C++ architecture and renderer alternatives | Proposed; ADR-005 blocked |
| `docs/architecture/RENDERING_BENCHMARK_PLAN_v0.1.md` | Low-end Windows/Android profiling recipe | Ready for future testing (no tests executed) |
| `docs/architecture/OFFLINE_ONLINE_SYSTEMS_v0.1.md` | Confirmed offline behavior, proposed online trust model | Mixed |
| `docs/design/WORLD_BIBLE_v0.1.md` | 10 region concepts/world geography/ecology | Proposed |
| `docs/design/FIRST_SIGNAL_ZONE_BLUEPRINT_v0.2.md` | Detailed first-session locations, events, art and triggers | Proposed |
| `docs/design/maps/FIRST_SIGNAL_BLOCKOUT_v0.2.json` | 48x32 four-zone schematic path layouts and portals | Draft validated for basic connectivity, not in-engine |
| `docs/design/LEVEL_MAPS_FIRST_SIGNAL_v0.1.md` | Legacy 15x11 maps | **DEPRECATED for coordinates** |
| `docs/design/VISUAL_BIBLE_v0.1.md` | Palette/material/lighting/asset art rules | Mixed (aesthetic approved, metrics proposed) |
| `docs/design/CHARACTER_NPC_BIBLE_v0.1.md` | Wayfinders and 3 physical NPCs + Sol memory | Proposed |
| `docs/design/CHARACTER_ASSET_SCHEMA_v0.1.json` | Sprite poses and NPC map IDs | Proposed |
| `docs/design/NARRATIVE_QUESTS_v0.1.md` | Q001-Q005 opening story state machine | Proposed |
| `docs/design/GAMEPLAY_SYSTEMS_v0.1.md` | Player/companion/puzzle state UX | Mixed |
| `docs/design/CREATURE_BIBLE_v0.1.md` | High-level creature design & owned art rights; **PR #1 branch** | Mixed |
| `docs/design/CREATURE_CATALOG_v0.1.json` | 100 original sprite IDs and temporary affinity/habitats | Art confirmed, species details proposed |
| `docs/design/CREATURE_CATALOG_GUIDE_v0.1.md` | Sprite extraction and art ledger procedure | Proposed |
| `docs/design/FIRST_NINE_CREATURES_v0.1.json` | Nine starter/wild candidates and 18 moves | Proposed |
| `docs/design/BONDING_ENCOUNTERS_v0.1.md` | Environmental interactions & invitations | Proposed |
| `docs/design/BATTLE_BALANCE_BIBLE_v0.1.md` | Turn resolution, Signal Energy, statuses | Proposed |
| `docs/design/BATTLE_TEST_VECTORS_v0.1.json` | Deterministic expected simulation checks | Calculated only, no engine execution |
| `docs/design/UI_UX_BIBLE_v0.1.md` | 26 screen concepts, controls, responsive UI rules | Proposed |
| `docs/design/UI_SCREEN_CONTRACTS_v0.1.json` | 26 screen IDs and 154 UI component drafts | Proposed |
| `docs/design/UI_WIREFRAMES_v0.1.md` | Text wireframes / art composition concepts | Proposed |
| `docs/design/UI_ACCEPTANCE_TESTS_v0.1.json` | 40 UI acceptance cases | Authored but 0 executed |

## Implementation policy for Codex
- Before each work packet, read decision register, canonical index and latest explicitly approved source version/commit.
- If design is `PROPOSED` or `BLOCKED`, ask for owner approval before implementing as production logic. Technical proof-of-concept may use temporary explicit assumptions if labeled so and isolated.
- No automated rewriting of canon on conflict. Record issue in `docs/planning/DESIGN_CONFLICTS.md` with source paths, concrete example and proposed resolution; wait for signoff.
- Task packet declares `design_commit_sha`, exact paths, owner-approved scope, expected behavior, unit/acceptance criteria and evidence location.
- When spec changes, identify dependent schemas, sprites, tests and UI so changes stay coherent.
- **Definition of readiness:** a plan, code or art task may start when dependencies and relevant design decisions are adequately specified. Whole game does **not** need to be pre-approved to run a renderer experiment.

## Immediate next requests for project owner
1. Approve pricing model and launch content boundaries from Product Vision doc.
2. Decide online/offline companion continuity and whether 2-player co-op in 1.0 is acceptable.
3. Supply low-end Windows hardware specs and Android test model to tune benchmark.
4. Review original creature source masters/permission to put in public Git.
5. Review first 3 starter assignments and 4-map visual reference fidelity.
6. Authorize time-boxed R1/R2 renderer benchmark, *not full game*.
