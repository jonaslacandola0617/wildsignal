# WILDSIGNAL — Next Pre-production Milestone: Asset & Engine Readiness v0.1

**Status:** PLANNED ONLY; no implementation authorized. **Planning approvals incorporated 2026-10-20.**
**Goal:** prepare original sprite assets, consistent reference compositions, minimum-device definitions, and research-ready architecture handoff so the first Codex rendering experiment can be approved *later* without guesswork. Do not implement source code, extract/crop/replace creator sprite assets, install SDKs, or initiate runtime benchmarks under this milestone without a separate user request.

## 0. Product guardrails — APPROVED FOR PLANNING
- Free-to-play on Android (Google Play) and Windows (Steam), with **optional cosmetic-only purchases**. No pay-to-win, loot-box power, real-money creature stats or paid access to story-critical routes.
- Aspirational commercial 1.0: **six** explorable zones (Z01–Z06), **18–24** creature species (from creator-owned art), **3–5 hours** of story. These are **feasibility-gated targets**, not release guarantees or authorization to mass-produce content.
- **Two-player Windows↔Android cross-platform co-op at commercial launch**; no silent postlaunch deferral. Trading and ranked PvP are later possibilities.
- **Two trust domains:** Local Adventure remains playable offline with locally stored creatures; Online Expedition uses server-verified characters/items/rewards. Casual online sessions may visually project the appearance of familiar offline companions **where feasible**, without transferring their ownership, combat stats, inventory or entitlements.
- Current first prototype requirement remains 15–20 min offline, four zones, three starters and guardian, independent of content aspirations.

## 1. Deliverables in this milestone — documents and reviewable plans

### PRE-ART-001: Owned creature master archive + provenance plan
**Known available original-art source:** creator's surviving combined 10×10 raster sheet `image(20261020-050620).png` (earlier catalog reference); **editable Aseprite master is unavailable**, formerly stored on a broken desktop. It is NOT a prerequisite. Plan lossless segmentation and creation of new editable derivative files; never request redraw of original creature designs.
**Documentation output:**
- Asset-source register with creator, original/canonical surviving flat image and version, ownership statement provided by creator, approximate creation date if known, 10×10 grid layout, padding, native sprite bounds (verify), and a clear note that original historical Aseprite layers are lost. Optional old-disk recovery does not block progress.
- Decide art visibility: whether originals go in public Git, Git LFS or private Drive/library (the user retains commercial ownership; collaborator access requires explicit choice).
- Naming strategy: `WS-001..WS-100` immutable slot identity; (WS-004, WS-008, WS-050) starter candidate names remain provisional until owner approval.
**Acceptance:** can unambiguously trace chosen sprite in production to source-cell ID without overwriting original. No undocumented AI-redraw of creator's creature artwork.

### PRE-ART-002: Sprite extraction & animation specifications (still no extraction work)
**Documentation output:**
- 3 starter crop-box/alpha-mask review method, plus guardian `WS-024` optional; background/grid removed in future using nondestructive workflow, inspected on light/dark/checkerboard backgrounds.
- Source-coordinate/pivot/origin metadata, transparent PNG export naming, facing conventions (front/left/right/back), idle/follow/battle state list, font-independent icon scales, safe atlas padding to prevent bleeding; estimate animation workload only after inspecting real source file.
- Visual acceptance plates for starter sprites alongside an NPC at low res in town, forest, ruin; no false "3D model" requirement.
- Prototype asset-production cost gate: 3 starters + guardian first, not all 100, unless user separately approves.
**Acceptance:** written per-sprite checklist and exporter contract sufficient to direct a future art/Codex task without loss of outlines/color/pixels.

### PRE-VIS-001: Three reproducible hero camera reference boards
**Documentation output:** an annotated art target for **Lantern Quay, Liltwood Forest, Echo Hollow** with layout landmarks, palette swatches, texture families, first-person camera directions, horizon/ceiling expectations, sprite layering, example foliage budget and HUD-safe areas.
**Inputs:** VISUAL_BIBLE, WORLD_BIBLE, FIRST_SIGNAL_ZONE_BLUEPRINT, user-favored pixel dungeon reference.
**Comparability rules:** same camera position/FOV target across renderer methods; no real third-party copyrighted reference art in shipped content; evaluate tall arches, ceiling, sky and irregular walls honestly.
**Acceptance:** art owner approves "this screenshot target feels like WILDSIGNAL" for each scene; no final hard-to-render complex architecture until proof.

### PRE-TECH-001: Device, toolchain, and benchmark **planning**, not tests
**Owner-supplied baseline devices:** Windows-capable Intel Core i3 **10th generation with 4 GB RAM** (exact CPU/GPU/Windows version unverified), and **Samsung Galaxy S10+** smartphone (actual RAM/SoC/Android version unverified; device family often 8–12 GB RAM). **Documentation output:** record their exact variants, native Windows + cloud Android build workflow, toolchains and source-control method. **No separate 2 GB RAM device has been confirmed available**, so 2 GB compatibility remains untested.
**Test protocol:** 320×180/400×225/480×270 world framebuffer, 30 FPS candidate, p95/p99 frame times, PSS/RSS, battery/thermals, sprite/fog overdraw, 20 min stress scene. Compare simple raycast and sector wall renderer in later R&D only if initial renderer cannot meet visuals.
**Acceptance:** named test hardware or explicit lack-of-hardware blocker, methodology with known measurement tools, signed-off test scenes and recorded reference camera compositions.
**Stop condition:** if no real low-end device/test strategy, do not assert final minimum specifications.

