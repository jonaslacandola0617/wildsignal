# WORLDLOOK-001A-R2 — two mesh clumps, small plot gate

Implements the first owner-review stage of engine issue39 and private art issue19.
Two actual closed opaque 3D grass treatments replace the rejected camera-facing
cards as the primary experiment. Original v1 files, instancing, shaders and assets
remain unchanged and available as B0. No full meadow, flowers, new gameplay,
production approval or merge. Latest owner instruction limits this gate to TWO
candidates despite art issue19's older request for three.

## Build

CMake3.24+, C++20, OpenGL3.3 and pinned SDL3/cgltf/stb (first configure needs network).

```sh
cmake -S experiments/worldlook001a_r2 -B build/r2 -DCMAKE_BUILD_TYPE=Release
cmake --build build/r2 --parallel 2
ctest --test-dir build/r2 --output-on-failure
```

Windows: Visual Studio2022 C++ desktop tools, use `-G "Visual Studio 17 2022" -A x64`;
append `--config Release` to build/install and `-C Release` to ctest.
Linux GL tests require Pillow/numpy and a context: `-DPIXEL3D_TEST_GL=ON`.
Headless Mesa: `-DSDL_UNIX_CONSOLE_BUILD=ON -DSDL_X11=OFF -DSDL_WAYLAND=OFF`,
`SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 ctest ...`.
Mesa software results never certify Intel UHD. Win7/Android NOT RUN; GL3.3
instancing translates conceptually to GLES3 but this executable is NOT an Android port.

## Private inputs / independent controls

Actual art lives ONLY in `wildsignal-art`, task branch
`feat/worldlook-001a-r2-3d-clumps`, directory `WORLDLOOK_001A_R2`.
Original source meshes and exports are privately authored in Blender; public CI
uses explicitly synthetic tetrahedra, never those art forms. No textures,
normal/PBR maps, alpha blending, geometry shaders or camera-facing mesh rotation.
Source/derived hashes, triangle counts, pivots and palette are in private mesh_kit.json.
Missing input fails; there is no runtime art generation or substitute.

```sh
wildsignal_worldlook001a_r2 --r2-kit PRIVATE/exports/mesh_kit.json \
 --r2-model M1 --r2-ground patches --r2-placement clustered --r2-count 24 \
 --look data/TOON_DIAGNOSTIC.json --no-art-materials --no-smooth-lighting \
 --study B1 --shadow-proof stable --shadow cached --shafts off \
 --resolution 480 --pose 0 --pacing precise --cap 30 --output PRIVATE/new-run
```

- `--r2-model off|B0|M1|M2`: geometry control; B0 additionally requires original
  `--grass-kit PRIVATE/grass_kit.json`. This is a small-plot recreation using v1
  art/renderer, NOT a relabelled old full-garden screenshot.
- `--r2-ground original|patches`: original garden **albedo** versus authored broad
  green/earth colors. Identical 8×8m mesh, normals, lighting and camera. Original
  full garden geometry is not duplicated. Rear slope rises only0.24m.
- `--r2-placement uniform|clustered`: same count, source mesh, yaw/size sequence,
  ground and camera. Exactly24 authored roots maximum, clear center path.
- `--r2-count 0..24`: use8/16/24 for this gate; no density tier approved.
- `--r2-lod on|off`: defaultOFF for shape review; optional geometric shrink crossfade
  near/far8–10m, shrink-out14–18m. No dither, alpha blend or wind. At18m distance-cull.
- `--r2-frustum on|off`: bounded plot chunk then conservative per-object bounds.
- `--r2-cast on|off`: genuine geometry shadow in inherited cached512 map, one
  extra instanced shadow draw when cache builds; no camera-dependent shadow LOD.
- `--r2-diagnostic off|albedo|normal|shadow`: mesh only, not a whole-image filter.
- `--pose 0..4`: front, side, top, eye-close, low-close. Primary evidence includes
  ordinary1.65m-eye/2–8m views, not only macro shots.
- `--r2-inspect --r2-count 1`: center a single source clump; poses0/1/2/3 provide
  front/side/top/1.65m-eye inspection. Same actual engine, NOT main gameplay view.
- `--r2-route walk|strafe|lookdown|orbit|turn`: real deterministic first-person
  cameras, with walk following the slight uphill terrain. No rotating grass to
  face camera. Interactive mouse/WASD follows1.65m terrain height within plot.

F1 resolution, F3 pose, F7 shadow, F12 real framebuffer capture, O outlines,
B bands, P legacy/precise pacing, N cap, V vsync. Shafts toggles disabled in R2.
Resolution640 secondary; nearest presentation preserved. Model/ground/layout
changes use separate launches for reproducible comparisons.

## Rendering and accounting

One material per family, actual vertex normals and COLOR_0. Fixed rooted
transforms; single-sided closed geometry. At most2 visible instanced draws per
family during optional LOD overlap; one draw normally. Existing opaque
color/depth/normal pipeline receives mesh surfaces, independent binary visibility;
blending disabled. Category .025 avoids inking/highlighting every tiny blade.
Uses existing sun/ambient/toon controls; no atmosphere added. Mesh surface lighting
uses actual normals/positions, unlike the preserved billboard root-light control.

`mesh_frames.csv` logs visible/culled/near/far/draws/triangles/cached shadow draws
and CPU mesh submission. `mesh.json` logs controls and logical GPU buffer bytes;
`mesh_roots.csv` roots; `frames.csv`, summary.json and pacing.json retain inherited
full-frame CPU/query/RSS/working-set/pacing instrumentation. Total triangle CSV
includes opaque world + grass + actual shadow submissions. GPU buffer bytes are
logical allocations, not measured VRAM. Small GL query samples can be missing.
Do not interpret cap/application/swap API timing as displayed FPS or scanout.
Capture/PNG/video costs must be excluded from benchmark runs.

## Visual limitations / review gate

M1 upright broad blades; M2 spreading leaf-rosette. Both deliberately tiny
prototypes, not final grass. Source geometric thickness/faceting and strong toon
bands remain visible. M2 may read as a leafy herb rather than ordinary grass.
Ground only has broad vertex colors, no terrain texture/large meadow/architecture.
Shadows use inherited bounded512 map and can step at tiny tips. Optional shrink
LOD changes shape and is not owner-approved. No wind yet: apparent camera-facing
rotation is fixed, but animation/movement feel needs owner assessment.
Optional YouTube reference was unavailable (throttled); no technique/parity claim.

Existing original-garden OFF regression and all inherited comparisons remain
available without R2 flags. Owner package intentionally contains only small plot.
Tests/builds are technical proof, not visual approval. Stop at
`WORLDLOOK_001A_R2_ENGINE_OWNER_VISUAL_AND_HARDWARE_REVIEW_PENDING`.
