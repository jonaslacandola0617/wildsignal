# Owner grass and visible sun-shaft comparison

Follow-up to EmaceArt Hearthgrove draft PR #60. Default grass now uses the existing approved R2 large clumps, original root/mid/tip ramps and original near/far meshes. No artwork is generated or repainted. Original light green is approximately75%of placements; green15%, teal8%, deep shade2%, assigned in coherent patches. Roots, yaw, scales, terrain, cottages, trees, rocks, lighting and gameplay are preserved. The previous EmaceArt grass remains available with `--grass-style emaceart`.

Default sunlight uses a separate `--shaft-profile streaks` experiment. It integrates real shadow-map visibility in lit channels adjacent to actual canopy/building shadows, reducing the old full-scene glow. World-space sampling keeps the channels anchored when looking around. This is a stylized shadow-aware volumetric approximation, not a painted overlay or generated light-card mesh. It uses additional shadow taps and must be hardware-tested. `--shaft-profile glow` preserves the old broad-scattering control; `--shafts off` disables either profile. B toggles glow/streaks while playing. Haze and shadows retain independent H/O controls. Turning shadows OFF removes the occlusion information required for geometric shafts.

Use `PLAY_GAME.cmd` for owner grass/high density at480; `PLAY_640.cmd` selects640. `EMACEART_CONTROL.cmd` and `GLOW_CONTROL.cmd` select the comparisons. Existing save/gameplay controls remain unchanged. No cottage, tree, rock, ground-texture or creature artwork changes. No merge or final artistic approval.

## Windows build

```sh
cmake -S experiments/hearthgrove_emaceart -B build/emacegrove -G "Visual Studio 17 2022" -A x64
cmake --build build/emacegrove --config Release --parallel 2
ctest --test-dir build/emacegrove -C Release --output-on-failure
cmake --install build/emacegrove --config Release --prefix out/hearthgrove-emaceart
```

Use the separately supplied private `assets` runtime directory. Licensed models, textures and captures are not in this public source tree. The pack's custom EmaceArt license permits game use but prohibits redistribution as a standalone asset library. Code-only CI artifacts omit all private art.

## Play and inspect

Run `PLAY_GAME.cmd`. WASD/mouse; E rests at cottage markers or encounters the wild Petalyn. Battle uses 1 attack, 2 defend, 3 befriend; Enter returns. F5 saves, F9 loads; Esc pauses and releases the mouse, Q saves/quits from pause. Existing full-loop and two-companion Idle/Fly animations remain. New saves default to `saves/hearthgrove_emaceart.json`.

R toggles 480/640; G cycles grass OFF/LOW/MEDIUM/HIGH; F switches original diffuse versus pixel-toon; O shadows; V volumetric sunlight; H distance haze; L LOW/MEDIUM/HIGH effect quality. All environmental modes use identical original geometry, UVs and palette pixels. Source mode disables both outlines and convex highlights. Source mode is the renderer's diffuse interpretation, not a reproduction of the original Godot PBR shader.

Example genuine native capture:

```sh
wildsignal_hearthgrove_emaceart.exe --assets assets --benchmark --pose 2 --frames 4 --cap 0 --resolution 640 --shafts on --shadow on --haze on --quality medium --output runs/canopy
```

`final_world.ppm` is the actual internal framebuffer. `--pose 0..8`, `--render-mode original|toon`, `--resolution 480|640|1280`, `--grass-density off|low|medium|high`, `--shafts on|off`, `--haze on|off`, `--shadow on|off`, `--shaft-strength 0..3`, `--quality low|medium|high`. Source assets are required; legacy R2/procedural grass switches fail closed in this target. Earlier executables retain their controls.

## Rendering and limits

Original opaque grass meshes are instanced with original UVs and vertex colors. Nested density and conservative distance/frustum/projected-size culling avoid thousands of separate draws. No grass mesh simplification or new distant artwork. Grass receives object shadows and casts none. A shared original 32×32 palette stays resident once. Ground height is a numeric cache of actual placed source triangles, not generated terrain geometry.

Sunlight uses the existing depth-aware, shadow-map-aware quarter-resolution bounded volumetric ray integration with 8 samples (12 HIGH), reconstructed separately from crisp material pixels. Shadows are world-anchored/cached 1024² (512² LOW), covering the village; haze is independently toggleable. Turning shadows OFF also removes occlusion information for shafts, so the best physical comparison leaves shadows ON. Large-area shadow resolution, low-sample shafts and density cutoffs can show aliasing or popping; inspect motion and use LOW_SPEC if needed. No expensive general-purpose fog, bloom or real-time sky simulation.

This is an owner review, not a claim of final artistic approval, displayed FPS, physical Intel UHD performance, Android support or Windows 7 certification. Native software-Mesa results must be labeled separately from Windows hardware results. No new combat, account, inventory, quest or online systems.
