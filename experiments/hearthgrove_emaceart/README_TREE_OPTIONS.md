# Single-tree owner visual comparison

Experimental branch based on `0e3eaa1`. Previous shadow-map god rays remain
unchanged. No village-wide tree replacement or final artistic approval.

The private runtime supplies `tree_scene.json`, `tree_game.json`,
`tree_study.json`, `tree_cards.bin` and an exact RGBA crop from the licensed
Quaternius Nature MegaKit leaf atlas. These files and all captures remain in
`wildsignal-art`, never in the public engine repository.

Use `--tree-scene --tree-study shader` with the complete private assets.
Press **T** without moving the camera:

| Mode | Geometry | Treatment |
|---|---|---|
| A ORIGINAL | Unchanged original EmaceArt tree | Previous Hearthgrove balanced rendering |
| B SHADER | Same vertices, UVs, indices, placement and silhouette | Canopy-volume normals, coherent toon tones and object-anchored licensed-leaf tonal breakup; brown trunk excluded |
| C FIXED LEAVES | Original trunk/branches; fixed instanced leaf cards replace the selected canopy | Existing licensed leaf image, source-tree palette, coherent volume lighting and matching alpha-cutout shadows |
| D BILLBOARD LEAVES | Same anchors/card sizes as C, camera-facing orientation | Motion diagnostic; shadows deliberately use C's fixed orientation for stability, so visible/shadow geometry differs |

Only the manifest-selected instance changes. Other instances and the original
source files remain intact. Without tree-study flags the original playable
village remains available, without extra foliage texture/buffer allocation.
Missing or malformed private inputs fail closed. No fallback artwork is made.

The isolated test keeps first-person movement, companion animation and existing
game systems. Its save is separate from the village save. R switches 480/640,
O toggles object shadows, J highlights, K outlines, F source/toon, F2 HUD.
Grass retains casts OFF, outlines OFF, highlights ON. Shafts/haze start OFF for
the comparison; V/H remain available. The previous shaft shader is preserved.

The private owner palette now uses **#052e16** for object-shadow grass and green
terrain, **#65a30d** ordinary light and **#a3e635** concentrated surface sunlight.
Only shadow entries in the lighting/beam ramps change; brown terrain and legacy
regional comparisons retain their prior treatment. These are palette anchors,
not a promise every final lit pixel has exactly one hex value.

Cards are a private numeric instance stream: `WST1`, little-endian uint32 count,
then 16 little-endian float32 values/card (center+height, right+width, up+roll,
volume shading normal+reserved). Count, size, orthonormal frames, finite values,
dimensions, source triangle selections and dependencies are checked. Rotational
sphere bounds enclose fixed and billboard cards for conservative culling.
Leaf clusters use alpha discard, depth writes and one instanced draw per tree
pass; no alpha blending, temporal noise or per-frame placement generation.
Internal card edges are suppressed using tree ownership, while external
silhouettes and sun-facing highlights remain available.

`frames.csv` reports actual submitted world triangles (including replaced tree
geometry), `tree_mode` and visible `tree_leaf_cards`. CPU submission, application
loop, swap-call/completion intervals, GPU queries when valid, draw calls and RSS
are separate. A software-GL render does not establish Intel UHD performance or
displayed FPS. One-tree cost does not establish a forest budget.

Build: `cmake -S experiments/hearthgrove_emaceart -B build/tree -A x64`,
`cmake --build build/tree --config Release --parallel 2`,
`ctest --test-dir build/tree -C Release --output-on-failure`, then install with
`cmake --install build/tree --config Release --prefix out/tree-review`.
The Windows artifact is code-only; install the complete private runtime as
`assets/` before running its single `PLAY_GAME.cmd`.

Research: [David Holland, 3D Pixel Art Rendering](https://www.davidhol.land/articles/3d-pixel-art-rendering/)
describes mesh-distributed leaf billboards and flags their cost. Our fixed-card
comparison adapts that concept to a first-person camera; it is not a claim of
identical implementation or appearance. No reference artwork is copied.
