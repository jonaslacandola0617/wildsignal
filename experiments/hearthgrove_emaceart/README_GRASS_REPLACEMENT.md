# Reference-guided grass integration

The owner accepted private art PR #65's R2 model as usable and requested dense meadow coverage, varied sizes, dominant light green with smaller standard/teal regions, and terrain variation affecting green only. Source model and all original EmaceArt files remain unchanged. The pack supplies actual model paths and transforms; no licensed artwork is committed here.

The existing instanced grass path now supports optional manifest `asset_family`, `align_to_surface`, three `instance_color_grades` (linear RGB multipliers, .25–2), and per-root `color_variant` (0–2). Missing optional fields preserve old grass settings. Rotation aligns a clump to its support normal without changing vertices. A clump texture can be loaded into the existing owned/deduplicated texture pool even if no environment material uses it. Expected placement counts fail closed if mismatched. Grass shadow submission stays empty.

Optional `ground_green_variation` (0–1) in each style preset enables broad world-anchored terrain color variation. It is gated by ground material classification and the ORIGINAL source texel's green hue/saturation. Brown soil, bark, rocks, architecture, creatures and other non-green colors bypass it. Omission defaults to zero; earlier packs retain their rendering.

Use the complete private runtime package and **PLAY_GAME.cmd**. It starts HIGH grass density, BALANCED styling, outlines/highlights and clean object shadows ON; grass casting, haze and shafts OFF. R switches480/640, G cycles grass density, J/K/O retain live effects controls. Saves/gameplay/followers are unchanged. No additional launcher is required.

Build from this branch with:

```sh
cmake -S experiments/hearthgrove_emaceart -B build/game -DPIXEL3D_BUILD_GL=ON
cmake --build build/game --config Release --parallel 2
ctest --test-dir build/game -C Release --output-on-failure
cmake --install build/game --config Release --prefix out/hearthgrove
```

Windows: use Visual Studio2022 x64 with CMake (`-G "Visual Studio 17 2022" -A x64`). Copy the verified private `assets/` directory next to the installed executable. The Windows CI artifact contains code only; art stays private. Actual Intel UHD/Android performance and artistic approval require owner review.

The reference-guided model does not include EmaceArt's paid shader, alpha-card material or wind. Existing C++ grass is static; no automatic wind behavior is claimed. Private provenance, genuine native-engine captures and test/timing evidence accompany the review package. Earlier branches/PRs are preserved; no merges.
