# WILDSIGNAL — Canonical Design Decisions & Open Questions v0.1

**Date:** 2026-10-20. **Status:** Four commercial/trust planning decisions approved, plus Windows 7–11 game compatibility explicitly requested by owner, 2026-10-20. Technical feasibility, content lock, prices, detailed online implementation and Codex build authorization remain open.
**Git history:** Creature Bible resides in **draft PR #1**; world/systems/proposed scope and engine files in **draft PR #2**. Both were checked OPEN/DRAFT/UNMERGED this turn.
**Protocol:** One row per actual decision, with source and affected work. Owner approval updates status `CONFIRMED` and approval date; a prior proposed spec does not become confirmed merely by appearing in multiple files.

## Status vocabulary
- **CONFIRMED:** user explicitly selected it or it was established earlier in this conversation.
- **RECOMMENDED:** assistant's reasoned default, requires approval before production lock.
- **OPEN:** alternatives presented; no default locked.
- **BLOCKED_BY_TEST:** can only select safely after code/performance/art test.
- **DEFERRED:** long-term aspiration, deliberately not a current release commitment.
- **SUPERSEDED:** older draft replaced, retained for history.

## 1. Binding requirements / confirmed decisions

| Decision ID | Decision | Status | Source/impact |
|---|---|---|---|
| D-001 | WILDSIGNAL working title (name trademark clearance not performed) | CONFIRMED working title | branding, Steam/Play naming |
| D-002 | Original pixel-art monster companion RPG with user's OWN 100-cell hand-created art | CONFIRMED ownership per user | creature catalog, sprite asset rights |
| D-003 | First-person retro pseudo-3D/2.5D appearance inspired by user's dungeon screenshot | CONFIRMED visual direction | renderer R&D, world geometry |
| D-004 | Mood "Beautiful but mysterious": cute companions, lush nature, ruins and discovery | CONFIRMED | art and narrative |
| D-005 | Persistent creature companions follow, respond and help exploration | CONFIRMED | sprite art, sim/navigation |
| D-006 | Creature battles are turn-based; world exploration real-time | CONFIRMED | battle engine, UI |
| D-007 | C++ with lightweight libraries; user lacks local heavy game engine/high-end GPU | CONFIRMED tech constraints | architecture |
| D-008 | Android/Google Play and Windows/Steam publishing targets | CONFIRMED target | CI, platform adapters, compliance |
| D-009 | Full solo story progression playable offline; online multiplayer optional | CONFIRMED | save trust, UX, account boundaries |
| D-010 | Multiplayer included in overall vision and optional purchases intended for revenue | CONFIRMED intent; exact release scope OPEN | product roadmap |
| D-011 | ChatGPT leads planning/spec; Codex implements after decisions approved | CONFIRMED workflow | PRs, task packets |
| D-012 | Owner's development PC runs **Windows 11** (i3 10th gen, 4 GB). WILDSIGNAL targets **64-bit Windows 7, 8, 8.1, 10 and 11** only; **32-bit OS and 32-bit game binaries are out of scope**. Windows 7 SP1 is the proposed oldest test baseline; actual compatibility unverified. | **CONFIRMED x64-only compatibility target, 2026-10-20; verification GATED** | build architecture, legacy Win7–11 binary, Steam distribution separation and OS/security tests |

| D-013 | **Lantern Quay and Liltwood Forest are APPROVED as setting concepts; the actual RENDER-001 prototype environmental graphics are REJECTED as a visual/art baseline.** Do not claim final visual approval based on technical screenshot/CI success. | **CONFIRMED concept approval + visual rejection, 2026-10-21; VIS-001 visual fidelity proof required** | visual quality gate, art targets, renderer geometry comparison, production art signoff |

| D-014 | **Owner APPROVES the visual style and atmosphere of generated ART-ENV-001 Lantern Quay + Liltwood Forest reference illustrations; does NOT approve crop candidates as finished seamless/alpha-clean/geometry-correct game art or public publication.** Keep owner-drawn creatures separate from generated environmental concepts. | **CONFIRMED reference-style approval, 2026-10-21; production asset QA GATED** | ART-ENV-002 reusable tiles and prop cleanup, game art manifests, AGENTS.md, controlled renderer integration and owner in-game visual signoff |

