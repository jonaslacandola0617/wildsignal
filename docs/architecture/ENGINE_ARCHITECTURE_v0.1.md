# WILDSIGNAL — C++ Engine Architecture & ADR-001 v0.1

**Status:** Proposed technical architecture, **NOT** implementation approval. Date 2026-10-20.
**Confirmed product constraints:** Low-end Windows PC + Android, native C++ and lightweight libraries rather than heavyweight installed game engine, first-person pixel art 2.5D/pseudo-3D, offline complete adventure, optional online multiplayer, cross-platform publishing via Steam/Google Play.
**Related:** PRODUCT_VISION_RELEASE_SCOPE_v0.1.md, WORLD_BIBLE_v0.1.md, VISUAL_BIBLE_v0.1.md, UI_UX_BIBLE_v0.1.md, OFFLINE_ONLINE_SYSTEMS_v0.1.md, RENDER_BENCHMARK_PLAN_v0.1.md.
**Principle:** Separate gameplay from platforms/renderers so renderer changes do NOT force rewrites of battle, creatures, save data or network protocol.

## ADR-001: lightweight native C++ baseline (RECOMMENDED, awaiting prototype)

**Language:** C++20, standard RAII ownership patterns and memory tests. C++ is performance-friendly but not automatically memory-safe. Avoid deep inheritance trees, raw owning pointers, per-frame heap churn and hidden nondeterministic physics/state.

**Foundation library:** SDL3 for window lifecycle, Android app adaptation, keyboard/touch/controller input, audio device abstraction and framebuffer presentation. SDL3's zlib license permits commercial inclusion subject to keeping required license records. SDL3 itself is a *cross-platform library*, not a heavyweight engine.

**Build system:** CMake + Ninja for small local Windows edits; Android uses NDK/CMake, Gradle wrapper and a minimal Android project/SDLActivity adaptation. Prefer building demanding Android/Windows release variants in cloud CI. Do not assume a high-end desktop GPU or local Android Studio install. Official SDL3 docs describe Android AAR/Prefab and CMake routes, including JNI/Activity glue; versions must be pinned to tested toolchains.

**Rendering:** custom depth illusion, rendering from 2D-authored world/sector data, textures and billboard sprites. **Candidate-first benchmark:** fixed-height grid raycaster with floor/ceiling casting and depth sorted sprites. **Alternative test:** modest sector/portal wall columns with variable height. **Experimental fallback:** CPU triangle raster path for demanding arched/offset geometry if measured viability. No choice is ratified until visual parity and CPU/battery benchmarking; no claim that simple raycaster reproduces the user reference's elaborate arches/overhangs.

**Presentation backend:** CPU software framebuffer generated from our own code (ARGB/RGBA; exact byte packing explicit), uploaded/displayed through SDL3. SDL3 provides a software renderer for SDL_Surface and conventional window renderers; compare software output vs accelerated framebuffer upload/present. **No dedicated PC graphics card requirement** is the goal; Android phones still use integrated/mobile display hardware. Do not promise CPU-only *display* on every Android device.

**Audio:** begin with SDL3's audio interfaces or a pinned SDL3_mixer version after checking dependency and distribution licenses. Use modest streamed OGG music and compressed short SFX; never decode an entire long soundtrack into memory. Resolve on actual audio compatibility rather than choosing based on name.

**Serialization:** simple data-driven JSON for authoring content/spec and validation; local save uses versioned stable IDs and atomic backup. Avoid code requiring a particular local DB service. A small embedded database may later be justified for online backend; not first client benchmark.

**Networking (deferred):** server-authoritative C++ service, transport comparison ENet reliable UDP vs WebSocket/TCP, credential/security requirements, authoritative turn-based sim, fixed protocol IDs and versioning. No single network library locked now. Local game works even with network subsystem absent.

## 1. Dependency/architecture layers

```text
Windows SDL Platform               Android SDL Activity/JNI adapter
          \                                    /
         [platform: window / touch / mouse / audio / lifecycle]
                              |
                        [game client shell]
                        /                 \
            [UI read-only views]      [World renderer]
             /        |                   |
      [typed input]  [asset service]  [render backend interface]
             |          |                 |
        [game simulation: quest / creatures / deterministic battle]
             |                    |
       [offline save service]   [optional online net adapter]
                                   |
                          [authoritative server]
                                   |
                      [account / trade / commerce verification]
```

