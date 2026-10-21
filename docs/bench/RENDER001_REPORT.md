# RENDER-001 implementation and validation

Status: implementation under validation on `feat/render-001-three-scenes`, based on ENGINE-001 `f647bb1691d31ef050e10950a375d15cd86e726e`. ENGINE-001 owner acceptance covers the initial Windows 11 experience only. Sustained performance, minimum specifications, Windows 7 and Android remain pending.

## Implemented

The existing software grid raycaster now loads independent 24×24 Lantern Quay and Liltwood micro-fixtures alongside the unchanged Echo Hollow fixture. CLI `--scene echo|quay|forest|FILE` selects them. Original programmer art supplies coastal skies, distant scenery, timber facades, roof cutouts, a beacon, lanterns, generic NPCs, forest canopy cutouts, trunks, ferns, mushrooms and a Singing Root silhouette with mint signal glyph. Original private species art was neither accessed nor published.

Outdoor views use the same grid DDA, extended to walk beyond lower foreground surfaces to taller distant surfaces. Ground remains one plane. Outdoor per-pixel depth handles elevated scenery and cutout cards above low surfaces. Echo retains its original wall-column occlusion and exact 1/8 actor color rendering. Existing W/S and A/D controls, collision, nearest-neighbor scaling, HUD, resolutions and telemetry remain.

The 1/8/24 actor pools are separate from scenery. CSV records submitted cards, final visible actors, candidate pixel tests, opaque writes and final sprite pixels. These distinguish population from visibility and expose cutout/overdraw cost. `creatures` retains its legacy field name and means requested generic actors, including NPCs. Camera rotation and wall/card occlusion can reduce visibility. JSON includes scene identity, median/P95/P99 frame times, component timings and measured process memory; unmeasured target-device properties remain null.

## Architecture assessment and limitations

This extension can demonstrate a legible harbor square and layered forest clearing at small resolutions. It does not settle the production renderer choice. Rectangular building blocks, axis-aligned trunks and a single floor remain visible constraints. Roofs, canopy masses and the Singing Root are camera-facing cutout illustrations: they rotate toward the camera, have no solid roof surface and can intersect walls or each other from side views. Equal-depth card intersections have no geometric resolution. No pitched camera, slopes, bridges, walkable upper floors, mesh architecture, true arches, portals, shadows, dynamic lighting or blended transparency exist. Elevated props do not add collision; trunk/building/rock grid cells do. Water and perimeter boundaries are invisible colliders. Sky silhouettes are decorative and cannot be reached.

Leaf/fern cutouts use binary alpha, opaque depth writes and back-to-front sorting; this avoids blended foliage artifacts but produces hard pixel edges. Sway only moves complete cards. Signal glow is painted/pulsed art, not illumination. Water only scrolls a small texture. There is no navigation, interaction or AI. Actor stress pools are static.

Texture storage is fixed at 655,360 bytes. At 480×270 the color, float-depth and ownership arrays plus column depth use 1,170,240 bytes (color upload alone 518,400 bytes). They allocate at resolution changes, not per frame. The fixed telemetry reservation is additional process memory. Original Echo also reserves the outdoor depth/ownership buffers; this is an explicit memory regression to measure, not a claim of equal memory use. The world renderer allocation audit covers all scenes/resolutions/populations across twelve rotations.

A future RENDER-002 comparison is justified if approved production scenes require irregular footprints, stable roof geometry, indoor/outdoor connections or vertical traversal. This task adds no second renderer and makes no final architecture selection.

## Validation record

Local GCC 14.2.0 Release: existing 200 checks and black-box export tests passed. Existing ASan/UBSan checks passed with `detect_leaks=0` because this managed execution environment cannot run LeakSanitizer's process tracing. New outdoor and pixel regression checks and Windows CI results will be recorded after execution. Direct local commands are in `tools/check_engine001.sh`; CMake/Windows instructions are in `README.render001.md`. The local host has no installed CMake or SDL runtime, so SDL execution and Windows compilation run in CI.

Evidence will be attached after capture. CPU-only results exclude texture upload, UI and presentation. The owner's Windows 11 60+180-second/device validation remains pending for each scene. No Android package, Windows 7 claim, minimum hardware claim or thermal result is supplied. No production gameplay or online systems were added. PR #4 and the protected baseline branches remain unmerged and unchanged.