## 2. Recommended but awaiting owner approval

| Decision ID | Current recommendation | Status | Downstream blocker |
|---|---|---|---|
| D-020 | **Free-to-play on Steam and Google Play with OPTIONAL cosmetic-only purchases**, no competitive/stat/progression benefits. Exact pricing/SKUs and payment integration unapproved. | **CONFIRMED for planning, 2026-10-20** | monetization, economy, store/billing compliance |
| D-021 | **Aspirational commercial 1.0 scope: 6 zones (Z01–Z06), 18–24 completed species, 3–5h main story**, expressly subject to feasibility and playtests, not a fixed promise or content freeze. | **CONFIRMED planning target, 2026-10-20; feasibility GATED** | narrative/art/QA scope, costs, real production evidence |
| D-022 | Four expanded zones / nine species remains a **contingency option** if D-021 is not feasible; changing the approved aspirational target requires an explicit scope review with owner, not automatic silent shrink. | **OPEN contingency; not approved as new release scope** | production capacity and scope review |
| D-023 | **Two-player cross-platform co-op at commercial 1.0 launch (Android↔Windows)**. Online trading and ranked PvP remain later optional phases; do not silently defer launch co-op. | **CONFIRMED for planning, 2026-10-20** | network authority, backend and operating costs |
| D-024 | **Local offline story inventory and server-verified online progression are separate**; in casual co-op, show appearance of familiar offline companions **where technically feasible** without importing their unverified stats, trade rights or rank. Exact presentation/recovery remains to design. | **CONFIRMED for planning, 2026-10-20; projection feasibility GATED** | companion UX, accounts, server validation, trust boundaries |
| D-025 | Original starter art mappings WS-004 Glintle/Flora, WS-008 Driftfin/Tide, WS-050 Brumlet/Ember | RECOMMENDED; original sprite art is confirmed | sprite animation, battle schema |
| D-026 | First-nine species proposal, including Tumbletail/Puddlefin/Mossmelt/Signalit/Craglet/Hushflame | RECOMMENDED | roster approval, habitats |
| D-027 | Trust-building Invite/Observe/Help/Friendly Spar instead of capture device | RECOMMENDED | core loop/progression |
| D-028 | 3 carried creatures, 1 active 1v1, 4 moves, energy 0..8, integer formulas, basic statuses | RECOMMENDED | combat backend and battle UI |
| D-029 | 3 local save slots; source sprite pivots and extra angles created conservatively | RECOMMENDED | save UX and art effort |
| D-030 | Landscape-first on Android; controller support by commercial 1.0 | RECOMMENDED | controls/QA |
| D-031 | Original NPCs Mara/Fen/Iri and Sol inscription (not full NPC in opening) | RECOMMENDED | quests, character art |
| D-032 | No UI-based minimap by default, compass-based discovery | RECOMMENDED | wayfinding and accessibility |
| D-033 | Cosmetically useful but honest online avatar presentation despite first-person solo | RECOMMENDED | art, monetization |

## 3. Technical unknowns requiring measured proof

| Decision ID | Options | Status | Required evidence |
|---|---|---|---|
| D-040 | CPU raycaster vs sector/portal wall renderer vs small CPU rasterizer | BLOCKED_BY_TEST | renderer/low-end visual benchmark |
| D-041 | 320×180 / 400×225 / 480×270 baseline, optional 60fps | BLOCKED_BY_TEST | P95/P99 frame time and pixel readability on devices |
| D-042 | x64 is approved and 32-bit game/OS support is explicitly excluded; actual Windows 7 SP1 x64 minimum-runtime feasibility, legacy CRT/SDL imports, Windows RAM/performance and Android min OS remain unverified | BLOCKED_BY_TEST | 64-bit Win7/8/8.1/10/11 functional tests, standalone distribution, hardware/OS and security evidence |
| D-043 | FOV/pitch, vertical architecture, wall/floor height and outdoor vista illusion | BLOCKED_BY_TEST | screenshot comparing original mood, measured fidelity |
| D-044 | Memory/cache budget and threading to maintain 30 FPS with animated sprite foliage | BLOCKED_BY_TEST | 20-minute soak, RSS/PSS, battery, memory trace |
| D-045 | SDL3 software-presentation vs framebuffer upload and UI crispness strategy | BLOCKED_BY_TEST | actual Windows and Android behavior |
| D-046 | C++20 + SDL3/CMake as standard client tech | RECOMMENDED, verify build | pinned toolchain Win/Android smoke builds |
| D-047 | 30Hz fixed world tick with independent rendering | RECOMMENDED; input comfort proof | cross-device movement test |
| D-048 | Transport library (ENet vs other) and headless server infrastructure | OPEN | online 2-client prototype |
| D-049 | Versioned local saves + backup and migration; exact format | RECOMMENDED; design to verify | corrupted/interrupted save tests |

