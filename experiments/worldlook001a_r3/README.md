# WORLDLOOK R3 — fine-grass visual gate only

The owner rejected R2's broad, rigid plant-like grass. This isolated continuation
loads original private slender ribbon meshes into the existing C++20/SDL3/OpenGL
renderer. One 2×2 m vegetation footprint, with open context terrain. No complete
meadow, Unreal dependency, gameplay expansion, wind, volumetric effects or merge.

Two near variations (12/14 blades, 60/70 triangles) and one far mesh (18 triangles)
are in private `wildsignal-art/WORLDLOOK_001A_R3`. No private artwork or captures
are in this public repository. R2 source files/tests/assets remain unchanged.
The copied R2 opaque renderer differs only at its grass-object initialization
and projected-size parameter hook; its world lighting/post/shadow shaders remain
byte-for-byte unchanged. The existing GL instancing/upload machinery is retained.

Build (CMake 3.24+, C++20; pinned SDL3/cgltf/stb fetched at configure):

```
cmake -S experiments/worldlook001a_r3 -B build/r3 -DCMAKE_BUILD_TYPE=Release
cmake --build build/r3 --parallel 2
ctest --test-dir build/r3 --output-on-failure
```

For headless Mesa also set `-DSDL_UNIX_CONSOLE_BUILD=ON -DSDL_X11=OFF
-DSDL_WAYLAND=OFF`; run with `SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1`
and explicitly pass `--allow-software`. This is software OpenGL, not Intel UHD.
Windows build/hardware testing is deliberately deferred at this visual gate.

```
wildsignal_worldlook001a_r3 --kit PRIVATE/fine_kit.json \
 --look data/TOON_DIAGNOSTIC.json --profile FINE_R3 --resolution 480 \
 --output PRIVATE/new-run
```

Profiles: `OLD_M1`, `OLD_M2`, `FINE_R3`, `R3_GRASS_OFF`. Old profiles require
`--old-kit PRIVATE/R2/mesh_kit.json`; these original files are never regenerated.
All profiles share terrain, sun, exposure, 60° field of view and a 1.65 m eye.
Old controls retain original R2 scale and 24 tufts; R3 studies dense coverage with
360 small overlapping tufts. Counts are explicit, not approved density presets.

`--pose 0|1`: ordinary slightly downward view / steeper look downward, same eye
height. `--route --frames 240 --capture-every 1`: actual 8-second scheduled
walk/strafe/turn/look-down camera route. Frames are bounded to 240 for this gate.
`--no-cull` / `--no-lod` preserve controlled visibility comparisons. Pose14 is an
internal legacy-renderer regression view, not an artistic evidence camera.
Click for mouse-look, WASD movement, Esc release/quit, F1 480/640, F3 pose,
F12 actual framebuffer PPM. No image generation or baked screenshot stand-in.

Fine grass uses fixed world transforms, two-sided opaque geometry, exact
per-tuft terrain albedo samples, restrained upward-normal lighting and no
per-blade outlines/cast shadows. Source tips vary gently; roots embed into the
surface. Shared ground toon bands preserve calm root lighting rather than
faceting every blade. Terrain colors/marks remain at distances where geometry
is reduced. No alpha cutout/blend, stochastic screen noise, bloom or extra
full-screen passes. Up to three instanced batches (A/B/far) during distance
transitions, conservative patch/object frustum culling and bounded streams.
LOD uses projected tuft height in pixels: near/far changes over 10–14 pixels;
geometry collapses toward the ground over 3–7 pixels. It is experimental and
can still change perceived density; fine-tip aliasing is not claimed eliminated.

`frames.csv`, `transforms.csv` and `summary.json` record actual camera, batches,
triangles, memory, CPU submission and raw/validated asynchronous GL queries.
Capture, startup compilation and pacing are included: these are NOT benchmark
runs or displayed FPS. Impossible startup query durations are retained raw and
marked invalid rather than reported as GPU time. No hardware performance claim.

`tools/visual_proof.py` creates only the requested small comparison: ten native
stills plus short matched motion, no benchmark suite. Optional checks verify
frustum image parity and exact grass-OFF output against the real parent R2
executable at the legacy camera. Private assets are validated separately in the
private repository. Status: `R3_FINE_MEADOW_ART_REVIEW_PENDING`.
