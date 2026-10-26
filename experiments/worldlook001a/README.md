# WORLDLOOK-001A — isolated meadow card proof

**WORLDLOOK_001A_OWNER_VISUAL_AND_INTEL_UHD_REVIEW_PENDING**. Engine issue37, private art issue17 / draft art PR18. Stacked on HOLLAND-004 PR35 exact head `ab32088d43e8ee3a0092a47cafaad6c84b2ccb9f`. Older branches, models, creatures, Pixel Crawler and Quaternius sources are preserved. No merge, biome expansion, tree conversion, gameplay or sunlight redesign.

The owner endorses the grass appearance and base-lighting approach described by [David Holland](https://www.davidhol.land/articles/3d-pixel-art-rendering/#grass). The screenshot and explanation are Holland's own work, not owner-original assets. We independently implement the idea; no Holland pixels/code are copied. The private reference screenshot is not distributed in this experiment.

## Actual art input

`data/grass_source.json` pins the original private kit source commit. The original short/medium/tall grass and yellow/pink/ivory flower variants use a128x128 straight/binary-alpha atlas,12 cropped32px cells, defined source pivot(16,30) and provisional world dimensions. Source JSON/SVG/Python, alternatives, exports and QA live only in `wildsignal-art`. This workspace fetched the actual atlas+manifest back from that private commit and verified the atlas SHA256 against the source export. No procedural green cards are used as final visual evidence; only public CI's clearly labeled engineering fixture generates simple test cards.

Runtime uses the explicit JSON atlas/quad contract rather than glTF: the existing importer expects external PNGs, but an instanced unit-card path needs neither a GLB per tuft nor one material/draw per sprite. `--grass-kit PATH --grass medium` fails when required private input is absent. The native loader bounds/checks12 unique IDs, families, world dimensions, non-overlapping cells, root pivot and card/cutoff contract; GPU upload checks dimensions, binary alpha and zero RGB outside silhouettes. SHA256 is verified by export/readback/package and the Windows suite, not by an invented native cryptography implementation.

## Rendering

Grass OFF is default. No kit, shader, atlas or instance buffer is allocated in that mode, apart from the explicit single-asset inspection view25. The original opaque pipeline is retained; new masked grass draws between opaque and blended objects. The local post fallback only adds the reserved grass-category exclusion; fixed poses/routes must remain pixel-exact versus PR35 when grass is OFF.

One new GL3.3 program and one instanced mesh template: near2 shallowly staggered parallel view-facing quads, mid1quad. Two streaming instance buffers support up to800 clumps. Shared atlas binds once; at most2 `glDrawElementsInstanced` submissions for visible grass, not per-clump or per-blade calls. Existing world draw counts remain independently visible. The full-frame async elapsed query includes grass; `grass_frames.csv` separately reports CPU cull/upload/submission time and actual grass draw/triangle counts.

Each tuft uses its root position and upward terrain normal for directional light, toon bands, ambient and cached512 shadow visibility. Root shadow is computed in the vertex stage with the inherited analytic receiver-plane equation and1/4 taps; its flat result is constant over the card and independent of height/facing. Actual card geometry still writes depth and the matching normal/category only after alpha discard. The bottom material blends to the existing unmodified terrain albedo; no fake disks or transparent blending. Grass neither casts tiny shadows nor receives outlines/highlights; existing architecture/tree/prop edge treatment is preserved.

Default view-facing cards avoid exposed X seams and retain area in a top-down approximation. `--grass-facing cylindrical|crossed` offers controls; the crossed control can visibly expose seams and is not the selected default. Wind is absent. Volumetric shafts are rejected/disabled while grass is active; previous experiments retain all their sunlight controls.

## Placement, culling and candidate LOD

0/150/400/800 seeded clumps share nested position prefixes across densities. Six small garden-local patch zones, minimum separation, prop/building keepouts and a clear entry approach prevent lawn carpeting over the unchanged props. The current scene has a flat plane at y−0.04; this is not a terrain-height sampler or a production placement tool. All artwork dimensions and ranges are provisional.

Conservative4m chunk and per-clump frustum boxes account for billboard width, full height, layering and rotation. Distance disappearance stays active as an LOD rule. `--grass-frustum off` disables frustum rejection for image/work comparison, not distance LOD. Near layer2 continuously shrinks8–13m; the main card shrinks20–28m into its fixed root before culling. No stochastic alpha, per-frame seed, temporal noise or translucency. Subpixel silhouettes can still step at native resolution; this is an owner motion-quality gate, not a claim of perfect temporal stability.

## Build / test

```
cmake -S experiments/worldlook001a -B build/worldlook -DCMAKE_BUILD_TYPE=Release
cmake --build build/worldlook --parallel 2
ctest --test-dir build/worldlook --output-on-failure
```

Dependencies remain pinned SDL3/cgltf/stb. Public CI never fetches private art or owner logs. For actual software GL tests add `-DPIXEL3D_TEST_GL=ON -DSDL_UNIX_CONSOLE_BUILD=ON -DSDL_X11=OFF -DSDL_WAYLAND=OFF`; run CTest with `SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1`, Python Pillow/NumPy installed. ASan/UBSan CPU build uses `-DPIXEL3D_BUILD_GL=OFF -DPIXEL3D_SANITIZE=ON`.

Inherits all12 H4 CPU/GL regressions, plus grass contract and actual instanced-GL tests: nested deterministic densities, bounds/keep-clear roots, LOD continuity and empty culling submissions; alpha/depth/normal coherence, opaque occlusion, root-light rotation/height invariance, actual root cast-shadow reception/OFF control, <=2draw batching, conservative-culling image parity, invalid alpha rejection. Public synthetic fixtures are tests, not visual proof. Native deterministic placement is stable within the same implementation/platform; bit-identical floating math across different C++ standard libraries is not promised.

## Review tools and limits

See README_RUN_WINDOWS.md. The private package includes actual grass source/atlas hashes, original garden assets, native before/after PNGs, top/eye/side views, strafe/orbit replay comparisons and raw frame statistics. Capture runs are separate from noncapture timing runs. `--debug-effects` additionally exports actual grass category mask and scene depth for diagnosis; never benchmark that flag.

The Windows suite measures off/sparse/medium/dense uncapped cost separately from legacy/precise capped pacing at480/30 and640/60, same original19-instance garden/routes, present-scale2, shaftsOFF. Original strict API loop+return P95 gates remain <33.34/<16.67ms. PR35's actual owner hardware review already found modest strict-gate failures with precise pacing; this experiment does not relabel those results as a pass or add more spinning to hide grass cost. Async query timings are not displayed FPS; missing queries stay missing. Logical atlas/buffer size is not physical VRAM; working set is measured per process. Owner Intel UHD grass performance is NOT RUN until the new suite is executed on that device. Win7, Android, thermals, battery and compositor scanout remain NOT RUN.

No source texture downscaling, realistic blade noise, alpha-sort field, extra full-resolution pass or general-purpose foliage system. Textured/non-flat terrain lighting, contact-AO matching outside the selected contactOFF profile, grass collision and arbitrary scene placement remain unsupported. Dense and distant quality must be judged visually and on hardware before any tier is approved. HOLLAND-005, deferred round trees/#30, creature work and WORLDLOOK-001B atmosphere are outside this change.
