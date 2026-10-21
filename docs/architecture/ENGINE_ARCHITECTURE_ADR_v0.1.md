# WILDSIGNAL — C++ Engine Architecture Blueprint & ADR v0.1

**Status: ARCHITECTURE PROPOSAL; NOT ENGINE IMPLEMENTATION / NOT APPROVED.** Date: 2026-10-20.  
**Goal:** native 2.5D retro first-person pixel art, 2D-authored maps, software-first rendering, offline full adventure, optional small-room online multiplayer, Android + Windows, minimal RAM/power usage.  
**Companion document:** `RENDERING_BENCHMARK_PLAN_v0.1.md`; product constraints: `PRODUCT_VISION_RELEASE_SCOPE_v0.1.md`.  
**Important:** low-end device target and likeness to screenshot unproven. No final renderer has been selected.

## 1. Decisions and assumptions register (architectural)

| ADR | Recommendation | State / reason |
|---|---|---|
| ADR-001: C++ standard | **C++20**, no exceptions across C ABI, RAII/smart ownership | Proposed; aligns with developer preference |
| ADR-002: Platform layer | **SDL3** input/window/audio/events/timing/gamepad baseline | Proposed; official Windows & Android support |
| ADR-003: Build system | **CMake + Ninja** local; Android Gradle + SDK/NDK/CMake for packaging; cloud CI | Proposed; Android tooling still needed *somewhere* |
| ADR-004: World representation | 2D cell/sector map plus separate props, collision, spawn and quest layers | Proposed, built on 48×32 blockouts |
| ADR-005: Rendering | **Software framebuffer pipeline with selectable backends**, compare grid raycast, sector/portal, lightweight CPU raster | **BLOCKED UNTIL BENCHMARK** |
| ADR-006: Presentation | pixel framebuffer + independent high-resolution UI + OS/SDL surface or streaming-texture presentation | Benchmark CPU-only vs optional GPU-assisted upload; do not require discrete GPU |
| ADR-007: Simulation | fixed-step deterministic battle; world simulation independent of render rate; C++ shared logic | Proposed |
| ADR-008: Save format | stable IDs, schema versioning, atomic writes + backup; local story never needs network | Detailed spec still needed |
| ADR-009: Online | server-authoritative online ownership; distinct local/online realms **pending owner decision** | OPEN PRODUCT DECISION |
| ADR-010: Memory | per-zone lifetime arena + RAII pools, bounded allocs, on-demand atlases | Proposed, memory ceilings to benchmark |
| ADR-011: Audio | low-cost SDL3 audio path; SDL3_mixer evaluated, not assumed mandatory | Proposed |
| ADR-012: Store APIs | Android JNI/Kotlin bridge for Play Billing when required; Steamworks adapter optional until store launch | Proposed; no secrets in repo |
| ADR-013: Diagnostics | frame-time counters, peak RSS/heap metrics, structured logs and screen capture in test builds | Proposed |

## 2. Explicit dependencies and source-of-truth APIs

Official references checked 2026-10-20:
- SDL3 CMake introduction: https://wiki.libsdl.org/SDL3/INTRO-cmake
- SDL3 Android README, Gradle, CMake and JNI: https://github.com/libsdl-org/SDL/blob/main/docs/README-android.md
- SDL3 software renderer from `SDL_Surface`: https://wiki.libsdl.org/SDL3/SDL_CreateSoftwareRenderer
- SDL3 surface window update: https://wiki.libsdl.org/SDL3/SDL_UpdateWindowSurface
- SDL3 window software renderer/other backends: https://wiki.libsdl.org/SDL3/SDL_CreateRendererWithProperties
- Google Play target requirements: https://support.google.com/googleplay/android-developer/answer/11926878?hl=en
- Steamworks API: https://partner.steamgames.com/doc/sdk/api
- Steam microtransactions: https://partner.steamgames.com/doc/features/microtransactions

SDL3 and SDL2 wiki content can differ; **use SDL3 documentation and pin a tested SDL3 release**, not unversioned examples. Target/Gradle/NDK versions need matching at build time, and Steamworks SDK is not assumed freely vendorable.

**Google Play policy (checked 2026-10-20):** new mobile apps and updates submitted since Aug 31, 2026 must target **Android 16/API 36+** under current published requirements. This targetSdk requirement is **different** from minSdk: minSdk should be determined after testing hardware/library compatibility. Review official Play policy again before submission.

## 3. High-level program modules

```text
apps/win_client    apps/android_bridge           services/online_server [later]
       \                  /                               |
          platform_sdl3                                   |
             |                                            |
     +-------+----------------------+                     |
     | input | presentation | audio |                     |
     +-------+----------------------+                     |
             |                                            |
         client_app -------- local/world simulation ------+--> shared_protocol
             |                   |                        |
     +-------+-----------+ +-----+------------+           |
     | render_world_cpu  | | map/collision     |           |
     | render_ui_2d      | | companion AI      |           |
     | atlas/asset mgr   | | encounters/quests |           |
     +-------+-----------+ | combat_core       |-----------+
             |             | save_local       |
      data-authored zones  +------------------+
```

