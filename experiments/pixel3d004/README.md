# PIXEL3D-004 — pixel material and rendering-style gate

Isolated C++20 / SDL3 / OpenGL3.3 study, based exactly on PR25 commit
2068a27cc07ead26300be473855fd6e15d14f10f. Owner review6398862476 was read before
coding:003 was technically working but visually rejected as low-resolution3D.
The owner's inline AI cottage is an artistic reference, not texture data or an
engine frame. This experiment does not imply visual approval, a renderer rewrite,
a new game direction or a merge. Earlier versions remain unchanged.

## Build and launch Windows x64

Git, CMake3.24+, Visual Studio2022 Desktop C++ tools / Windows SDK:

```powershell
cmake -S experiments/pixel3d004 -B C:/private/p4-build -G "Visual Studio 17 2022" -A x64 -T v143
cmake --build C:/private/p4-build --config Release --parallel 2
ctest --test-dir C:/private/p4-build -C Release --output-on-failure
cmake --install C:/private/p4-build --config Release --prefix C:/private/pixel3d004
cd C:/private/pixel3d004
./wildsignal_pixel3d004.exe --pack assets/pack.json --scene data/sunlit_cottage.json --look data/sunlight_look.json --style C --shadow cached --shafts volume --resolution 640 --atmosphere-width 320 --pose 5 --output runs/new-C
```

The private review package supplies assets/pack.json. No Python needed to run.
Use a NEW output path each time. OpenGL3.3 driver required. Native SDL3/cgltf/stb
pins, static SDL/static MSVC CRT, license notices and x64 restriction are retained.
No new game runtime library. Windows graphics/Intel UHD performance require
actual hardware tests; CI compile is not graphics validation. Legacy Windows and
Android remain unverified. No private art or captures belong in the public repo.

## Three controlled styles

All use identical source geometry, UVs, cameras, sun/profile and scene composition.
No new decoration is added. sunlit_cottage.json is byte-identical to003's180-object
fixture; canopy.json is the same separate29-object/10-tree fixture.

- A:003 material sampling, light bands, shadow comparison, outlines and noisy
  volume reconstruction. The new harness can hold extra private textures for
  instant switching; it is not a claim of identical003 CPU/memory overhead.
  Actual003 executable framebuffer equality is separately verified at shared
  resolutions/poses. A960 is the same algorithm at a new diagnostic resolution,
  not a claim that the old003 CLI supported960.
- B:source-guided private material derivatives and refined architectural ink.
  Retains003 surface lighting/shadow method and ray integration for comparison.
- C:B plus continuous authored-normal lighting, receiver-plane-corrected shadow
  filtering, separately resolved/filtered atmospheric light. Source geometry and
  normals are not globally smoothed. Materials retain nearest sampling.

StylesB/C fail with STYLE_ASSETS_MISSING if required private derivatives are not
provided; they never silently substitute unchanged textures and claim treatment.
Use --no-art-materials only for explicitly labelled renderer diagnostics.

## Private material system

```powershell
python -m pip install Pillow numpy
python experiments/pixel3d004/tools/materials.py C:/private/native-source C:/private/002-balanced C:/private/004-assets
python experiments/pixel3d004/tools/materials.py C:/private/native-source C:/private/002-balanced C:/private/004-resample-control --treatment resample
```

native-source is the unchanged83-file pack from tools/prepare.py;002-balanced is
the unchanged optimized pack from tools/optimize.py. Original Standard ZIP/model/
PNG/UV/license bytes are preserved. Both source inventory and balanced texture
hashes are verified. Output must be a new private directory outside the repo.

