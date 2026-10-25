# PIXEL3D-003 — atmospheric sunlight proof

Isolated C++20 / SDL3 / OpenGL3.3 experiment, based exactly on PR24 commit
04956fc4cc399a6657fff7d36c6aa7c9f906b91e. PR24's two owner-review comments were
read before coding: brighter colors alone were not approved; actual cast shadows,
visible shafts, meaningful occlusion and moving-camera evidence were requested.
Previous experiments, sources and draft PRs remain unchanged and unmerged.

## Windows x64

With Git, CMake3.24+ and Visual Studio2022 Desktop C++ / Windows SDK:

```powershell
cmake -S experiments/pixel3d003 -B C:/private/p3-build -G "Visual Studio 17 2022" -A x64 -T v143
cmake --build C:/private/p3-build --config Release --parallel 2
ctest --test-dir C:/private/p3-build -C Release --output-on-failure
cmake --install C:/private/p3-build --config Release --prefix C:/private/pixel3d003
cd C:/private/pixel3d003
./wildsignal_pixel3d003.exe --pack assets/pack.json --scene data/sunlit_cottage.json --look data/sunlight_look.json --shadow cached --shafts auto --resolution 480 --pose 5 --output runs/new-review
```

The private review package includes `assets/pack.json`. Use a NEW output directory
every time. OpenGL3.3 driver required; Windows graphics compatibility/performance
is not established by a CI compile. Windows7–10 and Android remain unverified.
SDL3 3.2.10, cgltf1.15 and stb retain the prior full-commit pins, static linkage,
static MSVC runtime and bundled license notices. No new runtime dependencies.

## Original assets and composition

Use the existing private PIXEL3D002 balanced pack unchanged. Native Quaternius
Standard glTF/bin geometry, original UVs, vertex colors, alpha and licenses are
preserved. Texture overrides retain the same46.77MiB base /62.36MiB full-mip logical
RGBA8 budget for the cottage. No Quaternius PNG/ZIP is edited; no generated art.
Only scene-used models load; masked leaves retain their original texture pixels.
See the inherited `tools/prepare.py` and `tools/optimize.py` for reproducible
source validation and private derivatives. Do not push packs/captures to GitHub.

`data/cozy.json` and `cozy_look.json` preserve the002 fixture/profile exactly.
`sunlit_cottage.json` keeps every002 instance and adds54 small, genuine grass
objects in layered groups near path edges/rocks/trunks. The original cottage,
three trees and architecture are unchanged. This breaks up some path boundaries;
it is not terrain blending or an authored landscape. `canopy.json` is a separate
small10-tree occlusion study using only already-authorized tree/fern/grass models.
The ground remains a flat engineering quad sampling the genuine grass palette.
No fake roofs, new village, gameplay or creature artwork changes.

`sunlight_look.json` lowers the sun to about29 degrees elevation and reduces
ambient fill/haze to reveal light-shadow depth. Exposure/sun color remain002
values. The owner's inline AI cottage is an art-direction reference only.
Cast shadows and world-space scattering provide the substantive lighting change.

## Independent methods and limits

All new effects default OFF. Explicitly select:

- `--shadow off`: existing analytical ground contacts only (F5 disables those).
- `--shadow live`: actual geometry directional depth map, refreshed each frame.
- `--shadow cached`: same actual map rendered once for this static scene/sun.
  Invalidate on sun/size changes. Same resulting image as live, lower recurring
  draw cost. Scene objects are immutable in this proof; moving actors require
  invalidation or live mode. Not an offline lightmap or saved baked asset.
- `--shadow ground`: bake a256² R8 ground visibility atlas from the real map
  once. One atlas lookup shades only the flat ground; no eave/object self-shadow
  or stone-path receiving shadows. This deliberately limited static alternative
  retains the depth map for interactive mode changes/optional volume shafts.
- `--shadow-size 512|1024`: bounded world-anchored50×50 light-space map, depth1–150,
  centered at world(0,4,0). No camera-following cascades. Outside is unshadowed.
  512 requires1MiB logical depth allocation,1024 requires4MiB (driver cost unknown).
- Default soft shadows: four fixed PCF taps. `--hard-shadow`: single tap. No
  general blur, large kernels or shadow-map mipchain. Slope bias reduces acne;
  low-resolution aliasing/detachment remain visible at some angles. Alpha-masked
  foliage uses its real texture cutoff and vertex/material alpha. Glass does
  not cast. Light-frustum caster culling is independent of view-camera culling.
- `--shafts screen`:24-tap low-resolution radial integration of visible-sky
  occlusion toward the projected sun. Depth-aware reconstruction and zero radiance
  on solid receiver pixels prevent a painted ray overlay across solid objects.
  This is explicitly a screen-space cheat, NOT volumetric lighting. It fades
  smoothly at screen edges and vanishes off-screen/behind camera. It cannot
  scatter in front of geometry; it is weak in heavily blocked canopy views.
- `--shafts volume`: bounded shadow-aware view-ray integration.8/12/16 steps,
  maximum16 world units, half or quarter internal width/height. Each step reads
  three actual light-depth samples: center visibility plus two transverse taps
  to emphasize lit channels near occluders. This **stylized locality weighting**
  suppresses a uniform veil; it is not unbiased atmospheric physics. Density is
  restricted to the small study area/under-canopy heights, with warm scattering,
  distance attenuation and a simple forward-scattering phase approximation.
  Rays stop before the nearest opaque/masked scene-depth surface. No general fog
  volume, multiple scattering, expensive shadows, bloom or generated beam sprites.