### PRE-SYS-001: Cross-system design reconciliation
**Documentation output:** review approved/planning/source-of-truth status for all 4 first-slice maps, entity trigger IDs, quest flags Q001–Q005, actual sample wildlife placements, starter labels/skills and 18 move definitions. Remove unresolved contradictory rules or mark them BLOCKED before Codex task.
**Acceptance:** small invariants table: each portal resolves to one spawn; all critical story puzzles solvable by every starter; Battle Test Vectors align with move JSON, and save schema owns core progression flags.
**Note:** Previous 15×11 map superseded by 48×32 blockout v0.2 for coordinates, but the latter is graybox only.

### PRE-NET-001: Two-player co-op user-journey + offline-companion visual-only contract
**Documentation output:** paper flow for Local Adventure → Online Home → invite/join → two-player instance → departure → return to Local Adventure. Illustrate how local companion **appearance** may be used noncompetitively without implying online ownership, what happens when visual projection is unavailable, and how online-only cosmetics and server inventory differ.
**Acceptance:** a newcomer can tell "my offline companion" from "verified online creature" without reading technical language; no local save import to competitive inventory; no compulsory online startup; plan two devices/OS types for multiplayer test later.
**Out of scope:** implementing backend, authentication, ranked PvP, creature trading or real-money purchase.

### PRE-COMM-001: F2P cosmetics and store-readiness policy paper
**Documentation output:** cosmetic categories, free/default outfits, no-power design rules, future SKU/licensing/provenance ledger, server receipt verification/refund principles for separate Steam vs Google purchases, legal/privacy/age-rating checklist and initial infrastructure/server-cost estimate template.
**Acceptance:** monetization does not influence battle move effectiveness, XP, encounter odds, bond points, exploration access, stats or progression; no platform-circumvention checkout flow. **Prices and SKU count not approved until cost and store policy review.**
**Out of scope:** processing purchases or showing payment checkout in app.

## 2. Phased review order (NOT programming schedule)

**Phase A — Lock sources and specs:** review starter identity/name assignments and asset ownership/storage choice; create canonical source ledger; reconcile four first-zone quest/map/battle IDs. Outputs PRE-ART-001, PRE-SYS-001.

**Phase B — Design camera/production references:** approve the exact 3 first-person scene compositions and low-resolution sprite QA contract. Outputs PRE-ART-002, PRE-VIS-001.

**Phase C — Confirm test environment:** identify lowest Windows + Android hardware available, minimum audience device claims to test, cloud-build intent, metrics; validate whether technical experiment can begin later. Output PRE-TECH-001.

**Phase D — Resolve launch experience expectations:** paper-flow two-player cross-play and appearance-only offline companion, monetize via optional cosmetics without compromising offline mode, define account and entitlement boundary. Outputs PRE-NET-001, PRE-COMM-001.

**Phase E — Owner review for IMPLEMENTATION AUTHORIZATION** (separate future interaction): approve/deny small `ENGINE-001` and `RENDER-001` future Codex task only, with exact git commit and test evidence. No automatic go-ahead just because documentation is complete.

## 3. Proposed acceptance gate checklist

- [ ] Source sheet/master provenance and public/private storage method approved.
- [ ] Starter WS-004/008/050 (and guardian WS-024) art-cell mapping visually confirmed by owner.
- [ ] Sprite extraction/pivot/transparency QA specification approved.
- [ ] Three hero compositions checked against source mood and 2D/pseudo-3D rendering limitations.
- [ ] Exact reference camera/zone/map fixture version approved for renderer test.
- [ ] Intro quests/battle/puzzle/type and event IDs crosschecked without contradictory rules.
- [ ] Lowest available Windows and Android test hardware recorded or blocker stated.
- [ ] Engine architecture candidate C++20 + SDL3/CMake and benchmark criteria reviewed.
- [ ] Two-player co-op user journey and cosmetic-only offline visual projection fallback approved.
- [ ] F2P cosmetic principles and storefront compliance checklist prepared, **not implemented**.
- [ ] Draft PR #1 and PR #2 cross-reference order resolved before production code integration.
- [ ] Explicit owner message approves **only** the initial technical proof's scope.

## 4. Cross-checked deliverables and responsibilities