- `combat_core` is a pure/platform-independent C++ module that accepts state + commands + integer seed, returns new state + event list; no SDL, filesystem or UI dependencies. Tests derive from `BATTLE_TEST_VECTORS_v0.1.json`.
- `world` runs deterministic logical events where useful; render camera can interpolate player and companion movement between fixed simulation steps. Quest flags and item ownership live in simulation/state storage, never sprite components.
- `render_world_cpu` consumes immutable visible-scene data and draws texture columns/planes/sprites to a small RGB(A) framebuffer; no native 3D assets required for baseline.
- `render_ui_2d` consumes read-only UI view models and draws crisp fonts/touch buttons after world upscale; internal world resolution must not make UI text illegible.
- `platform_sdl3` handles SDL events, audio device, window, file paths, frame timer and Android activity lifecycle; no game rules.
- `online_server` later uses shared battle/quest validation but owns server persistence and identities; avoid C++ object-layout serialization across network.

## 4. Rendering choices — engineering bake-off, not a hidden final selection

| Candidate | Strengths | Missing visuals / risks | Minimum test scenes |
|---|---|---|---|
| R1 — Classic grid raycaster | Small, fast concept; collision and texture columns simple; fits 2D map | Mostly axis-aligned vertical walls; heights, arches, bridges and stacked geometry are harder; outdoor vistas can feel maze-like | Town courtyard, forest paths, basic ruin hall |
| R2 — Sector/portal/column software renderer | Variable sector height/openings more faithful to Doom-like ceilings, arches/steps and vistas | Significant clipping, depth ordering and portal recursion complexity, potential worst-case fill/overdraw; mobile CPU risk | same scenes + high lintel/arch/floor change |
| R3 — Tiny CPU triangle rasterizer or hybrid 2.5D | Flexible geometry and perspective within CPU framebuffer | More geometry/Z/depth, sorting, optimization and maintenance; less low-end predictability | exactly equivalent art geometry/effect budget |

Provisional technical strategy: **begin with R1 to establish an executable baseline, but do NOT lock maps/art to R1 until R2 and R3 visual/perf test results are compared.** A carefully designed portal/sector renderer may offer necessary atmosphere at cost; this requires evidence, not intuition.

### Required test fidelity
Reference screenshot has detailed brick walls, big arches, overhead structures, lush greenery, sprite creatures and depth fog. Our benchmark scene must contain:
- textured and varied wall heights or a visually honest substitute,
- floor/ceiling texture + fog/lighting tint,
- one animated original owned creature sprite billboard with transparency and occlusion,
- one lantern/emissive accent, 8–16 sprite foliage/prop objects (configurable),
- visible outdoor sky/backdrop and at least two navigable paths,
- keyboard/mouse + touch camera control,
- crisp UI glyph and full-screen nearest-neighbor output,
- same basic camera trajectory and art palette across all rendering candidates.

Unsupported vertical look / arch must be called out clearly. Do not fake success by benchmarking bare colored walls against detailed target image.

### Frame buffer baseline
RGBA8 native `320×180×4 = 230,400 bytes` (~225 KiB) per framebuffer; `480×270×4=518,400 bytes` (~506 KiB). Double/triple buffering, full-resolution UI and upload surfaces cost extra memory. Pixel-only memory is not representative of process RSS. Compare 320×180, 400×225, 480×270 world resolution.

**Presentation pathways to test separately:**
A. CPU upscale -> SDL window software surface/present (minimal GPU demands but CPU memory bandwidth can be high).
B. CPU world buffer -> SDL streaming texture -> a lightweight renderer for scaled display (may use integrated/mobile GPU composition; no dedicated graphics card needed but not strict GPU-free).
C. Optional UI render target composition with nearest pixel world scaling, testing blending and sharp UI. Do not conflate "CPU world rasterized" and "never touches GPU"; platform screen compositors commonly use a GPU.

## 5. Frame scheduling, camera, movement

- World update: fixed tick **30 Hz provisional** with monotonic timing; clamp long resume deltas and handle app suspend. World tick should not depend on 30/60 Hz render frame rate.
- Battle: discrete commands/turns, no wall-clock physics; all damage and status resolution integer. Animation is an output consumer.
- World renderer: try 30fps cap first, 60fps on faster devices; disable expensive dynamic effects at low preset.
- Camera: XY position in 2D authored map, height/pitch if renderer supports; yaw look. Prefer continuous analog yaw and comfortable touch sensitivity. Collision uses circle capsule in 2D nav grid; no wall corner tunneling.
- Companion: bounded pathfinding (e.g. A* / BFS on local occupancy grid) and trailing behavior; fail-safe teleport only out of view; navigation independent from render.
- Level transitions are portals with stable IDs and save checkpoints, not open-world seamless streaming requirement.
- First person view cannot allow motion sickness through excessive head-bob, shake or sudden forced camera snapping; provide motion reduction options.

