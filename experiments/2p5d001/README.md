# 2P5D-001 — hybrid world feasibility experiment

**2P5D_001_OWNER_REVIEW_PENDING.** This bounded experiment does not change the
owner's official top-down direction. Stop for visual review; no production art,
scale, renderer or gameplay approval is claimed.

Builds on TOPDOWN-001 PR #17 (`f21148a`), which already contains the unchanged
RENDER-002 PR #13 geometry implementation. No merge/cherry-pick or old engine
edit is needed. The new standalone experiment links TOPDOWN-001's real private
RGBA loader, map, animation, collision/LOS and bounded follower navigation.
RENDER-002's six-plane clipper, canonical intersections, subpixel top-left
coverage and perspective UV/depth algorithm are adapted into an isolated small
rasterizer. It does not inherit the old facade's art or fixed house dimensions.

## Build and run

Windows x64: VS 2022 Desktop C++, Git and CMake >=3.24. SDL3 is pinned to the same
immutable 3.2.10 commit as the old prototypes; static SDL/CRT. No runtime PNG decode.

```powershell
cmake -S experiments/2p5d001 -B C:/private/hybrid-build -G "Visual Studio 17 2022" -A x64 -T v143
cmake --build C:/private/hybrid-build --config Release --parallel 2
ctest --test-dir C:/private/hybrid-build -C Release --output-on-failure
cmake --install C:/private/hybrid-build --config Release --prefix C:/private/hybrid-runtime
C:/private/hybrid-build/Release/wildsignal_hybrid001.exe --assets C:/private/hybrid-pack/pack.json --data experiments/2p5d001/data --output C:/private/runs/unique
```

Use the authorized private review's verified native crop catalog with the existing
`experiments/topdown001/tools/import_assets.py` and the original Pixel Crawler
Free Pack 2.11 ZIP from `wildsignal-art`, branch
`art/pixelcrawler-freepack-2.11-intake-v0.1`. Expected ZIP SHA-256:
`b7b228ca232da9958f191b01db87764dddb67610116151e06e8dd8ade6de94b8`.
The importer validates ZIP, Terms.txt, every source PNG and crop bounds. It writes
only native RGBA crops, without rescaling, painting, recoloring or alpha removal.
Keep catalog/cache/source sheets and captures outside Git. Public CI uses
synthetic colors solely for engineering tests. Terms permit functional commercial
use and alterations, optional credit and no standalone art sale/marketing.
Anokolisa does not endorse WILDSIGNAL. Canonical creature masters are never loaded.

```powershell
python -m pip install Pillow
python experiments/topdown001/tools/import_assets.py C:/private/original.zip C:/private/hybrid-catalog.json C:/private/new-hybrid-pack
```

Linux: `-DCMAKE_BUILD_TYPE=Release`. Desktop SDL requires platform development
libraries. Console-only CI may pass `-DSDL_UNIX_CONSOLE_BUILD=ON -DSDL_X11=OFF
-DSDL_WAYLAND=OFF` and run with `SDL_VIDEODRIVER=dummy`. This creates a real SDL
window/renderer lifecycle and processes injected events; it is not an on-screen
desktop, hardware mouse test or owner-device acceptance. `-DHYBRID_BUILD_SDL=OFF`
builds the deterministic CPU executable/tests; `-DHYBRID_SANITIZE=ON` enables
ASan/UBSan. The original independent root/topdown CMake projects are unchanged.

## Controls and transition

- Outdoors: W/S forward/back, A/D strafe; mouse horizontal turn (70-degree FOV).
  Arrow keys are additional movement inputs. No vertical look in this experiment.
- E enters when facing the door within the interaction area. A visible prompt
  marks it. Movement pauses during a 0.36-second fade, switches modes at midpoint.
- Indoors: WASD/arrows move a real four-direction animated Body_A base template;
  E at the visible exit returns to the exact saved outdoor position and yaw.
- Esc releases a captured mouse. Esc with the mouse released exits. Click the
  outdoor window to recapture; indoors the mouse is released. Window close exits.
  Focus loss clears keys and pauses. Physical input comfort needs owner playtest.
- F1 toggles native framebuffer 320x180 / 480x270, F2 toggles CPU/FPS/RAM HUD;
  F12 saves raw framebuffer PPM and displayed BMP. Nearest integer upscale and
  letterboxing preserve sharp pixels; small-window fallback is fractional nearest.

One Session owns the authoritative player and explicit mode state. The outdoor
navigation instance mirrors that position only to reuse bounded companion BFS;
it never advances a second player. The follower waits outside during interior
visits. Speeds are experimental: 16 source pixels/world unit, outdoor player
2.5 units/s, indoor 4 units/s, companion 3.0875 units/s with a data-defined 3.75-unit outdoor stop radius. The longer
first-person spacing keeps the follower readable without vertical look.
A 4-source-pixel foot box and <=2px substeps prevent tunneling. No final human,
facade or creature scale contract is established.

## Scene and rendering

