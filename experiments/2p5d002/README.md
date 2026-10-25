# 2P5D-002 — sample-guided visual v0.2

**2P5D_002_OWNER_REVIEW_PENDING.** One controlled composition candidate on
`6e99f53` / PR #18. This is environmental assembly and bounded presentation work;
the official top-down direction, original creatures and hybrid mechanics remain.
No production art/scale approval or engine replacement is claimed.

The owner handoff is on draft PR #19, branch
`docs/2p5d-002-pixelcrawler-sample-study`, path
`docs/architecture/2P5D002_PIXELCRAWLER_SAMPLE_VISUAL_HANDOFF.md`.
Implementation stays on its own feature branch from #18; no old PR is merged.

## Verified source and reference study

Private art repository `jonaslacandola0617/wildsignal-art`:

- Original Free Pack2.11, branch `art/pixelcrawler-freepack-2.11-intake-v0.1`,
  ZIP SHA256 `b7b228ca232da9958f191b01db87764dddb67610116151e06e8dd8ade6de94b8`.
- Official sample ZIP, branch `art/pixelcrawler-sample-references-v0.1`,
  commit `cad8b8a7e9473b487b04af7015ea1cd1f2e69b53`,
  `references/pixel-crawler/official-samples-v0.1/Pixel-Crawler-Samples.zip`,
  SHA256 `f79c8f2f3b4621a51e0163be270a5fabfd25e56b9bb48d3818652a477bb788d4`.
  All10 member hashes, dimensions/frame counts match private intake.
  The intake's old “not uploaded” statement is superseded by actual verified bytes.

Private native/4× sample/source studies and baseline critique are delivered with
review evidence. aNwLIs is five house variants, not five animation frames. Cottage
studies show grouped roof/trim/facade bays; ScogZt layers vegetation at irregular
path margins; eQIQ3J places furniture in functional zones around clear walk lanes.
Underground/snow/workstation samples are reference only, not new zones/features.
Sample screenshot pixels are never imported into the game.

## What changes

`data/` is a selectable v0.2 scene. Existing `experiments/2p5d001/data` stays
unchanged. A small backward-compatible extension of the same renderer enables:

- Main pitched ridge across the frontage, secondary gable retained; same real
  roof shell, eaves/thickness and correct depth. No roof billboard or fake porch.
- A native96×48 framed/brick facade at16 source pixels/world unit. A6×3-unit wall
  fits its source aspect exactly; smaller faces crop the module rather than
  stretching it. Source limits reject larger walls for this candidate.
- Fixed crossed plant cards and horizontal ground decals. Plants do not rotate
  with camera yaw; cross-card seams, repeated silhouettes and baked top-down
  perspective remain visible adaptations. They are not volumetric tree assets.
- Actual stemmed lupines, grounded rosettes/ferns, attached window boxes,
  clustered trees/bushes and quieter dirt with native grass-fringe tiles over an
  explicit opaque underlay. No per-frame decoding, placement randomization or
  generated artwork. Unknown prop modes/roof axes fail clearly.
- A quieter native room floor, wall window, sleeping/dining/storage groups,
  small tabletop detail, and an open foreground threshold replacing the oversized
  opaque exit sprite. Hearth is removed from this bedroom, not secretly resized.

Cottage footprints/heights, FOV70, eye1.45, player/follower sizes/speeds, portal,
mouse/strafe controls, fade/roundtrip and follower BFS remain. Tree canvas changes
3.2×6.4→2.8×5.6 units (0.875 ratio). Room walkable floor18×8→15×8 tiles (5/6 area),
with native furniture pixels unchanged. These are experimental exact comparisons,
not a final numerical world-scale contract. The room is still an independent
map, not a physically embedded cottage interior. Orc/Body_A remain placeholders.

## Build/play (same C++20 SDL3 executable)

```powershell
cmake -S experiments/2p5d001 -B C:/private/visual002-build -G "Visual Studio 17 2022" -A x64 -T v143
cmake --build C:/private/visual002-build --config Release --parallel 2
ctest --test-dir C:/private/visual002-build -C Release --output-on-failure
python -m pip install Pillow
python experiments/topdown001/tools/import_assets.py C:/private/original.zip C:/private/visual002-catalog.json C:/private/new-pack
C:/private/visual002-build/Release/wildsignal_hybrid001.exe --assets C:/private/new-pack/pack.json --data experiments/2p5d002/data --output C:/private/runs/unique
```

Use the PRIVATE reviewed catalog from this delivery, never sample scenes or
arbitrary tile indices. All runtime crops retain exact original native RGBA.
Actual Terms.txt permits functional commercial use/changes, optional credit and
no unapproved standalone art sale/marketing; no Anokolisa endorsement. Source
ZIP, references, catalogs/caches and media remain outside public Git/CI.

WASD/arrows move, A/D strafe outdoors, mouse turns, E enters/exits; Esc releases
captured mouse then exits, click recaptures. F1 native320×180/480×270, F2 HUD,
F12 capture. `--demo --frames 1440` gives the same real24-second controller route.
Existing001 data/pack still select the old look. Install places baseline data in
`data/` and candidate data in `visual002/`. Owner package includes launch scripts.

Linux and sanitizer options are unchanged; use dummy/software SDL only as an
explicit console test, not a desktop/target-device playtest. See the baseline
README for timing, memory and excluded costs. No new runtime dependency is added.

## Evidence and tests

Private review pairs actual executable output at matched front/corner/side/street,
near-door, foliage yaw/orbit, physical companion-follow and room/exit poses.
Native PNG/PPM and exact3× nearest copies are retained, plus both real24-second
playthroughs. References/contact sheets are labelled separately from engine output.
Scene/camera/asset/executable hashes and exact commands make the comparisons
reproducible. Legacy001 pixel comparisons guard the backward-compatible path.

Public synthetic tests cover both fixtures, modes/resolutions, native module
limits/alpha, ground underlay, roof axes, occlusion/UV/shared edges, collision,
roundtrip and no added hot allocations; SDL tests inject actual events. Private
asset validation independently compares crop payloads to original ZIP pixels.

Owner decides visual quality. Fixed crossed foliage is still a compromise and
source roof crops can stamp/shimmer. No pitch, cast shadows, true foliage volumes,
new avatar/creature artwork, final interior scale, gameplay systems or region
expansion. Win11 i3/4GB, legacy Windows and Android remain NOT RUN until actual
device tests. Stop after this single v0.2 and report remaining failures honestly.
