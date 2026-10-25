# PIXEL3D-002 — cozy sunshine and texture residency study

Isolated C++20 / SDL3 / OpenGL 3.3 executable, extended from PR #23 at
`632dc087e44876825104c977971515a3e3f072dd`. Prior experiments are unchanged.
One existing Quaternius cottage, exactly three trees and modest additional
native vegetation. This is an owner-review experiment, not production approval.
The owner's inline AI cottage image is an art-direction reference, not a texture
or an engine screenshot. No source art, geometry or license notices are edited.

## Windows x64 build and run

Install Git, CMake >=3.24 and Visual Studio 2022 Desktop C++ / Windows SDK.
From the repository root:

```powershell
cmake -S experiments/pixel3d002 -B C:/private/pixel3d002-build -G "Visual Studio 17 2022" -A x64 -T v143
cmake --build C:/private/pixel3d002-build --config Release --parallel 2
ctest --test-dir C:/private/pixel3d002-build -C Release --output-on-failure
cmake --install C:/private/pixel3d002-build --config Release --prefix C:/private/pixel3d002
cd C:/private/pixel3d002
./wildsignal_pixel3d002.exe --pack assets/balanced/pack.json --scene data/cozy.json --look data/cozy_look.json --resolution 480 --output runs/first-review
```

The private review package supplies `assets/balanced`. Do not place assets/runs
inside the public repository. OpenGL 3.3-capable Windows graphics drivers are
required. Static SDL3/CRT; no Python needed to run. Windows 7 compatibility and
Android are unverified. CMake pins SDL3 3.2.10, cgltf 1.15 and stb to full commits;
licenses accompany installation. Use `FETCHCONTENT_SOURCE_DIR_*` for verified
offline dependency checkouts. Public CI builds code only, without private art.

## Private source and derivatives

Using the same authorized Standard ZIPs from PIXEL3D-001:

```powershell
python -m pip install Pillow numpy
python experiments/pixel3d002/tools/prepare.py C:/private/village.zip C:/private/nature.zip C:/private/props.zip C:/private/native-source
python experiments/pixel3d002/tools/optimize.py C:/private/native-source experiments/pixel3d002/data/cozy.json C:/private/assets/native --profile native
python experiments/pixel3d002/tools/optimize.py C:/private/native-source experiments/pixel3d002/data/cozy.json C:/private/assets/balanced --profile balanced
```

The preparer validates exact authorized ZIP hashes, CC0 notices and file hashes.
The optimizer verifies all 83 staged originals, copies scene-used glTF/bin
unchanged, and records every source/derivative hash and dimension in private
`OPTIMIZATION.json`. Runtime texture overrides select content-addressed PNGs;
original model URIs remain unchanged. Unsupported normal/roughness maps and the
unreferenced RockPath model/map are omitted from the runtime pack, not deleted
from source. The renderer uses base color, COLOR_0, alpha and authored UVs;
PBR/normal/roughness maps remain unsupported. Only opaque constant-alpha atlases
are reduced using linear-light box averages: six minor atlases to 512px, other
opaque 2048px atlases to 1024px. All alpha-sensitive foliage retains exact source
bytes. Native comparison packs retain original texture bytes. No recoloring,
redrawing or model simplification is performed. Texture filtering/palette detail
can differ in small masonry, metal and furniture: review paired screenshots.

Residency: scene-used imports only, pixel-content texture deduplication, material
state pooling, cached consecutive material bindings, CPU mesh release after GPU
upload. Bounds/materials/index counts remain available for conservative culling.
The balanced set is 49,044,160 bytes base RGBA8 (46.77 MiB), 65,390,248 bytes with
all mip levels (62.36 MiB), versus 197,941,952 base bytes in the original pack.
These are exact logical texel budgets, not measured VRAM/driver overhead.

## Look and controls

`--look legacy` is the default and retains the baseline shader. Explicit
`--look data/cozy_look.json` enables warm directional sun, blue hemispheric fill,
soft bands, source-grass palette variation, a sky gradient, mild distance haze,
and softened foliage outlines. Edit JSON to adjust lighting without source-model
changes. `data/hero.json` retains the baseline 72 instances; `data/cozy.json`
preserves every original placement and adds 54 genuine grass/flower/bush objects.
The larger flat ground remains an engineering surface, not terrain art.

Ground-only analytic contact ellipses and a cottage footprint box select the
nearest 12 eligible objects within 40 units. They approximate contact darkening;
there is no baked AO, cast-shadow occlusion, real-time shadow map or SSAO. They
cannot cast correct angled roof/tree shadows or shade model-to-model contacts.
Haze is distance color blending; no volumetrics, bloom or ray marching.

