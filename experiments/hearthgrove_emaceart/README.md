# Hearthgrove — sparse original EmaceArt grass

Owner review rejected the owner-grass replacement. The default is restored to the original EmaceArt grass meshes, original palette/UVs and sparse LOW density. No density boost, owner color-family override, grass remodeling or new artwork. Direct executable launches and PLAY_GAME/PLAY_640/SHAFTS_OFF/EMACEART_CONTROL/LOW_SPEC all select the sparse EmaceArt behavior. The current lighting, shadows, sun-streak profile, canopy arrangement and gameplay remain unchanged.

The previous owner-grass implementation is retained only as an explicit comparison (`--grass-style owner` / OWNER_GRASS_GLOW.cmd) and in the preserved earlier build. It is not the approved/default vegetation direction.

## Build

```sh
cmake -S experiments/hearthgrove_emaceart -B build/emacegrove -G "Visual Studio 17 2022" -A x64
cmake --build build/emacegrove --config Release --parallel 2
ctest --test-dir build/emacegrove -C Release --output-on-failure
cmake --install build/emacegrove --config Release --prefix out/hearthgrove-emaceart-sparse
```

All private art stays outside this public source tree. The owner-only Windows game ZIP includes the validated assets. Code-only CI artifacts do not include models, textures or screenshots.

## Play and compare

Extract the entire private ZIP and run PLAY_GAME.cmd (480×270, original EmaceArt grass, sparse LOW density, shafts/shadows/haze ON). PLAY_640.cmd selects 640×360. LOW_SPEC.cmd reduces grass and disables shafts. EMACEART_CONTROL.cmd uses supplied EmaceArt grass and the previous glow; OWNER_GRASS_GLOW.cmd isolates the grass change; SHAFTS_OFF.cmd retains shadows and haze but disables volumetric light.

WASD/mouse; E rests/interacts; 1 attack, 2 defend, 3 befriend; Enter returns; F5 saves, F9 loads; Esc pauses, Q saves/quits from pause. Existing gameplay, two-companion following/animations and saves are unchanged.

F source/toon; O shadows; V volumetric light; B old glow/new streaks; H haze; L quality; G grass density; R internal resolution. Toon mode retains existing banded lighting, restrained outlines and architecture/foliage edge highlights. Source mode disables both outlines and convex highlights.

```sh
wildsignal_hearthgrove_emaceart.exe --assets assets --grass-style owner --grass-density high --shaft-profile streaks --shadow on --shafts on --haze off --benchmark --pose 13 --resolution 640 --frames 5 --cap 0 --output runs/shafts
```

Controls: `--grass-style owner|emaceart`, `--shaft-profile glow|streaks`, `--shafts on|off`, `--haze on|off`, `--shadow on|off`, `--shaft-strength 0..3`, `--quality low|medium|high`, `--grass-density off|low|medium|high`, `--resolution 480|640|1280`, `--pose 0..15`. Pose 13/15 are eye-height grove views; pose 14 is explicitly an elevated diagnostic. `final_world.ppm` is the genuine internal framebuffer.

## Implementation and limits

Owner grass uses the original 16 near/far pairs (32 meshes), original vertex-region semantics and original four color ramps. Roots, yaw, scale and terrain normals match the EmaceArt comparison. Instancing, conservative culling, existing LOD and grass-cast-OFF/object-shadow-receiving behavior remain. No new ground artwork: short owner grass reveals more of the original EmaceArt field rows and soil. Density/color counts do not imply a particular percentage of visible screen pixels.

The streak profile combines low-resolution shadow-aware scattering with up to eight configurable sun-aligned lighting accents at real sunlit canopy gaps (`sun_shafts.json`). Analytic ray/cylinder intervals and three quadrature samples per accent prevent narrow-beam sampling misses; actual shadow visibility and scene depth clip the result. These are deliberately composed volumetric lighting accents, not a claim of unbiased atmospheric simulation, a screen overlay, light-card geometry or new art assets. Four additional instances of original EmaceArt willows overlap at the existing forest exit; earlier scene instances and village bounds remain intact. Both comparison modes use the same augmented scene/cameras.

The old glow remains a control. Streaks depend on shadows and are disabled when occlusion is unavailable. Atmosphere is reconstructed separately from crisp material pixels; no full-resolution material blur, bloom, high-resolution shadow maps or heavy general-purpose fog. LOW keeps shafts OFF. The default world-anchored cached map remains 1024² (LOW 512²). Effects remain experimental: beam placement is authored lighting data, distant field/vegetation repetition and cutoffs remain, and coarse shadow/depth reconstruction can show artifacts in motion.

The original complete building is reused three times; no new interiors, NPCs, quests, creature art or gameplay systems. All EmaceArt environment source art and owner grass originals are preserved. The custom EmaceArt license allows game use but prohibits standalone asset-library redistribution. Keep the integrated review private.

Run BENCHMARK_EFFECTS.cmd for bounded matched OFF/glow/streak measurements at 480 and 640. Report application/API timings, frame pacing and RAM separately from displayed FPS. Mesa software results do not certify Intel UHD, Android, Win7 or final artistic approval. Stop for owner visual and hardware review; no merges.

## Thicker edges review

The default toon view now uses two-native-pixel outlines and sun-facing convex edge highlights, previously one pixel. Source textures, geometry, lighting and grass suppression are unchanged. This is a bounded second depth/normal sampling ring in the existing post pass; it adds no framebuffer or texture allocation. Foliage keeps its lighter ink and only outer-mass highlights. Ground and individual grass blades remain excluded.

Use `THIN_EDGES_COMPARE.cmd` for the previous one-pixel width at otherwise identical sparse EmaceArt settings. Individual controls: `--outline-width 1|2 --highlight-width 1|2`. Width is measured at the internal render resolution before nearest-neighbor presentation. Source-material mode still disables both effects. The two-pixel reach can emphasize small overlapping leaves at distance; it is a review setting, not final style approval.