`data/approach.json` and `data/interior.json` hold semantic ground aliases,
separate collision and anchored objects. `data/hybrid.json` defines cottage body
and wing, props, portal markers and demo waypoints. Body footprints must agree
with collision; numeric/count limits reject oversized data. No placement is
randomized inside the render loop.

Cottage: two real pitched roof volumes with opaque gables, undersides and thick
eaves, framed walls and a lower attached wing. Doors/windows are attached planes
with their original canvas aspect. Roofs are never billboards. Perspective UVs,
near/far/side clipping and shared per-pixel depth cover roof, walls, ground,
cutout plants and the temporary follower. Ground uses an analytic horizontal
plane with matching forward depth; 16px material modules repeat at one world unit.
Camera eye is 1.45 units, bounded render distance 48 units.

Trees/bushes/flowers and the Orc are upright camera-facing cutouts. Fence planes
stay fixed. Source binary alpha is preserved; fractional-alpha outdoor cards are
explicitly rejected because ordered blending is not implemented. The top-down
blitter supports the original straight partial alpha. Indoor sprites use native
pixels and feet-based ordering at both framebuffer sizes, with pixel-aligned
framing around a data-defined fixed room center. At 480x270 more room margin is visible.

No geometric roof cast shadows, dynamic lights, perspective foliage volumes,
vertical pitch, traversable roof collision, animated opening door/interior mesh,
water animation, polished avatar or four-direction companion art. The door is a
mode portal in front of a solid facade, not a physically walked-through hole.
Trees collide at trunks, fences/bushes at authored cells; flowers are nonsolid
plant decoration. Side-only Orc animation and baked top-down foliage/roof
perspective remain asset compatibility limitations, not solved art production.
Solid sky color is engineering background; no sky artwork is generated.

## Genuine evidence and instrumentation

```powershell
wildsignal_hybrid001_headless.exe --headless --assets C:/private/pack/pack.json --data data --demo --frames 1440 --resolution 320x180 --sequence C:/private/new-sequence --every 4 --output C:/private/new-run --capture C:/private/final.ppm
python experiments/2p5d001/tools/capture.py --exe C:/private/hybrid-build/Release/wildsignal_hybrid001_headless.exe --pack C:/private/pack/pack.json --data experiments/2p5d001/data --output C:/private/new-evidence
```

The route uses the same movement/collision/interact controller: no teleports or
rendered mockups. 1440 fixed 60Hz ticks / 24 simulated seconds, sampled every 4
ticks, produces 360 actual framebuffer captures per resolution. Native/enlarged
stills cover front, corner, street, close, side, oblique vegetation, companion,
room movement/exit and return. APNG retains exactly 360 frames at 15fps, each decoded and checked against its executable PPM; optional ffmpeg MP4 is a lossy convenience preview.
`--pose X Y YAW_DEGREES --frames 1` supplies validated outdoor still cameras.
`--room` starts indoors for isolated measurements. `--debug`, `--readback FILE.bmp`
and `--smoke-test` expose native SDL evidence/testing.

JSON/CSV report outdoor/indoor CPU median/P95 separately, raw per-frame samples,
loop time, process RSS/peak, actual art/framebuffer/depth/geometry/navigation bytes,
positions/yaw, route stages and transitions. CSV
also counts depth-surviving companion pixels. CPU time includes step/render and
invariants; excludes CSV, memory polling, capture, SDL upload/present and pacing.
Loop time includes those operations before CSV. Native mode is capped at 60fps
by default; headless runs uncapped fixed simulation. RSS uses Linux VmHWM or
Windows working-set APIs, not Android PSS. Percentiles use up to the latest
36,000 samples per mode, with total counts retained. No warmup samples are silently
removed. Renderer allocation tests exclude telemetry, SDL internals and loading.

## Validation and review boundary

Synthetic tests cover native cache errors, malformed maps/materials, both
resolutions/modes, full round trip, input/resize/focus, collision/substeps,
perspective UV/depth against independent ray-plane math, shared-edge coverage,
alpha and roof order, near clipping and zero hot CPU allocations. The unchanged
TOPDOWN-001 tests also register here. Earlier first-person tests build independently.

Private evidence/report records actual results and hashes. Windows CI compilation
and dummy-video tests are distinct from private-art Windows execution and physical
keyboard/mouse/latency testing. Windows 11 i3/4GB, Windows 7 and Android remain
NOT RUN until measured on those devices. No deployment or platform certification.

The top-down interior fits the art's authored perspective most naturally. The
outdoor experiment tests spatial presence at the cost of billboard turning,
texture minification, more adaptation work and a second renderer. CPU correctness
and low measured host times cannot approve charm or responsiveness. Compare the
existing top-down slice, this exterior and the transition in real motion. Owner
must choose whether to pursue any further art adaptation or renderer work. No
new region, gameplay or product-direction decision is authorized by this proof.

The interior is a separate map, not a room spatially embedded in the exterior
mesh: its tile footprint is deliberately larger than the visible cottage. Native
source proportions are preserved, but exterior/interior physical scale and the
large exit-door/hearth proportions still need owner review. The foreground exit
door can occlude the avatar near the threshold. Neither limitation is final
architecture or finished character art.
