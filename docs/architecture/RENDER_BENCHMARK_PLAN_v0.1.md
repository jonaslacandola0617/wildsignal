# WILDSIGNAL — Renderer, Memory & Platform Benchmark Plan v0.1

**Status:** Design for tests; **NO executable benchmarks performed yet**. Date 2026-10-20.
**Decision informed:** Select fastest/most visually sufficient native 2.5D renderer for low-end Android and Windows; determine whether user's tall-arche dungeon reference is achievable in software or needs constrained art geometry.
**Companion:** ENGINE_ARCHITECTURE_v0.1.md. **Art:** VISUAL_BIBLE_v0.1.md. **Maps:** FIRST_SIGNAL_ZONE_BLUEPRINT_v0.2.md.
**Codex implementation ONLY after ENGINE-001/RENDER-001 task approval.**

## 1. Measurable goals vs currently unverified targets

**Minimum hardware goal:** Windows PC with ~2 GB installed RAM and no dedicated GPU, plus a low-end Android phone with ~2 GB RAM. Whether actual Windows 10/11 + Steam + background OS services are tolerable with only 2 GB RAM is **not established**; a 2 GB system may struggle just to run a modern OS. Do not claim it as a guaranteed minimum system requirement.

**Experimental performance gates (not claims):**
- 30 FPS stable foreground gameplay on each *selected actual device* over **at least 20 continuous minutes**, with consistent controls and no critical crash.
- Frame time P95 <=33.3ms for a 30 FPS cap; P99 <=50ms desirable; budget GPU upload/UI/present separately. A P95 alone is not a full smoothness guarantee.
- Aim for 60 FPS **optional** on capable devices without excessive battery drain.
- Candidate process footprint objective: Android PSS <250 MiB and Windows process RSS <350 MiB during test scene, **subject to device evidence and OS headroom**. These are screening budgets, not system requirements/guarantees.
- 320×180 (225 KiB RGBA), 400×225 (352 KiB), 480×270 (506 KiB) world pixel buffers evaluated; UI drawn at higher display resolution.
- No hard requirement for graphics APIs unavailable on integrated/mobile graphics; graphics presentation cost still profiled.
- Sustained Android energy/thermal quality: compare device temperature/performance at start, 10 and 20 minutes; throttle indicators documented. Battery drain metrics only when instrumentation reliable.
- Subjective pixel-art success: environment silhouette/layout, lighting, arches/pillars, sprite occlusion, floor/ceiling, vegetation and optional looking up/down accepted by creative owner after side-by-side screenshots.

## 2. Test matrix — these are specific experiments, not hardware findings

| Axis | Cases |
|---|---|
| Platform | Windows x64 with integrated/basic graphics; Android arm64 low-cost test phone |
| Operating system | actual build/version recorded, SDK min target decided later |
| Rendering method | A grid raycast; B sector/portal software columns; C selective custom rasterizer if required |
| Resolution | 320×180, 400×225, 480×270 |
| Display pipeline | CPU framebuffer -> SDL present; compare SDL software/accelerated window renderer where viable |
| Scene | Z01 Quay plaza; Z03 Liltwood forest; Z04 Echo Hollow high-arch room |
| Camera | stationary and walking/turning 360°, forward and pitch attempts where supported |
| Sprites | 1 creature, 8 mixed actors, 24 stress sprites with foliage overdraw |
| Session | cold launch, 1min warm-up, 3min capture, 20min thermal soak |
| UI | none, exploration HUD, battle UI overlay |
| Input | keyboard/mouse with fixed test path; phone virtual joystick and look drag |
| Device condition | unplugged/cool start and plugged-in reference if relevant; note power-saving settings |

**Never compare one renderer drawing only simple walls against another drawing tall arches/floor/ceiling:** fix content feature parity or mark feature NOT_SUPPORTED and explicitly score visual limitations.

## 3. Standard benchmark scenes

### BENCH-Z01: Lantern Quay (town)
- Representative warm-harbor hero view from existing blueprint `Z01_SPAWN_SQUARE(24,29)` facing north to beacon/plaza; add 4–8 actor billboards, a few lanterns and static skyline.
- Features: large sightlines, textured building faces, broad walkable plaza, sky/background, visible far structures.
- Record draw distance, active texture/atlas footprint, sprite sort count, shadow/backdrop tricks and FPS while turning.
- Success criteria: believable town not a maze of corridor walls, navigable path with distinct beacon landmark.