- `game_core`: fixed-timestep simulation, deterministic battle, creature/quest/world data, stable IDs and event streams. No SDL includes, GPU or network dependency.
- `platform`: SDL3 lifecycle, events, timers/input collection, audio, paths and device signals; Android-specific Java/Kotlin bridge used **only** for platform services.
- `world_data`: authored tile/sector zone definitions, entity metadata, collision and navigation graph. Maps and quests loaded/validated from versioned content packs.
- `renderer`: framebuffer ownership, raycast/portal implementations behind `IRenderer`, camera, visibility/depth, texture/atlas caches, sprite billboards, UI compositor.
- `ui`: state machines and read-only view models. Typed actions instead of direct mutable world state. Combat animations consume deterministic simulation events.
- `storage`: local save, migrations, corruption recovery, content-version stamping and offline ownership. UI only requests save via typed action.
- `net_client`: optional login/lobby/co-op messages and interpolation, compiled/initialized only when online features requested. Never gates offline game initialization.
- `server`: independent headless executable using shared combat/world protocol, not SDL window code.
- `commerce`: platform-specific adapters + server verification, completely **excluded from proof-of-concept**; don't trust client receipt contents.

## 2. Tentative repository organization (proposal, no folders created by this document)

```text
CMakeLists.txt
cmake/                            # compiler presets and warnings
src/core/                         # stable ids, sim tick, events, deterministic math
src/world/                        # map collision, NPCs, AI, navigation
src/render/                       # IRenderer, raycast and/or sector, sprite sorting
src/assets/                       # atlas IO, resource cache, sprite metadata
src/ui/                           # state router and presentation
src/platform/sdl3/                # SDL3 lifecycle, audio/input, desktop presentation
src/platform/android/             # Android adapter, optional platform services
src/storage/                      # local save and migration
src/network/                      # future client network adapter
src/client/                       # game executable entry / composition root
src/server/                       # optional headless server
include/wildsignal/               # narrow public interfaces
assets/                            # owned source sprites, packed immutable content
data/                              # creatures, maps, quests, UI contracts
tests/                             # unit, replay, map reachability, save corruption
tools/                             # asset packing, validators, benchmark harness
android/                           # thin Gradle/SDL activity wrapper, later
.github/workflows/                 # CI: win-x64 and android-arm64 in cloud
docs/                              # all design and technical decisions
```

**Rule:** game-content assets and test fixtures use stable `WS-###`, `Z##`, `Q###` IDs; no hardcoded per-creature branching in renderer. Owners' base sprites are preserved. Do not add 100 sprite assets at runtime until first art extraction/atlas passes.

## 3. Fixed simulation, frame rendering, pause

- Proposed simulation tick **30 Hz** (33.333ms dt logically) for world, independent from visual presentation rate. Turn-based battle resolves in discrete authoritative steps. Tick choice may change after smooth movement tests; don't introduce nondeterministic real elapsed time into battle math.
- Input pipeline: platform events → normalized actions (MOVE, LOOK, INTERACT, COMPASS, etc.) → world sim → immutable `Snapshot` → presentation. Render interpolated camera/creature visuals when target device can sustain; preserve consistent collision simulation.
- Battle uses proposed integer formulas/data and events from `BATTLE_BALANCE_BIBLE_v0.1.md` + verified test vectors; no floating point damage or status order dependence. Combat math can run without UI or renderer.
- Offline pause freezes local simulation and battle. Online overlay pauses local input only, not server state; product must define turn deadlines/disconnect policy later.
- Ignore expensive physics engine: use authored grid/sector walk collisions, circle/capsule proxy, basic navigation graph/BFS or A* follower, collision recovery. World-height/vertical walk capability gated by renderer experiment.

## 4. C++ memory and safety approach

