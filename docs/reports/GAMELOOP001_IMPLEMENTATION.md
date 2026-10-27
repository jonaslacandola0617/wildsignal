# GAMELOOP-001 implementation and review boundary

Implements the offline playtest authorized by Issue47 and private art Issue35 on the existing GAMELOOP branches/draftPR48 and private36. The original Petalyn, HOLLAND, R4/R5 and Quaternius experiments remain unmerged and unchanged. This is an isolated first-person playtest, not a new official camera decision.

## Actual functionality

Native C++20/SDL3/OpenGL3.3 exploration, circle-vs-AABB collision and sliding, mouse look/focus-loss pause, contextual interaction, animated PetalynA following with real16-joint Idle/Fly/Attack skinning, a second in-world wild Petalyn, temporary attack/defend/befriend turns and win/lose return. Recruitment visibly changes the second actor into a companion. The genuine modular MegaKit cottage provides an exterior shelter/rest interaction. Interior traversal is not implemented.

The save records position/view, HP, recruitment and cottage-visit flags. Temporary validated writes, file flushing, atomic replacement and a valid backup support reopen and corruption recovery. Mid-combat closing preserves the last exploration checkpoint. Invalid positions fall back safely; asset absence fails with an error rather than rendering substitute art.

## Integration audit

- Base: Petalyn runtime PR46, `27dcaffcd1ae9ff0edaf81d75ee8a6285dddbc1e`; existing gameplay scaffold `c36c0531bcf2fc5f02b4eea88747f4996efedeea`.
- Bounded instancing adaptation: R5 `694668b5e37f64cf72293fcd0994fcc03a48f2c1`, with six existing near/far mesh variants. No blanket branch merges.
- Actual private Petalyn animation source: artPR28 `e371820898853614400b109baeab4d28b674f8fe`, unchanged source GLBs and atlas. No canonical creature redesign.
- Actual private glade source: artPR32 `74e407e696f1289a4f33cceb3b7d52291bf64022`. Existing terrain/trees/shrubs/rocks/flowers are exported as real meshes. World transforms are baked and Blender-only texture projection is adapted to UVs; original masters remain unchanged.
- Authentic Quaternius Standard Medieval Village and Fantasy Props archives are SHA-verified against the prior PIXEL3D001 inventory and their CC0 notices. Cottage buffers, hierarchy, UV0 and normals are retained. Derived runtime material subsets retain compatible base-color/alpha channels; normal/ORM engine-specific shading is not implemented. Barrel, Bench and Lantern are actual pack models.
- All art, binaries containing private art, captures and provenance files stay in private `wildsignal-art`. This public repository contains source and technical descriptors only.

Historical rootAGENTS references to `VISUAL_QUALITY_RESET_v0.1.md` and `ART_ENV001_APPROVAL_AND_HANDOFF_v0.1.md` were absent on this experimental baseline; current explicit Issues47/35 and verified actual sources govern the scoped implementation. No missing asset was silently recreated.

## Measured coverage and comparison method

`tools/coverage.py` produces actual matched 480×270/640×360 renders from five gameplay-height cameras plus a near-overhead diagnostic camera. A designated visible-meadow mask excludes paths, building/rock footprints and the encounter clearing. Blade silhouettes and explicitly painted R4-palette ground-cluster footprints are measured separately; plain base green does not count.

The final private report records all percentages and native masks. All five first-person views exceeded75% combined cover at both resolutions (approximately90–95%); geometry alone is lower and separately reported (roughly60–80%). The distant overhead view relies on the authored ground representation and exceeded82% combined coverage. These are projected image-area measurements, not botanical density, hardware performance or owner artistic approval.

Density profiles are fixed deterministic subsets of the same short R5 roots. Grass uses instanced draws, conservative bounds culling, distant source-mesh LOD and authored ground fill. Blade casting, wind and shafts are OFF. Cached512px environment shadows exclude animated actors; moving companions do not invalidate the static cache. The grass surface uses the soft-even authored-color/diffuse treatment.

## Validation and hardware limits

Local Linux/Mesa testing passed15 CTests including the actual SDL-control exploration/cottage/battle/recruit/save/reopen/corruption/loss-recovery loop, genuine skin/mesh loading and inherited GL regressions. The final code is also built with MSVC x64 in Windows CI, which runs source-only logic tests and parses the hardware launcher. CI compilation and unit tests are not a physical Windows graphics or IntelUHD playtest.

Private deliverables include native source/unbanded versus toon, source-texture versus256-budget, houseOFF/ON and density comparisons, actual input-driven motion, raw software measurements and the portable Windows build. `PLAY_GAME.cmd`, `BENCHMARK_LOW.cmd` and `BENCHMARK_HIGH.cmd` are included. Hardware launchers use identical routes with capture disabled, exclude warmup, record valid GPU sample counts, process working set and CPU/animation/submission/pacing separately. Swap-completion intervals are not verified displayedFPS.

IntelUHD i3/4GB, physical Windows gameplay, Android, Windows7 and online systems are NOT RUN. No final art, performance or architecture approval is claimed. The cottage is technically usable and needs a WILDSIGNAL material/style pass (`USABLE_WITH_TEXTURE_STYLE_PASS`), not automatic adoption for a full town. The finite12m scene, simple local follower avoidance, repeated grass silhouettes and low-resolution shimmer remain prototype limitations. No sound, interior, full quest/progression economy, village expansion, multiplayer or monetization was added.

**GAMELOOP001_DENSE_GLADE_MEGAKIT_OWNER_PLAYABILITY_AND_INTEL_REVIEW_PENDING**
