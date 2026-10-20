# WILDSIGNAL — Master Pre-Production Plan v0.1

**Status:** Mixed: confirmed product decisions + proposed implementation and world details; updated 2026-10-20. **Developer:** Codex, *after* approvals. **Intent:** Remove unbounded interpretation, not eliminate the need for implementation testing.
**Other specs:** WORLD_BIBLE_v0.1.md; VISUAL_BIBLE_v0.1.md; CREATURE_BIBLE_v0.1.md in separate draft PR #1.

## 1. What's truly decided
- Original game; working title WILDSIGNAL; original 100-sprite creature sheet authored/owned by the user.
- Real-time first-person pseudo-3D/pixel-art exploration; "beautiful but mysterious" forests and ruins.
- Creatures physically accompany players and meaningfully affect exploration.
- Turn-based combat; Windows/Steam and Android/Google Play, low-end-first; C++ and lightweight native libraries.
- **CONFIRMED 2026-10-20: Offline-first story adventure + optional online multiplayer.** Offline mode must support exploration, creature bonding/battles, quests and local saves with no login or internet. Online is additive: co-op, presence, trading, PvP and account services. Exact cross-mode data bridge is a design proposal pending explicit approval.
- **CONFIRMED 2026-10-20 planning baseline:** free-to-play with optional cosmetic-only purchases; aspirational launch 6 zones/18–24 species/3–5h story *subject to feasibility*; two-player Android↔Windows co-op at commercial launch; distinct offline local and server-verified online progression with visual-only familiar offline companion presentation in casual co-op where feasible.
- ChatGPT handles planning and specifications; Codex handles implementation.

## 2. What is NOT yet decided (major gaps and accountable deliverables)

| Domain | Missing decisions | Required specification / approval |
|---|---|---|
| Product | core differentiator, target audience and age rating; **offline play requirement is confirmed** | Product Vision + feature priority |
| Scope | first release size, first playable vs public release, cost/time/solo capacity | Scope Matrix & milestones |
| World | topology, each zone's actual grid/collision, portals, scale, travel time, secrets | World Bible; annotated level maps |
| Narrative | protagonists, factions, NPC identities, quest/cutscene scripts, lore timeline | Narrative Bible and dialogue sheets |
| Creatures | IDs for all 100, species names, official starter choices, ecology, rights ledger, sprite cutouts | Creature Catalog + Art Ledger |
| Combat | **Full formula/status/turn-order proposal and test vectors drafted**; balancing and PvP rules still unapproved | BATTLE_BALANCE_BIBLE_v0.1.md + BATTLE_TEST_VECTORS_v0.1.json |
| Economy | rewards, creature acquisition limits, crafting, shops, progression pacing, anti-exploitation | Economy Balance Bible |
| UI/UX | **UI bible, 26 screen contracts, wireframes and 40 acceptance tests drafted**; final art, touch testing and flow approvals outstanding | UI_UX_BIBLE_v0.1.md; UI_SCREEN_CONTRACTS_v0.1.json; UI_WIREFRAMES_v0.1.md; UI_ACCEPTANCE_TESTS_v0.1.json |
| Art | camera treatment, texture pixel density, sprite direction sets, lighting/fog, character customization | Visual Bible + asset templates |
| Engine | raycaster vs other 2.5D renderer, collision, save schema, resource loading | Architecture Decision Records + benchmark |
| Multiplayer | **offline/online model chosen**; server authority, zone capacities, instancing, matchmaking, trade, sync and security decisions outstanding | Offline/Online Systems v0.1; Network/Backend Spec |
| Monetization | **F2P cosmetic-only model approved**; pricing, SKU design, server receipts, refunds, entitlements, compliance and operating budget still missing | Store + Entitlement Spec |
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

**Approved for planning 2026-10-20:** online-account/verified character progression is separate from offline-local progression; familiar offline companions may be **shown cosmetically** in casual co-op *where technically feasible*, without importing their unverified battle stats, inventory, trading rights or rewards. Exact projection UI/fallback, quest-sharing, save migration and store receipt rules remain unapproved. Do not imply seamless offline inventory import.

New companion documents on this PR:
- `docs/design/LEVEL_MAPS_FIRST_SIGNAL_v0.1.md`: precise prototype tile grids, triggers, portal links and spatial acceptance tests.
- `docs/design/NARRATIVE_QUESTS_v0.1.md`: first-session story, NPCs, dialogue and state/trigger matrix.
- `docs/design/GAMEPLAY_SYSTEMS_v0.1.md`: companion state, command rules, turn-based battle, access/input and game-state acceptance tests.
- `docs/architecture/OFFLINE_ONLINE_SYSTEMS_v0.1.md`: offline-first runtime, account/trust separation, local saves and server-authoritative online systems.

