# Petalyn A opt-in animation reviewer — Issue #43

Build this directory to get `wildsignal_petalyn`; the HOLLAND/Driftfin targets retain their previous behavior. Reuses the real HOLLAND-003 GL 3.3 toon/outline/shadow renderer, glTF importer and GPU skinning. No private artwork, creature files or captures are included here.

Provide private `pack_lod0.json`, `neutral_1.json`, the owner's exact `TOON_DIAGNOSTIC.json`, three animated LOD GLBs and external base-color PNG. The private art branch holds all assets and launchers; do not copy them into this repository.

`cmake -S experiments/creature3d004 -B build-petalyn -DPETALYN_ASSETS=/private/runtime -DPIXEL3D_TEST_GL=ON`

`cmake --build build-petalyn`

`ctest --test-dir build-petalyn --output-on-failure`

Use the inherited CLI: `--pack`, `--scene`, `--clip Idle|Fly|Attack`, `--lod 0|1|2`, `--instances 1|4|12`, `--creature-view front|side|back|three-quarter|gameplay`, `--resolution 480|640`. Keys 1/2/3 select/restart Idle/Fly/Attack. Idle/Fly loop; Attack clamps once and can be restarted with 3. Deterministic multi-actor offsets start from the first animated instance, excluding floor nodes. `--fixed-animation --animation-time T --capture-every 1` supports reproducible actual engine captures.

Petalyn disables Driftfin's grounded contact blob. Cached shadows retain static environment casters only; `--shadow live` includes true animated skin casters at an explicitly separate cost. No fake static grounded shadow for this hovering creature. Conservative 0.30-unit animated bounds padding is separately checked in the private Blender validation suite.

Windows GNU cross-builds are not MSVC builds or physical Intel UHD tests. Owner hardware, Windows runtime and Android results must be reported independently. Frame pacing remains the inherited experimental implementation; this branch does not fix Issue #33 or touch grass experiments.

Telemetry adds `animation_ms` for actual TRS sampling/palette construction; `update_ms` includes it. Application-loop and GPU-query domains remain separate. No capture/readback during performance measurement. Inherited frame cap behavior is explicitly outside this task.

Stacked on engine `8e6e4161b7a4e98f73c719a835c920769aaceffe`; art animation draft PR: https://github.com/jonaslacandola0617/wildsignal-art/pull/28 (private owner access). Runtime draft PR must remain unmerged.