- WASD / click-and-mouse free camera; Esc releases/exits.
- 1 BASE (native drawable size), 2 TOON, 3 PIXEL3D.
- F1: 480×270 / 640×360; C: conservative chunk/object frustum culling.
- F2: deterministic orbit; F3: six fixed poses (0 front, 1 corner, 2 side,
  3 near, 4 far, 5 tight corner); F12: genuine PPM screenshot.
- F4: legacy/cozy look; F5: contact darkening; F6: distance haze.
- O: outlines; B: bands; G: experimental raw camera snap (default OFF).

PIXEL3D outlines use one cardinal internal pixel. Cozy normal crease threshold
is .25, legacy .48; depth thresholds remain .018 relative / .055 absolute.
Foliage suppresses noisy internal normal creases and uses gentler depth edges.
Nearest mip selection and nearest final integer scaling/letterboxing remain.
Perspective motion still shimmers; no temporal anti-aliasing or snap compensation.

## Reproducible comparisons and owner hardware validation

Use a new output directory every run. `--hidden` matches drawable/internal sizes
for BASE comparisons. Keep `--scene`, resolution, mode, camera/route and cap equal.
For texture-only comparisons use `data/hero.json --look legacy` with source,
native and balanced packs. For lighting-only comparisons use the same scene/pack
and change only `--look`. Full scene comparisons separately disclose decoration
and ground changes. Poses 0–4 and orbit are unchanged from PIXEL3D-001.

```powershell
./wildsignal_pixel3d002.exe --pack assets/balanced/pack.json --scene data/cozy.json --look data/cozy_look.json --resolution 640 --pose 5 --frames 180 --warmup 60 --cap 0 --output runs/cozy640
./wildsignal_pixel3d002.exe --pack assets/balanced/pack.json --scene data/cozy.json --look data/cozy_look.json --resolution 480 --orbit --frames 1200 --warmup 120 --cap 60 --vsync --output runs/paced480
./wildsignal_pixel3d002.exe --pack assets/balanced/pack.json --scene data/hero.json --look data/cozy_look.json --resolution 480 --stress --orbit --frames 600 --warmup 120 --cap 0 --output runs/stress-on
```

Repeat stress with `--no-cull`, then ON, then OFF (ABBA), both resolutions.
Repeat native/balanced in ABBA order, matching scene/look, for memory comparisons.
Add `--capture-every 4` only for separate preview runs. `tools/review.py` supports
the inherited baseline capture matrix; explicit commands above select cozy mode.
Never mix capture runs with timing claims. Test 30/60 caps and VSync separately;
record actual accepted swap interval. Uncapped queued throughput is not displayed
FPS. SDL swap completion intervals include scheduling but do not prove scanout;
use an external Windows presentation tool for sustained displayed-FPS validation.

Per-run `frames.csv` records camera/effective camera, culling/draws/triangles,
CPU update/cull/submission/swap/loop, successive swap intervals, query times,
look/contact/haze state and sampled process RSS. `summary.json` records process
peak working set on Windows (RSS on Linux), logical texture/mip/buffer budgets,
released CPU geometry, GL identity and timer coverage. GPU queries are associated
with submitted frames using a nonblocking 64-slot ring; missing samples remain
explicit. More query slots improve coverage but do not guarantee valid timers.
No per-frame glFinish. Timings on software GL are diagnostic, not iGPU results.
Cold OS-cache state and actual VRAM are unmeasured.

## Tests and limits

```sh
cmake -S experiments/pixel3d002 -B /tmp/p3-tests -DPIXEL3D_BUILD_GL=OFF -DPIXEL3D_SANITIZE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/p3-tests --parallel 2
ctest --test-dir /tmp/p3-tests --output-on-failure
```

For actual synthetic GL tests install Pillow/numpy and configure
`-DPIXEL3D_TEST_GL=ON`; Linux cloud uses explicit software offscreen rendering.
Tests cover importer rejection/normalized colors, scene-only loading and unsafe
overrides, 10,000 independently checked culling boxes, geometry-release metadata,
lighting configuration, linear-light averaging, three GL modes/both resolutions,
culling framebuffer equality, query association and exact 3× nearest readback.
Private actual-model comparisons supplement code-only tests.

Remaining: flat terrain/horizon, cheap approximation shadows, repeated vegetation,
source-model/atlas detail and texture minification differences, alpha sorting,
no wind/animation, no GPU context-loss recovery. No new gameplay, village,
networking or alteration of the working hybrid prototype. The reference has
richer grounded shadows and painterly foliage than this implementation; this
experiment does not reproduce Holland's full system or establish final quality.

Stop state: `PIXEL3D_002_OWNER_REVIEW_PENDING`. Hardware/visual approval remains
with the owner. Do not merge the draft PR as part of this experiment.