## 6. Memory, resource ownership and asset packaging

- **Ownership:** `std::unique_ptr`, value types, spans/views with explicit owner lifetime, `std::vector` reserved at load, no raw owning pointers. RAII for SDL handles via wrappers. Shared_ptr only for genuinely shared lifetime, otherwise avoid cycles.
- **Allocation:** split long-lived game state from temporary per-frame and per-zone arenas; monotonic allocation reset only when outstanding references are invalidated/checked. Use bounds checks/debug guardrails.
- **Textures:** compressed on disk, decode as needed to 8/32-bit indexed/packed surfaces; smaller atlases per zone; unloaded species should not stay in RAM.
- **Creature artwork:** user owns original 100 sprites; no need to load all 100. Vertical slice only draws 3 starter options plus guardian/ambient. Use sprite original pixels and alpha QA; do not automatically key out lime backdrop without inspection.
- **Caches:** bounded LRU or explicit zone unload; no unbounded font glyph/sprite cache or live animation history.
- **Data:** versioned JSON useful during prototyping; optional binary compiled packs after content authoring stabilizes. Validate IDs and schema in build tooling.
- **Audio:** stream longer tracks and decode compact effects; prefetch minimal cues; all audio must have commercial license/provenance.
- **Per-zone memory ceilings:** *not final*. Suggested initial engineering observation budget: ≤160 MiB process working set on menu; ≤250 MiB typical scene; ≤350 MiB high-water for intended 2GB-class devices. These are hypotheses/benchmarks, NOT device compatibility claims. Measure on actual Windows and Android low-memory devices.
- OS apps, Android runtime, driver and OS compositor share device RAM; total device RAM is not free memory. Simulate background app pressure and Android low-memory kill/restart.

## 7. Android specifics

- Build native C++ via NDK, hosted in standard Gradle Android app using SDL Activity/JNI layer. `arm64-v8a` primary ABI recommended. `armeabi-v7a` only if explicitly needed for low-end device coverage and dependency compatibility; check Google Play policies and support costs before deciding.
- **Target SDK 36+ per current published Google Play requirement**, if shipping under 2026 policy; minimum supported OS to be chosen after tests. Set proper native library alignment/package compliance when store specs require.
- Need lifecycle: background/foreground, touch orientation, display cutout/safe area, back navigation, audio interruption, controller/IME, permissions and scoped app storage.
- Build Android .apk for internal QA and **.aab** for Play publishing; CI signing keys in encrypted secrets, not Git, with a recoverable key-management plan. Test app install and real launch (not just successful compilation).
- Play Billing is a Java/Kotlin library integrated into native UI via dedicated JNI/platform service **only at commercial gate**; server verifies entitlements, handles refunds/revocations, idempotence and store restrictions. No payment logic in `combat_core`.
- A phone includes an integrated/mobile GPU. Goal is **no discrete GPU requirement and low CPU/memory use**; never promise Android hardware is literally GPU-free.

## 8. Windows and Steam

- Windows 64-bit primary deliverable; verify operating-system baseline and low-end hardware in device matrix. Compiler MSVC/clang-cl or MinGW after CI proof; prefer CMake targets without Windows-specific game logic.
- Steamworks integration behind `IPlatformServices` adapter; basic game runs without Steam process for offline local development if distribution requirements allow. Avoid mandatory Steam-only runtime assumption for Android.
- Steam cloud/achievements/overlay optional future integration, no direct Steam entitlement equals Google entitlement unless custom backend policy reviewed.
- Steam micropayments require Steam Wallet-oriented microtransaction APIs under Valve developer docs, while Android store integration uses Play Billing; do not reuse client-only entitlement flags.
- Crash reporting/telemetry optional and privacy-consistent; dedicated offline-first builds can run without uploads.

## 9. Audio and asset IO APIs

