# WILDSIGNAL — Master Pre-Production Plan v0.1

**Status:** Mixed: confirmed product decisions + proposed implementation and world details; updated 2026-10-20. **Developer:** Codex, *after* approvals. **Intent:** Remove unbounded interpretation, not eliminate the need for implementation testing.
**Other specs:** WORLD_BIBLE_v0.1.md; VISUAL_BIBLE_v0.1.md; CREATURE_BIBLE_v0.1.md in separate draft PR #1.

## 1. What's truly decided
- Original game; working title WILDSIGNAL; original 100-sprite creature sheet authored/owned by the user.
- Real-time first-person pseudo-3D/pixel-art exploration; "beautiful but mysterious" forests and ruins.
- Creatures physically accompany players and meaningfully affect exploration.
- Turn-based combat; Windows/Steam and Android/Google Play, low-end-first; C++ and lightweight native libraries.
- **CONFIRMED 2026-10-20: Offline-first story adventure + optional online multiplayer.** Offline mode must support exploration, creature bonding/battles, quests and local saves with no login or internet. Online is additive: co-op, presence, trading, PvP and account services. Exact cross-mode data bridge is a design proposal pending explicit approval.
- Planned online multiplayer/cross-play and optional non-pay-to-win monetization.
- ChatGPT handles planning and specifications; Codex handles implementation.

## 2. What is NOT yet decided (major gaps and accountable deliverables)

| Domain | Missing decisions | Required specification / approval |
|---|---|---|
| Product | core differentiator, target audience and age rating; **offline play requirement is confirmed** | Product Vision + feature priority |
| Scope | first release size, first playable vs public release, cost/time/solo capacity | Scope Matrix & milestones |
| World | topology, each zone's actual grid/collision, portals, scale, travel time, secrets | World Bible; annotated level maps |
| Narrative | protagonists, factions, NPC identities, quest/cutscene scripts, lore timeline | Narrative Bible and dialogue sheets |
| Creatures | IDs for all 100, species names, official starter choices, ecology, rights ledger, sprite cutouts | Creature Catalog + Art Ledger |
| Combat | damage, stat formulas, turn priority, moves, statuses, PvP rules, retreat/capture/bond logic | Battle Rules v1 + test vectors |
| Economy | rewards, creature acquisition limits, crafting, shops, progression pacing, anti-exploitation | Economy Balance Bible |
| UI/UX | HUD, inventory, menus, touch layouts, onboarding, accessibility, controller support | Screen flow + UI component specs |
| Art | camera treatment, texture pixel density, sprite direction sets, lighting/fog, character customization | Visual Bible + asset templates |
| Engine | raycaster vs other 2.5D renderer, collision, save schema, resource loading | Architecture Decision Records + benchmark |
| Multiplayer | **offline/online model chosen**; server authority, zone capacities, instancing, matchmaking, trade, sync and security decisions outstanding | Offline/Online Systems v0.1; Network/Backend Spec |
| Monetization | purchase SKUs, store platforms, entitlements, refunds, receipts, cosmetic design | Store + Entitlement Spec |
| Audio | licensed music plan, biome cues, feedback, mixing, audio platform budget | Audio Bible |
| Distribution | NDK/Windows CI, store signing, Steamworks, Google Billing and review | CI/CD & Compliance Checklist |
| QA | minimum supported hardware, regression tests, profiling, user testing, acceptance criteria | QA Matrix + release gates |
| Production | dependency register, issue naming, owner signoff, build documentation | Codex Task Packets & Decision Log |

No finite plan will remove every unknown. The desired condition is **no unapproved design choices in implementation work**, with measured performance and clearly scoped tasks; bugs/unforeseen hardware issues still require iteration.

## 3. Progressive specification freezes
1. **Creative lock:** world tone, scope boundaries, core user fantasy, title provisional, art references and ownership ledger.
2. **Vertical-slice lock:** Quay → Mosswake → Liltwood → Echo Hollow topology, battle loop and three starters. Finalize simple maps and quest logic. No other regions implemented.
3. **Renderer proof:** first-person test scene matching essential reference aesthetic; verify Android and Windows CPU/frame/memory budgets and correct collision/occlusion.
4. **Systems lock:** approved deterministic battle spec, saves, companion behavior, minimal NPC interactions.
5. **Networking proof:** two real players in one zone; authoritative state for battles, progress and ownership; reconnect support.
6. **Content expansion gate:** first slice playtested for joy, no dead-ends, pacing, stable memory and input.
7. **Commercial gate:** confirmed original/owned content, final name clearance, accounts/privacy/payments/store-specific requirements, safety and age rating.