| Deliverable | Decision owner | Future maker | Required evidence |
|---|---|---|---|
| Original art source/provenance | User | Art/tool workflow | source hash and inventory |
| Starter identity and view-angle brief | User with design planning | Pixel artist / future tooling | owner-reviewed side-by-side contact sheet |
| Hero scene references | User with design planning | Environment artist | approved annotated layouts |
| Device benchmark plan | User approves tradeoffs | Codex later, after permission | target hardware description and method |
| C++ architecture choice | Owner approval after tradeoffs | Codex later | ADR + platform tests |
| Online visual-only projection | User approves UX | Codex after network task approved | mocked screen flow and trust test |
| Cosmetic store spec | User approves SKU policy | Game backend/platform integration later | no-pay-to-win and receipt test plan |

**Success:** when this milestone is complete, an ENGINE-001 technical *experiment* can be separately approved and handed to Codex with exact specs, realistic goals and reviewable evidence. The first playable/game implementation remains a later project gate.


## 5. Hardware and lost-editor-source intake (owner update 2026-10-20)

See `docs/planning/HARDWARE_AND_SPRITE_RECOVERY_INTAKE_v0.1.md` for the verified user-supplied device inventory, unknown hardware fields and non-destructive flattened-sheet reconstruction workflow. The Galaxy S10+ is useful for actual Android touch, notches and thermals, but is **not a proxy for a 2 GB entry-level phone**; the 4 GB i3 PC is the available low-memory Windows baseline. Cloud Android builds are preferred over heavy local SDK installations if the machine struggles.

**New source-policy invariant:** Original editable Aseprite file is unavailable from a broken desktop; this is not a reason to discard or replace the user's 100 designs. The flattened surviving sprite sheet is the primary available art source. Preserve it unmodified, crop each of the 100 immutable grid IDs losslessly, manually verify transparent masks and create **new** editable layered files for later animation. Seek old drive recovery only as an optional enhancement, not a release blocker. No sprite cleanup or new art has been performed merely by publishing this plan.


## 6. Legacy Windows compatibility extension (2026-10-20)

Owner's desktop OS confirmed **Windows 11**; desired game support explicitly **Windows 7, 8, 8.1, 10 and 11**. PRE-TECH-001 must therefore select a Win7 SP1 compatible `x64` ABI/toolchain candidate without locking unsupported imports, define legacy startup/save/audio/SDL test rigs, distinguish Steam (supported modern Windows) versus standalone (Win7/8/8.1) distribution, and avoid mandatory online/Steam calls in offline mode. An extra task `PRE-WINLEG-001`: record Win7/Win8/Win8.1/Win10/Win11 per-OS acceptance matrix and test-hardware availability; document 32-bit OS question and secure-legacy-online risk. Actual builds remain unauthorized. Detailed `docs/architecture/WINDOWS_7_11_COMPATIBILITY_v0.1.md`.


## 7. Planning deliverables produced from the owner's "proceed" instruction (2026-10-20)

**Completed AS DOCUMENTS ONLY; no art edits or implementation:**
1. **PRE-ART-001** — `docs/design/assets/ORIGINAL_SPRITE_SHEET_MANIFEST_v0.1.json`: inspected actual user-owned Library PNG (850×832 RGB), recorded its source SHA256 and 100 immutable row/column source records, with **candidate** crop boxes (not exported sprites). The source PNG remains outside public Git.
2. **PRE-ART-002** — `docs/design/assets/PILOT_SPRITE_RECOVERY_SPEC_v0.1.md`: four pilot source IDs WS-004/008/050/024, original appearance observations, lossless alpha/restoration QA, reconstructed editable art protocol and proposed minimal animation budget. Actual sprite extraction and new animations **NOT STARTED**.
3. **PRE-VIS-001** — `docs/design/HERO_SCENE_TARGETS_v0.1.md`: three comparable first-person benchmark composition briefs (Z01 Quay / Z03 Liltwood / Z04 Echo), lighting/palette, camera starts, sprite count tests and unsupported renderer features. Actual images and environment assets **NOT CREATED**.
4. **PRE-TECH-001** — `docs/architecture/DEVICE_VALIDATION_MATRIX_v0.1.md`: owner Win11 i3/4GB and Galaxy S10+ as real available devices, explicit x64-only Windows 7–11 functional test plan, unavailable legacy/2GB baseline cells and performance-report template. No results recorded.
5. **PRE-NET-001 + PRE-COMM-001** — `docs/planning/COOP_COSMETIC_HANDOFF_v0.1.md`: two-player Win↔Android casual co-op onboarding paper flow, server-verified progression boundary, optional cosmetic local-companion appearance, non-pay-to-win IAP constraints, pending receipt/account/security questions. No checkout/network implementation.
6. **PRE-SYS-001** — `docs/planning/FIRST_SLICE_CONSISTENCY_GATE_v0.1.md`: Quest/move/map ID traceability, explicit discrepancies resolved in Q003/Q004/Q005 prose and battle tie-summary, and 14 remaining implementation/geometry/asset validation blockers.

**Review outcome required before moving to code:** owner approves/private storage strategy for original art exports; pilot source IDs and derivative transparency policy; three scene visual briefs; available actual baseline device details; future small ENGINE-001 implementation test packet **separately**. Completion of written deliverables is NOT proof of renderer performance, battle implementation, 2GB Android support or Windows 7 startup.