### BENCH-Z03: Liltwood Forest
- View near Singing Root (29,14) and Veiled Wall (36,10); 10–20 foliage sprite cards at different depths, 1 following owner-owned starter, 1 wild sprite, 1 signal animation.
- Features: occluding near-foliage, transparent overdraw, shaded distance fog, clear walk paths, puzzle visibility and follower navigation.
- Success: no alpha sorting artifacts on common angles, creature silhouette legible, forest does not visually look like square dungeon maze.

### BENCH-Z04: Echo Hollow
- Hero view into russet-brick arch chamber from (24,17), with tall columns, ceiling texture, hanging vines, multiple billboards, illuminated sigil/pedestal and faux higher alcove.
- Features: tall wall illusion, floor/ceiling mapping, pitch option, perspective distortion, consistent occlusion.
- Success: evokes user's provided reference *without copying that game's copyrighted art*. Critically record unsupported bridges, true arches, stacked rooms, pitch motion or vertical structure as feature gaps. Do NOT mark visual pass just because a screenshot has orange brick and green vines.

### BENCH-UI: combat overlay
- Keep forest/ruin world image underneath low-res art; present crisp 4-card action panel, HP, Signal Energy, character sprites; rapidly toggle menus 200 times while tracking memory.
- Success: button legibility on real phone, no major dropped taps or rising retained memory.

## 4. Candidate renderer decision criteria

Weighting is a planning proposal; owner can revise creative importance:
- 35% **visual viability** against reference: immersion, nonrectilinear shape illusions, sky/outdoor forest, ruins, depth, user approval.
- 30% **low-end performance:** 30 FPS P95/thermal/memory on Android and Windows.
- 15% **engineering complexity:** Codex maintenance, source readability, debugger visibility, asset tooling and collision interactions.
- 10% **art authoring cost:** tile kit reuse, consistent geometry and environment scale.
- 10% **cross-platform risk:** SDL presentation portability, OS lifecycle, CI build reliability.

