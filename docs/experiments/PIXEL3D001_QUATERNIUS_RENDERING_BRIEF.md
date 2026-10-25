# WILDSIGNAL — PIXEL3D-001: Quaternius Stylized-3D Pixel-Art Rendering Experiment

**Owner authorization:** 2026-10-25.  
**STATUS: ENGINEERING TASK AUTHORIZED — NO RENDER RESULT YET.**  
**Game repo:** `jonaslacandola0617/wildsignal` (PUBLIC).  
**Experiment branch:** `experiments/pixel3d-001-quaternius-renderer`, based on the verified 2P5D-002 commit `afc0e2f410579f8143664f95f146d06ccb15f9ae`.  
**No merge, no official engine or art-direction change.** Existing top-down, hybrid, sprite, house-editor study, source and documentation preserved exactly.

## Purpose

Determine whether a **true GPU-rendered 3D environment** using the owner's **Quaternius Standard free assets**, stylized into a **bright, cute and readable pixel-art-inspired visual presentation**, can improve exterior visual quality *without compromising performance* on an Intel Core i3 10th gen / 4 GB RAM / modest integrated-graphics Windows PC.

This is a bounded **visual + performance experiment**, not a full open world, production scene migration, or another 2.5D raycaster overhaul. Existing top-down Pixel Crawler interiors remain the baseline. The new rendering path is experimental and isolated.

Inspired by **David Holland's 2024 article**, https://www.davidhol.land/articles/3d-pixel-art-rendering/ . Reproduce general principles (deliberate material colors, toon bands, controlled 1px outlines / highlights, temporal stability, stylized foliage), **not his proprietary scene/art/model assets or unverified-license shader code**. Do not claim exact visual identity with David's demo. Original WILDSIGNAL expression and models are required.

## 0. Required existing project audit

First read `AGENTS.md`, current design supersession docs, the 2P5D-001/002 renderer code, TOPDOWN-001 architecture/asset rules, current PR #20 and house editor spec PR #21 if relevant. Older AGENTS first-person phase statements may be superseded by the newer dated decisions; do not overwrite their historical status.

Inspect actual repo state, test commands, dependency pins and licenses. Branch off the experiment baseline. Implement in `experiments/pixel3d001/` with separate CMake targets and synthetic test assets; do not alter current hybrid/top-down production loops or force-merge other PRs.

## 1. Actual owner source packs (Google Drive)

Folder: https://drive.google.com/drive/folders/1BTja-TCX98SiBpBcQK1eUq3BpPJFIdT4

- **Medieval Village MegaKit[Standard].zip** (~161 MB): https://drive.google.com/file/d/10byfkWcdF3mcNpsmCdCLN4rDCHK_iIaj/view
- **Stylized Nature MegaKit[Standard].zip** (~104 MB): https://drive.google.com/file/d/154NKaGvdVGQwBh_OgAPzLR2688n6MGwM/view
- **Fantasy Props MegaKit[Standard].zip** (~150 MB): https://drive.google.com/file/d/1TLUqKJHIFKJW9Oo2359jYl37addfr6ty/view
- **Wordline Assets.zip:** deliberately OUT OF SCOPE.

Quaternius describes the Medieval Village and Stylized Nature kits as **CC0** on official pages:
- https://quaternius.com/packs/medievalvillagemegakit.html
- https://quaternius.com/packs/stylizednaturemegakit.html
- Verify precise license texts/provenance packaged with the downloaded **Standard** editions before integration; record source URLs, file versions and SHA-256.

**Codex Cloud access warning:** These are user-owned Drive files. They are NOT automatically in the Codex filesystem or publicly downloadable merely because links are in this document. Authenticate through an authorized connected Drive interface where available, or use owner-supplied file uploads / legitimately available official Standard archives. If inaccessible, report `SOURCE_ASSETS_BLOCKED`, stop asset-dependent rendering, and request the exact files. Do not silently substitute synthetically drawn cottages and claim artwork proof. Do not download the paid PRO/SOURCE tier or assume its shader projects are in Standard.

**Private source handling:** Keep source ZIPs, original meshes/materials and rendered evidence out of the public game Git repo by default; staged private runtime assets and private capture packages belong outside the Git worktree. Quaternius CC0 still permits different handling later if explicitly authorized; default is privacy and small repo size.

