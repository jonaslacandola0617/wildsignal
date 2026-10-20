# WILDSIGNAL — Production Readiness & Missing-Work Audit v0.1

**Audit date:** 2026-10-20. **Scope:** pre-production documentation only. **Status:** USER REVIEW REQUIRED — not production locked.
**Repository:** `jonaslacandola0617/wildsignal`.
**Current PR status checked via connected GitHub:** [PR #1 Creature Bible](https://github.com/jonaslacandola0617/wildsignal/pull/1) and [PR #2 World/Preproduction](https://github.com/jonaslacandola0617/wildsignal/pull/2) are both OPEN, DRAFT, UNMERGED; neither changes main. They are separate branches. Merge reviewed Creature Bible first (or intentionally consolidate with PR #2), then align remaining references, rather than implying docs are on main.

## 1. Actual completion vs draft inventory

| System | Present now | What is missing | Status |
|---|---|---|---|
| Product identity | Working title WILDSIGNAL, beautiful-but-mysterious retro first-person look, owned creature sheet | trademark/domain/store naming clearance, exact audience/platform pricing | PARTIALLY LOCKED |
| Creature art inventory | Owner asserts ownership of 10×10 pixel sheet; WS-001..WS-100 indexed | original editable masters, extracted transparent PNGs, atlas metadata and in-engine validation | ART OWNED / PIPELINE UNFINISHED |
| Starter/wild ecology | 3 proposed starters + 6 proposed wild species and bonding behaviors | owner signoff on names, types, evolution/capture philosophy and artwork integration | PROPOSED |
| Battle | 18 typed moves, integer-based rule proposal, deterministic sample vectors | formal balancing, live playtest, edge tests, verified C++ sim implementation | PROPOSED |
| World | 10 conceptual locations/biomes, first 4 zones have 48×32 schematic blockout, quest positions | art-ready layout, real navigation geometry, larger region maps, full lore/quests/endgame | PROPOSED |
| NPCs & avatar | 3 physical NPC concept sheets + Sol inscription, avatar customization proposals | approved original production sprites/portrait animation, dialogue script, characterization | PROPOSED |
| UI | UI Bible, 26 draft screen contracts, 154 component bounds, wireframes and 40 unexecuted tests | actual illustrated mockups, responsive/layout input validation, UI implementation | PROPOSED |
| Offline | Offline local adventure + optional online multiplayer approved | completed save/migration contract, account bridge decision, cloud conflict policy | CONNECTIVITY LOCKED / TECH UNCLEAR |
| Online | server-authority principle and draft endpoints | account service, region instancing, party matchmaking, moderation, authoritative storage, reconnect, PvP/trades | CONCEPT |
| Engine | native C++ approach, low-end first, candidates for raycasting/sector software renderer | architecture decisions, actual frame-rate/RAM tests, Android + Windows build proof | BLOCKER |
| Art/audio | style/palette/biome kits and high-level audio identity | finished modular textures, full sprite animation pipeline, soundtrack/SFX provenance/license, accessibility review | PROPOSED |
| Economy/monetization | cosmetics non-pay-to-win aspiration | free-to-play vs premium decision, economy curves, receipts/entitlements/refunds, budget | UNDECIDED |
| Shipping | Steam/Google Play target | min OS/device support, CI signing/builds, store accounts, legal/privacy/support/compliance/review | NOT SPECIFIED FULLY |
| Team/workflow | Codex implementer and GitHub source repo | finalized issue/task dependency graph, definition of done, branches/review rules, production estimates | PROPOSED |

**Important:** 40 written UI tests are *not* 40 passed UI tests; authored maps are *not* validated in engine; 100 drawing cells are *not* 100 production-ready animated species. No production code or benchmark results are claimed by this audit.

## 2. Priorities — what must happen before Codex starts large production

### P0 — Foundational decisions (close before making unbounded implementation choices)
1. **Product/scope lock:** define release business model (free-to-play cosmetics vs premium purchase), audience/age-rating intent, exact launch feature list, vertical slice vs public demo vs version 1.0, postlaunch roadmap and explicit NOT-IN-SCOPE list. Current planned fantasy MMO features may exceed solo indie capacity.
2. **Offline/online ownership contract:** decide two profiles with nontransferable offline inventory vs some clearly limited casual imports. Clarify the *player emotional expectation*: a beloved offline companion should not silently disappear when switching mode. Decide account/online avatar linkage, server-authoritative rewards, and save compatibility rules.
3. **Renderer technical feasibility:** benchmark simple raycaster against sector/portal / software raster options using ONE representative town, forest and ruin scene. Determine camera pitch, height changes, sky, arches, fog, sprite sorting, atlas loading and Android battery impact. Do not pre-author expensive maps requiring unsupported vertical geometry.
4. **Supported device matrix:** actual lowest Windows/integrated GPU and Android test devices/OS versions, current device owner can test, observed memory/CPU/thermals and fps minimum; 2 GB RAM is a goal, not validated.
5. **Design review/approval protocol:** mark authored ideas CONFIRMED / PROPOSED / BLOCKED / DEPRECATED; owner approve design decisions; reconcile PR #1 vs PR #2 cross-references, different level map versions and any story-to-data drift. Freeze IDs and format version conventions.
6. **Asset rights & actual source pipeline:** preserve original user-owned sprite-sheet master in project-controlled storage; choose whether source art lives in private storage or public Git; deliver separated alpha sprites and pivots; inventory future music/fonts/texture licenses.
7. **Combat/encounter approval:** final decisions on first three starter species, 3 additional wild species for vertical slice, recruitment/bond vs battle capture, status rules, party/reserve behavior, compulsory intro guardian balance.
8. **Vertical slice map/story lock:** exact first 4 zone entrances/gates with renderer-compatible geometry, Q001–Q005 events and actor states, no softlocks with any starter, visual hero-shot approval and 15–20 min target.

### P1 — Detailed specs needed before expanding prototype into game
9. **Architecture decision records (ADRs):** C++ version, SDL3 usage vs alternatives, rendering, collision/pathfinding, resource lifetime/memory pools, serialization/schemas/version migrations, threading, audio, input, deterministic sim, build system and common client/server library. Security-related ownership boundaries.
10. **Economy & progression:** quest currency/rewards, creature leveling and XP, bond/evolution requirements, drop tables, creature inventory/reserve/storage, healing/rest costs, anti-grind goals, shops, unlock timing, quest flags.
11. **Narrative/mission content:** complete story outline, factions, mystery reveal plan, NPC arcs, all dialogue for Chapter 0, chapter cadence, environmental lore props and tutorial/localization style.
12. **UX/accessibility:** production UI comps from both Android/Windows, native font and license, optional minimap/quest clarity, touch layout/safe areas/notch, controller support plan (important for Steam), reduced motion/motion sickness, color-blind/contrast, subtitle and audio settings.
13. **Art/animation production bible:** hero shot signoff, tile material source scale, foliage overdraw, creature 4-way vs 8-way sheets, world-to-screen scale, human directional sprite system, particle budgets, character customization composition, asset IDs/build-time atlas.
14. **Audio bible:** music motifs per biome, ambient layers, effects/creature calls, audio and codec budget, legal provenance. Music/SFX can be authored/later commissioned, but placeholders must be tracked.
15. **Multiplayer/network spec:** authenticated identity, lobby, presence, zone state/instances, protocol messages, latency expectations, cheating defenses, trading atomicity, PvP command deadlines/disconnects, bans/reports/block lists, account deletion/export.
16. **Data/telemetry/privacy:** minimum data collection, opt-in analytics, save error logging/crash telemetry, child player policy, moderation, privacy notices and retention. Online name chat and user-created text introduces safety/support obligations.
17. **Deployment and finances:** cloud build pipeline for Android NDK and Windows, signing/key management, Steamworks/Google Play workflows, backend hosting/upkeep and scale, download/update sizes, plan for support/refunds, store fees and projected revenue risk.

### P2 — Before public beta or store release
18. **QA automation and testing strategy:** unit/property tests for combat, schema validation, map reachability/softlocks, save corruption/migration, session disconnect/retry, accessibility, localization, input and performance profiling. Set a real device-test schedule.
19. **Store compliance & legal:** verify current Google Play SDK/API targets and billing policies nearer submission, Steam AI declarations as applicable, age-rating content questionnaires, ratings, trademark/name searches, privacy/data security, asset/software license audit.
20. **Marketing & discoverability:** logo and key art rights, screenshots/trailer, Steam coming-soon page, store tags, gameplay videos, community feedback process, wishlist plan, localization priorities, analytics goals.
21. **Operations after release:** content update cadence, moderation/support, live server incident response, backups/restore, abuse and cheating reports, chargebacks, data retention/deletion, backward compatibility and patch notes.

## 3. Current cross-document reconciliation tasks

- **PR layering:** PR #1 contains Creature Bible; PR #2 contains newer battle/catalog/world/UI data referencing it. Merge order or consolidate with a clear authoritative hierarchy.
- **Map versioning:** 15×11 `LEVEL_MAPS_FIRST_SIGNAL_v0.1.md` is explicitly superseded by 48×32 `maps/FIRST_SIGNAL_BLOCKOUT_v0.2.json` and `FIRST_SIGNAL_ZONE_BLUEPRINT_v0.2.md`. Delete or mark superseded in any Codex task packets.
- **Narrative/quest consistency:** final acceptance test must compare all portal and entity IDs with quest flags, UI actions and C++ event contracts; do not assume prose, JSON and future art all agree.
- **Scope mismatch:** UI contract catalogs 26 screens, including online trade/store; first vertical slice does not require building all 26. Phase labels `vertical_slice` are planning defaults; create a smaller critical-path implementation subset for Codex.
- **Creature content:** first-nine proposal includes six new wild placements; some spawns are NOT in the 48×32 map fixture. Either add approved spawn metadata or keep feature out of initial slice.
- **Starter names:** Glintle, Driftfin, Brumlet are provisional working names. Battle numbers, trust thresholds and map names likewise not owner signed off.
- **Trust semantics:** a player "inviting" a wild creature uses a proposed trust system; ensure combat, collection, first player quest and reserve inventory all share one specification.
- **Store/online tension:** first-person mode doesn't visibly display purchased outfits often; online avatar preview, silhouettes and first-person sleeves should justify cosmetic value without pay-to-win.
- **2 GB target:** target must be tested; exact GPU requirements not known. Android devices normally have mobile GPUs, but goal is low CPU/memory usage, not absolute "no graphics processor".
- **Offline expectations:** clarify whether noncompetitive online co-op can show offline companions. Never suggest that offline-save cryptographic hashes prove competitive/tradable item legitimacy.

## 4. Proposed order for next planning and validation work

**Gate A — Game identity, scope and trust decisions (planning):**
- Publish one Product Vision/Scope Matrix, Decision Register and Versioned Canonical Spec Index. Resolve the 3–5 highest-cost decisions first.
- Select what version 1.0 actually ships with, explicitly noting whether it needs multiplayer on day 1 and how it will be funded.

**Gate B — Engine readiness (Codex minimal technical experiment):**
- Create C++ engine ADR and independent rendering benchmark task. Test representative town, forest, ruin geometry at target phones/Windows.
- Cloud CI builds Windows and Android packages; RAM/CPU/fps/battery measured, not guessed.
- This should happen before generating polished environment art for unsupported geometry.

**Gate C — Playable 15–20-minute offline vertical slice:**
- Freeze first-three creature artwork/source assets, scene layouts, Q001–Q005, deterministic battle rules and small-screen input behaviors.
- Complete one companion-follow puzzle, one battle, one safe save/reload and test all starter choices.
- Playtest for clarity and fun; revise numerical rules and wayfinding from actual observation.

**Gate D — Online technical proof:**
- Decide offline/online trust integration; two clients and one small hub, server-authoritative creature/reward state; reconnect tests and exploit prevention.
- Do not ship trading, PvP ranking or store until full server/entitlement design.

**Gate E — Expand with evidence:**
- Only after performance/gameplay proof build more habitats and species; then balance progression, late-game goals, monetization and storefront operations against budget.

## 5. Minimum "ready for Codex" criteria

A task is ready only if it references a precise **approved spec version**, states in/out of scope, uses stable data IDs, defines platform controls and constraints, enumerates error/recovery behavior, includes an executable acceptance test, and marks unknown performance numbers as **measure** rather than promise. Codex may not fabricate game systems to fill holes silently.

**Reality check:** Full planning before development cannot guarantee error-free implementation. Technical unknowns and player response require experiments. We can eliminate unapproved design guesses, avoid contradictory documentation and keep art coherent by freezing decisions progressively.