**Design/development gate:** precise prototype grids are REVIEW DRAFTS, not approved final maps. Codex should not build beyond experimental graybox until the user signs off on the topology, game modes and essential UX.


## 12. World-layout iteration v0.2 (draft on this PR)

- `docs/design/maps/FIRST_SIGNAL_BLOCKOUT_v0.2.json` is a **validated navigational fixture** with four 48×32 tile grids, portal destination IDs, creature/quest entities and visible path cells. Every designated entity/portal is flood-fill reachable from the principal spawn. These checks do not prove quest-state sequencing or visual composition.
- `docs/design/FIRST_SIGNAL_ZONE_BLUEPRINT_v0.2.md` annotates every region, quest-critical location, environment art kit, hero camera, soundscape, companion puzzle, rescue behavior and acceptance gate.
- `docs/design/LEVEL_MAPS_FIRST_SIGNAL_v0.1.md` is historical and **superseded for geometry and tile coordinates**. Do not mix coordinate versions.
- The proposed Echo Hollow sigil puzzle uses switch state A=0/B=1/C=0 toward A=1/B=0/C=1; it is distinct from the forest veil A→B→C pattern and not final until user approval.
- User said **proceed** with environment planning. This authorizes continuing documentation, *not* final creative signoff of maps or irreversible programming/production choices.
- Offline adventure is confirmed; the recommended two-trust-realm online economy is **still proposed**, not officially confirmed.


## 13. Character & NPC art design v0.1 (proposed)

Added `docs/design/CHARACTER_NPC_BIBLE_v0.1.md` for playable Wayfinder body/skin/hair/outfits, first-person compass view, animation source standards, cosmetics, quest-linked dialogue and NPC story roles; `docs/design/CHARACTER_ASSET_SCHEMA_v0.1.json` records data bindings. Existing NPC spatial identities remain: Mara `NPC_001_MARA` Z01 (18,11), Fen `NPC_002_FEN` Z01 (33,8), Iri `NPC_003_IRI` Z03 (21,25). Bellkeeper Sol is ONLY a memory/inscription in Z04 first chapter and has no physical map actor. These art/design proposals do not authorize purchased cosmetic SKU implementation, character sprite production or multiplayer profile sync yet. Character design approval and art-pipeline performance prototype are the next gates.


## 14. Full creature artwork catalog v0.1 (2026-10-20)

- `docs/design/CREATURE_CATALOG_v0.1.json` now inventories **all 100 cells**, WS-001 through WS-100, row-major; each has a readable visual motif, provisional affinity and proposed region. **Only the user's ownership and existing artwork are confirmed; species identities, habitat/type choices, names, gameplay balance and evolution lines remain for approval.**
- `docs/design/CREATURE_CATALOG_GUIDE_v0.1.md` covers original-file preservation, manual sprite extraction and alpha review, pivot metadata, directional animation, resource atlases and commercial rights ledger.
- The provided sheet is RGB 850×832 with a lime background/grid, not a ready transparent sprite atlas. Do not assume a fixed native sprite resolution or auto-chroma-key the original unreviewed.
- Three provisional starter mappings remain WS-004 / R01C04 Glintle (Flora), WS-008 / R01C08 Driftfin (Tide), WS-050 / R05C10 Brumlet (Ember). Names/affinities remain creative proposals.
- Production gate: initial playable uses three starters + 3–6 selected wild species, **not all 100 full animation sets**. Existing art may be used with user authorization, but decide whether to commit original files in a public repo separately.

### Immediate design next steps
1. Choose 3–6 wild-species IDs for Mosswake/Liltwood/Echo Hollow and define ecology/trust interactions.
2. Define actual battle stats, move logic and starter abilities via deterministic combat specification and test vectors.
3. Specify capture/bonding behavior, evolution families and progression to avoid inconsistent content tags.
4. Test sprite extraction/alpha quality on the three starters before setting texture-packing or CPU memory budgets.


## 15. Battle and first-nine wildlife design v0.1 — draft review package

