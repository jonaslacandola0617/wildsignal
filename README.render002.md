# RENDER-002: static house geometry feasibility

A bounded hybrid pass adds one spatial gable roof to the unchanged VIS-003 C
building. It is an experiment, not a replacement renderer or final art approval.
Private input, artwork, derived caches and captures belong outside Git checkouts.
The public repository contains only code, synthetic tests and instructions.

## Build and run

Windows x64, Visual Studio 2022 Desktop C++, Git, CMake >=3.24, Python + Pillow:

```powershell
python -m pip install Pillow
cmake -S . -B build/win -G "Visual Studio 17 2022" -A x64 -T v143
cmake --build build/win --config Release --parallel 2
ctest --test-dir build/win -C Release --output-on-failure
python tests/engine001/geometry_test.py build/win/Release/wildsignal_engine001_headless.exe . --native build/win/Release/wildsignal_engine001.exe
cmake --install build/win --config Release --prefix out/render002-windows-x64
```

SDL3 is pinned to the existing immutable release commit and statically linked;
no new runtime library or Windows API is introduced. First configuration needs
network access or the existing offline SDL3 source override. Actual Windows 11
i3/4 GB, Windows 7 and Android compatibility/performance remain unverified until
measured on those devices. A CI software/dummy-video boot is not hardware testing.
Linux without CMake/SDL3: `ASAN_OPTIONS=detect_leaks=0 bash tools/check_engine001.sh`
(disables leak detection only for environments where tracing prevents it).

With authorized private inputs (replace these example paths):

```powershell
.\wildsignal_engine001.exe --sample-pack C:\private\sampling_256.wst --facade-layout C:\private\facade_layout.json --facade-scale human --facade-look c --facade-treatment C:\private\treatment.json --facade-projection perspective --house-geometry C:\private\house.json --camera 19 17 -48 --resolution 480x270 --warmup 0 --fps 30 --output C:\private\unique_run
```

Omit **only** `--house-geometry` for the unchanged flat VIS-003 C comparison.
Choose `sampling_64.wst`, `sampling_128.wst` or `sampling_256.wst` independently
of the world framebuffer. F1 cycles 320x180 / 400x225 / 480x270; W/S move,
A/D turn, F2 HUD, F12 native display screenshot. Scene pixels use nearest
scaling; HUD is rendered separately. `--screenshot FILE.ppm` saves the raw
world framebuffer. Every telemetry output prefix must be new.

## Private geometry input

Schema `wildsignal-house-geometry-experiment-v1` contains `roof` and `wood`
regions, each `{material: integer, u0: number, v0: number, u1: number, v1: number}`.
They reference the already validated sampling pack; all sampled pixel centers in
these regions must be opaque. Normalized UV bounds must contain sampled texels
at **each** desired cache resolution. `roofStatus` must explicitly be
`UNAPPROVED_CANDIDATE` or `TEMPORARY_ENGINEERING`; no production approval is implied.

`rise` is in [.6,2], `eave` in [.1,.5], `thickness` in [.06,.18], and `recess`
in [0,.25], all in existing experimental world units. These are bounded test
parameters, not a final architectural scale contract. Body footprint stays
x=[9,14], y=[7,11], height=3.2; ridge runs along Y. Human door/window source
canvas scales stay 3.2/2.4, with native anchors and unchanged X/Z placement.
The optional recess shifts only the door support plane inward. Setting recess=0
retains the flat door. Camera eye and FOV remain unchanged (1.6 units, 75 degrees).

## Implementation and limits

- 38 authored triangles with the recess, 28 without; fixed capacity 96. Slopes,
  undersides, front/back fascia, eave thickness, end gables and raised solid ridge.
- Six-plane view-space clipping (near .08, far 40), canonical edge intersections,
  fixed subpixel coverage with a top-left rule, perspective-correct UV and forward
  depth interpolation. Orthographic calibration is also supported.
- Existing per-pixel float depth is authoritative for geometry and lamp occlusion.
  Legacy column depth remains grid-only. Final lamp visibility is counted after
  geometry. This distinction must be preserved in any future render paths.
- Static triangle storage and clipping scratch do not allocate during rendering.
  Telemetry exports geometry time, post-clip triangles, covered tests and depth
  writes separately, alongside process RSS and cache/static geometry bytes.
- The door surround is a **rectangular shallow niche**, not a curved arch or
  traversable doorway. Its painted arch remains artwork. Footprint collision is
  unchanged and does not let the player enter the solid building.
- No pitch, interior, stacked floor, general mesh importer, scene graph, dynamic
  lighting, cast shadows, roof shadow on plaster, or collision with the overhang.
  Eye-height views close to a steep roof show its underside; the roof can leave
  the top of the frame at the original close camera. Wider A/B views are supplied
  without changing eye or FOV. These are geometric/camera limits, not art defects.
- No side-wall redesign: VIS-003 plain side plaster remains unchanged. Repeating
  a nonseamless reference crop cannot turn it into approved roof or paving art.

## Evidence protocol

`tools/capture_render002.py` takes `--exe`, `--packs`, `--layout`, `--treatment`,
`--house` and a **fresh private** `--output`. It records exact commands/input
hashes and genuine CPU-rendered framebuffer images for six identical A/B views
(front/corner/side/close/normal/rear), both requested world resolutions and all
three texture modes. It captures identical moving/turning poses and APNG previews,
then runs ABBA screening at each configuration. Default 2000 frames/run, first
500 omitted from comparison summaries; raw CSV retains all samples. Uncapped
shared-host Linux timing excludes SDL upload/presentation and is not the standard
sustained hardware benchmark or a claim of 30 FPS on the owner's machine.

Tests use synthetic materials only. They check shared-edge coverage, triangle
order, oblique UV/depth against independent ray-plane math, six-plane/near-plane
clipping, scenery depth, footprint collision and zero hot allocations, plus
loader/CLI rejection, sampling/resolution comparisons and optional native SDL
pixel equality. Existing scenes and goldens must continue passing.

Private reports should separately assess geometry correctness, artistic quality,
performance and remaining risk. Owner review of both still and moving captures
is required for visual acceptance. No gameplay or online system is added.