- `--shaft-scale 2|4`, `--shaft-steps 8|12|16`, `--shaft-strength 0..3`.
  Default: half-resolution,12steps,strength1. RGBA16F holds radiance/termination
  depth. Four depth-weighted reconstruction taps join the existing pixel outline
  pass; no repeated full-resolution blur. Dither is static, not temporal noise.
  Finite samples cause grain/banding and some discontinuity at thin foliage or
  depth edges. No TAA, temporal reprojection or motion-stability guarantee.
- `--shafts auto`: start volume12/half. After warmup, two consecutive60-sample
  windows with P95>33.3ms downgrade to volume8/quarter; two further slow windows
  turn shafts OFF and convert live shadows to cached. Monotonic, no oscillation.
  Prefer valid submitted-frame GPU queries (eight-frame lag); otherwise use full
  swap-completion intervals, a queued-application heuristic, not scanout timing.
  Automatic fallback does not certify30FPS. Fixed modes never change quality.

This is a bounded feasibility test. The real volume pass produces visible shafts
in the staged canopy; the screen-space method is not equally convincing. The
open cottage view has fewer beams and retains a flat horizon. Do not describe
those limitations as solved or infer final owner approval from automated tests.

## Camera and keys

WASD/click+mouse free camera; Esc releases/exits.1/2/3 BASE/TOON/PIXEL3D;
F1 internal480×270 /640×360; C view-culling; O outlines; B bands; G raw camera snap
(defaultOFF). F2 orbit; F3 cycles poses0–8; F12 genuine engine screenshot.
Poses0–5 and original orbit are002-compatible.6 under canopy;7 cottage eave;
8 cottage sun-facing rear/corner. `--canopy-route` translates under branches;
`--orbit` retains the original cottage route. Both routes are frame-deterministic.
F4 legacy/cozy lighting; F5 contacts; F6 ordinary distance haze; F7 shadow method;
F8 shaft method (turns autoOFF); F9 hard/soft; F10 half/quarter shaft resolution.
Lighting remains editable without source-model changes. Nearest final scaling,
source nearest mip filtering, crisp one-pixel depth/normal outlines remain.

## Actual proof and measurements

```powershell
python -m pip install Pillow numpy
python experiments/pixel3d003/tools/proof.py C:/private/pixel3d003/wildsignal_pixel3d003.exe C:/private/pixel3d003/assets/pack.json C:/private/proof003 --stills --motion --benchmark --benchmark-frames 600 --data experiments/pixel3d003/data
```

ffmpeg on PATH creates side-by-side motion MP4s. Each side is the actual engine
at identical camera frames: LEFT shadows/no shafts, RIGHT shadows/volume shafts.
Default600frames/capture-every2 gives300 images replayed at15fps:20seconds. This is
recording playback speed, not verified real-time FPS. Original PPM/PNG evidence is
retained; H264 may add compression artifacts. Matched OFF/shadow-only/screen/
volume sheets include both resolutions, the canopy and front/corner/sun-facing
cottage views. Live/cache and actual-model culling equality are asserted.

Benchmarks use separate non-capture runs. All cases share geometry, sun, route,
resolution, total frames and cap0. RepeatedOFF/cached controls expose variation;
compare live/cached/ground/hard/512/1024/screen/volume12-half/volume8-quarter.
Default cloud120frames excludes first30; use600/longer on owner hardware. Static
map/bake startup costs occur in frame0 and remain in raw CSV; steady summaries
exclude warmup. No automatic quality changes in the fixed comparison matrix.
Test60/30 caps and `--vsync` separately for presentation pacing.

Per-frame telemetry includes total draw calls, view-visible triangles, shadow
update draws and CPU shadow/shaft submission durations, update/cull/swap/loop
and full swap-completion intervals, API query association, feature settings,
quality stage and sampled RSS. Query ring64, no per-frame glFinish. Summaries
retain missing-query counts, logical texel/effect allocations and process peak
working set on Windows /RSS on Linux. Logical allocations are not measured VRAM.
Cold-cache state, isolated per-pass GPU timings and display scanout are unmeasured.
GL timer covers all rendered passes plus presentation blit. API query results
from llvmpipe are software timings, NOT physical GPU timings or Intel UHD proof.

## Tests and review gate

`ctest` covers the inherited importer/10000 independent culling boxes/mesh release,
light projection, bounded options and deterministic auto fallback. With Pillow,
numpy, GPU/Mesa and `-DPIXEL3D_TEST_GL=ON`, actual synthetic GL tests check all
modes/resolutions, nearest scaling, query association, real cast shadows,
cache/live equality, ground bake, culling with shafts, and no atmospheric leakage
through a near solid wall. Linux cloud explicitly requires `--allow-software`;
software renderer names are rejected by default. Public CI contains no artwork.

Windows x64 build/code tests are distinct from Windows Intel UHD rendering tests.
No hardware target, final cozy look, production readiness or official direction
change is approved by this experiment. Preserve earlier branches/PRs; do not merge.
Stop state: **PIXEL3D_003_OWNER_REVIEW_PENDING**.
