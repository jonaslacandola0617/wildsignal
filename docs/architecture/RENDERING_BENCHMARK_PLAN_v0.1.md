# WILDSIGNAL — Software 2.5D Renderer Benchmark Plan v0.1

**Status:** TEST PLAN ONLY, no benchmarks executed. **Date:** 2026-10-20.
**Depends on:** ENGINE_ARCHITECTURE_ADR_v0.1.md and first-four-zone world blueprint/Visual Bible. **Codex scope** after owner allows experiment: small testbed, not polished game implementation.
**Hypothesis:** lightweight CPU-side 2.5D raycasting, column/sector or tiny rasterization can create visually compelling first-person depth on low-end Windows and Android. Hypothesis remains unproven until device tests.

## 1. Tested alternatives and common fixture

| Test ID | Rendering strategy | Intent |
|---|---|---|
| R1 | grid raycast + floor/ceiling + billboards | fastest baseline |
| R2 | sector/portal column renderer | supports more interesting spatial composition |
| R3 | tiny triangle/scanline software raster or hybrid | flexible but complex fallback |
| P0 | direct CPU window-surface presentation | strict software pipeline control |
| P1 | CPU world buffer uploaded to streaming SDL texture | optional integrated/mobile GPU display path, no discrete GPU requirement |

Measure R1, R2, R3 with same RGB(A) source art, camera path, prop count, shading modes and output resolution; then P0 vs P1 separately to avoid attributing presentation costs to renderer. **No GPU-powered world drawing** in baseline; rasterized 2.5D world remains C++ CPU-side.

## 2. Representative benchmark scenes

- **TOWN-01 (Lantern Quay):** outdoor plaza with 2–3 large building faces, beacon landmark, 8–16 billboard props/characters, lantern glow, static sky horizon. Camera move from dock toward workshop via cobbled square, including one 90° yaw and one pause to inspect avatar.
- **FOREST-01 (Liltwood):** winding 2D trail, density-graded foliage 16/32/64 visible billboards, partial transparency, fog fade, 1 animated owned creature with correct occlusion, Singing Root clue. Camera path loops around obstacle/forest junction. Worst-case alpha overdraw tested at 64 prop setting.
- **RUINS-01 (Echo Hollow):** detailed block brick walls, floor+ceiling, 2 lanterns, 1 segmented archway/upper lintel or visible fallback if unsupported, mint sigils, 1 animated creature. Camera enters hall, looks at/through arch, turns toward sigil room. Test honest camera pitch if claimed. Add collision and entity ordering in scene.
- **UI-01:** fullscreen battle overlay with 4 move cards and high-resolution text over ruin background; test crisp taps/overlays in 16:9 and wide mobile.
- **TRANSITION-01:** switching forest→ruins and back, verifying freeing old zone atlas, audio and props, tracking fragmentation.
- **THERMAL-01:** forest camera path 20 minutes on battery-powered Android device. Record CPU/fps trend and OS thermal state as available.
- **NETWORK-EXCLUDED:** network not used in these tests, isolate renderer memory and CPU.

### Scene/source rules
- Use approved owner-art creatures if available; otherwise explicitly mark a temporary test silhouette, no proprietary reference screenshot texture sampling.
- Same material counts/resolution and objective camera spline. A renderer that omits ceiling, sky, translucency or arches must report it as missing visual capability, not inflate score for being fast.
- Scene setup automated from versioned fixture and test seed, no manual camera variance. State game version, SDL3 revision, compiler/build flags, renderer ID and platform.

## 3. Test devices and profiles (minimum)

| Device profile | OS/hardware | Requirement |
|---|---|---|
| WIN-LOW | Windows system with ~2GB total RAM, integrated graphics/no discrete GPU | Physical test strongly preferred; exact CPU, RAM and display resolution recorded |
| WIN-MID | 4–8GB Windows laptop/PC with integrated GPU | Establish scaling headroom |
| AND-LOW | 2–3GB Android phone/tablet, modest CPU/GPU, thermal constraints | Physical device test essential, Android version/SoC/RAM recorded |
| AND-MID | 4–6GB Android, midrange chipset | Higher-preset reference |
| DEV-EMULATED | emulator / VM or memory pressure tool | Regression only; **not valid proof** of 2GB performance |

