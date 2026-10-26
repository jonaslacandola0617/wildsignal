# HOLLAND-001 — actual Wordline 3D pixel-rendering study

Implements issue27 on a separate branch from PR26 exact head
`f2d77e6edc349d0e84609e0d60f8d875a00277d8`. Earlier experiments, main runtime,
creature sources and draft PRs remain unchanged. This is an isolated source copy
and bounded extension of PIXEL3D-004, not a production renderer/game-direction
decision. See [Windows run guide](README_RUN_WINDOWS.md) for the portable package.

## Build and reproduce assets

C++20, CMake3.24+, pinned SDL3/cgltf/stb, Windows x64 Visual Studio2022/v143:

```powershell
cmake -S experiments/holland001 -B C:/private/holland-build -G "Visual Studio 17 2022" -A x64 -T v143
cmake --build C:/private/holland-build --config Release --parallel 2
ctest --test-dir C:/private/holland-build -C Release --output-on-failure
cmake --install C:/private/holland-build --config Release --prefix C:/private/holland-review
python experiments/holland001/tools/fetch_wordline.py C:/private/wordline-source
blender -b --python experiments/holland001/tools/convert_wordline.py -- C:/private/wordline-source/upstream C:/private/wordline-source/UPSTREAM_VERIFIED.json C:/private/holland-assets
python experiments/holland001/tools/validate_assets.py C:/private/holland-build/Release/pixel3d_audit.exe C:/private/holland-assets/pack.json C:/private/import-validation.json
```

Copy the asset output directory to installed `assets/` for launchers. Blender4.3.2
is offline conversion tooling only. No new game runtime dependency. Linux cloud:
same CMake configuration with `-DCMAKE_BUILD_TYPE=Release -DPIXEL3D_TEST_GL=ON
-DSDL_UNIX_CONSOLE_BUILD=ON`; use `SDL_VIDEODRIVER=offscreen` for synthetic checks
and explicit `--allow-software` for Mesa-only evidence. Pillow/NumPy/ffmpeg serve
offline tests/review layout/videos; no generated scene imagery is used as proof.

## Actual source and conversion

Pinned `jonaslacandola0617/wordline-animation` commit
`e7e803fe1faeea07b1332790cc16b8994bd3b8b7`, under `assets/claude generated assets/`:

- `WORLDLINE_ASSET_PACK_03A_80/rural_landmarks/RURAL_05.obj/.mtl` (Barn798tris).
- Same pack `plants/PLANT_01`, `PLANT_03`, `PLANT_05` (756/3472/3988tris).
- Same pack `terrain/TERRAIN_01` (SmallRock320tris).
- `WORLDLINE_LAB_ASSET_PACK_46/models/trees/TREE_01`, `TREE_06` (1322/2140tris).

Seven authored models:12796 unique triangles. Original OBJ/MTL/metadata bytes are
private, hashed and unchanged. File URLs, Git blob hashes, SHA256 and sizes are in
`data/wordline_sources.json`; no raw artwork is committed. Owner issue27 declares
these own assets and authorizes this private test; no missing file license is
silently inferred as CC0/public redistribution permission. Source README/metadata
and exact hashes accompany the private package. These are provisional test assets.

Converter parses original triangles/groups/MTL, uses headless Blender to construct
and export actual geometry, preserving group/material boundaries and source Y-up
metres/ground pivot. X+90 rotation into Blender Z-up is undone by glTF Y-up export;
no model heading or production scale is invented. Explicit instance transforms
live in garden.json; source Barn front is +Z (verified door/siding vertices), so
barn yaw0 is used. The general03A vehicle -Z convention does not rotate statics.

Blender custom-normal storage can slightly vary nominally flat loop normals.
Post-export oriented triangle matching restores exact normalized authored normals
and split corners; no triangles are added, removed, rewound, smoothed or decimated.
Original/converted group totals, bounds, dimensions, material data and hashes are
recorded. Repeating with the same Blender version is tested byte-identical for
GLBs. Cross-format byte-identity is never claimed.

MTL Kd is explicitly treated as linear RGB -> PBR baseColorFactor, d/Tr -> alpha,
Ke -> emissiveFactor. No texture atlas, normal map or painted substitute exists.
All selected sources have zero emissive factors. Actual importer checks these
factors and alpha modes; actual GL tests exercise color and nonzero emission.
GLB embedded buffers and untextured primitives without UVs are supported. Textured
models still require valid UVs; invalid buffers/URIs/NaN are rejected.

`oracle/` is a separate external-buffer glTF mirror for the original PR26 importer,
which rejects embedded GLB and insists on UVs. Constant zero UVs are added only to
this untextured compatibility mirror; geometry, authored normals and material
factors match. Actual original004/B0 framebuffer equality is checked on that
mirror versus real GLBs in the new renderer, not asserted from copied code alone.