Seven opaque atlases receive256² derivatives: timber, plaster, roof, rock trim,
brick, uneven stone and rocks. All masked foliage/flowers/grass retain original
bytes/resolution. Texture-space treatment uses linear-light reduction, an atlas-
specific source-learned base palette, local edge-aware medoids, then short strokes
traced along existing source-color ridges. Timber follows horizontal atlas grain;
roof wear follows vertical tile direction; plaster uses sparse restrained diagonal
marks; stone traces local boundary tangents. Source UV placement carries these
orientations onto real beams/tiles. Endgrain/metal atlas zones are protected from
long-grain tracing. No random seed, screen noise, regular big-block grid, UV warp
or full-frame posterization. These are automatic experimental derivatives, not
hand-authored final pixel art. Palette budgets describe base palette learning;
stroke highlights/shadows extend those local ramps. Output color counts recorded.

The resize-only control has identical256² dimensions without palette/cluster/
stroke treatment. Its actual-engine unlit comparison separates artistic processing
from simply shrinking textures. --treatment cluster isolates medoid processing.
MATERIAL_PROVENANCE.json records every source/derivative SHA, size, rule and stroke
count. A/B/C use the same geometry and all original optimized textures are retained.

Area/UV audit: `pixel3d_audit --density pack.json scene.json private/density.json`.
Derived building density is approximately138 timber,115 plaster,59 roof and
90–100 stone texels/world-unit RMS. This is a provisional material calibration,
not a final game-world scale contract or a guarantee of isotropy for every trim UV.
No UVs are rescaled. Different source trim regions retain some density variation.

Original14 maps:46.77MiB base. Seven derived maps add1.75MiB base to the review
cache:48.52MiB total base /about64.69MiB full mip levels. Both source/styled sets
stay resident for instant A/B/C switching; report this overhead, not a fictitious
per-mode texture saving. Scene-only model loading/material pooling/CPU mesh release
are retained. No per-frame image generation/upload. Logical texels are not VRAM.

## Architectural outlines

Reuse the existing normal attachment's alpha for semantic classes. B/C distinguish
timber/plaster/roof/stone architectural boundaries, major normal creases and depth
silhouettes. Small normal differences do not ink every triangle. Left/down tests
keep shared creases on one side; silhouette footprint remains one internal pixel.
Convex edge highlights use a signed reconstructed-neighbour plane test, limited
to meaningful sun-facing creases; --no-edge-highlights disables them. Foliage
uses gentle depth silhouettes with no architectural material/crease ink. Ground
is excluded. Lines are dark warm accents rather than thick black dilation.
No mesh edge geometry, geometry simplification or global normal averaging.

## Smooth surface light and atmosphere

C removes discrete toon thresholds and retains authored normals/real shape.
Limited leaf wrap softens foliage shading; source vertex color/AO remains.
Shadow depth map/casters are unchanged: surface comparison predicts receiver depth
at each of four neighbour texels using the local depth-plane gradient, then
bilinearly combines comparisons. Lower residual bias avoids relying on a large
constant bias to hide self-shadow striping. Hard mode uses one corrected texel.
This removes false comparisons on planar receivers, not true geometric silhouettes.
Tile edges, eaves and actual triangular occluder forms remain in raw depth maps.

Atmosphere width is independent of material/geometry resolution:
--atmosphere-width240|320|480|640. Default320 forC;480 geometry fallback should use240.
A/B retain003 half/quarter sizing. Same bounded12 steps by default; C uses fixed
midpoints, bilinear hardware shadow comparisons and one small3×3 depth-weighted
filter on the LOW-RES radiance buffer. No temporal random offsets or heavy full-
resolution blur. The existing depth-aware reconstruction composites radiance;
architectural ink stays crisp. Material albedo/color buffers are never blurred.
No bloom, general fog simulation, new shadow maps or unbounded ray marching.

Finite samples, thin-depth edges and perspective textures can still shimmer or
band. This is stable spatial filtering, not temporal AA or a flicker-free claim.
C foreground occlusion and zero-radiance material-invariance are tested. Raw
shadow bounds/static-cache limits from003 remain. Automatic quality can lowerC
air to240/8 steps, then shaftsOFF; it remains a timing heuristic, not proven30FPS.

## Forensics and controls