- `IAssetStore`: `loadZone(zoneId)`, `loadAtlas(atlasId)`, `unloadZone(zoneId)`, `openResource(id)`, `contentVersion()`; calls return explicit failure types.
- `IWorldRenderer`: `render(view, settings, framebuffer)` with no authority to mutate world.
- `IInputProvider`: maps SDL keyboard/mouse or touch events into canonical action IDs from UI_SCREEN_CONTRACTS.v0.1.
- `IBattleSimulator`: `resolve(battleState, commandPair, seed)` -> `state, eventLog` with serializable, platform-independent structures.
- `ISaveRepository`: `readSlot(id)`, `commitAtomic(slot)`, `listSlots()`, `restoreBackup(id)`, `migrateSchema(fromVersion)`; returns state, not UI.
- `IPlatformServices`: `isOnlineAvailable()`, `safeAreaInsets()`, `getAppSavePath()`, `getInputText()`, and **later** `initStore`, `requestPurchase`, `restorePurchases` isolated from gameplay.
- `INetworkClient`: optional connection/snapshot/actions; must never be initialized as blocking dependency to load offline game.

## 10. Data versioning, networking and security

- IDs `WS-001..WS-100`, `Z01..`, `Q001..`, `NPC_001..`, `SCR_..` from existing bibles; no object pointers in saves or packets.
- Data cross-validate: map portal IDs and destination spawns, quest flag references, move names/energy costs, sprite atlas metadata, UI actions; reject invalid content at build time rather than crash at runtime.
- Save: atomic write / backup checksum for corruption detection, not proof against cheating; loading fails safely if incompatible content without backup migration.
- Online server: should be authoritative for online species ownership, rewards, battles, trade and purchases; local saves are untrusted. No multiplayer server in first renderer benchmark.
- Network library (ENet/custom UDP/WebSockets) **not selected** until connection/session model and target mobile NAT/network conditions are specified. Never bake protocol into renderer.
- Defensive input validation, size caps and rate limits as soon as online proof is attempted.

## 11. Suggested portable C++ repo layout (PROPOSED; no files created)

```text
CMakeLists.txt
cmake/
third_party/SDL/               # pinned release/submodule; licenses documented
apps/
  desktop/
  android/                     # Gradle project + SDL Activity bridge
src/
  core/                        # math, IDs, resource lifetime, event logging
  platform/                    # SDL3, Android hooks, Windows wrapper
  world/                       # map, collision, companion, quest
  renderer/                    # framebuffer, raycast, portal prototypes, sprites
  ui/                          # screen router, fonts, layout, input mapping
  battle/                      # deterministic integer core
  assets/                      # atlas loader, cache and schema validator
  saves/                       # local save with migration
  network/                     # optional online client abstraction
tests/
  unit/                        # combat/state/IDs
  integration/                 # portal/save/quest
  performance/                 # reproducible scenes
assets/
  manifest.json
  env/
  creatures/
docs/
  design/
  architecture/
  planning/
.github/workflows/            # Windows and Android CI after toolchain proof
```

## 12. Codex delivery order (no feature-code authorization yet)
1. **ENGINE-001**: CMake+SDL3 Windows window/input init and Android Gradle/NDK smoke install; clean exit and save-path introspection; document reproducible build.
2. **RENDER-001**: render reference R1 raycaster with one 2D map and billboard on software framebuffer at 3 resolutions, diagnostic overlay, benchmark data output.
3. **RENDER-002**: run exact same scenes/camera paths against R2 sector/portal and optionally R3 simple raster candidate, measuring costs and visual omissions; do not commit to final art.
4. **INPUT-001**: map keyboard/mouse and touch into identical actions; verify lock/unlock and safe area.
5. **DATA-001**: IDs + schema validator for maps, creatures, quests and moves; reject missing portal/spawn and references at startup/build.
6. **COMBAT-001**: deterministic C++ library accepting test vectors, only after battle rules are approved.
7. **SAVE-001**: atomic local save, backup + schema migration, after data layout approval.
8. **FOLLOW-001**: companion-follow paths and fail-safe anti-clipping, dependent on approved world geometry.
9. **NET-001**: 2-player small-room server proof, after online trust decision and threat model.
10. **STORE-001**: platform billing ONLY when entitlement spec and merchant account ready.

Each task needs build commands, acceptance assertions, logs/screenshot evidence, error handling and bounded scope. Do not silently implement all ten at once.

## 13. Approval conditions before locking engine
- Visual side-by-side comparisons R1/R2/R3 on all three benchmark scenes and primary art reference objectives.
- Real Windows integrated-graphics/no-discrete-GPU and Android run logs: FPS/p95 frame, peak RSS, load times, thermals, battery where available, crash rate.
- Actual support of ceiling/arch geometry, wall height/pitch, entity occlusion and mobile UI readability; list compromises honestly.
- If no candidate meets visual goal on 2GB-class hardware, recommend bounded scope revision (small zones, fewer layers, lower internal res) rather than pretending both are achievable for free.
- Confirm viable Android toolchain and store-target compliance under selected pinned SDL3/NDK versions.
- Owner signoff on cross-mode trust, commerce and shipping feature set.

**Decision still open:** R1, R2 or R3 selected only by measured benchmark. Choose lightweight 2.5D, not full third-party 3D engine by default.