## 4. Future aspirations intentionally outside first engineering scope

| ID | Future feature | Status |
|---|---|---|
| D-060 | Remaining portions of 100-creature sprite collection | DEFERRED until production capacity |
| D-061 | Full 10-region world, Caldera/Marsh/Stormwatch/etc. | DEFERRED |
| D-062 | Trading, ranked PvP, MMO-scale hubs, seasonal battles, guilds | DEFERRED by candidate 1.0 plan, approval required |
| D-063 | Rich vertical geometry, multi-floor buildings, 3rd-person avatar camera | DEFERRED pending renderer proof |
| D-064 | Breeding, random loot/gacha, paid power boosts | REJECTED as initial product recommendation; not user-confirmed |
| D-065 | Complete creature evolution families for 100 sprites | DEFERRED |
| D-066 | Cross-store purchases/entitlements automatically syncing | NOT PROMISED; requires policy/verification |

## 5. Source priority and reconciliation

**When Codex encounters contradictions, do not silently infer the rule.** Proposed hierarchy when ratified:
1. This decision register for approved vs pending statuses.
2. Approved `PRODUCT_VISION_RELEASE_SCOPE` for release scope and feature phase; no proposal becomes obligation until signoff.
3. Approved master technical ADR and renderer benchmark conclusion for engine/rendering.
4. Approved scoped system Bible for exact rules (e.g., Battle Bible for turn tie-break/status; Offline/Online for trust).
5. Versioned data schemas/fixtures for numeric IDs, move effects, map positions and test vectors — schema+spec disagreements are BLOCKERS, not tie-breaker by name.
6. Earlier unsuffixed proposals, sketches and general user inspirations are NOT automatically implementation details.

**Known overlaps/contradictions:**
- PR #1 Creature Bible and PR #2 catalog/battle docs live on separate branches; review/merge carefully.
- Older 15×11 level document is superseded by 48×32 blockout v0.2 for prototype coordinates.
- Brief Gameplay Systems turn tie originally mentioned PRNG tie breaks; detailed proposed Battle Bible uses **alternating seat parity** to avoid an RNG draw. Detailed Battle Bible is CURRENT PROPOSAL; reconcile summaries when locking.
- The UI lists 26 screen contracts but the vertical slice is a **subset**; do not implement 26 screens as a single Codex feature.
- First nine proposed habitats include some wild spawn metadata not yet added to 48×32 map JSON.
- Proposed 1.0 (6 zones/18–24 species) is an upper-bound commercial product scenario, whereas engine proof/first slice only require 3 starters, 4 zones, one guardian. A small 4-zone/9-species fallback exists if production cannot support larger goals.
- Store payments and online ownership policy not confirmed; avoid announcing exact cross-save or cross-buy.

## 6. Approval log template

```text
Decision ID: D-0XX
Choice: <exact option and limits>
Owner approved: <date, link to issue/PR discussion>
Spec updated: <file + revision>
Impacted docs/data: <paths and migrations>
Decision rationale: <why>
Next review trigger: <measured evidence or milestone>
```

**Owner decisions D-020, D-021, D-023 and D-024 were resolved for planning on 2026-10-20.** Remaining decisions: exact cosmetics/pricing/store operations, launch pacing/cost and scope feasibility, co-op protocol, casual companion projection UX, and real-device renderer feasibility. **No Codex implementation has been authorized by these approvals.** Next pre-production milestone: `docs/planning/NEXT_MILESTONE_ASSET_ENGINE_READINESS_v0.1.md` (docs and validation plans only).

