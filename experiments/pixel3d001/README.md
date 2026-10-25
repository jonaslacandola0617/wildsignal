# PIXEL3D-001 — isolated Quaternius OpenGL study

C++20/SDL3/OpenGL 3.3 core, in a separate executable. This is an experiment,
not a replacement for the working hybrid prototype or a new official direction.
Actual Standard glTF meshes and native PNGs load from a private external pack.
The public repository/CI contains code and synthetic fixtures only.

## Build Windows x64

Install CMake >=3.24, Git and Visual Studio 2022 Desktop C++ tools / Windows SDK.
From the repository root:

```powershell
cmake -S experiments/pixel3d001 -B C:/private/pixel3d-build -G "Visual Studio 17 2022" -A x64 -T v143
cmake --build C:/private/pixel3d-build --config Release --parallel 2
ctest --test-dir C:/private/pixel3d-build -C Release --output-on-failure
cmake --install C:/private/pixel3d-build --config Release --prefix C:/private/pixel3d-runtime
```

SDL3 3.2.10 (`877399b2`), cgltf 1.15 (`360db1a`) and stb (`003501f`)
are pinned to full commits in CMake. SDL uses static linkage and static MSVC CRT.
The cgltf and stb MIT licenses and SDL license accompany the install. No runtime
FBX importer, heavyweight engine or Python runtime dependency. First configure
requires Git network access; `FETCHCONTENT_SOURCE_DIR_*` accepts verified offline
checkouts. No dependency is shared with or changed in older prototypes.

## Prepare the authorized private assets

The complete brief is `docs/experiments/PIXEL3D001_QUATERNIUS_RENDERING_BRIEF.md`.
Read its Drive source links. Obtain all three owner-authorized Standard ZIPs;
do not use Pro/Source, screenshots, mockups or substituted assets.

```powershell
python -m pip install Pillow
python experiments/pixel3d001/tools/prepare.py C:/private/village.zip C:/private/nature.zip C:/private/props.zip C:/private/quaternius-pack
C:/private/pixel3d-runtime/wildsignal_pixel3d001.exe --pack C:/private/quaternius-pack/pack.json --scene C:/private/pixel3d-runtime/data/hero.json --output C:/private/runs/unique
```

The importer verifies the exact authorized ZIP SHA256s, packaged CC0 notices,
topology, indices, source PNG dimensions and all staged file hashes. It copies
original glTF/bin/PNG bytes unchanged; no resizing, palette changes or decimation.
Original meshes, PNGs, ZIPs, provenance inventories and captures stay private.
The runtime additionally validates indices/material references, scene hierarchy,
finite transforms/normals/UVs and bounded external URIs. It rejects skins, morphs,
compressed geometry, unsupported required extensions and non-triangle topology.
Base-color textures/factors, authored `COLOR_0` (RGB/RGBA, including normalized
integer channels) and alpha mask/blend are supported. Small <=0.001 float export
overshoot is clamped for rendering; source files remain unchanged. Secondary UV
sets used only by ignored non-base-color maps do not affect this shader. Normal/roughness/
metallic maps, source-tier shaders and PBR lighting are deliberately unused.

One 4×4-unit cottage uses authentic 2-unit wall/door/window modules, separate
gable faces, solid roof, frame/door geometry, chimney, lantern and bench/barrel.
Exactly three common trees use explicit uniform scales. A little grass/flowers,
ferns, rocks/fence and stone path surround it. A permitted basic ground quad
samples an existing grass palette texel; it is not generated replacement art.
All placement/scales are experimental, editable in `data/hero.json`.

## Controls and comparisons

- WASD moves; click captures mouse, mouse turns/looks. Esc releases then exits.
- 1 BASE (flat geometric lighting, native drawable resolution), 2 TOON, 3 PIXEL3D.
- F1 selects 480×270 / 640×360 internal resolution for TOON/PIXEL3D.
- O toggles outlines, B lighting bands, C chunk/object frustum culling.
- F2 deterministic orbit, F3 five fixed camera poses, F12 actual PPM capture.
- G toggles an **experimental raw camera snap** at a 10-unit reference plane.
  Disabled by default: perspective depths cannot all share a pixel grid. There
  is no snap-error compensation, no pixel-perfect or jitter-elimination claim.

TOON/PIXEL3D render to color/normal/depth attachments; PIXEL3D's post-pass uses
four cardinal neighbours, radius **one INTERNAL pixel**, one-sided silhouettes
and normal creases. Thresholds are `.018` relative depth / `.055` absolute and
`.48` normal dot, recorded in telemetry. No multi-pixel dilation, blur or MSAA.
Nearest mip sampling on genuine textures, nearest final scaling, integer scale
and letterboxing where the window permits. BASE has a different resolution cost
in an ordinary large window: `--hidden` creates the exact requested drawable
size for rigorously matched BASE/TOON/PIXEL3D benchmark/capture comparisons.