The user has not provided specific test machine specs. This is a **test-matrix proposal**, not evidence the game will run on these devices. If a 2GB Windows machine is unavailable, do not claim the goal met by testing only on 4GB machine.

**Android build policy note (2026-10):** Google Play new phone app submissions require target API 36+, but minSdk remains a distinct compatibility decision; set it after SDL3/NDK test. Pin SDK/NDK/Gradle versions in CI and document supported ABI.

## 4. Per-run fixed parameters

- Build modes: `Release` with symbols for profiling and `Debug` for correctness/ASan where supported; don't compare Debug performance with Release.
- Common output: 1280×720 or device-native reference, and internal world buffers at `320×180`, `400×225`, `480×270`. UI renders at screen-safe output resolution.
- Frame budget: experimental 30fps low preset and 60fps performance mode; vsync/frame pacing settings must be logged.
- Repeat each scene for ≥60s after warm-up, three runs, with five-minute mixed navigation session and 20-minute Android thermal session.
- Include cold start + zone transition + reload to catch transient allocations and caches.
- Expose number of visible billboards, draw columns, pixels filled, alpha overdraw proxies and atlas size; reproducibly vary foliage [16,32,64].
- Do not compare one alternative with distance fog clipping all far geometry against another showing everything; record exact near/far clipping distances.
- Run integrated gfx Windows without requiring a dedicated GPU; on Android do not assume a literal CPU-only compositor.

## 5. Metrics and definitions

For each sample:
- `frame_total_ms`: input processing to present (include upload/composition if CPU measured); report median/p95/p99 and worst case.
- `world_render_ms`: CPU elapsed world rasterization, excluding device presentation.
- `present_ms`: CPU time staging/upscaling/output; may underreport GPU-side work; measure actual presented frame pacing separately.
- `ui_ms`, `sim_ms`, `asset_upload_ms`, `audio_ms`.
- `fps_effective`: actual rendered/presented frames over wall-clock period, not ideal timer FPS.
- `working_set_mib`: resident/process memory measured appropriately per platform; `peak_rss_mib`, `alloc_count_per_frame`, `zone_atlas_mib`, `audio_mib`, `framebuffers_mib`.
- `load_cold_ms`, `load_zone_ms`, `save_load_ms` after save implementation.
- `battery_percent_delta` and `thermal_status` on battery test, with brightness and external conditions noted. Battery % coarse; no false precision.
- `missed_touch_interacts` and `frame_input_delay_ms` for mobile stress runs.
- `feature_support`: floor textured, ceiling textured, sky, pitch up/down, varying height, arches/openings, translucency/foliage, layered depth, billboard occlusion, crisp UI, dynamic light approximation.

### Suggested early pass gates (provisional, must be ratified)
- **Low mode goal:** ≥30 delivered FPS with p95 total frame interval ≤33.3ms on target hardware; no sustained thermally driven collapse after 20min.
- **Response:** no collision tunneling or severe input latency under designed scene load. Time-to-interaction target ≤100ms subject to device sampling precision.
- **Memory:** ≤250 MiB typical steady state and ≤350 MiB peak process RSS on 2GB profiles as working hypotheses. Measure real totals; if exceeded, review atlas/decode/UI before declaring failure.
- **Load:** prototype cold start ≤8s, zone transition ≤3s on low device as *targets*, not shipped guarantees.
- **Visual:** signed-off hero shots from same camera coordinates resemble target aesthetic for chunky brick, warm light, arches and lively sprite creatures. If R1 cannot create key visual, choose an R2 option or revise world geometry transparently.
- **Reliability:** no crash, out-of-bounds access, visible missing textures or corrupted sprites in repeated runs; Android resume from background works.

Do not claim pass until run results and logs are attached for *both platforms*. A faster benchmark with broken visuals or missing world feature is NOT equivalent.

