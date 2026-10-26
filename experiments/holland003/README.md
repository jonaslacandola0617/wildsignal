# HOLLAND-003 — Wordline edge / shadow / sunlight integration proof

Issue #31, isolated from verified PR28 head
`a764710f61c5979b4acb79b899f3bdd9509e951b`. Original experiments, seven Wordline
models, source normals, MTL albedo, creatures and gameplay remain untouched.
No newer HOLLAND-002 implementation branch existed at inspection. The owner
supplied the exact strong diagnostic JSON and launcher during implementation;
`data/TOON_DIAGNOSTIC.json` preserves those values. `--no-smooth-lighting` selects
the existing toon shader, unchanged; this does not claim Issue29's broader toon
UI/tunable-band work is completed. The deferred Issue30 round-tree experiment is
not implemented. This proof neither smooths/rebuilds foliage nor adds leaf art.

## Reproduce

```powershell
cmake -S experiments/holland003 -B C:/private/h3-build -G "Visual Studio 17 2022" -A x64 -T v143
cmake --build C:/private/h3-build --config Release --parallel 2
ctest --test-dir C:/private/h3-build -C Release --output-on-failure
cmake --install C:/private/h3-build --config Release --prefix C:/private/h3-review
```

CMake3.24+, C++20, pinned SDL3/cgltf/stb, static SDL/static MSVC CRT. No additional
runtime library. CPU import/math/lighting-config code and baseline shaders are
reused from unchanged `../holland001`; only the isolated executable is extended.
Original OBJ/MTL conversion/provenance is documented in HOLLAND001 README and its
fetch/convert/validate tools; this proof loads the exact same private GLBs, without
reconversion or edits. Copy the private converted pack to installed `assets/`.
Actual authorized model IDs: RURAL_05, TREE_01, TREE_06, PLANT_01, PLANT_03,
PLANT_05, TERRAIN_01; TEST_GROUND is a labeled engineering quad. Source commit
`wordline-animation@e7e803fe1faeea07b1332790cc16b8994bd3b8b7`; original file hashes,
conversion metadata and rights statement travel privately. No standalone public
redistribution license is inferred. No model or capture bytes enter public CI.

Linux: Release configuration with `-DPIXEL3D_TEST_GL=ON
-DSDL_UNIX_CONSOLE_BUILD=ON`; `SDL_VIDEODRIVER=offscreen` and explicit
`--allow-software` permit labeled Mesa diagnostics. `-DPIXEL3D_BUILD_GL=OFF
-DPIXEL3D_SANITIZE=ON` runs CPU/import/config tests with ASAN/UBSAN.

```powershell
python tools/proof.py ./wildsignal_holland003.exe assets/pack.json C:/private/h3-proof --stills --motion --benchmark --baseline C:/private/h1/wildsignal_holland001.exe
```

Offline Pillow/NumPy/ffmpeg provide lossless framebuffer format conversion,
contact sheets, crops and actual-camera replay clips. Never use generated images
as rendering evidence. Benchmark runs contain no per-frame capture readbacks.
Clips replay one real capture per two route steps at10fps, not measured FPS.

## Stage1 — independent convex highlights

`--edge-mode legacy|off|only|combined` retains the exact PR28 B1 post shader when
legacy is selected. OFF has no ink/highlights; ONLY adds highlights with no dark
outlines; COMBINED keeps selective B1 ink plus highlights. `--no-outline` and
`--no-edge-highlights` remain independent. Four cardinal neighbors only, no
extra full-resolution pass, normals attachment alpha supplies semantic classes.

Architecture uses signed two-plane convexity plus the angular discontinuity,
near-depth test and sun-facing threshold. Concave/coplanar pairs are rejected.
Foliage receives selected sun-facing outer mass contours, never normal-only
triangle edges. Silhouettes must point toward the projected sun; sky pixels are
never brightened. Optional map visibility suppresses highlighting inside cast
shadow. Bounded warm, color-aware addition avoids white wireframe borders.

- `--highlight-strength 0..0.6` (architecture default.30)
- `--foliage-highlight-strength 0..0.6` (default.24)
- `--highlight-threshold 0..1` (sun dot default.18)
- `--highlight-color r,g,b` (default1,.72,.32)
- `--highlight-mask all|architecture|foliage`; `--edge-mask` outputs the actual
  effect mask, red architecture/green foliage, a diagnostic rather than game art.

H toggles highlights; J cycles edge modes. Lighting band thresholds, albedo and
source normals remain unchanged. These are restrained accents, not extra leaves,
painted material detail or a complete transformation of flat Wordline surfaces.

## Stage2 — bounded shadow correctness