TEST_GROUND is a clearly labeled two-triangle engineering quad, not an authored
Wordline asset or final terrain art. Full garden:19instances/27062tris including
quad; sparse fallback:10instances/14310tris (verify runtime/audit if edited).
Native model scales are recorded, but scene scaling is provisional.

## B0 / B1 / B2

Principle reference: [David Holland, 3D Pixel Art Rendering](https://www.davidhol.land/articles/3d-pixel-art-rendering/).
Cardinal depth/normal edges and snap-error compensation are independently
implemented here; no proprietary shaders, Godot patches or reference code copied.

- B0: existing PIXEL3D-004 C lighting/ink/shadows, `--no-art-materials` explicitly
  because no private Quaternius texture variants belong in this proof.
- B1: same colors/geometry/light, selective one-cardinal-pixel depth outlines,
  major building creases and optional restrained convex highlights. A semantic
  normal-alpha channel excludes vegetation normals from polygon ink. Reconstructed
  plane checks reject oblique-plane depth gradients; shallow flat facets do not
  receive blanket triangle borders. Trunks/rocks get depth silhouettes only.
- B2: same refined pass with `--pixel-camera off|raw|comp`. Raw retains the original
  reference-plane snapping method. Comp shifts the final nearest presentation by
  the measured fractional snap error; no material/scene color blur is introduced.

All controls share model geometry, scene, sun, FOV60, screenshots and deterministic
routes. Game position/yaw/pitch stay untouched; culling uses the effective render
camera. G is the live OFF/RAW/COMP toggle; F11 changes study. `--snap` retains raw
legacy compatibility; use the explicit camera flag in new comparisons.

Step=2*referenceDepth*tan(FOV/2)/internalHeight. Snap camera right/up world
coordinates; final image translation is snapped-minus-game camera displacement
divided by that step. Tests independently project a reference-plane point and
explicitly expose nonzero near/far residual. Perspective, occlusion and free-look
cannot be perfectly aligned at arbitrary depths by a single2D translation. No
temporal AA, hidden input quantization or pixel-perfect-all-depths claim.

At native1×, nearest sampling cannot show most fractional offsets; COMP/RAW share
the same native geometry pixels. Actual displayed captures at integer2×/3× show
pixel-boundary translation. Default buffer is read BEFORE swap. Border pixels
clamp; there is no costly oversized scene target. No overscan guarantee is made.

Cached512 shadows and existing effect toggles retained. Shafts default OFF;
screen/volume shaft costs are separate opt-in measurements. No SSR, reflections,
TAA, new volumetric framework, gameplay or village added.

## Evidence and tests

```powershell
python -m pip install Pillow numpy
python experiments/holland001/tools/proof.py C:/private/holland-build/Release/wildsignal_holland001.exe C:/private/holland-assets/pack.json C:/private/holland-proof --stills --motion --benchmark --baseline004 C:/private/004/wildsignal_pixel3d004.exe
```

ffmpeg required for motion assembly. 480/640 native pixels, front/corner/foliage/
medium views, detail crops, actual displayed stills, OFF/RAW/COMP strafe and orbit
clips and additional B0/refined-off motion are saved. Replay10fps/captureevery2
is not engine/display FPS. Original captures and complete commands accompany the
review. Optional `--quaternius-pack` makes a separately labeled prior004 context
comparison; different models/scene mean it is not an isolated shader comparison.

Seven local test groups: inherited math/culling/import/style GL tests, linear
texture test, new camera math, GLB errors and actual selective-outline/display
compensation/material/emission readbacks. Test passes do not prove owner art
approval. CI builds Windows x64, sanitizes core/import and checks synthetic Mesa;
no private Wordline bytes are uploaded as CI artifacts.

Benchmarks exclude warmup; default cloud150frames/30warmup, reverse mode order,
no readback during timed runs. CPU submission/update, complete swap-return timing,
asynchronous query associations, draw/triangles/culling, logical texture/effect
bytes and OS process memory are logged. Linux RSS and software OpenGL queries
are never called Windows working set or Intel UHD GPU timings. Physical VRAM and
display scanout are not measured. See Windows guide for the actual-device suite.

Art review must answer whether this actual Wordline barn/vegetation plus bounded
outline/camera treatment reads meaningfully cleaner at medium distance. Compilation
is not the answer; flat source materials do not reproduce the richer cozy reference.
Source palm/tree silhouettes, sparse engineering ground and perspective shimmer
remain visible limitations. No commercial or visual approval is implied.

Stop at **HOLLAND_001_OWNER_VISUAL_AND_HARDWARE_REVIEW_PENDING**. Do not merge.
