# WILDSIGNAL — 2P5D-002: Pixel Crawler Sample-Guided Visual Refinement

**Status:** OWNER AUTHORIZED TO USE OFFICIAL PIXEL CRAWLER SAMPLES AS COMPOSITION REFERENCES; IMPLEMENTATION NOT STARTED.  
**Technical baseline:** [2P5D-001 draft PR #18](https://github.com/jonaslacandola0617/wildsignal/pull/18), commit `6e99f53b716d166097022e8cd973c6cdfa403cb3`.  
**Art source:** private `jonaslacandola0617/wildsignal-art`, Pixel Crawler Free Pack 2.11 ZIP at `third-party/pixel-crawler/free-pack-2.11/` on `art/pixelcrawler-freepack-2.11-intake-v0.1`.  
**Reference samples:** `Pixel-Crawler-Samples.zip`, supplied by the owner in ChatGPT; expected SHA-256 `f79c8f2f3b4621a51e0163be270a5fabfd25e56b9bb48d3818652a477bb788d4`. Intake metadata in **private** `wildsignal-art` branch `art/pixelcrawler-sample-references-v0.1`, `references/pixel-crawler/official-samples-v0.1/REFERENCE_INTAKE.md`. **Important: samples archive not yet uploaded to GitHub**.

## Mission

Keep the working two-mode hybrid system: **2.5D first-person exterior and top-down interior**. Perform one **visually disciplined scene refinement** grounded in the official Pixel Crawler showcases and **actual sprites within the licensed Free Pack 2.11**, without losing CPU performance or technical correctness. This is an art/asset-assembly pass, **not a replacement engine or a production pivot**.

## Required prerequisite

Verify the original archive and private sample ZIP SHA. If sample bytes aren't reachable in Codex Cloud, **stop the sample-dependent work and identify the missing file**, without fabricating what the examples show. Do not rely on the sample filenames alone to pick asset rects. Keep all original images, sample screenshots, extracted atlases, private asset catalogs, private render captures and build input caches private. Public PR may contain code, semantic tile IDs, nonimage metadata, shader/render algorithms, synthetic tests and report text.

Review `AGENTS.md`, `docs/planning/TOPDOWN_PIXELCRAWLER_DECISION_v0.1.md`, `docs/design/VISUAL_BIBLE_v0.1.md`, `experiments/2p5d001/README.md`, data fixtures and current code/tests. Old RENDER-002 and TOPDOWN-001 branches are historical tests; do not merge or rewrite them.

## Phase 1 — Study the artist's visual grammar

Use official reference examples to document **composition principles, not just palettes**:
- `aNwLIs.gif`: cottage silhouette, roof layering, well-proportioned porch/wall/trim grouping; note 5 design variants ≠ 5 animation frames.
- `ScogZt.png`: lush foliage layers, canopy overlap, cliff/cave thresholds, natural path width, dense-but-readable plant composition.
- `eQIQ3J.png`: furnished interior functional zones, room diversity, floor borders, believable furniture spacing and player path.
- `yI9eK1.png`: underground room/corridor continuity, occlusion, mysterious lighting palette (future zone only).
- Secondary scene GIFs `Ddb8VX.gif`, `FVMrrT.gif`, `Ogxo1u.gif`, `rbT_uK.gif`, `Z+HJXA.gif`: examples of interactive outdoor details and decorative clustering, not automatic permission to ship promotional sample pixels as game sprites. `QDHSsj.png` is a snowy forest composition study, not a mandate to recolor Lantern Quay as snow.

**Deliver PRIVATE source/example contact sheets** with native and 4× nearest-neighbor views, plus a **short written critique of existing 2P5D-001 actual stills**. The separate reference pack contains rasterized finished scenes; cannot infer exact map coordinates from screenshots alone.

## Phase 2 — Asset semantic mapping and compatibility

Inspect full genuine Pixel Crawler Free Pack 2.11 art. Create a private verified atlas mapping:
- sprite alias; original ZIP member path; pixel crop rectangle and canvas dimensions; alpha; anchor/pivot; source art perspective; allowed projection (ground/vertical/front-facing billboard/flat top-down prop); texture repeatability; usage/license status.
- **Never** use arbitrary tile-index cycling, crop cells by visual guess or rename a mismatched texture as a different material.
- Explicitly classify each exterior candidate: `NATIVE_2P5D_SAFE`, `PERSPECTIVE_ADAPTATION_REQUIRED`, `REFERENCE_ONLY`, `UNAVAILABLE_IN_FREE_PACK`.
- Avoid top-down roofs pasted without correct normal/UV, trees that rotate like a cardboard card when the camera passes behind, floating props, mismatched sprite scale.
- Use Pixel Crawler-native colors and clusters. No resynthesized art, no shader that blurs pixels, no artificial 'retro filter'.

If every asset needed for a photogenic pitched roof/doors/windows lacks proper first-person representation, declare **art dependency blocked**. You may reuse tested geometric roof construction, but do not counterfeit missing artwork with programmer-art and call it approved.

## Phase 3 — One controlled composition improvement

**Outdoor:** one convincingly charming *Lantern Quay cottage and approach*, NOT expansion to more houses.
- composition priority: (1) roof/wall/window/door proportion, (2) believable material UVs/correct lighting balance, (3) flora perspective/grounded scaling, (4) paths/trees/fence/floral grouping, (5) readable companion spacing.
- sky/ambient treatment may be adjusted only with measured renderer impact; preserve vibrant cute fantasy and clear distant forms.
- maintain camera FOV/collision and current entry trigger where practical; any proposed world-scale change must be reported with exact ratios and before/after.

**Top-down interior:** reuse the source samples' placement principles to fix current empty space, oversized hearth/exit, foreground occlusion and awkward pathway. Require actual walkable floorplan and clear navigation; do not just paste sample screenshot into the scene.

**Hybrid mechanics:** current E-entry fade/return, mouse look/strafe, collision and follower BFS remain working. No extra combat, creatures, networking, new zones, NPC or quests.

Stop after one deliberate v0.2 candidate. If results remain ugly, report failure rather than iterating indefinitely.

## Phase 4 — Quality gates and evidence

**Visual (owner decides):** screenshot feels bright/cute, cottage reads spatially from front + oblique views, trees less billboard-like through turning, props grounded, no dominant texture stamping, coherent room furnishing, companion readable.

**Technical (automatable):** source PNG provenance intact; correct alpha and crop bounds; valid UVs/occlusion, collision/transition roundtrip, no regression in renderer shared-edge tests; no increased hot allocations; meaningful frame/RSS numbers at 320×180 and 480×270.

**Evidence:**
- PRIVATELY store actual executable images for exactly matched baseline and improved poses: front, corner, side, street, near-door, foliage turn, companion-follow, top-down room/exit.
- Before/after pairs at native and 3× nearest resolution, a real 15–30s continuous playthrough with entry and exit, and texture/foliage look-through checks during yaw turn.
- Real scene measurements with context (Linux vs owner Windows), median/P95, peak RSS, asset memory. Do not confuse 60fps capped loop with CPU workload.
- Private proof ZIP + a direct owner visual review page if supported.
- Explicit `2P5D_002_OWNER_REVIEW_PENDING`, draft PR only; preserve PRs #17/#18 unchanged.

## Implementation ownership and scope

Codex should start **a NEW implementation branch** from `feat/2p5d-001-hybrid-world`, NOT implement inside this documentation branch. Codex must use the private art reference intake and sample ZIP once uploaded/access is verified. If the environment cannot reach the references or export private previews, surface that blocker instead of publishing licensed sample screenshots into public GitHub.

**Decision gate:** this experiment tests whether artist-authored composition can make hybrid exteriors consistently charming; official top-down direction remains the safe baseline until owner explicitly chooses hybrid.