- RAII for window, surface, renderer, texture buffers, files and audio resources. Resource handles with generation/version to avoid stale references; `unique_ptr` for exclusive ownership, references/IDs for temporary access. Fixed ownership model in headers.
- `std::vector`/dense arrays for NPC and creature state, data-oriented separation of cold content/active entity state; prefer small struct-of-arrays hot data only when profiling demonstrates advantage.
- Pre-allocate renderer color buffer(s), 1 depth column array, sprite sort buffers and common visible-NPC arrays at zone entry; avoid new/delete inside per-pixel hot loops.
- Resource cache is bounded and LRU/zone-scoped, with explicit decoded sprite count and byte accounting. New zone unloads old zone textures after transition; avoid bulk loading the 100-creature catalog.
- Async asset decode/IO optional after main-thread baseline proven; SDL window calls remain on SDL-required thread. No thread races in battle, quest or save.
- Sanitize authored map/texture dimensions, integer overflow in pixel stride, malformed asset indices and file corruption. Internal CI uses ASan/UBSan on supported platforms and integration tests for lifecycle.
- Distinguish allocated bytes, resident set size, GPU/driver resources and Android process/PSS. Report all consistently with method in benchmarks.
- If target Android low-memory notification occurs, discard caches/nonessential assets and allow safe pause/return to menu; no silent loss of progress.

### Raw framebuffer math (reference only)
- 320×180 RGBA8888 = 230,400 bytes (~225 KiB) per world buffer.
- 400×225 RGBA8888 = 360,000 bytes (~352 KiB).
- 480×270 RGBA8888 = 518,400 bytes (~506 KiB).
- Double buffering 480×270 is ~0.99 MiB raw pixels; atlas caches, audio, UI and OS overhead dominate more than this framebuffer. These calculations do **not** represent full game RAM usage.

## 5. Android and Windows specifics

### Windows
- Default target x86_64 executable running on integrated-graphics systems; installable with Steam packaging later. SDL window, mouse capture, keyboard mappings, optional controllers. Initial local development via small CMake/Ninja toolchain if user has no heavy editor.
- Test windowed/fullscreen behavior, alt-tab, native DPI, no GPU-driver-specific assumptions, offline cold launch.

### Android
- Primary target arm64-v8a and landscape fullscreen with display cutouts. Minimum Android OS/API and ABI support are **not yet locked**; derive from test phones and store policy.
- NDK C++ library and SDL3 Java Activity + JNI adapter built using cloud Android SDK/NDK/Gradle/CMake; debug APK first, signed AAB only when store ready. SDL3 Android README lists SDK>=35, NDK>=r28c and supported minimum API 21 at the time consulted, but these are library-specific requirements, **not automatically WILDSIGNAL's launch min API/target API**. Re-check pins during CI setup.
- OS pause/resume, audio focus, rotation lock, notifications/cutout safe area, lifecycle saving, keyboard IME, Android back and low-memory events must be tested.
- Actual rendering often uses mobile GPU/display composition even if 3D world is raycast on CPU; target is no dedicated PC GPU and modest phone demands, not total absence of GPU.

### Continuous integration / code quality
- Pin compiler/SDL3/NDK/Gradle versions and hashes. Use reproducible dependency lock, license ledger and artifact checksums.
- CI jobs: Linux unit tests + sanitizers, Windows x64 release build + headless tests, Android arm64 debug native build + package smoke check. CI emulators optional; **manual real-device Android test is still required** and cannot be replaced by a successful cloud build.
- Build secret/cert signing should never be in public Git or Codex prompt; GitHub Actions encrypted secrets/environment protections if installed and configured.
- Prefer compiler warnings-as-errors within project, clang-format, static analyzer as budget permits; build debug/profiling variant with optional instrumentation disabled in release.

## 6. Risks that can overturn architecture

1. User's reference image includes tall textured arches, high ceiling, movable camera pitch, irregular vertical architecture and large room views. Grid raycaster might be unacceptably restrictive; if sector/portal test cannot reach desired look at low-end frame budget, revise map layout/vertical ambitions, not quietly switch to high-poly GPU 3D.
2. CPU floor/ceiling casting, transparent foliage and multiple billboard sprites may dominate mobile power. Profile sustained thermal/battery effects.
3. SDL3 native software renderer and framebuffer upload paths differ in CPU/GPU overhead across devices. Separate **world rendering time** from **present/UI**.
4. Complex animation of 100 source sprites, 4-way direction sets and player cosmetics can exceed CPU/art budgets before raw framebuffer memory does.
5. Separate offline/online profiles impose UX and companion-emotion cost; test player satisfaction and cheating/trust before deciding shared features.
6. Cloud CI lowers local PC requirements, but signed Android/Steam publishing, SDK/NDK packages and app store accounts still require time and setup.
7. C++ cross-platform determinism needs documented integer overflow, order of operations, endian/persistence migration and replay fixtures; no compile-time assumption of identical floating point behavior.

