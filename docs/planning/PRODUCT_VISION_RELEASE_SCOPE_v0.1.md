# WILDSIGNAL — Product Vision & Release Scope v0.1

**Status: PRODUCT PLANNING BASELINE APPROVED IN PART, NOT PRODUCTION LOCK.** Date: 2026-10-20.  
**Owner explicitly approved D-020, D-021, D-023 and D-024:** free-to-play with cosmetic-only purchases; aspirational six-zone/18–24-creature/3–5-hour release target subject to feasibility; two-player Android↔Windows co-op at commercial launch; distinct offline and verified-online progression, with offline companion cosmetic appearance in casual co-op where feasible. Prices, billing, art, technical implementation and release date remain unapproved.
**Binding user requirements (confirmed):** original pixel-art creature companion adventure; "beautiful but mysterious" Doom-inspired first-person *2.5D* presentation; light enough for low-end Android and Windows PCs; built around C++ and lightweight libraries; offline adventure plus optional online multiplayer; intended commercial release on Steam and Google Play; optional purchases for revenue; original user-owned creature sprites.
**Design sources:** WORLD_BIBLE_v0.1.md, VISUAL_BIBLE_v0.1.md, FIRST_SIGNAL_ZONE_BLUEPRINT_v0.2.md, CREATURE_CATALOG_v0.1.json, UI_UX_BIBLE_v0.1.md, OFFLINE_ONLINE_SYSTEMS_v0.1.md, BATTLE_BALANCE_BIBLE_v0.1.md.

## 1. One-sentence pitch
**WILDSIGNAL is an immersive first-person pixel-art creature-bonding adventure where a living companion helps you follow mysterious signals through lush forests, welcoming settlements and forgotten ruins — playable entirely offline, with optional small-group online expeditions.**

Target feelings: wonder > discovery > trust > strategic confidence > community. WILDSIGNAL's heart is ecology, a creature's personality and companion participation outside combat, not the pursuit of big stats or the size of the collection.

### Audience and platform hypotheses — not researched/validated market segments
- Primary: players attracted to pixel-art fantasy exploration and creature collecting who want handheld/mobile-friendly play.
- Secondary: cooperative friends and retro-PC enthusiasts without discrete GPUs.
- Age/ratings goal: broadly family-friendly adventurous mystery (no graphic gore, gambling or sexual content); official platform age classifications to be determined after content review. **Do not assert PEGI/ESRB/IARC rating prematurely.**
- Gameplay sessions: 10–25 minutes common, with optional longer expeditions; story quests have safe save points and no forced time-limited offline tasks.
- Localization: English text first for prototypes; Filipino/Tagalog and other target languages **candidate later**, contingent on professional/community translation QA and font support. Avoid assuming locale just from developer's geography.

## 2. Product pillars and rejection tests
1. **Explore to discover:** visual landmarks and signal clues, not random corridor grinding. Reject feature if it needs aimless travel to fill a timer.
2. **Bond with distinct companions:** every species has 1 meaningful world interaction and readable personality. Reject a roster species with only battle stats and no ecological behavior.
3. **Readable turn-based tactics:** short but engaging decisions on mobile; no high-APM input, pressure, paid stat increases or dependent-online PvE progress.
4. **Performance and accessibility:** 30fps target and readable touch controls on validated lower-end devices, with memory/time/thermal proof. Reject costly visual effects that break budgets.
5. **Multiplayer as optional enrichment:** small co-op expeditions/lobbies, then more advanced trading/PvP if safe. Reject mandatory login for story progress.
6. **Fair commercial relationship:** free core access is a *proposal*, purchases are cosmetic only, no stamina timer, purchase-only stronger creatures, battle boosts or loot boxes.

## 3. Three product gates, with deliverables

### M1 — Technical engine proof (NOT a shipped game)
- **Content:** one representative scene each for warm town, leafy forest and ruined chamber using provisional blockout/canonical palette; one owned creature sprite if asset ready, otherwise documented temporary stand-in.
- **Systems:** walking, turn/look controls, camera collision, sprite depth/occlusion, floor/ceiling illusion where supported, 2D maps; performance overlays/logs, Windows exe, Android debug APK.
- **Not needed:** playable quest, battle, server, accounts, cosmetics, cloud saves or real money.
- **Pass:** relevant scenes run and provide measured stable frame time, peak memory, thermal and visual limitations; renderer ADR records tradeoffs.

### M2 — 15–20-minute vertical slice (NOT a commercial release)
- **Content:** Lantern Quay → Mosswake Path → Liltwood Forest → Echo Hollow; Q001–Q005; three original starter sprite candidates (WS-004, WS-008, WS-050); one guardian WS-024, 1–2 ambient creatures as art workload permits.
- **Systems:** starting character, compass, choosing a starter, following companion, one environmental puzzle, turn-based 1v1 against guardian, local save/reload, two device control schemes, clear UI/audio feedback.
- **No online dependency;** a separate two-player connection lab may follow but is not a feature required to enjoy this slice.
- **Pass:** all starters can finish with no softlocks; reasonable pace confirmed by user tests; no critical mobile tap failures; controlled memory and reproducible combat.