## 6. Selection scoring (proposal)
Allocate 100 weighted points; this is a decision aid, not proof:
- **Visual fidelity (30):** reference-room visual features, object scale/occlusion, legibility.
- **Low-end performance (30):** frame time, memory, thermals and battery.
- **Engineering risk/maintainability (20):** bug risk, clipping edge cases, platform portability, developer effort.
- **Map authoring flexibility (10):** town/forest/ruin reusable kits, portals and corridor variability.
- **UI/integration (10):** crisp overlay, Android touch, Windows present/resize.

Reject options before scoring if critical feature unworkable, real test impossible, or platform crashes repeatedly. Visual feature omissions are explicit penalties. Scores need written evidence from same fixture.

## 7. Required raw benchmark artifact schema

```json
{
  "benchmark_run_id": "date-device-candidate-scene-resolution",
  "build_commit": "<git sha>",
  "engine_candidate": "R1",
  "presentation_candidate": "P0",
  "scene_id": "RUINS-01",
  "internal_resolution": [320, 180],
  "display_resolution": [1280, 720],
  "scene_manifest_sha256": "<hash>",
  "device": {
    "os": "Windows/Android version",
    "cpu": "<reported model>",
    "total_ram_mib": 2048,
    "gpu": "integrated/mobile/none reported",
    "android_thermal": "available or null",
    "screen_refresh_hz": 60
  },
  "compiler": "<compiler version + optimization>",
  "run_duration_s": 60,
  "samples": {
    "median_frame_ms": null,
    "p95_frame_ms": null,
    "p99_frame_ms": null,
    "fps_effective": null,
    "world_render_p95_ms": null,
    "present_p95_ms": null,
    "peak_rss_mib": null,
    "allocations_per_frame_mean": null
  },
  "feature_support": {
    "textured_floor": null,
    "textured_ceiling": null,
    "open_archway": null,
    "sprite_depth_occlusion": null,
    "vertical_camera_pitch": null
  },
  "render_screenshot_path": "<relative archived artifact path>",
  "raw_trace_csv_path": "<relative archived artifact path>",
  "result": "NOT_RUN"
}
```

The example values are template placeholders; never substitute them for real device details. A run must include a timestamp, attached screenshot or screen recording, raw frame traces, a short note on compromises, and an immutable commit/build ID.

## 8. Tests and evidence required

**RENDER-001**: R1 core first-person column renderer, floor/ceiling and single creature depth, full low-res framebuffer, correct nearest upscaling; automated predictable camera path, no netcode.

**RENDER-002**: R2 prototype based on sector/portal/height wall architecture or architectural approximation; report true feature support and bugs. If R2 impractical, document why with code complexity and performance evidence rather than skipping.

**RENDER-003**: R3 only if R1/R2 cannot meet visual requirements; run pared-down comparison or formally record deferral.

**PRESENT-001**: show same CPU world framebuffer via software surface and via streaming texture on both device types; identify display path compatible with sharp native-resolution UI and energy consumption.

**ASSET-001**: load/unload zone atlas with different creature counts; memory returned after exit without persistent leak.

**INPUT-001**: camera mouse/touch control parity, 48dp touch targets measured in actual dp, not logical mockup units; 3% gesture insets.

**QUALITY-001**: compile sanitizer builds where supported, test out-of-bounds map rays, near clipping, degenerate portals and zero-length vectors; no crash on impossible camera geometry.

**THERMAL-001**: Android 20-minute test with identical path and 50–70% brightness noted (not a strict brightness metric; record real), device thermal state, battery start/end and charge status.

**CI-001**: Windows executable and Android installable apk created reproducibly in cloud; all logs collected; no private signing keys in repo.

## 9. Renderer decision record after tests
Create `ADR-005_RENDERER_FINAL.md` with:
- candidate comparison table with test commit, device, scene, metric, screenshots
- visual deviations and art consequences (ceiling/pitch/arches/outdoor height)
- reasons selected and rejected
- max map/sprite/light scope for first game
- actual verified devices and minimum supported profile
- fallback low preset / scalability approach
- signoffs, date and rollback risks.

Only after this is signed off should level designers freeze vertical relief, bridge/arch structures, native sprite scale and environment complexity.

**Bottom line:** A plan can specify what to benchmark; it cannot guarantee a game "runs anywhere". Performance is empirical.