## 7. Architecture ADR decision record

| ADR ID | Decision | Status | Validation needed |
|---|---|---|---|
| ADR-001 | C++20 + SDL3 + CMake, platform-neutral game core | Recommended | cloud builds on Windows/Android |
| ADR-002 | CPU raycaster baseline renderer | Candidate, not locked | side-by-side same-scene benchmarks |
| ADR-003 | Sector/portal software renderer | Candidate | visual fidelity and runtime complexity |
| ADR-004 | Custom CPU rasterizer | High-risk candidate | only if cheaper paths inadequate |
| ADR-005 | Stable versioned JSON content + local save backups | Candidate | schema/migration and corruption tests |
| ADR-006 | 30 Hz fixed sim / separate render | Candidate | input feel/reconciliation tests |
| ADR-007 | Online authoritative server with isolated local saves | Security principle; bridge undecided | 2-client proof + product signoff |
| ADR-008 | Cosmestic-only cross-store purchase adapter | Monetization category proposed | current policies, backend receipts, refund/restore |
| ADR-009 | Minimal local build + cloud CI Android | Recommended | reproducible binaries, deployment protection |

## 8. What Codex may build only after an approved task packet

**ENGINE-001**: standalone native startup, SDL3 window, input and test framebuffer, device metrics.  
**RENDER-001**: one 2D map, textured walls + billboard depth, collision, floor/ceiling demo.  
**RENDER-002**: same reference geometry with alternative sector/portal implementation, data and screenshots.  
**CI-001**: Windows + Android build proof, pinned toolchain, license audit and artifact retention.  
**BENCH-001**: standardized FPS/frame-time, peak RSS/PSS, battery/thermals and visual-feasibility comparison using same scene content.

None of these authorizes quest production, shop/IAP, social network service or shipping all 100 species.

## 9. Official references (consulted 2026-10-20; verify versions as needed)
- SDL3 Android native: https://wiki.libsdl.org/SDL3/README-android
- SDL3 CMake: https://wiki.libsdl.org/SDL3/README-cmake
- SDL3 software renderer: https://wiki.libsdl.org/SDL3/SDL_CreateSoftwareRenderer
- SDL3 repo & license: https://github.com/libsdl-org/SDL
- Android NDK/CMake: https://developer.android.com/games/develop/develop-as
- Steam microtransactions: https://partner.steamgames.com/doc/features/microtransactions
- Google Play Billing backend: https://developer.android.com/google/play/billing/backend

**Review gate:** Do not promote ADR-002/003/004 to "approved" or finalize complex map geometry until measured renderer tests on real target hardware and gameplay camera art approval.


## 10. Owner-confirmed Windows 7–11 portability requirement (2026-10-20)

**Host:** owner has **Windows 11**, Intel i3 10th-gen/4GB RAM. **Game target:** Windows 7, 8, 8.1, 10, 11 (per owner, D-012), with **Windows 7 SP1 x64 initially proposed as compatibility floor** pending actual tests. **Do not equate Windows 11 build success with legacy support.** See `WINDOWS_7_11_COMPATIBILITY_v0.1.md` for the platform/distribution test matrix, native 32-bit scope question, legacy CRT/Windows API imports and online security concerns.

**Architectural consequence:** game's platform-neutral core and core offline save must never depend on Steamworks/modern Chromium/Win10-only APIs. Test SDL3 and pinned C++20 runtime loading on Win7 SP1 x64; keep Win7-compatible build flags, native filesystem/audio/window APIs and alternate SDL presentation path. Steam client is not supported on Win7/8/8.1 since Jan 2024, so future standalone non-Steam distribution is needed if owner wants gamers on those systems. Steam on modern Windows, Android Google Play and the approved Android↔Windows co-op launch plan remain distinct from this legacy delivery route. No choice of renderer, CRT, toolchain or x86 target is locked until tested. No implementation started.