Every lock is tracked as Approved / Proposed / Blocked. Changes require recording impacted files/asset types and tests.

## 4. Project decisions required before Codex builds
Prioritized immediate decisions:
- Confirm world/region names and map topology OR designate them as placeholders.
- Starter assignment names to source sprite cells; define if there is battle-to-bond, social befriending, or a mix.
- Choose 1v1 only for vertical slice; determine max active and carried companions.
- Define how map interactions avoid starter-based story locks.
- **Resolved:** Offline exploration, battles, bonding, story and local save are required without internet. **Unresolved:** online progression separation/import and server-verified creature ownership. Compare proposals in OFFLINE_ONLINE_SYSTEMS_v0.1.md; do not implement speculative sync.
- Specify device test matrix; budget is benchmarked not assumed.
- Confirm renderer feasibility with a separate technical experiment before committing map assets.
- Set assets license/provenance policy and decide which source art may be in public GitHub.
- Define how multiplayer works in hubs vs dungeons and how private/world state persists.

## 5. Codex work packet template

Each approved task must contain:
- **ID / one-sentence outcome**
- **Dependencies / relevant approved design doc and exact git ref**
- **In scope / out of scope**
- **Data models / file paths / contracts**
- **Expected behavior** including keyboard and Android touch variants
- **Acceptance criteria:** executable test or precise inspection procedure
- **Performance expectations** with hardware and metrics, or explicit 'measure only'
- **Error and recovery behavior**
- **Integration instructions**, rollback considerations, build/run commands
- **Evidence required:** screenshots/video, logs, unit tests, profiler captures as applicable
- **Unresolved ambiguity policy:** do not invent game design, ask for approval or use clearly marked temporary placeholder.

Example task:
> ENGINE-001: A minimal C++ application initializes cross-platform input and a low-resolution framebuffer; loads one authored 2D collision map; camera walks without clipping; demonstrates walls, floor and billboard creature; shows FPS, render time and RAM; executable on Windows; Android CI smoke build. No account, network, monetization, full world or polished assets. Attach benchmark from target devices.

## 6. Milestone definitions and acceptance gates

**M0: Documentation**
- Three bibles plus story/product/battle specs have reviewed versions; original asset provenance recorded; decisions register up to date.
- Each approved gameplay feature has corresponding acceptance test.

**M1: Rendering/Controls feasibility**
- Windows native and Android packaged minimal startup.
- Software-rendering options compared with equivalent scenes; record frame times/peak RAM on test hardware.
- Touch + mouse/keyboard movement, camera yaw/pitch within chosen renderer capabilities.
- Sprite positioning, transparency, depth occlusion and visibility correct.

**M2: Companion + Encounter**
- One owned starter follows, stops, reacts to clue; does not obstruct view or clip when path blocked.
- Turn-based deterministic battle against one opponent; game resumes world reliably.
- Save/restore preserves companion and completed puzzle.

**M3: First Signal vertical slice**
- Full route Lantern Quay → Mosswake Path → Liltwood → Echo Hollow and back.
- All 3 starters can solve essential story puzzle and beat intro battle, no softlock.
- Estimated 15–20 minutes verified by playtest.
- Unique art kits and cues in town, forest, ruin; map readability tested on phone.
- No surprise increased device requirements.

**M4: Online proof**
- Two clients connect to same instance; player+companion positions sync.
- Session disconnect/reconnect documented; no duplicate reward/trade exploits.
- Server validates battle state.

**M5: Content and commerce planning**
- Only after fun/performance validated: creature catalog, greater regions, player customizations, backend, commerce and storefront integration.
- Cross-store purchases/entitlements must be designed against current platform terms; no assumptions about direct cross-buy.

