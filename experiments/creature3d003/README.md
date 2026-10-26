# CREATURE3D-003 — opt-in Driftfin skinning proof

Owner Issue #36. Based on HOLLAND-003 commit `95d50b3afaaddd4c01cc0ace9ffe1e251c9d2d57`.
Only this experiment is new; original renderer experiments and production runtime are unchanged.
Private artwork, study models, captures and binaries are delivered separately in the owner's private art repository. Do not add those bytes to this game repository or public CI.

## Build

```sh
cmake -S experiments/creature3d003 -B build-creature -DCMAKE_BUILD_TYPE=Release -DCREATURE3D_ASSETS=/absolute/private/review/assets
cmake --build build-creature --config Release --parallel 2
ctest --test-dir build-creature -C Release --output-on-failure
```

Windows: MSVC 2022 v143, x64, CMake 3.24+, static SDL/static CRT. Add `-G "Visual Studio 17 2022" -A x64` when configuring. No Windows executable is certified merely because Linux builds. Linux GL diagnostics: add `-DPIXEL3D_TEST_GL=ON -DSDL_UNIX_CONSOLE_BUILD=ON`, then `SDL_VIDEODRIVER=offscreen` and `--allow-software` explicitly permit labeled Mesa evidence. NumPy/Pillow are needed for Python GL capture tests; CPU tests and normal runtime do not use Python.

Pinned dependency revisions are inherited unchanged from HOLLAND-003. This small pilot duplicates its baseline modules inside the experiment to keep old import rules, layouts and shaders intact. Once approved, shared-module consolidation is separate work.

## Run the private review assets

Run with this experiment's `data` folder beside the executable:

```sh
wildsignal_creature3d003 --pack assets/pack_lod0.json --scene assets/neutral_1.json --look data/TOON_DIAGNOSTIC.json --study B1 --no-art-materials --no-smooth-lighting --sunlight-profile off --shadow off --clip Idle --creature-view three-quarter --present-scale 3 --cap 30 --output runs/unique-name
```

- `--clip Idle|Walk|Attack`; keys **1/2/3** switch clips live. Other inherited camera/debug keys still work.
- `--lod 0|1|2` selects the corresponding neutral pack; `--instances 1|4|12` selects its scene. Restarting with these arguments changes LOD/count without loading all LOD geometry at once.
- `--creature-view front|side|back|three-quarter|gameplay`; review cameras use glTF Y up, +Z forward. The Blender source is Z up, -Y forward.
- `--fixed-animation --animation-time 0 --capture-every 1 --frames 48 --cap 0` samples exactly 24 animation frames per second for reproducible proof clips. Capture replay FPS is not measured rendering FPS.
- For the actual environment: `--pack assets/environment_pack_lod0.json --scene assets/environment_creature.json --look data/sunlight_look.json --shadow cached --shadow-proof stable --shadow-size 512 --edge-mode combined --creature-view gameplay`. Keep `--sunlight-profile off`; no bloom/refraction/normal map is needed.
- Output directories must be new. `--output` writes raw frames when requested, frame telemetry and a hardware/software-labeled summary. Native framebuffer masks are diagnostic semantic coverage, not painted artwork.

## Bounded skin/animation contract

One skinned mesh node, one skin, 1–24 joints, four normalized influences, indexed triangles, ordinary base-color material, external PNG pipeline. The deliverable also includes self-contained GLBs; `prepare_runtime.py` reproducibly changes only their image reference to the accompanying identical PNG for this existing renderer's texture loader. Embedded PNG import is not implemented.

cgltf validates buffers/indices. The new importer checks joint indices, influence sums, bind matrices, finite poses, unique clip names, strictly ordered key times and supported channel shapes. Unsupported morph, sparse geometry, extended material, CUBICSPLINE and mixed skin layouts are rejected rather than guessed. LINEAR translations/scales, shortest-arc quaternion rotations and STEP samplers are supported. The pilot uses rotations/translations and positive uniform scale. Weighted rotating normals are normalized in the existing toon shader; no tangent/normal map is required.

Node hierarchy evaluation multiplies joint world pose, inverse bind and inverse rest mesh transform to operate on the inherited loader's world-baked vertices. Each instance uploads its own joint palette; deterministic phase offsets are `instance_index * 0.173` seconds. GPU vertex skinning is used for both color and optional live shadow draws. The CPU position implementation exists for tests, not as a hidden production fallback. Conservative 18 cm local culling padding covers this pilot's bounded clips; it is not arbitrary-animation bounds support.

The original vertex programs are selected verbatim when no skin is loaded, retaining exact static capture parity. In the cached512 profile, **only static environment meshes cast cached map shadows**. Animated creatures receive those shadows and use the inherited bounded analytic ground-contact approximation when the selected look enables contacts. `--shadow live` skins actual creature casters each frame at additional measured cost; no stale cached bind-pose shadow is presented as dynamic shadowing. Optional air effects remain off in the review profile.

## Review and limits

Skin math, actual GLB bind/deformation tests, independent Blender pose comparisons, actual shader/atlas/clip capture tests and unchanged-environment framebuffer comparisons are distinct checks. Software GL timings are not Intel UHD measurements. The inherited frame cap is unchanged; Issue #33 / HOLLAND-004 pacing remains separate.

Owner visual approval, i3-10110U/Intel UHD/4GB performance, physical Windows execution, Win7 and Samsung S10+ remain review/test gates. No Android backend, gameplay mechanics, other species or automatic merge are introduced.

Stop at **CREATURE3D_003_OWNER_VISUAL_AND_HARDWARE_REVIEW_PENDING**.
