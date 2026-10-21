# WILDSIGNAL — Codex Handoff: ENGINE-001 Echo Hollow Renderer Proof v1.0-brief

**DOCUMENT STATUS:** READY FOR OWNER REVIEW, **NOT AUTHORIZED TO IMPLEMENT**. This is an executable-style specification for Codex to use **only after** explicit permission to begin this narrowly scoped coding task.
**Prepared:** 2026-10-21.
**Repository:** `jonaslacandola0617/wildsignal`. **Planning source:** `docs/world-preproduction-v0.1` (draft PR #2). **Do not implement on the planning branch**; after authorization, create a dedicated feature branch from an explicitly selected baseline, reconciling PR #1/#2 as needed.
**Owner workflow:** ChatGPT documents design; Codex writes implementation only when instructed.
**Goal:** prove first-person pixelated rendering with controllable camera, textures, floor/ceiling, a billboard, UI overlay and measurable performance on owner's **Windows 11 / Intel i3 10th gen / 4 GB RAM**. Full commercial compatibility target is **64-bit Windows 7 SP1, 8, 8.1, 10, 11** plus Android, but this experiment is **Win11-first**; neither Android build nor Win7 claims are an ENGINE-001 acceptance requirement.

## 1. Exactly what Codex must produce when authorized

A small **C++20 + SDL3 + CMake** Windows x86_64 prototype named `wildsignal_engine001` containing **ONE** test scene derived from **Z04 Echo Hollow**, suitable for visual comparison with the user's reference. Must:
- start into a window without network, Steam, sign-in, installation of an engine, or game data download;
- render low-resolution first-person world buffer with distinct textured walls, floor and ceiling, and a simple arch/pillar **silhouette**; no false claim of true high-level multistory geometry;
- move/look with keyboard, collide against impassable blocks, and show one correctly depth-occluded **placeholder** creature billboard;
- render a simple signal pedestal/glyph cue and crisp high-resolution text overlay separate from world buffer;
- switch internal render resolution among **320×180**, **400×225**, **480×270** (16:9) using nearest-neighbor output; adjust viewport to letterbox non-16:9 windows;
- record test telemetry to a structured JSON report and frame sample CSV, including frame times, render and present durations, buffer size, scene ID, build identifier, FPS and available/unknown process-memory measurements;
- document supported/unsupported features, packaging/build reproducibility, profiling methods and concrete device observations.

**Definition of success:** A useful honest instrumented prototype with evidence, not a finished WILDSIGNAL game and not guaranteed 30 FPS on every low-end computer.

## 2. Do not implement (strict exclusion list)

**No** networking, accounts, friends, Steamworks, cloud saves, payment/store code, online companions, battle rules, creature ownership, full AI/pathfinding, JSON save migrations, quest dialogue, all world maps, heavy asset-packing or all 100 creatures. **Do not copy third-party game art** or publicly commit the user's original sprite sheet. Use temporary programmer art or permission-cleared placeholder textures/sprite. No external telemetry/network requests. No Android Studio requirement in this first Windows-only implementation; later Android task separately approved.

## 3. Canonical design sources and conflicts

Read **in order**, and stop to surface contradictions rather than inventing:
1. `docs/planning/DECISION_REGISTER_v0.1.md` (confirmed vs proposed; D-012 x64 Windows only).
2. `docs/architecture/ENGINE_ARCHITECTURE_v0.1.md`.
3. `docs/architecture/RENDER_BENCHMARK_PLAN_v0.1.md`.
4. `docs/design/HERO_SCENE_TARGETS_v0.1.md` — specifically HERO-Z04.
5. `docs/design/maps/FIRST_SIGNAL_BLOCKOUT_v0.2.json` — source topology/reference IDs, **not finished collision geometry**.
6. `docs/architecture/DEVICE_VALIDATION_MATRIX_v0.1.md`.
7. `docs/architecture/WINDOWS_7_11_COMPATIBILITY_v0.1.md`.
8. `docs/design/assets/PILOT_SPRITE_RECOVERY_SPEC_v0.1.md` — note actual creature PNG alpha assets are not yet prepared.

The 15×11 older map is superseded by the 48×32 map *for reference coordinates*. Use a **separate micro-scene fixture** built only for this benchmark. Avoid binding the prototype architecture to unapproved battle/quest IDs.

## 4. Implementation boundaries / proposed repository tree

This is a *requested future change list*, **NOT current files**:
```text
CMakeLists.txt
CMakePresets.json                   # windows-local-debug/release; only supported toolchains
README.engine001.md                # clean build/run/debug/known limitations
third_party/README.md              # dependency retrieval/version/license ledger, not giant vendored SDK
src/engine001/main.cpp             # composition root, graceful shutdown
src/engine001/platform_sdl.cpp     # SDL3 window, event loop, timing, presentation
src/engine001/platform_sdl.hpp
src/engine001/input.cpp            # typed movement/look/switch-resolution
src/engine001/input.hpp
src/engine001/world.cpp            # tiny authored test scene + collisions
src/engine001/world.hpp
src/engine001/camera.cpp           # movement/collision, bounded pitch
src/engine001/camera.hpp
src/engine001/render_buffer.cpp    # RGBA framebuffer ownership, resize, row stride
src/engine001/render_buffer.hpp
src/engine001/raycast.cpp          # simplest candidate renderer
src/engine001/raycast.hpp
src/engine001/billboard.cpp        # one depth-tested creature marker
src/engine001/billboard.hpp
src/engine001/ui_overlay.cpp       # simple crisp text/debug overlay; no full menu framework
src/engine001/ui_overlay.hpp
src/engine001/telemetry.cpp        # timing counters / CSV / JSON serialization
src/engine001/telemetry.hpp
data/bench/echo_hollow_micro_v0.1.json   # scene IDs, obstacles, camera, colors
assets/bench/                         # ORIGINAL temporary programmer art only
tests/engine001/                      # headless deterministic 2D map/collision/math checks
docs/bench/ENGINE001_REPORT_TEMPLATE.md
```
Combine files if simple code is clearer. The requirement is **clean modular ownership**, not artificial number of source files. A small deterministic fixture embedded in a human-readable file is preferred; a giant generalized resource manager is not.

## 5. Scene micro-fixture definition (proposed until screenshot approval)

**Fixture ID:** `BENCH_ECHO_Z04_MICRO_001`. **Source inspiration:** Echo Hollow Z04, **not** a verbatim conversion of the full 48×32 map. Use a **24×24** grid with border walls and walkable chamber. Exact 24×24 tile grid, spawn, camera, placeholder actors and documented features are supplied in `docs/architecture/ENGINE001_SCENE_FIXTURE_v0.1.json` (**present on this planning branch; test validations still NOT RUN in C++**). All camera/entity world coordinates reference this fixture, not Z04 final positions.

Aesthetic requirements:
- background/ceiling tinted deep stone/plum; warm russet brick walls/floor;
- tall pillar/arch silhouette formed from *supported raycast rectangles and/or explicitly documented decorative sprites*; **true transparent arch geometry is OPTIONAL, not to be claimed**;
- a central brighter mint signal pedestal; one placeholder creature card placed at depth and correctly occluded by pillars;
- nearest-neighbor pixel sampling; one or two patterned textures are sufficient;
- a simple collision-aware walkthrough that exposes near/far sprite scaling and wall occlusion.

Renderer assumptions for first experiment:
- **A: grid raycaster** preferred for earliest measurable sample. CPU per-column wall rays, simple floor/ceiling drawing or floor casting, a depth array and billboard sprite occlusion;
- fixed wall height, no real overlapping floors, no dynamic volumetric lighting; future sector renderer compared in separate RENDER-002 experiment if visual fidelity inadequate;
- camera FOV **75° proposal**, horizontal baseline pitch 0°. If ±10° pitch feature would require major refactor, **document as unsupported**, do not invent fake 3D camera pitch or hurt performance;
- wall-block collision circle radius **0.20 fixture tiles** initial proposed value; movement speed **2.0 tiles/s**; rotation speed **90°/s**. These are benchmark constants, not shipping movement settings.
- deterministic position integration with `dt` capped against huge hitches; collision can be swept/axis-separated 2D circle; speed independent of graphics frame rate. No full gameplay sim needed.

### Renderer pipeline (reference)
```text
SDL pump events → normalize actions → camera/collision tick
  → clear low-res world buffer
  → wall columns + floor/ceiling
  → visible/depth-tested placeholder sprite
  → upload nearest-neighbor world image
  → composite high-res debug/UI overlay
  → SDL present
  → sample + write telemetry
```
Do not make UI text blurry by drawing it into the 320×180 framebuffer. A tiny public-domain bitmap debug font or self-authored glyphs with license ledger is fine; avoid shipping fonts lacking redistribution rights.

## 6. Explicit input specification

| Input | Action | Constraints |
|---|---|---|
| W | forward | speed-independent; collision blocks walls |
| S | backward | same collision |
| A | turn left | no strafe required in ENGINE-001 |
| D | turn right | no strafe required in ENGINE-001 |
| Esc | exit/paused overlay | either exit immediately or pause with clear Esc legend; never corrupt output |
| F1 | cycle 320×180 → 400×225 → 480×270 | reallocate safely, log change |
| F2 | hide/show debug UI | world graphics unchanged |
| F3 | toggle one versus eight placeholder billboards | sprite-sort stress test, no species content |
| mouse | optional | if implemented: explicit capture/release and configurable sensitivity |
| Android touch | **NOT ENGINE-001** | next stage adapter must be possible structurally |

On focus lost, release keys and freeze movement; on resume no runaway turn. Pressing multiple movement keys should not cause diagonal speed boost (forward/back axes only).

## 7. Memory, performance & portability requirements

- Native **Windows x86_64** only. **No Win32/x86 build**. Development host Windows 11 i3 10th-gen/4GB RAM; compile dependency versions chosen with **Win7 SP1 x64 portability in mind**.
- Optimize for no dedicated graphics card but do not claim display/present uses zero GPU resources; SDL chooses supported backend. For Win7 do not use mandatory D3D12, modern Vulkan or Win10-only CRT imports.
- Buffer bytes: RGBA8888 `width × height × 4`; e.g. 320×180 = 230,400 bytes; 480×270 = 518,400 bytes. Check for overflow and bound all writes.
- Own buffers with RAII / `std::vector<uint32_t>`; reallocate only on resolution change, not per frame. Avoid leaked SDL handles, frame heap churn, huge texture atlases or allocating 100 species at boot.
- Choose a documented fixed simulation step or constrained variable dt; report exact choice and numerical stability. For this proof the camera tick is not a gameplay-canonical 30 Hz network sim.
- No network I/O and no hidden user information. Log anonymous device metadata only. Output to writable project-relative or SDL per-user data directory, not administrator-protected system location.
- No requirement to have SDL3 globally installed: document predictable dependency acquisition and static/dynamic linking with license notes. Do not bundle proprietary Windows runtimes without rights.
- On 4GB host prioritize Release build test; a Debug-only build is insufficient for performance comparison.
- Performance targets (unmeasured): sustained ~30 FPS, p95 frame under ~33.3ms, p99 under 50ms desirable, peak Windows process RSS goal <350MiB, all dependent on actual test results.

## 8. Tests and expected evidence

**Automatable unit/smoke tests:**
- `T001` 24×24 fixture dimensions, map bounds and passability valid.
- `T002` camera begins on walkable tile; collision prevents crossing full walls for cardinal and diagonal movements.
- `T003` billboard in front of wall visible; behind wall occluded; no negative/infinite projection widths.
- `T004` 320×180, 400×225 and 480×270 buffers allocated with exact byte counts and no out-of-bounds writes.
- `T005` framebuffer/window resize and F1 cycle preserves valid resources; no leaks/corrupt images.
- `T006` 0 input / lost focus yields no phantom movement, repeatable initial camera state.
- `T007` malformed fixture or missing placeholder texture produces safe error/controlled fallback, not crash.
- `T008` benchmark export JSON schema fields and CSV sample counts consistent; `NOT_MEASURED` is permitted, fabricated zero metrics are not.
- `T009` startup works with networking disabled; no login or online dependency.
- `T010` output binary and dependency import inspection for Win7 compatibility **documented**; real Win7 execution handled in separate WINLEG task, not claimed passing.

**Manual tests required before approval:**
- real owner i3/4GB Win11 machine: application starts, keyboard navigates, F1/F2/F3 work, wall/pillar occlusion plausible;
- screenshot(s) at all three resolutions, stationary/turning, companion at near/far distance, debug HUD clear;
- at least **5-minute scene walkthrough**, p50/p95/p99 frame time report and peak process memory, with build SHA, actual compiler/SDL versions and display settings;
- record crash, stutter, focus/minimize/alt-tab/resize behavior; no claim a 5-minute run proves 20-minute thermals (longer stress is BENCH-001);
- clearly score whether crude Echo arch silhouette approximates the desired atmosphere; owner approval or failed visual outcome documented.

**Evidence deliverables:**
1. Exact `git commit SHA` of code branch and pinned dependency versions;
2. README build/run/keyboard/hardware steps, no local absolute paths;
3. executable artifact or repeatable build instructions, logs and `bench/results/ENGINE001_<run>.json`;
4. two or more WILDSIGNAL *prototype* screenshots;
5. tests list and console/log result, known unsupported features;
6. one-page recommendation: proceed to forest/Quay, do sector renderer comparison, or alter scene ambitions.

## 9. Codex execution instructions WHEN user authorizes

```text
Project: WILDSIGNAL
Task: ENGINE-001 Echo Hollow Windows native renderer proof
Read: docs/architecture/CODEX_HANDOFF_ENGINE001_v1.0.md
Read its dependency references including ENGINE_ARCHITECTURE and DEVICE_VALIDATION_MATRIX.

Permissions: Windows x86_64 technical prototype ONLY. No gameplay content, no multiplayer,
no monetization, no public creature source art.

Create a NEW feature branch from a repository base explicitly confirmed by owner;
do not edit approved docs or merge to main.
Build native C++20/SDL3/CMake Echo Hollow micro-demo with small fixture,
nearest-neighbor 320×180/400×225/480×270 world framebuffer, textured
wall/floor/ceiling, depth-tested placeholder billboard, crisp overlay,
basic collision/input and JSON/CSV benchmark output.

Report exact files, build steps, automated tests, measured/unmeasured results,
unsupported architectural features and screenshot evidence.
Do not claim Android, Windows 7 runtime, or 2GB RAM compatibility
without running respective real-platform validation tests.
Ask when requirements conflict; do not invent battle/quest systems.
```

**This text is a copy-ready FUTURE prompt, not an instruction to run Codex right now.**

## 10. Handoff acceptance / approval gate

This file may be considered **specification-ready** when the owner has reviewed the narrow POC and chosen:
- first platform: Windows-only vs Windows+Android CI simultaneous (recommend Windows-only for cost);
- use placeholder art (recommended), preserving private source sprites;
- first renderer raycast baseline (recommended, compare sector later);
- exact micro-fixture data/source and Git branch baseline.

Until those are explicitly authorized: **task status PLANNED, NOT STARTED**.