## 2. Audit model costs BEFORE coding graphics

Extract and inspect glTF 2.0 versions of exact candidate models (along with .bin and texture dependencies), or their accessible validated counterparts. Emit a private `ASSET_INVENTORY.json` with:
- source archive/member path and SHA-256, license; selected model name; primitive/material count; actual indexed **triangle count** (from topology; support nontriangular modes only with appropriate triangulation or reject); vertices, material/texture count, alpha/blend modes; native bounds/scale; textures sizes, decoded bytes.
- identify correct assembly from the modular Medieval Village parts: walls, window/door pieces, corners, gable/roof modules; don't assume the kit has a preassembled cottage.
- identify three coherent nature items (e.g. tree types), one small prop, and optional fence/rock. The true components must be visually selected rather than random file ordering.
- use only one modular cottage, **three trees**, and a few optional props for the hero study. World floor/path may use explicit basic geometry plus a verified Quaternius material; don't repurpose top-down Pixel Crawler wall tiles to cover close first-person walls.

If the archive has too-heavy individual models or unsupported materials, record exactly what is missing; simplify/decimate only in a **derived private copy** with a report of geometry and silhouette impact. Never alter originals.

## 3. Rendering architecture

Use **C++20 + SDL3** with a **hardware-accelerated OpenGL** path for this test, a minimal GPU API choice broadly practical on older Windows desktops. Explicitly query GPU vendor/renderer, OpenGL version, graphics driver where possible and framebuffer capabilities. Prefer a simple compatible OpenGL 3.3 implementation on Windows, but **test real availability** and report fallback requirements; do not claim Android GLES 3.0 equivalence without porting tests. A different existing GPU backend is permissible if measured simpler and preserves Windows/Android targets, but explain the deviation.

Use a small, audited **glTF importer** (existing dependency if present, otherwise pinned small permissive library with license) for POSITION/NORMAL/TEXCOORD/index buffers, transforms and opaque material texture support. Reject or handle missing material data clearly. No runtime FBX parser. GL buffers, textures and draw batches created once on loading, no per-frame asset decoding. Preserve source vertex/mat/UV orientation.

Scene:
- one carefully composed **bright fantasy medieval cottage**, *real modular geometry*, **three trees**, small path and a few optional flower/rock/fence props.
- camera movement: W/S forward/back, A/D strafe, mouse yaw, Esc to release/exit, freeze/camera presets to compare identical poses; static camera debug/automatic orbit path for deterministic captures. The experiment MUST show more than an asset thumbnail.
- one directional sun, ambient fill, simple shading; initially NO dynamic shadows, fog volumes, reflections, transparency depth peeling or costly water features.
- renderer uses depth buffer and correct transparency ordering/masking where foliage requires it. Verify tree leaves don't become black rectangles or unstable overdraw.

### Three exact, runtime-toggleable quality modes — SAME assets and camera:
A. `BASE`: flat/simple material 3D; native presentation.
B. `TOON`: controlled directional lighting bands, charming restrained palette, no edge post-process.
C. `PIXEL3D`: same toon result plus depth/normal **screen-space 1-pixel outlines**, optional convex edge highlight, low-resolution render target with **nearest-neighbor final upscaling**. Outline thickness must be measured at internal framebuffer resolution and not swell into 5–10 pixel strokes. Masking/detection thresholds are scene-tunable but tests report them. Avoid offscreen normals if no actual visible benefit.

Compare **480×270** and **640×360**. Let owner toggle outline, light bands, pixel-grid snap compensation and pixel target resolution individually to isolate quality costs. Optional 960×540 / uncapped native comparison as a diagnostic only. Test in motion: perspective camera snapping may cause jitter if copied from orthographic methods; any compensation must show before/after turning/walking clips, with no false “pixel-perfect” claim.

## 4. Culling and scalability, START EARLY but STAY SIMPLE

**Two-stage CPU frustum culling**:
1. Each *static instance* has validated world-space bounding sphere/AABB from the imported mesh and transform.
2. Partition static instances into logical spatial **chunks** (start from a sensible size relative to actual Quaternius model bounds; do NOT blindly carry over 16px tile units). Build chunk bounds from member bounds. For each frame test camera frustum against chunk bounds; if potentially visible, test object bounds. Skip rejected instance draw submissions.
3. Optional distance culling for tiny ground decals/grass; preserve near-camera pop-free margin.
4. Store static mesh/texture resources once and **reuse** them; avoid duplicating GPU buffers per placed object. Where beneficial, batch repeated meshes/materials; do not implement complex occlusion queries/streaming/world paging yet.

