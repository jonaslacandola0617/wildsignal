# TOPDOWN-001 — Lightweight renderer implementation contract

## Aims

- Presentation: 2D top-down / gentle three-quarter world, **16×16 logical tiles**, game framebuffer **320×180** trial, integer nearest-neighbor scaling.
- Source: real, unaltered source art from **Pixel Crawler Free Pack 2.11**. Build import/atlas manifest from its actual file inventory; no placeholder artwork or reauthoring by polygon loops.
- C++20 / SDL3 for window and input; simple private offline PNG import path. No requirement for per-frame libpng decoding; decode/cache once with memory limits.
- Layer order: terrain, secondary ground decals, lower structures, sprites sorted by Y-footprint, overhang/canopy foreground, HUD.
- Map validation: in-bounds tile indices, explicit collision grid, tile alias references with provenance, deterministic test map, no 32-bit-only semantics.
- Motion: WASD/arrows four-direction smooth controls; player walk/idle sprite directional data if available in pack; placeholders must not replace user-owned canonical monsters; companion placeholder permitted for prototype only.
- Captures: real CPU framebuffer PNG/PPM output, stills at 1x and nearest enlarged scale, short moving camera path, FPS and RSS, tests on available system. Mark device-specific tests honestly.
- Draw direction: bright cute fantasy; reuse pack colors/cluster style and 16px units; avoid recoloring user creature masters or importing first-person roof and facade materials.
- Private asset handling: no original PNG or Aseprite sheets or compiled asset pixels committed to public `wildsignal`; load via external user-owned private asset directory. License terms from original ZIP kept with private masters.

## Stages

1. Validate exact license, full ZIP SHA, asset inventory, tile grid, animation layouts and mappings.
2. Run minimal C++ headless original-sheet scene; inspect framebuffer.
3. Integrate SDL3 interactive prototype with player walking, camera follow and collision.
4. Add physically following placeholder companion; do not modify user-owned original monsters.
5. Render a deliberately composed Lantern Quay scene and gather owner art-direction feedback, then later Liltwood forest and Echo Hollow underground previews.
6. Preserve Windows and Android requirements; platform benchmarks follow once real builds exist.

**Note:** No external process that only returns a styled screenshot counts as successful engine execution. Tests, artifacts and QA must originate from the executable that performs the tile/sprite rendering.
