# WILDSIGNAL — HOUSE-EDITOR-001: User-Authored Hybrid Cottage Builder

**Status:** OWNER PROPOSED / SPEC READY FOR CODEX; IMPLEMENTATION NOT STARTED.  
**Owner review baseline:** 2P5D-002 v0.2 / [draft PR #20](https://github.com/jonaslacandola0617/wildsignal/pull/20), commit `afc0e2f410579f8143664f95f146d06ccb15f9ae`.  
**Product direction:** candidate hybrid outdoors first-person 2.5D + top-down indoors. No merge, engine replacement, new canonical art or permanent pivot authorized by this specification.

## 1. Why build the tool?

The owner's direct review of the actual 2P5D-002 cottage screenshot identifies:
- Window/shutter art colliding visually with / obscuring attached planter boxes.
- Dark vertical facade stripes repeated across the wall even though such stripes are corner/end-cap/beam trim, **not seamless center plaster**.
- Green open-shutter windows inappropriate to this house's architecture.
- Codex's automatically designed facades have repeatedly failed owner aesthetic approval.

Root architecture in existing `experiments/2p5d001/src/render.cpp`: `Renderer::buildHouse` builds simple axis-aligned box + pitched-roof volumes and applies a facade module to whole faces; separate beam trim is generated; `Renderer::Renderer` hardcodes door and window planes (see current `panel` calls). `experiments/2p5d002/data/hybrid.json` stores rough `houses[]` dimensions and several `windowbox` props, but **does not encode editable authored window placements/material-region intent**. These are implementation limitations, not just texture export defects.

**Correct division of work:** OWNER composes the house visually; Codex engineers the modeling tools, precise rendering, validation and import/export. Code must not silently alter designer-authored art placements.

## 2. Tool concept

Implement a **small native Windows desktop house editor** using existing C++20 + SDL3 and *the same renderer functions* the prototype uses. A lightweight MIT-licensed GUI layer (e.g. pinned Dear ImGui via SDL3 backend), or a minimal native SDL custom inspector, is acceptable only if license/pinning/build size and reliability are verified. No Blender, Godot, Electron, browser runtime or large mesh editor in this milestone.

Editor process is a **development tool only**; not distributed as the Android/mobile game runtime. The player-facing game loads authored blueprints and does **not** ship editor dependencies. Design to run acceptably on the owner's i3/4 GB Windows PC.

### UI — mandatory
1. **Left panel — Structure/Asset Palette:** add/select/remove (a) one main rectangular house body, (b) one optional attached wing, (c) roof type `gable` with selectable ridge direction, eave/rise/thickness, (d) wall material role, (e) doors/windows/planters, (f) optional timber trim (corners, posts, endcaps). Every selectable asset comes from a *verified privately loaded Pixel Crawler original source rect*, with native-size thumbnail and provenance. Missing compatible art must appear as missing/blocked, not silently substituted.
2. **Center — 2D orthographic editing canvas:** plan (X/Y footprint) and **front facade elevation (X/Z)** tabs. Visible axis, world-unit scale and configurable grid snap (default 0.25 unit), drag/move/resize/rotate supported elements; anchored elements snap to surface and move with parent. Deterministic picking and clearly indicated selection. A 2D elevation view is essential because window/planter overlap cannot be fixed reliably in plan view alone.
3. **Right panel — Live same-engine perspective preview** with mouse orbit and fixed camera presets (front, oblique/corner, side, street, door close). Preview update after every edit without restarting the engine. Use the actual textured/depth-tested rasterizer, not a simulated UI mockup. Show native 320×180 and 480×270 nearest-neighbor 2x/3x options.
4. **Inspector:** exact numeric placement `x, y, z, width, height, yaw`, roof parameters, material region assignment, facing, anchor, overlay depth and asset ID. All drag operations reflected numerically. Unit labels consistent with current 16px source scale.
5. **Editor actions:** New, Open, Save, Save As, Undo/Redo (>=20 operations), Duplicate, Delete, Select, Camera presets, Validate, Screenshot. Save/export must not alter any historical input file unless user chooses it.

### Material semantics — avoid the dark-line problem
Do **not** assume a source `facade_brick` or a complete framed module is a seamless wall tile. Classify texture provenance as:
- `WALL_FILL_TILEABLE`: genuinely edge-seamless center plaster (verified by native 2x2 and 3x3 repeat inspection),
- `WALL_FILL_NON_TILEABLE`: source panel/artwork to be placed intact or scaled only within user-chosen bounds,
- `CORNER_TRIM` / `EDGE_ENDCAP`,
- `BEAM` / `BASE_TRIM`,
- `ROOF_MATERIAL`, `DOOR_OVERLAY`, `WINDOW_OVERLAY`, `PLANTER_OVERLAY`.

A repeated framed panel **must not** generate full-height black borders every repeat. Use plain warm plaster for wall interior; apply true framing only where owner places posts/corner trim. Disable default auto-generated duplicate framing when a wall material already contains trim; reflect that decision visibly in editor.

The tool must **not** invent a seamless material by simply forcing left/right edge pixel equality. If Free Pack 2.11 has no valid plain plaster, report it and allow a separately owner-supplied material, stored privately with provenance and terms.

### Door, window, planter rules — avoid overlap
- Each window is an explicit surface-anchored object with an independently selected frame/style and user-set lower-left `z`, width, height and depth offset. No hardcoded `panel(h.x + 1.1, ...)` calls in runtime build path for newly authored blueprints.
- Planter is an **independent attached facade object**, not a duplicate floating world prop: its center/width/height/z/forward depth is editable, optionally linked to a window.
- Preview should highlight red when window image bounds (including shutters/feet) overlap planter's image bounds in facade elevation, or when depth ordering causes a visual occlusion; allow an explicit “decoratively intentional overlap” override with owner confirmation.
- Provide a **no-shutter** option or compatible style choice when source archive supports it. Do not assume "hide shutter" is possible by cropping a flattened complete window without damaging pixels; warn/source dependency if not separable.
- Door actual sill/collision/interaction anchor should follow visually placed door, rather than an unrelated hardcoded portal location. Geometry apertures are out-of-scope except owner-selected portal trigger placement, so label doors rendered as overlays honestly.

### Geometry and controls — intentionally tiny scope
- Support **rectangular massing volumes only**, axis-aligned and snapped (one main, optional one wing); no arbitrary freeform topology or mesh import.
- Real pitched roof on each volume: ridge X/Y, pitch/rise, eaves, thickness, side/gable variants in existing renderer.
- Support authored timber beams/pillars as optional simple rectangular surfaces, not baked repeated borders.
- Preserve proper wall/roof depth, UVs, clipped triangles, stable mip/nearest behavior, no surface z-fighting.
- Detect severe overlaps between volumes and allow intentional attachment/joint. If geometry cannot support a chosen layout, block/export warning instead of generating corrupt mesh.
- Do not add terrain, caves, large world editor, furniture editor, battle systems or NPC tooling.

## 3. Blueprint asset schema and runtime separation

Save an editable, versioned file such as `house_blueprint_v1.json` **outside licensed asset Git**, with:
- stable UUID-like IDs for volumes/faces/objects; schema version; user-facing house name; parent-child attachment relationship;
- each building volume: footprint `x,y,w,d`, wall height, roof rise/eave/thickness/ridgeAxis;
- each wall face: semantic material role/asset ID, UV scale/offset, optional trim identifiers;
- each applied window, door and planter: parent volume + face, `face_u, z, width, height, depthOffset`, native asset ID and source provenance;
- portal anchor optionally linked to door ID, but no actual scene transition logic in editor;
- editor-only camera and grid preferences stored separately or under a distinct `editor` field;
- validation: bounded positive dimensions, no invalid atlas refs, no non-finite numbers, no roof-inverted geometry, collision/door anchors in valid area.

Add an explicit, deterministic **export to runtime** action that instantiates the new user-authored mesh/overlays in the current hybrid renderer. Legacy `houses[]`/hardcoded layout must remain functional until migration reviewed; do not rewrite the old 2P5D-001/002 scenes. Support optional one-way legacy import. Round-trip saved blueprints must generate pixel-identical previews for the same camera and same immutable art pack.

Private assets remain in `wildsignal-art` and an external user-provided cache; **public game GitHub may include editor code, synthetic fixtures, semantic IDs and schema descriptions, but never Pixel Crawler sprites or derived images**.

## 4. MVP staged acceptance

### Stage A — owner first uses editor
- Editor executable opens on Windows x64, loads real private Pixel Crawler atlas/catalog and current v0.2 house as a *separate working copy*.
- Owner can drag **both windows**, remove the window boxes, move them below the windows independently, swap compatible window art or disable window if no better sprite exists.
- Owner can replace the erroneous repeated framed material with unframed wall fill (if present) and place corner beams ONLY at actual corners.
- Owner can resize house/wing and change roof pitch/ridge/eaves; see all edits in real perspective immediately.
- Owner can save blueprint, close/reopen editor, and see pixel-identical 2.5D preview from same camera.

### Stage B — reproduce and verify
- Automated tests for JSON roundtrip, stable IDs, selection/drag snapping, geometry invariants, instance generation, facade UV roles, invalid atlas use, screenshot determinism, and existing hybrid regression suite.
- Unit/golden **synthetic-only** fixtures may be published to public CI; test with real art privately, screenshots delivered privately.
- Windows native owner-device hardware tests and user-driven drag & drop are **NOT_RUN** until executed.
- Track retained CPU med/P95, peak memory, sprite and mesh instance counts; editor GUI overhead measured separately from shipping runtime.

### Explicit STOP
**HOUSE_EDITOR_001_OWNER_REVIEW_PENDING.** Deliver a downloadable Windows editor build, a save file editable by the owner, an actual native-window video, before/after front/corner/side views, the new draft PR and a change log. No merges or changes to the official camera-direction decision. Focus on *usable authoring experience*, not generated scene beauty.

## 5. Codex execution handoff

Start a new implementation branch **from `feat/2p5d-002-sample-visual-refinement`**. Read `AGENTS.md`, current `experiments/2p5d001/src/render.cpp`, `hybrid.hpp`, `experiments/2p5d002/data/hybrid.json`, existing tests, actual source catalog and this document. Capture exact current v0.2 camera screenshot as baseline. Implement Stage A only plus essential Stage B validation; owner has final artistic control. Branch + draft PR, no merge.

The **tool is not a pixel-art painter**; it is a simple 2.5D house **layout/model authoring** program. We can consider a broader village or terrain editor later, after this standalone house editor is proved useful.