`--shadow-proof legacy|stable` independently compares the inherited shadow path
with the new bounded one. The old caster filters matched Quaternius names and
therefore selected no Wordline building/vegetation/roof. Stable mode recognizes
Wordline models and actual OBJ group names; roof-only selects roof groups within
the single barn GLB. Ground does not cast; blended transparency still does not
cast and alpha-masked casters retain their cutoff. Raw depth maps verify shapes.

Stable mode uses a world-fixed28×28 light-space extent rather than the inherited
50×50 extent. Both remain centered at(0,4,0), fixed under camera motion. This is
specific to the small garden, not a camera-following cascade or arbitrary-world
solution. Outside the map, surface receiving is unsupported/unshadowed. Compare
512 and1024 at the SAME extent; logical depth allocation1MiB vs4MiB. No enormous
PCF kernel: hard one comparison, soft four bilinearly weighted comparisons.

The receiver plane is derived from the geometric triangle normal transformed by
the inverse-transpose light matrix. It does not estimate a plane from neighboring
screen surfaces. Small residual depth bias is configurable through inherited
`--bias-scale0..4`; zero/high controls separate acne and detachment. Real angular
caster silhouettes are not described as map artifacts. Remaining small roofline
marks with shadows OFF belong to the geometry/coverage/outline investigation.
Cached map invalidation includes sun, size, caster mode and instance transforms;
immutable GPU topology/materials still require reloading to change. Camera movement
does not invalidate a static world map. No general dynamic-world system is added.

`--wordline-contacts` adds three bounded analytic ground contacts from the actual
barn base and two tree roots, using `--contact-strength0..0.25` (default.12).
This is an explicit analytic approximation, not baked AO. It subtracts a small
ambient term and attenuates inside an existing cast shadow to avoid double dark
puddles. It does not apply opaque black disks or change original art. M toggles
it; K toggles shadow proof. F7 shadow mode/F9 hard-soft remain available.

## Stage3 — existing bounded volume pass

`--air-proof legacy|bounded` preserves the original integrator as a control.
`--sunlight-profile off|low|standard|haze|auto` selects the bounded path:

- OFF default low tier: zero shaft passes.
- LOW:8 steps, quarter internal dimensions (120×67 at480;160×90 at640).
- STANDARD:12 steps, half dimensions (240×135 /320×180).
- HAZE: shafts OFF, ordinary distance haze only, strength.12 over3..32 units,
  unchanged haze RGB. This is explicitly not a sunlight beam method.
- AUTO: starts12/half; inherited monotonic timing heuristic downgrades to8/quarter,
  then OFF. It is not proof of30FPS or a hardware-approved default.

Bounded mode reuses the existing ray-march framebuffer, shadow sampler, small
low-resolution depth-weighted filter and depth-aware reconstruction. Three
longitudinal quadrature samples replace each step's old center/transverse taps:
24 or36 filtered shadow queries per ray, same query budget as inherited3/step.
No screen noise, TAA, bloom, SSR, new geometry tracer or full-resolution blur.
Density remains spatially bounded, max16-unit segment; near-opaque depth stops
integration. A conservative source-depth footprint prevents background radiance
from crossing foreground low-resolution cells. Map-boundary visibility fades to
zero rather than fabricating lit air outside the supported volume. A lower
forward-weighted scattering coefficient replaces the old locality cloud heuristic.

These controls do not alter material albedo or source normals. Only radiance is
filtered/composited; exact zero-radiance material invariance is tested. Finite
samples can still miss thin foliage, change coverage, band or halo at depth
edges. Quarter resolution may over-occlude narrow gaps. The open front view can
have little visible scattering; sun-facing pose20 tests actual palm occlusion.
Do not claim shafts are visible in every camera view or physically unbiased.
L toggles8/quarter; F10 changes half/quarter; `--shaft-strength0..3` remains.

## Evidence and limits

Retained camera poses0..15 and original routes are unchanged. New16 left-front,
17 canopy,18 directly under broad canopy,19/21 backlit broad tree,20 backlit palm.
`--route strafe|orbit|look|sun` adds only a short deterministic sun-facing camera
move; original three paths remain untouched. No new scene models or geometry.
`--debug-effects` exports true raw float PFM maps and light matrices, including
matching maps at captured motion frames. Fixed-range grayscale previews are
labeled depth diagnostics, not fabricated scene images.

All old B1/OFF and owner toon controls must retain exact-image parity versus the
actual PR28 executable. Tests separately exercise convex/concave rejection,
category reachability, independent toggles, nearest presentation, culling,
shadow geometry versus receiver settings, static map stability and near-solid
shaft leakage. Passing them is not owner visual approval. New Windows launchers
and instructions are in README_RUN_WINDOWS.md. Full private results include an
honest visual gate and NOT RUN matrix; Intel UHD and Windows graphics require
on-device validation even if CI compiles successfully.

Stop: **HOLLAND_003_OWNER_VISUAL_AND_HARDWARE_REVIEW_PENDING**. No merge.