**New docs on world-preproduction draft branch:**
- `docs/design/FIRST_NINE_CREATURES_v0.1.json`: 3 starter candidates + 6 wild creature art IDs, original sprite mapping, proposed roles/stat blocks, **18 typed moves** with integer energy cost/power and explicit status effects. These are candidate names and values, not canon.
- `docs/design/BATTLE_BALANCE_BIBLE_v0.1.md`: proposed deterministic integer formulas, 1v1/3 party structure, status lifecycles, priority, end-round effects, encounter balance and server trust requirements.
- `docs/design/BATTLE_TEST_VECTORS_v0.1.json`: arithmetic fixtures, seed=1 xorshift32 sequence, status and two-turn replay expected state. Fixture calculations verified during planning; no C++ executable test has run.
- `docs/design/BONDING_ENCOUNTERS_v0.1.md`: six ecological encounter/assist/invite flows, ambient animations, species-specific habitat placement and no-starter-softlock policy.
- `docs/design/CREATURE_CATALOG_v0.1.json` updated to cross-reference proposed wild names/habitats. All 100 artwork positions remain distinct; none automatically treated as evolutions.

**First wild candidate locations:** Tumbletail WS-005 and Puddlefin WS-021 in Mosswake; Mossmelt WS-012 and Signalit WS-001 in Liltwood; Craglet WS-024 guardian and Hushflame WS-067 in Echo Hollow. Additional proposed spawn IDs are **not yet present** in the 48×32 world fixture. Do not fabricate completed map data or claim all recruitments are active in game.

**Next approval gates:** exact first nine assignments/names; bonding progression versus classic capture model; whether Signal Energy supports compelling tactics; status durations; final boss difficulty; reserved party space; PVP level normalization; map placements. Do not start a comprehensive 100-creature roster or commercial economy until these are playtested.


## 16. UI/UX pre-production v0.1 (DRAFT, 2026-10-20)

- `docs/design/UI_UX_BIBLE_v0.1.md` defines root navigation, player input parity, minimal first-person HUD, battle move-energy cards, local save behavior, fieldbook/party UI, accessibility, mobile/PC layout constraints and later online commerce gating.
- `docs/design/UI_SCREEN_CONTRACTS_v0.1.json` contains **26 screens and 154 component definitions** with proposed rectangles in normalized 1000×562 safe-content reference space. This is not yet physical dp/sp validation and not finished artwork.
- `docs/design/UI_WIREFRAMES_v0.1.md` includes readable monochrome layout mockups and cross-references all screens. Final compositions require pixel-art review and on-device screenshot tests.
- `docs/design/UI_ACCEPTANCE_TESTS_v0.1.json` has **40** design acceptance cases (21 P0 first playable, 14 P1 pre-beta, 5 P2 online/commerce) — **all unexecuted**.
- **First demo UI scope** should be a functional subset: MAIN_MENU, AVATAR_CREATE, STARTER_PICK, EXPLORE, COMPASS, DIALOGUE, PUZZLE, BATTLE_SELECT/PLAYBACK/RESULT, PAUSE. SAVE_SLOTS + SETTINGS required before broader alpha; Fieldbook/Party/Bond screens phase in only when owned creature recruitment system exists. **Screen catalog scope is not authorization to implement every screen at once.**
- The UI must not invent creature/battle truth or save success. All transitions dispatch typed intents to local simulation or online authoritative backend. Offline mode never requires an account.
- Critical architecture still needs approval: rendering backend, exact mobile minimum device and UI safe areas, final trust-realm policy, creature-bond method, combat math and initial 3-6 wild species.


## 17. Gate 1 and Gate 2 plans (2026-10-20)

New review-only artifacts on this draft branch:
- `docs/planning/PRODUCT_VISION_RELEASE_SCOPE_v0.1.md` defines the renderer proof, first 15–20min slice, and **owner-approved aspirational** 1.0 planning target of six zones, 18–24 species, 3–5h story, two-player co-op at launch, with **owner-approved F2P cosmetic-only purchases**. Scope quantities remain feasibility-gated, not guarantees or authorization to develop everything immediately.
- `docs/architecture/ENGINE_ARCHITECTURE_ADR_v0.1.md` proposes native C++20/SDL3/CMake with Android NDK/Gradle, a CPU-rendered pixel framebuffer and platform-separated modules; renderer R1/R2/R3 still **unselected**.
- `docs/architecture/RENDERING_BENCHMARK_PLAN_v0.1.md` specifies equivalent town/forest/ruin test scenes, 30fps/2GB-class memory hypotheses, device matrix, raw capture schema, Android thermals and scored architectural bakeoff. **Zero benchmarks performed.**
- `docs/planning/DECISION_REGISTER_v0.1.md` and its matching `DECISION_REGISTER_v0.1.json` mirror **identical D-IDs** (legacy JSON IDs were reconciled 2026-10-20); both distinguish CONFIRMED planning choices from pending proposals/blocked decisions.
- `docs/planning/CANONICAL_SPEC_INDEX_v0.1.md` explicitly tracks PR #1 and PR #2, maps superseded, battle data authority, review/merge order and version pinning. Review PRs before attempting implementation against floating branch heads.
- `docs/planning/CODEX_ENGINE_PROOF_TASK_PACKETS_v0.1.md` bounds the first possible Codex work to technical feasibility and acceptance evidence, **not** full game implementation.