**Benchmark toggle CULL_ON/OFF** (identical scene, route, resolution, mode, hardware). Count visible/culled chunks, visible/culled objects, triangles **submitted**, draw calls. Record **culling CPU overhead**, CPU update/render submit timings, and GPU timestamp query timing ONLY when available and synchronized correctly (avoid benchmarking with `glFinish` per draw). Record actual *presented frame time* and estimated FPS, not just a small isolated CPU function benchmark.

For the hero composition, few objects may mean culling makes little difference. Add a **separate labeled synthetic stress arrangement** using multiple instances of the actual selected models on an extended plane (e.g., 8×8 or 12×12 layout), no new source assets. Check visual culling correctness from front/back/corner and during camera rotation, no false negatives, screen-edge popping or stability regression. Disclose this is a test load, not a designed game zone.

## 5. Performance and compatibility gates

Measure real hardware characteristics and cold/warm loading. Track frame median and P95 (CPU, GPU when valid, end-to-end), max spikes, RAM working set / peak RSS, decoded texture memory, GPU memory where measurable, model/instance counts and resolution.

Budgets are **design goals only**:
- Aim for 60 FPS light scene on the owner's actual Intel i3 10th-gen / 4 GB Windows machine, with 30 FPS low-spec floor.
- Trial target ≤250 MB *game process RAM* for this tiny study, ≤64 MB resident texture storage where practical, no heavyweight engine runtime, no per-frame model/PNG loading.
- Keep the total test lightweight; do NOT artificially downsample/strip assets until they look ugly and claim success.

Cloud CI results or shared-host software renderer numbers **cannot validate the owner's Intel integrated GPU**. If no GPU/display is available in Codex Cloud, complete importer/build/synthetic tests but mark visual hardware execution `GPU_NOT_AVAILABLE`; do NOT generate a fake screenshot from Photoshop or substitute a nonmatching CPU rasterizer without explicit labeling. Ship a small Windows x64 executable with a runnable private asset directory and ask the owner for a local capture/benchmark. Windows 7/Android are separate NOT_RUN until tested.

Technical correctness gates: mesh index bounds/material refs, UVs/normal transforms, chunk frustum conservative correctness, pixel upscale/no unintended filtering, alpha edges, OBJ/glTF coordinate handedness, GL resource lifetime and context loss, camera move/turn correctness, no regression of hybrid prototype.

## 6. Proof and acceptance

Deliver PRIVATE:
- C++ executable and instructions for Windows x64; if possible locally tested on available GPU.
- Actual engine outputs from fixed viewpoints (front, corner, side and 2 distances) for BASE/TOON/PIXEL3D in **both resolutions**, exact same geometry and cameras; nearest-neighbor enlarged review sheet.
- Actual **15–30 second native gameplay video** / framebuffer sequence showing camera turn/walk around cottage and foliage in PIXEL3D; no art mockups.
- Stress scene ON/OFF captures with culled stats and identical camera.
- Source-backed selected model list with triangles/material/texture counts and real memory usage.
- CSV/JSON benchmark A/B tables plus machine + GPU/driver details; code-only public unit tests; license/provenance.
- Honest critique: charm, chunky-pixel visibility near walls, shimmering and outlines in motion, foliage readability and color, proportion accuracy, design labor, first-person feasibility, low-end costs. Explicit best/worst cases.
- Draft implementation PR, on an isolated branch; no merge, no modification to old art or first-person/top-down branches.

### STOP condition

`PIXEL3D_001_OWNER_REVIEW_PENDING` only after actual renders, honest test status and all accessible proofs. If essential model bytes or GPU are unavailable, use `PIXEL3D_001_BLOCKED_INPUT` or `PIXEL3D_001_GPU_NOT_TESTED` — never claim a rendered result just because code compiles.

**Do not expand** into full world, live game integration, Android, 3D playable battles, new character models, editor/UI, MMO, full foliage shader or complete David Holland effect replication. This is a deliberately narrow artistic/performance gate.
