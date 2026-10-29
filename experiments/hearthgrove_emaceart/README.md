# HG-STYLE-TERRAIN-R1 — isolated EmaceArt Hearthgrove pilot

Start revision: PR #60, `7c58f5a6ebe7da724d79fd3126134890aed35661`. Private baseline: PR #59, `83b6d9e7dfbea96ec3fc97d33e080dd5abb33ec1`. Earlier experiments and assets remain unchanged on their branches. No merges or artistic approval.

This branch adds versioned per-family HSV controls applied in encoded material-color space with explicit sRGB conversion around the operation. Hardware sRGB sampling is decoded once; lighting remains linear. Alpha, UV sampling, source texture bytes and Petalyn palette are untouched. Instanced grass uses the same grading and band controls. Families: grass, tree/bush foliage, timber/roof, soil/path, stone and terrain; sky has separate look settings. Shared tree atlases classify bark separately by sampled hue. This heuristic can misclassify unusual texels; it is not a painted replacement texture.

The moderate `balanced` and stronger `rich` presets reduce warm overexposure, preserve three broad toon levels, and offer selective off/balanced/bold ink. BOLD extends only major architecture silhouettes to two native pixels within 28m. Interior strokes and foliage stay one pixel, grass/terrain receive no outlines, and added highlights have a brightness ceiling. Existing pale art can already exceed that ceiling; the effect never darkens it to enforce the cap. A ground-only value-contrast control reduces the original field-row color contrast without painting new art.

Default candidate: cast shadows/shafts/haze OFF, restrained inexpensive ground contacts ON. O/V/H retain independent comparison toggles. Shadow ON is an option, not rejected universally. The original atmospheric system remains available; no new volumetric work. Contacts include original EmaceArt house/tree/rock/prop roots, with no individual grass shadows.

## Build and run

```sh
cmake -S experiments/hearthgrove_emaceart -B build/style-r1 -G "Visual Studio 17 2022" -A x64
cmake --build build/style-r1 --config Release --parallel 2
ctest --test-dir build/style-r1 -C Release --output-on-failure
cmake --install build/style-r1 --config Release --prefix out/style-r1
```

C++20, SDL3, OpenGL 3.3; dependencies retain pinned inherited versions. Code-only CI artifacts lack licensed runtime assets. Obtain the complete **private game review ZIP**, extract into a new folder, and run `PLAY_GAME.cmd`. `PLAY_640.cmd`:640×360; `PLAY_BOLD.cmd`:rich/bold; `LOW_SPEC.cmd`:sparse LOW grass; `BASELINE_ORIGINAL.cmd`:original scene/settings; `SOURCE_MATERIALS.cmd`:source shading on final geometry; `SHADOWS_ON.cmd`:balanced with casts. `ALL_EFFECTS.cmd` retains experimental atmosphere. Full source assets may not be redistributed as a standalone pack.

Settings: `assets/style_profiles.json`, revision1, copied from this branch's safe numeric `data/style_profiles.json`. `--style-settings FILE` overrides the path. `--style-preset baseline|balanced|rich`; `--ink off|balanced|bold`; `--contacts on|off`; `--hud off|compact|debug`. Baseline needs no new settings file and restores original lighting/bands/edge behavior. Use `assets_baseline` for exact original geometry as well. Candidate and baseline remain separate runtime folders. Grass defaults to MEDIUM; no source grass remodeling.

All existing controls remain: WASD/mouse, E rest/encounter,1attack/2defend/3befriend, Enter return, F5/F9save/load, Escpause/Qsave-quit, F source/toon, O shadows, V shafts, H haze, G density, R480/640, L quality, F3follower debug. F2cycles compact/off/debug HUD. HUD changes preserve the underlying game state and telemetry. Clean `final_world.ppm` is the native framebuffer; presented captures include actual HUD.

## Terrain and private runtime

The private assembly derives walkable top surfaces from original source meshes, retaining XZ footprints, UVs, texture/material bytes and recognizable source identity. Y/normals are reprojected to gentle relief with calm building pads. Side/bottom faces that collapse into coplanar duplicates are removed; source colors are decoded according to their original UINT16 normalization. Family/24m spatial batches stay within existing model budgets and conservative culling. Final rendered triangles are rasterized into the 257² height cache. Roads route around actual conservative colliders; one original entrance tree and its collider move together by0.9m to clear the spawn corridor. Props and existing grass roots are reprojected to the same surface. No cache-only smoothing, replacement building/tree/rock/grass or texture painting.

The grass distribution selects from original roots, using stable clustered masks, path/door/rest/wild-companion clearances and lower existing clump scale. Original mesh/palette files remain unchanged. This is a readable settlement pilot, not a dense-meadow target. Distant cutoff/LOD limitations and source field-pattern repetition remain.

Optional private validation:

```sh
python experiments/hearthgrove_emaceart/tests/terrain_mesh.py PRIVATE_ASSETS PRIVATE_ASSEMBLY.json terrain-validation.json
python experiments/hearthgrove_emaceart/tests/runtime.py PATH_TO_EXECUTABLE PRIVATE_ASSETS
```

Terrain QA independently raycasts the final glTF triangles and checks slopes, seam steps, cache agreement and route collision. It fails on any sampled blocked corridor. `GAMELOOP_ASSETS` plus `HG_TERRAIN_LAYOUT` adds this check to CTest. Windows hardware launcher records BASELINE_OLD/OFF/BALANCED/BOLD/LOW at480/640, adapter/driver identity, counters, process RAM, API timings and swap intervals. These do not prove sustained displayed FPS. Mesa software is not Intel UHD. Physical Windows GPU, Android and Win7 remain unverified until actually tested.

Private review includes matched baseline/Phase1/final matrices, terrain QA, real gameplay preview, source hashes and performance distributions. Automated tests cannot approve the visual direction. Stop at `HEARTHGROVE_EMACEART_STYLE_TERRAIN_R1_OWNER_REVIEW_PENDING`.