**Do NOT merge all design PRs solely because documentation exists.** Merge when reviewed, record approvals and then generate small Codex tasks.


## 7. Owner decision event — 2026-10-20

**Explicit user approval in current planning conversation:**
1. **D-020**: free-to-play with cosmetic-only purchases.
2. **D-021**: aspirational commercial 1.0 with six zones, 18–24 species, 3–5h story **subject to feasibility**.
3. **D-023**: two-player cross-platform co-op at launch.
4. **D-024**: separate server-verified online progression; cosmetic offline companion appearances in casual co-op *where feasible*.

**Meaning of approval:** Binding **product planning direction**, not approved prices, finished content quotas, build results, finalized art or permission to start coding. No automatic transfer of local creatures or items into online competitive/tradable inventories. D-022 (smaller launch fallback) remains a risk mitigation option to bring back for explicit review if budgets fail. No purchased item influences species rarity, power, encounters, XP, story progression or ranked rewards. See Product Vision and Offline/Online Architecture for proposed implementation consequences.


## 8. Windows platform compatibility approval — 2026-10-20

**D-012 confirmed by owner:** development is performed on a **Windows 11** i3 10th-gen / 4 GB RAM PC; the **game must target Windows 7, 8, 8.1, 10 and 11**. Owner has **explicitly approved 64-bit only**: no 32-bit Windows OS editions or x86 executables are planned. `Windows 7 SP1 x64` remains the proposed *earliest practical test baseline*, not yet verified running WILDSIGNAL. SDL3 documents legacy Windows support, but the complete pinned C++20 runtime/dependencies and game are **NOT yet tested**. Steam client support for Windows 7/8/8.1 ended Jan 1 2024, so the legacy-OS game needs a **separately planned standalone delivery path** and offline-first runtime independent of Steam. Secure online co-op on legacy OS needs separate verification; do not remove the general Android↔Windows launch co-op requirement or promise Win7 online parity without tests. See `docs/architecture/WINDOWS_7_11_COMPATIBILITY_v0.1.md`.

**No game implementation authorized** by this platform-planning amendment.


## 9. 64-bit scope explicitly confirmed — 2026-10-20

User responded **“64 bit is enough.”** This closes the architecture-scope question: the Windows game supports **x86_64 (64-bit Windows OS and 64-bit game binary)** as its sole planned Windows architecture. Do **not** include Win32/x86 builds, 32-bit compatibility test cases or store claims in initial release scope. The requested Windows version range remains **7, 8, 8.1, 10, 11**, with Windows 7 SP1 x64 proposed as oldest operational baseline and still requiring real tests. Windows 7/8/8.1 Steam-client support is not available; plan separate legacy standalone delivery. This is a confirmed planning constraint **not permission to start implementation**.


## 10. Explicit post-RENDER-001 visual feedback — 2026-10-21

Owner confirmed **Lantern Quay and Liltwood CONCEPTS are approved**, while the **actual current graphic look is NOT APPROVED**. This overrides any earlier imprecise statement that RENDER-001 is an art-direction success. First-person functionality/CI and visual quality are separate gates; the pixel-art environment kit/production renderer is not locked. See `docs/design/VISUAL_QUALITY_RESET_v0.1.md`. The user's uncapped Windows screenshots show instantaneous ~308.6 FPS (320×180, one placeholder) and ~137.2 FPS (480×270, 24 placeholders); these indicate headroom but not certified sustained FPS.


## 11. Approved ART-ENV-001 visual references — 2026-10-21

Owner reviewed the generated Quay/Liltwood environmental reference images and responded **“yes these are good.”** This confirms target *style and atmosphere* only. The 27 texture and 27 prop crops remain candidates and NOT game-ready; seamlessness, edge alpha, world scale, roof/trunk geometry and in-engine appearance still require ART-ENV-002. Artwork stays private, publication requires explicit authorization, and original hand-drawn creature source is unchanged. See `docs/design/assets/ART_ENV001_APPROVAL_AND_HANDOFF_v0.1.md`; persistent Codex guardrails added to root `AGENTS.md` on the planning branch.