- --diagnostic lit|albedo|normal|vertex|shadow|semantic|wire
- --mode BASE --diagnostic normal: geometric face-normal view; PIXEL3D uses
  authored normal view. --no-bands isolates light quantization.
- --shadow off|live|cached|ground; --shadow-size512|1024; --hard-shadow
- --casters all|building|vegetation|roof; --bias-scale0..4
- --sampling mip|base|linear; --no-vertex-tint; --debug-effects
- --no-art-materials; --no-refined-outlines; --no-edge-highlights;
  --no-smooth-lighting; --no-receiver-correction; --no-smooth-air

--debug-effects exports the actual float raw_shadow_depth.pfm and projection.
PNG depth displays use a labelled fixed range, not fabricated scene art. The
proof includes roof-only512/1024 shadows so geometric silhouettes are not
misattributed to map aliasing. Debug colors are labelled diagnostic outputs.

WASD/click+mouse, Esc; F11 cyclesA/B/C; F1 cycles480/640/960; F3 camera poses0–11;
F12 capture; F2 orbit; C culling; O outlines; B bands; F4 legacy/cozy profile;
F5 contacts; F6 ordinary haze; F7 shadow; F8 shafts; F9 hard/soft. F10 changesC
atmosphere240/320/480 independently; A/B retain half/quarter control. New poses:
9 wood/plaster/window,10 roof,11 stone/base; poses0–8 and routes are unchanged.
The full sunlight profile is required for the new surface-light comparisons.
960 is an optional quality/cost experiment; never assume Intel UHD affordability.

## Real proof and performance

```powershell
python tools/proof.py ./wildsignal_pixel3d004.exe assets/pack.json C:/private/proof004 --stills --diagnostics --benchmark --motion --benchmark-frames 600 --resample-pack assets/pack-resample.json
```

Optional --baseline003 executable verifies actual003/A byte equality. ffmpeg
creates native A|B|C motion triptychs:450 rendered frames/capture every3 produces
150 images replayed at10fps =15seconds. This is replay speed, not displayed FPS.
Original native screenshots/frame samples are provided; H264 can add artifacts.
Captures include medium/close-up, timber/plaster, roof, stone, trees and motion,
640mandatory,480fallback,960optional. Diagnostic resize-only/stroke controls share
texture dimensions, camera and pipeline. All evidence comes from the executable.

Non-capture benchmarks runABCCBA at each selected scene/resolution with identical
routes/frame counts. Default cloud150frames/30warmup; use600/longer on ownerPC.
CSV/JSON record each mode, actual atmosphere grid, CPU submission/update/culling/
swap/loop, complete swap intervals/P95, associated API elapsed samples/missing
counts, draw calls, triangles, logical textures/effects and OS peak working set
(Windows)/RSS(Linux). Static map/filter startup is in raw frames; steady tables
exclude warmup. No per-frame glFinish. API times from Mesa are software times,
not physical GPU results; queued throughput is not proven displayed FPS. Actual
VRAM, scanout, OS cold-cache state and isolated pass-GPU timings are unmeasured.
Public CI builds code only; no model/image bytes in GitHub.

CTest covers inherited importer/culling/GL correctness plus style bounds, genuine
sampler switching/missing-art rejection, corrected planar shadow receivers,
filtered-air near-solid occlusion, culling equality, exact nearest upscale and
zero-radiance material invariance. Texture tests cover source-guided deterministic
strokes/alpha/dimensions. Actual-model equality and original source hashes add
private checks. Test passes do not establish artistic success.

Remaining art gaps: automated strokes lack a pixel artist's selective placement;
roof/plaster/foliage still retain Quaternius identity and UV repetition; true
low-poly forms remain; flat terrain and composition are unchanged. The target is
not claimed achieved by improving shadows or reducing texture size. Owner must
judge the actual material controls and moving A/B/C proof before another step.
No village, gameplay, online systems or merge. Stop at
**PIXEL3D_004_OWNER_REVIEW_PENDING**.