**Scoring:** rate each 1–5 per scene/dimension with supporting screenshot/log; any critical failure (e.g. Android can't launch or visual architecture unacceptable) is an immediate no-go even if weighted score high. No renderer selected by aesthetic adjectives alone.

| Renderer | Anticipated advantage | Possible limitation | Decision |
|---|---|---|---|
| Grid raycaster | fastest path to early visual prototype, simple columns and 2D collision | rigid wall heights and near-square architecture, ceilings/arches are hard | Benchmark first |
| Sector/portal columns | irregular wall segments, variable sector heights and room portals, closer to old FPS | clipping/occlusion/pitch/collision complexity; CPU cost, tooling demands | Benchmark comparison |
| Small CPU rasterizer | broad 2.5D meshes/room dressing without GPU shaders | engineering/thermal cost high, bigger risk for 2 GB class | Contingency only |

Fallbacks should be explained to owner. Example: if tall arches are too expensive, compose them with static foreground frame/sprite trims and only render simple collision; do not claim literal 3D architecture support.

## 5. Telemetry schema and measurement protocol

Each benchmark run emits a machine-readable JSON report and CSV frame stream *without PII*:
```json
{
  "schema": "wildsignal-render-bench-v0.1",
  "buildSha": "REPLACE_WITH_ACTUAL_COMMIT",
  "renderer": "grid_raycast",
  "deviceLabel": "anonymous-hardware-profile",
  "osVersion": "REPLACE",
  "cpuDescription": "REPLACE",
  "gpuDescription": "REPLACE_IF_KNOWN",
  "installedRamMiB": null,
  "screenResolution": [0,0],
  "worldResolution": [320,180],
  "zone": "BENCH-Z03",
  "durationSeconds": 1200,
  "medianFrameMs": null,
  "p95FrameMs": null,
  "p99FrameMs": null,
  "drawWorldMsP95": null,
  "drawUiMsP95": null,
  "presentMsP95": null,
  "processPeakRssMiB": null,
  "androidPssMiB": null,
  "thermalObserved": "not-measured",
  "inputLatencyMsP95": null,
  "spritesVisibleMax": null,
  "textureCacheBytesPeak": null,
  "renderErrors": [],
  "memoryLeakTrend": "unknown",
  "result": "NOT_RUN"
}
```

**Measurement:**
- Use monotonic clock, carefully separate simulation, ray/cast, sprite sort/draw, UI and display/present, input, asset decode and I/O times.
- Sample frame times after 1 min warm-up; compute median, p95 and p99 from at least 3 min per test; 20 min thermal soak on Android.
- Log memory from platform-appropriate OS APIs and note if RSS/PSS not available, rather than replacing with guessed heap bytes.
- Report device temperature/thermal status only with instrument/provider method in report.
- Record 2–3 representative screenshots for each scene/resolution and a short camera movement clip.
- Run builds with pinned compiler flags; avoid comparing debug build against release. Test 30 FPS and uncapped separately.
- For every run capture full device class, CPU, RAM, OS and whether it uses integrated/display acceleration; store reproducible `run_id` and assets/test fixture versions.
- Do not claim cross-device determinism of GPU/driver software presentation. Battle deterministic checks are separate.

## 6. Hard checks / acceptance cases

1. Windows x64 release and Android arm64 debug package actually launch; Linux-only unit test is insufficient.
2. Cold boot in airplane mode loads benchmark/test without blocking on network.
3. The same owned sprite's outline, palette and transparent border look correct in town/forest/ruin at all candidate world buffer sizes.
4. A 20min run has no continuously rising texture count or retained memory beyond documented cache bounds.
5. Collision never allows stepping through textured wall or portal without flag; camera and companion don't clip near thick pillars.
6. Sprite in front of wall appears in front, behind wall appears behind, foliage drawn with correct alpha priority or acceptable explicitly documented simplification.
7. A 30 FPS cap reaches 33.3ms P95 goal on documented real baseline devices; if not, log failure and choose lower quality or architecture revision.
8. Looking up/down behavior recorded honestly as supported/limited/not supported; mobile camera comfort tested.
9. UI remains crisp at device resolution, no misaligned cutout or unusably small action buttons.
10. Pixel scale holds stable through resizing and Android OS lifecycle; no crash/resume lost work.
11. Renderer A/B comparison uses exactly the same world, texture set and stress actor counts or marks feature mismatch.
12. Source image screenshot is used for **creative comparison only**, not embedded or republished as game art.
13. Test time/memory/battery taken from actual run files. Never generate a green PASS from theoretical estimates.
14. Select baseline renderer only after reviewing visual evidence and P95/P99/peak-memory numbers for both target platforms.

## 7. Development sequence and cost control

1. Build common C++/SDL3 bootstrap and low-res framebuffer with Win + Android cloud build; run input and startup tests.
2. Write renderer A raycaster, first one small test map, one owner's sprite after extraction (or temporary placeholder).
3. Expand same authoring data into 3 benchmark scenes and overlay UI, log performance, correctness, camera and thermal.
4. Implement renderer B **minimal representative chamber**, not a production engine; compare cost/visual gap. Only experiment with C if both unsatisfactory.
5. Make side-by-side decision matrix, photos, measured logs and an owner-approved ADR.
6. Freeze allowable level geometry/art materials, then upgrade draft blockouts into art-ready production maps.
7. Only afterwards implement quests and large creature sprite libraries.

## 8. CI/profiling outputs and repositories
- `bench/results/<run_id>.json`, `bench/results/<run_id>_frames.csv`, `bench/screenshots/<run_id>_<scene>.png`; raw performance traces may be large and best kept as CI artifacts / separate measured dataset.
- Commit `docs/architecture/ADR_RENDERER_SELECTED_v1.0.md` **only after owner signs benchmark recommendation**.
- Store screenshots made from WILDSIGNAL's own test scene, not the original third-party reference frame, unless licensed.
- Avoid claiming memory headroom from empty levels or framebuffer bytes alone.

**Outcome desired:** evidence-based renderer decision for a genuine lightweight native game, not an untested promise that any 2 GB device will run the finished game.
