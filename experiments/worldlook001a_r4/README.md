# WORLDLOOK R4 — private pixel-surface visual gate

A controlled material experiment for engine Issue42/private art Issue22. R3's exact
mesh loader,360root transforms,12/14-blade meshes,18k-triangle terrain, camera,
1.65m eye,60° FOV, LOD and culling are reused. R3 remains unchanged. The default
(no `--surface`) renders the original R3 shader path. A separate isolated GPU
adapter adds the optional material without mutating inherited world/post/shadow
shader files or prior experiments. No production system or world expansion.

Private original art is in `wildsignal-art`, draftPR24, source commit
`4e82ff50094a257a6c63b4a4aafe1c70cea8faea`. Public metadata lists exact blob IDs;
no artwork, palette colors, reference footage or captures are in this repository.
The layered128px art is script-authored using explicit polygons/stamps, not claimed
as human hand-painted. No reference assets copied. Original R3 Blender/GLBs remain
byte-identical in private PR21. A new geometry export is neither needed nor used.

The source map spans the existing6m context, with grass restricted to the original
2×2m. Texels map from(-3,-3)worldXZ to128px, top PNG row=negativeZ. NEAREST
`texelFetch` uses world coordinates, with clamp at the context boundary and no
camera/time inputs or runtime random noise. No repeating checkerboard. Ground is
uploaded as SRGB8_ALPHA8; group IDs are linear RGBA8 data. Palette HEX is converted
from sRGB to linear, before the existing toon/exposure/tone mapping/gamma path.

Three broad authored world patches select body/tip color families for entire
tufts. The same ground texel colors each root. Upper local-height bands mix
root/body45% and root/tip70% above.135m, leaving some restrained thin highlights.
These ratios are experimental material controls, not new geometry or lighting.
Two opaque128²RGBA textures add131,072logicalGPU bytes with no mip chain. No new
render pass, alpha blending, grass shadows, blade outlines, wind, bloom or shafts.
Existing exact projected-size LOD is retained, including its known density/height
transition; the authored ground retains pattern when blades diminish.

Build (CMake3.24+,C++20; pinned SDL3/cgltf/stb fetched by CMake):

```sh
cmake -S experiments/worldlook001a_r4 -B build-r4 -DCMAKE_BUILD_TYPE=Release
cmake --build build-r4 --config Release
ctest --test-dir build-r4 -C Release --output-on-failure
```

Windows x64 source-build command is reproducible, but this visual gate does NOT
include a Windows build or IntelUHD/Android/Win7 certification. Hardware work is
intentionally deferred pending owner art review. Linux Mesa/offscreen build uses
`-DSDL_UNIX_CONSOLE_BUILD=ON -DSDL_X11=OFF -DSDL_WAYLAND=OFF` and existing pinned
FetchContent source directories as recorded in the private validation logs.

Run with the untouched private R3 kit plus verified R4 exports outside publicGit:

```sh
wildsignal_worldlook001a_r4 --kit PRIVATE/R3/fine_kit.json --look experiments/worldlook001a_r4/data/TOON_DIAGNOSTIC.json --surface PRIVATE/R4/pixel_surface.json --profile PIXEL_R4 --resolution 480 --output PRIVATE/new-run
```

Use `--resolution 640` for640×360. Choose `--pose 0`
or `--pose 1` (second view looks down at path edge). Click captures mouse; WASD
walk/strafe, mouse turns/looks down, Esc releases/exits; F1resolution,F3pose,F12 nativePPM capture. `--route --frames240 --capture-every1` must be written with
spaces (`--route --frames 240 --capture-every 1 --cap 30`) for the fixed8second
walk/strafe/turn/lookdown. Omit `--surface` for exactR3; `--profile R4_GRASS_OFF`
shows new ground alone; `--albedo` is allowed only with grassOFF. Legacy R2 profile
controls remain available with `--old-kit`; this delivery does not rerun them.
Use `--allow-software --hidden` and SDL_VIDEODRIVER=offscreen only for explicitly
labelled Mesa software captures. `--no-cull`/`--no-lod` remain diagnostic controls.

Focused verification and bounded visual comparison:

```sh
python experiments/worldlook001a_r4/tools/visual_proof.py --exe build-r4/wildsignal_worldlook001a_r4 --kit PRIVATE/R3/fine_kit.json --surface PRIVATE/R4/pixel_surface.json --parent PRIVATE/R3/wildsignal_worldlook001a_r3 --output PRIVATE/new-proof --motion
```

The helper renders exact A/B/new-groundOFF views at480/640 and two fixed poses,
checks R3 grassON/OFF RGB against actual unchanged R3 executable, default-view
culling equivalence, native PNG identity, identical camera/LOD/counts in motion,
a groundalbedo control and an ambient-only shade-profile control. The latter is a
material robustness test, not a claim of actual occluding geometry/shadow in this
open plot. CPU metadata tests cover bounded external mappings/sRGB/path rejection;
actual texture hashes/opaque dimensions/palette separation are recorded privately.
Movie comparisons replay actual framebuffers at30fps for synchronized cameras;
that is not displayedFPS evidence. A separate480native clip follows actualAPI
completion intervals. Mesa query/CPU/RSS capture values are software diagnostics,
not IntelUHD performance, and include capture/readback/startup overhead.

Known limits: fine grass can still produce subpixel speckle and native-nearest
edge aliasing in motion. No filtering/LOD solution is claimed; inheritedLOD can
change density. This small sterile context is not the reference's composed market,
lighting richness or a complete environment. Owner judges visual success; tests
cannot approve art. Optional silhouette passC is NOT implemented, to keep A/B
geometry exact and expose whether palette alone suffices. Do not expand or run a
hardware suite before the owner reviews this proof.

Stop: `R4_PIXEL_MEADOW_OWNER_VISUAL_REVIEW_PENDING`.