**M6: Pre-release**
- Security review, content ownership and trademark clearances, save migration, analytics privacy, age ratings, support workflows, localizations, crash handling, accessibility audits.
- Check current Google Play / Steam policies near submission.

## 7. World/creature implementation dependencies

```text
Sprite provenance -> creature species assignments -> animation/asset schema
                                     |
World fiction -> zone graph -> level blockout -> biome kits -> scripted quest
                   |              |
Renderer experiment + low-end benchmarks -> geometry and atlas constraints
                   |                              |
Combat rules -> deterministic battle logic -> save schema / server authority
                   |                              |
             Vertical slice playtest -> networking proof -> expansion
```

## 8. Specific safeguards
- Scope protection: no 100-creature production push until the first 3 sprites and loop work and benchmark.
- No AI-generated asset assumption: user has explicitly stated creature sheet is original, hand-created.
- Do not treat reference game screenshot as owned game art or clone its HUD.
- Do not substitute a fully 3D pipeline without discussion; technical fallback decisions require measured tradeoffs.
- Never deploy live payment systems before receipt/entitlement backend validation and refund flows.
- Keep personal credentials, developer keys, certificates and store secrets out of public repository.
- README and docs should say 'working title' until brand clearance.
- Cross-platform build is not 'done' until a real Android package and Windows executable launch on test devices.
- Accessibility and battery/thermals checked on mobile throughout, not at end.

## 9. Suggested next design documents, in order
1. **Game Design Document v0.1** (product fantasy, success loop, modes, progression, controls, onboarding, acceptance).
2. **Narrative Bible v0.1** (story arcs, NPC and faction briefs, tone, first 20-min quest script).
3. **Battle Specification v0.1** (numbers, stats, effect resolution and example logs).
4. **Level Maps v0.1** (tile/grid maps for the 4 first-slice zones; reproducible coordinates, collision).
5. **Creature Catalog & Asset Ledger v0.1** (all 100 source cells and chosen starters).
6. **Architecture ADR-001** (rendering options, constraints, low-end benchmark plan).
7. **UX Flow / Localization v0.1** and **Network Protocol v0.1**.

## 10. Review requests for user
- Approve the **world structure as a planning baseline** or revise geography/regions.
- Confirm the **first-person experience as permanent** vs optional third-person in social areas.
- **Decided:** offline adventure + optional online multiplayer. Approve or revise the *two-realm trust proposal* in OFFLINE_ONLINE_SYSTEMS_v0.1.md (local vs verified account progression).
- Confirm **creature bonding method** (battle, noncombat, or both).
- Prioritize immediate artwork prototypes: a first-person town view, forest view, ruin chamber view — all matching source sprite scale.


## 11. 2026-10-20 approved connectivity decision and immediate child specifications

**Approved functional requirement:** WILDSIGNAL must remain an offline-playable single-player adventure after installation and initial asset availability. No mandatory account, login, recurring entitlement verification or server call for offline story, creature companionship, battle and local saving. Online multiplayer is opt-in and requires connectivity. Updates/downloads and first-time installation may still require a store connection; that does not change runtime offline-game requirement.

**Not approved yet:** whether the online-account character and local-story character are separate progression profiles; whether any noncompetitive offline companion may appear in casual co-op; and whether story milestones may carry over. Do not imply seamless offline inventory import, which conflicts with secure PvP/trading.

New companion documents on this PR:
- `docs/design/LEVEL_MAPS_FIRST_SIGNAL_v0.1.md`: precise prototype tile grids, triggers, portal links and spatial acceptance tests.
- `docs/design/NARRATIVE_QUESTS_v0.1.md`: first-session story, NPCs, dialogue and state/trigger matrix.
- `docs/design/GAMEPLAY_SYSTEMS_v0.1.md`: companion state, command rules, turn-based battle, access/input and game-state acceptance tests.
- `docs/architecture/OFFLINE_ONLINE_SYSTEMS_v0.1.md`: offline-first runtime, account/trust separation, local saves and server-authoritative online systems.

**Design/development gate:** precise prototype grids are REVIEW DRAFTS, not approved final maps. Codex should not build beyond experimental graybox until the user signs off on the topology, game modes and essential UX.