Opaque and masked geometry writes real depth/normals. Sorted glass writes color
only with depth testing; no glass depth write. No black leaf rectangles or fake
billboard roofs are substituted. Single sun + ambient, no cast shadows,
volumetrics/reflections/water/wind or proprietary Holland art/shader code.
These missing effects limit contact/atmosphere and motion polish.

## Reproduce private proof

Use a NEW output path for every run. `--help` lists all flags. Example:

```powershell
./wildsignal_pixel3d001.exe --pack assets/pack.json --scene data/hero.json --output runs/hero --mode PIXEL3D --resolution 480 --orbit --frames 1200 --cap 60 --capture-every 4
./wildsignal_pixel3d001.exe --pack assets/pack.json --scene data/hero.json --output runs/stress_on --resolution 640 --stress --orbit --frames 600 --cap 0
./wildsignal_pixel3d001.exe --pack assets/pack.json --scene data/hero.json --output runs/stress_off --resolution 640 --stress --orbit --frames 600 --cap 0 --no-cull
```

`tools/review.py executable pack scene NEW_PRIVATE_OUTPUT --stills --motion
--correctness --benchmark` runs actual engine captures/20-second orbit clips,
It records actual output files and culling equality.
Its matrix uses five poses × three modes × two resolutions. The separate stress
load repeats **64 copies** of the same real objects with shared GPU resources
and one extended floor; it is not a designed game region. ABBA benchmarks hold
scene, camera sequence, mode, resolution and hardware constant, and compare OFF,
ON, ON, OFF. Motion diagnostics compare raw snap ON/OFF separately. Optional
ffmpeg makes a convenience MP4; native PNG/APNG are authoritative.

Every run writes GL vendor/renderer/version/video driver, CSV camera poses,
visible/culled object/chunk counts, submitted triangles and draw calls; CPU
update/culling/submission/swap and loop times, complete successive SDL-swap
completion intervals and an application-FPS estimate (including between-frame
telemetry/pacing), asynchronous GL timer-query time,
sampled process RSS, texture/mipmap estimates and import/upload CPU times.
First 120 frames are excluded from aggregates, retained in CSV. Capture/file
encoding is outside loop time but perturbs subsequent scheduling: use separate
non-capture runs for performance. Timers include geometry/post/final blit and are
associated with the frame of submission; unavailable slots are explicitly -1.
No `glFinish` per frame/draw: only once after load and after the final capture.
Swap/loop timings measure the application/present call, not proven screen scanout.
OS cold-cache state, total VRAM and driver allocation overhead are not measured.

## Cloud and correctness

No hardware GPU is exposed in the current cloud session. Software renderer names
are rejected by default. Explicit `--allow-software` permits genuine **same-engine
OpenGL diagnostic** rendering, always labelled SOFTWARE/GPU_NOT_TESTED. It does
not certify Intel i3/iGPU performance. `SDL_VIDEODRIVER=offscreen` uses EGL/Mesa on
Linux; no desktop/display claim. Do not install Mesa software DLLs over the owner's
graphics driver. A real Windows GL3.3 driver is needed for hardware review.

Configure with `-DPIXEL3D_TEST_GL=ON` (and Pillow/Mesa or GPU) to execute the
actual OpenGL synthetic test: both resolutions/modes, culling pixel equality,
frame query association, one-pixel silhouette width and exact3× nearest default
framebuffer readback. Cloud runs explicitly opt into software.

Code-only tests cover transforms/handedness/inverse-transpose normals, malformed
glTF/material/index/URI cases, 10,000 boxes against an independent clip oracle
and chunk membership correctness. Private same-camera ON/OFF framebuffer equality
checks supplement these. Context-owned GL resources release before the SDL
context; resize recreates targets. Actual device/context loss recovery is not
implemented; restart after a reset. No collision/gameplay integration or avatar
is added; this executable is a free-camera rendering study.

60 FPS / 30 FPS fallback, ≤250 MB process RAM and ≤64 MB textures are **goals**.
The native texture budget currently exceeds the latter: report measured costs;
do not uglify/strip the source to manufacture a pass. Owner Windows11 i3/4GB,
legacy Windows7–10 and Android remain NOT RUN until physically tested. Hardware
absence means `PIXEL3D_001_GPU_NOT_TESTED`, not rendered hardware approval.

Inspired by [Holland's general methods](https://www.davidhol.land/articles/3d-pixel-art-rendering/)
(toon bands, low-res targets, depth/normal edges). No copied proprietary demo
assets/code or claim of matching his complete lighting/foliage style.