**Immediate owner decisions still open:** release pricing, final 1.0 scope and multiplayer/trading phasing, local vs verified-online companion continuity. **Blocked technical decision:** benchmark-proven renderer and minimum supported low-end devices. **Avoid claiming production readiness based on authored design documents alone.**


## 17. Product and C++ engine readiness milestone (2026-10-20)

- **Canonical entry point for newer planning:** `docs/planning/SPEC_INDEX_v0.1.md`. This explicitly tracks two open DRAFT PRs, latest authoritative data versions, and owner-approval rules.
- `docs/planning/PRODUCT_VISION_RELEASE_SCOPE_v0.1.md` now proposes a commercial 1.0 envelope: **six zones, 18–24 integrated original creatures, 3–5 hours story, small optional two-player cross-platform co-op**, conditional cosmetic IAP; these are *aspirational upper-bound design targets*, NOT a commitment. Smaller four-zone / nine-creature fallback must remain feasible.
- **Monetization model confirmed for planning:** free-to-play with optional cosmetic-only purchases, never paid battle/progression benefits. Prices/SKUs, receipt processing and launch checkout sequencing remain gated, not approved.
- `docs/architecture/ENGINE_ARCHITECTURE_v0.1.md` proposes native C++20/SDL3/CMake with separate gameplay/core, renderer and platform adapters. Raycast vs sector/portal is **NOT selected**.
- `docs/architecture/RENDER_BENCHMARK_PLAN_v0.1.md` defines controlled Quay/forest/ruin scenes, 320×180..480×270, real Windows/Android, p95/thermal/peak memory metrics; no benchmarks have been executed and low-end RAM claims remain goals.
- `docs/architecture/CODEX_R_AND_D_TASKS_v0.1.md` gives small ENGINE-001, RENDER-001/002, BENCH-001, CI-001 planning packets. They require owner approval before coding and are NOT feature authorizations.
- `docs/planning/DECISION_REGISTER_v0.1.md` now distinguishes explicit user requirements, recommendations, test blockers and deferred expansion. Do not promote proposed docs by duplication or merge alone.
- **Process gate:** review product/multiplayer/economy choices; approve benchmark-only engineering task; measure feasibility before committing final dungeon geometry or massive art production.


## 18. OWNER-APPROVED PRODUCT BASELINE — 2026-10-20

The owner explicitly approved for **planning purposes**: `D-020` F2P with cosmetic-only IAP; `D-021` six-zone/18–24-species/3–5h aspirational 1.0 (dependent on realistic resources and playtesting); `D-023` two-player Android↔Windows co-op **at commercial launch**; and `D-024` separate local offline and server-verified online progression, with a familiar offline-companion **appearance projection** in casual co-op where feasible. Preserve the offline adventure with zero mandatory logins. Visual-only projection never carries untrusted stats, trading rights, creature ownership, rewards or purchase entitlements. If projection fails feasibility, prepare and review a graceful UX fallback before shipping.

**Product/engineering distinctions:** D-021 is approved as a *target*, not a fixed signed-off map/species deliverable; D-023 is a launch requirement under current plan, not a disposable option. Reducing scope or delaying co-op requires explicit owner review rather than quiet unilateral rescheduling. Product approval does not authorize coding, store integration or infrastructure spend.

**Next planning-only milestone:** `docs/planning/NEXT_MILESTONE_ASSET_ENGINE_READINESS_v0.1.md` — prepare artifact provenance, starter sprite extraction specs, three environmental benchmark camera references, exact device/testing plan, cross-system reconciliation, cosmetic/trust UX and commercial checkout compliance paper. Existing future `ENGINE-001` / `RENDER-001` Codex task packets remain **NOT STARTED and NOT AUTHORIZED** by this update.