### M3 — Commercial 1.0 (OWNER-APPROVED ASPIRATIONAL PLANNING TARGET; feasibility not established)
- **Platforms:** Windows via Steam; Android via Google Play, same base game/content and cross-play as supported.
- **Main story:** target 3–5 hours for first playthrough, plus optional exploration/creature discovery (estimate only; needs story production sizing).
- **Map content:** owner-approved *aspirational* six distinct explorable zones: Lantern Quay, Mosswake Path, Liltwood Forest, Echo Hollow, Tideglass Inlet, Ashbell Works. Only first 4 are preliminary blockouts. Final scene/quest count not approved.
- **Creature content:** owner-approved aspirational **18–24** fully integrated original species; feasibility and specific roster/animation approval remain gating conditions. Not automatic approval to use all 100 artworks. 3 starters, 9 candidate prototype species; remaining roster chosen after art/balance validation.
- **Combat/companions:** 1v1 turn based, party cap proposal 3, 4 moves, elemental/Signal Energy mechanics after balancing, creature follows and ecology interactions, growth and up to one evolution opportunity for select species if art exists.
- **Offline:** entire core campaign, discover, befriend, battle, quest and save without account/network. Local backup/migration included.
- **Online launch requirement (approved for planning):** lobby with friend invite/join code; player presence and visible companions; **two-player Android↔Windows cross-platform cooperative expedition** in 1–2 supported zones; server-verified online creatures/rewards, basic profile and reconnection recovery. This cannot silently be moved postlaunch.
- **Cross-play:** Windows/Android clients can join the same supported lobby and co-op zone; no compulsory Steam-only authentication for Android.
- **Paid items:** **F2P plus optional cosmetic-only purchases is approved for planning**. Proposed initial 3–6 cosmetics (scarves, backpacks, companion adornments); specific SKU/prices, launch store timetable and backend receipt verification are open production gates. No stat, capture, XP, story, signal-energy or matchmaking advantage. If storefront cannot safely launch, any staging change must be reviewed rather than silently shipping monetization without verification.
- **Suggested nonlaunch features:** ranked PvP, global creature exchange, auction house, large MMO map, clans/guilds, seasons, live economy, breeding, full 100-creature animated roster, user-generated text chat, voice chat and dozens of cosmetic layers.
- **Trading:** player-requested feature from early vision, **planned** but NOT necessarily safe for 1.0. Prefer limited server-validated trading in update 1.1 after ownership security and moderation are proven.
- **Casual PvP:** suggested **post-1.0** unless time/budget allow; authoritatively simulated PvP proof may be tested before release but ranked ladders deferred.
- **Explicit risk:** 3–5 hours + six zones + 18–24 animated creatures + cross-platform co-op + IAP is a substantial multi-discipline project; return for explicit owner approval before reducing scope, postponing the **approved launch co-op**, changing monetization, or announcing a new schedule.

## 4. Commercial model (APPROVED FOR PLANNING) and nonselected alternatives

| Model | User benefit | Costs/risks | Recommendation |
|---|---|---|---|
| A. Free-to-play + cosmetic IAP | Low access barrier, good for mobile adoption and shared friend invitations | Requires backend revenue to cover servers; modest conversion may not cover ongoing costs; demands purchase/store compliance | **OWNER APPROVED, 2026-10-20** |
| B. Premium upfront + optional cosmetic IAP | Predictable unit revenue per purchase, smaller need for high conversion | More friction for mobile trials/cross-play; must justify content price and value | Valid fallback if costs or market tests favor |
| C. Free demo + premium expansion | Easy trial and clearer content value | More entitlement complexity across Steam/Google; story gating risks audience confusion | Consider later if product structure supports |

**Do not invent retail prices** without market validation, currency review and platform margins. Never tie story-critical companions, progression speed, combat stats or power to payment; avoid loot boxes and predatory daily rewards. A separate store-ready spec is still required for taxes, platform billing, customer support, receipts, refunds, parental restrictions and platform rules.

## 5. Two progression/trust realms (APPROVED PRINCIPLE; implementation details OPEN)

The product MUST deliver meaningful full local offline play. The **following separate trust domains are owner-approved for planning (2026-10-20)**:
- **Local Adventure:** entirely device-owned character, quests, creatures and inventory; save file editable by its owner.
- **Online Expedition:** server-owned verified character/creatures/items for co-op rewards, trading and any PvP; shared species assets and optional same-looking starter; no unrestricted import of edited local inventory.
- A cosmetic-like companion projection is **part of the approved desired casual-co-op experience where feasible**: your offline buddy's **appearance** may be represented without importing stats/items, trade rights or ranked eligibility. Visual mapping, consent, anti-spoofing, fallback and UX require experiments. It is not a purchasable cosmetic entitlement.
- **Critical player-experience test:** after forming an emotional bond offline, does online's distinct character feel like abandoning that companion? If it does, revisit separate-realms design before coding.
- Nontradeable unverified local companions carrying actual gameplay stats into online sessions were **not selected**. Reopen only with a new owner decision. Never allow them into competitive/tradable economy without verifiable server provenance.
- The phrase "cross-platform progress" must not appear in marketing until actual interoperable account entitlements/saves are proven. Cross-play != cross-buy != cross-save.

