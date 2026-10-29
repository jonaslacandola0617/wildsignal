# ASSETCOMPARE-001 — native three-pack building comparison

A new isolated target using Hearthgrove PR #55's C++20/SDL3/OpenGL renderer, importer, collision/movement and UI code. Previous targets and PIXELSTYLE PR #57 are unchanged. No assets are generated, painted or remodeled. Raw art and native captures stay private. Source packs are supplied through private art Issue #56 / staging PR #57; engine Issue #58 defines the execution scope.

## Build and play

VS2022 x64, CMake 3.24+ and Python 3:

```
cmake -S experiments/assetcompare001 -B build/compare -G "Visual Studio 17 2022" -A x64
cmake --build build/compare --config Release --parallel 2
ctest --test-dir build/compare -C Release --output-on-failure
cmake --install build/compare --config Release --component AssetCompare --prefix out/compare
```

The private owner review ZIP adds the permitted runtime assets. Extract All, then run `PLAY_COMPARE.cmd`; no Python/Blender needed to play. OpenGL 3.3 required. Source-only CI does not certify Windows graphical execution, Intel UHD, Win7 or Android.

WASD/mouse; Esc pauses/releases mouse, Enter resumes, Q quits from pause. 1/2/3 select the three matched bay cameras. 4 is an elevated inspection view (use 1–3 to return to walking). 5 selects a 12 m corner view, 6 a nearer corner, 7 the back view. F switches source/pixel-toon **without swapping geometry or texture maps**. R cycles native 480×270 / 640×360 / 1280×720. Cottages are exterior comparison objects, not new playable interiors. Conservative source-bounds collision is retained. No creature, battle, save or other gameplay systems are modified.

```
wildsignal_assetcompare001 --assets assets --style source --resolution 640 --benchmark --pose 0 --angle 0 --frames 4 --cap 0 --output runs/source_a
```

`--style source|toon`, `--pose 0|1|2` (houses) or 3 (overview), `--angle 0|1|2|3` (front/corner/near/back). `--motion --benchmark --frames 450 --capture-every 2` gives the deterministic camera route. `--replay FILE` uses the inherited frame-indexed SDL down/up/mouse replay helper at 30-Hz simulation for actual-control regression and walking captures. Add `--hidden --allow-software` explicitly only for labeled offscreen software tests; normal launch refuses software rasterization. Native capture: `final_world.ppm`; HUD is presented separately. Do not interpret encoded preview fps or uncapped application throughput as verified displayed FPS.

## Source preservation and fair configuration

One genuine complete source building each: Hawtor A `h1` (5,026 tris), Hawtor B `Mid2` (7,956), EmaceArt Free `EA03_Town_Building_Administrative _01a` (16,969). These are selected-house counts, not pack totals. Authentic source SHA256, per-accessor/image byte preservation, source bounds, licenses and uniform placement/scale matrices are recorded privately. Extraction keeps the original node ancestors; external PNGs are copied directly from embedded byte ranges. No texels, normals or UVs are regenerated. Unused houses/towers/smoke and standalone source libraries are omitted from the runtime package.

All three are uniformly normalized to **6 m full bounds height** for this display calibration, preserving proportions. This is not a final human/door scale contract. Bay centers are -11.9/0/+11.9 m, same front offset 12 m, 1.65 m eye height relative to the unchanged terrain, vertical FOV60°, common sun/ambient/sky/exposure. Existing ground geometry is reused with one shared neutral runtime material and two-sided rendering; its original asset/color files and R2 history remain untouched. No new ground mesh is created. Floor tiling/seams are a display limitation, not a new world-art solution.

A: original base-color/vertex-color/alpha plus original source texture samplers and authored emissive image where present. B: same maps and source emission with existing banded lighting/architectural outlines, nearest mip sampling. No per-kit favorite tuning or replacement style atlas. Ground, camera, sun and exposure stay fixed. A small isolated shader extension binds the source emissive map (Hawtor A needs it); it is not replaced by uniform white emission. EmaceArt's optional PBR specular/IOR, normal maps and Godot-specific shaders are unsupported; source A means diffuse/base-color rendering here, not full external-engine material parity.

Shadows use the same cached512 map and fixed50 m directional volume for all bays. Volumetric effects OFF; no grass/foliage is loaded or redesigned. Texture, frame/CPU/API-GPU/memory and submitted/source triangle accounting are emitted in runs. Shader/driver caches and process memory vary; only measured results may be claimed.

Hawtor credit: Toni García Vilche, Medieval House Pack Pixel Art and Medieval House Pack Pixel Art 1, CC BY4.0; full source URLs and transformation notices must accompany the private runtime. EmaceArt (Maciej): custom license permits game use and restricts standalone redistribution. The original packs remain owner-restricted; distribute only integrated necessary runtime assets privately, never raw source libraries or artwork in this public repository.

Draft PRs only. Owner chooses the visual direction; no automatic kit selection or merge.

ASSETCOMPARE001_THREE_ORIGINAL_PACKS_NATIVE_VISUAL_OWNER_REVIEW_PENDING
ASSETCOMPARE001_NATIVE_THREE_PACK_OWNER_VISUAL_REVIEW_PENDING
