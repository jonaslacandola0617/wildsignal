# R5 soft-leaf grass — limited owner visual gate

This experiment compares the exact R4 control against short, rounded leaf shapes
mixed with the existing fine filler. It uses the same 2×2 m patch, terrain,
palette, first-person camera, 1.65 m eye height, 60° FOV and lighting.

A omits `--soft-kit` and renders R4 unchanged. B loads the actual private R5 GLBs
and replaces 108 of the 360 tufts, preserving every original root position,
orientation, source scale and tint. There are 252 fine tufts and 108 soft tufts:
3,276 fine blades and 1,404 soft blades, exactly 70/30 by blade and tuft count.
This is not a claim about visible area. Soft leaves naturally cover more pixels.

B also renders retained fine filler at 78% of its original height so it does not
bury the short leaves. This intentional presentation adjustment is recorded in
the private art manifest. Original fine meshes and source matrices remain intact;
A retains its original heights. New leaves are 0.125–0.175 m tall and 0.036–0.055 m
wide before existing instance scaling. They have narrow embedded roots, six
rounded width sections, asymmetric bends and softly tapering pointed tips.
They are two-sided curved mesh surfaces, not solid succulent leaves or billboards.
The original R4 terrain-biased diffuse normals are retained for consistent shading.

The main R4 GPU renderer, shaders, image loader and pixel material loader are
compiled directly from their unchanged sources. The grass adapter only extends
three instanced mesh slots to six. A uses the original three slots; B adds soft
A/B/far meshes with 132/154/42 triangles. No new textures, shading effects,
transparency, grass shadows, wind, flowers, world expansion or gameplay systems.
Existing LOD thresholds are retained. Actual changed mesh bounds affect culling,
so a few visible root counts can differ at frustum edges. Culling-disabled tests
verify identical LOD root counts; source transforms remain camera independent.

Private editable Blender source, GLBs and manifest are in `wildsignal-art`,
[draft PR #25](https://github.com/jonaslacandola0617/wildsignal-art/pull/25), pinned
at `a8410a37300e6a98a85897a30759f494e5673cb5`. No artwork is in this public directory.
R3/R4 sources, earlier branches and PRs are preserved; no merge is authorized.

Build with CMake 3.24+ and C++20 (the same pinned SDL3/cgltf/stb dependencies):

```sh
cmake -S experiments/worldlook_r5 -B build-r5 -DCMAKE_BUILD_TYPE=Release
cmake --build build-r5 --config Release
ctest --test-dir build-r5 -C Release --output-on-failure
```

Linux software capture uses `-DSDL_UNIX_CONSOLE_BUILD=ON -DSDL_X11=OFF
-DSDL_WAYLAND=OFF`, plus existing pinned FetchContent source directories recorded
in private build logs. Windows x64 source commands are provided for reproducibility;
Windows compilation/runtime, Intel UHD performance, Android and Win7 are NOT RUN.
This delivery is a small visual proof, not a hardware benchmark or Windows package.

Run B using private assets stored outside public Git:

```sh
wildsignal_worldlook_r5 --kit PRIVATE/R3/fine_kit.json --surface PRIVATE/R4/pixel_surface.json --soft-kit PRIVATE/R5/soft_kit.json --look experiments/worldlook001a_r4/data/TOON_DIAGNOSTIC.json --profile PIXEL_R4 --resolution 480 --output PRIVATE/new-B
```

Omit `--soft-kit` for A. Use `--resolution 640` for 640×360. `--pose 1` looks
farther downward at the same path edge. Click to capture the mouse; WASD moves
and strafes, mouse turns and looks down, Esc releases/exits. F1 switches resolution,
F3 switches fixed poses, F12 saves a native PPM. Free first-person movement remains.
`--route --frames 240 --capture-every 1 --cap 30` captures the fixed eight-second
walk/strafe/turn/look-down route. `--no-cull` and `--no-lod` remain diagnostics.
Use `--allow-software --hidden` with SDL_VIDEODRIVER=offscreen only for explicitly
labelled Mesa captures. New runs require a new output directory.

The bounded proof helper runs actual engine A/B and R4 parent comparisons:

```sh
python experiments/worldlook_r5/tools/visual_proof.py --exe build-r5/wildsignal_worldlook_r5 --kit PRIVATE/R3/fine_kit.json --surface PRIVATE/R4/pixel_surface.json --soft-kit PRIVATE/R5/soft_kit.json --parent PRIVATE/R4/wildsignal_worldlook001a_r4 --output PRIVATE/new-proof --motion
```

Focused CTests cover inherited patch/camera and surface contracts plus the exact
108-root mix and duplicate/bounds rejection. Private actual-asset checks validate
GLB geometry, pointed/rounded profiles, ratio, immutable matrices/tints, footprint,
conservative vertex culling and identical unculled LOD counts. Actual fixed A
and ground-OFF RGB must match the unchanged R4 executable at both resolutions.
A/B moving cameras are identical; draw/triangle counts can change with geometry.
Comparisons contain unscaled native frames; movie replay FPS is not displayed FPS.
Mesa software query/CPU/RSS data includes capture, driver and pacing overhead and
must not be labelled Intel UHD performance or hardware GPU execution timings.

Visual limitations remain for owner review: the change is easier to inspect in
closer oblique views, fine filler can still shimmer, leaves can look thin edge-on,
and inherited LOD can alter density. The test remains a bare context plane rather
than a composed environment. Do not claim final beauty, production readiness or
expand the patch before owner review.

`R5_SOFT_LEAF_OWNER_VISUAL_REVIEW_PENDING`