## 6. Product economics and operating assumptions

**Hard unknowns:** user acquisition, retention, purchase conversion, app-store review, hosting costs, infra capacity, code maintenance, art/sound labor and player support. Do not promise profit or a release date.

Measure before commercial commitment:
- build/download size; low-end installed memory and storage footprint
- number of online concurrent co-op rooms, egress/region, monthly hosting costs under idle and active load
- average session length, tutorial completion, companion selection, fun/discovery feedback
- estimated per-feature art hours and screen/code effort
- cash spend on store developer registration, business paperwork, signing/CI, moderation, music and assets
- user support load for store entitlements, game crashes and multiplayer reports

Offline mode must continue working if server subscription lapses. Never silently revoke owned local story progress.

## 7. Provisional feature matrix

| Feature | Technical Proof | Offline Slice | Commercial 1.0 Proposal | Post-Launch |
|---|---|---|---|---|
| First-person 2.5D maps | 3 test scenes | 4 zones | 6 zones | More archipelago |
| Owned creature art | 1 test sprite | 3 starters + guardian | 18–24 species | Up to 100 as justified |
| Following/bonding | basic follow | 1 puzzle/intro | full ecosystem interaction | richer ecology |
| Turn-based battles | isolated sim tests | 1 encounter | PvE progression | ranked PvP expansion |
| Offline saving | smoke test | local slot | robust versioned backups | optional cloud/local transfer |
| Multiplayer | none | independent proof | 2-person co-op & hub | trading, advanced PvP, guilds |
| Monetization | none | none | cosmetic-only model **approved**; real checkout gated by billing compliance and explicit launch plan | expand cosmetics |
| Real-money equipment/stat items | never | never | never | never |
| Accessibility | input proof | core readable UI | localization-ready accessible UI | extended controls |

## 8. Definition of done per release gate
- **Technical Proof DoD:** reproducible Windows and Android build, complete test footage, benchmark JSON/CSV with target hardware description and at least one rejected alternative documented.
- **Offline Slice DoD:** all starter routes, battles and puzzles exercised offline, zero reproducible quest deadlocks, save/load corruption recovery test, first-person readability on phone/PC and session playtests.
- **Commercial 1.0 DoD:** official store requirements verified near submission, all ownership/license tracked, performance acceptance on supported hardware, moderation for optional online features, server uptime/reconnect validation, local story completely playable with network disabled; store IAP verified end to end if sold.

## 9. Owner sign-off questions
1. **Resolved 2026-10-20:** free-to-play + cosmetic-only purchases approved; exact price/SKU, currency, refund and entitlement timing still open.
2. **Resolved as feasibility-gated aspiration:** six zones, 18–24 species, 3–5h story; owner must review evidence before changing target.
3. **Resolved:** two-player Android↔Windows co-op required in commercial 1.0; trading and ranked PvP remain later.
4. **Resolved principle:** separate offline and server-verified online progression; familiar offline companion may appear cosmetically in casual co-op where feasible. Exact client/UI projection protocol remains open.
5. Should we support optional controller for 1.0 on Steam? **Recommendation yes** after core keyboard/touch pipeline.
6. Target age range and localization priorities, to be determined by actual audience testing.

**Planning approval is not coding authorization:** Codex must not implement the proposed full scope, IAP or network stack without separately approved task packets, feasibility evidence and design/technical signoff. The next task is pre-production planning, not game implementation.


## 12. Windows 7–11 client reach — owner requirement 2026-10-20

**Developer hardware:** Windows 11 i3/4 GB. **Game compatibility intention:** native offline play on Windows 7, 8, 8.1, 10, 11; the technical proposal starts at Win7 SP1 **x64**, with 32-bit scope not approved. **Distribution channel is different:** Steam stopped supporting Win7/8/8.1 as of Jan 2024. Target Steam launch for supported modern Windows clients and plan a separately delivered legacy standalone executable, independent of Steam sign-in for core offline adventures. Standalone provider/installer is not yet selected; do not advertise a legacy Steam route. Approved Android↔Windows two-player online co-op remains launch scope but Win7/8.1 online security/authentication support is an unresolved feasibility gate (offline content cannot be blocked by account/login). Final minimum OS/RAM specifications await true runtime tests and separate owner review. See `docs/architecture/WINDOWS_7_11_COMPATIBILITY_v0.1.md` and decision D-012.
