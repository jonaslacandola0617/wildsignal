# Lantern Quay asset feasibility — 2026-10-22

Status: PRIVATE DRAFT ART CREATED; VISUAL QUALITY GATE NOT MET; ENGINE INTEGRATION BLOCKED.

## Scope and evidence

The owner supplied the required private handoff archive and separately approved lamp-post PNG/native source. Four handoff file size/hash checks and all 62 nested reference-manifest image hashes passed. The newer lamp-post was used, with its PNG and native source copied byte-for-byte. Older lantern candidates were not substituted. No creature sources were accessed or modified.

The private package contains 13 PNG/native-source pairs: five materials (cobble, ivory plaster, coastal foundation, blue and russet shingles), three timber modules, door, shuttered window, two house elevations, and the supplied lamp. These are feasibility candidates, not accepted production assets. Only the supplied lamp has prior owner visual approval.

Each newly authored native file has named layers. New files were written directly in the Aseprite binary format and reopened using the accompanying limited parser. This is **not Aseprite application validation**. The parser validates one RGBA frame, normal image layers, raw/compressed cels, chunk bounds and visible composite pixels against PNG. It intentionally rejects unsupported blend, hierarchy, cel and opacity cases. Pillow is required. Do not run validation with Python optimization, which disables assertions.

Private QA includes exact tiled 3x3 and 8x8 previews for all five materials, pale/dark/magenta transparency previews, file hashes, dimensions, native layer/pixel results, and negative checks rejecting bad native magic and a changed visible PNG pixel. These mechanical checks do not establish aesthetic acceptance.

## Quality assessment

Visual gate: **NOT MET**. The house and materials remain more diagrammatic and uniformly patterned than the approved reference and lamp. Repeated stone courses and timber knots are visible. Shingle contours are regular. The plaster is deliberately restrained, but larger architectural surfaces need stronger authored wear/shadow variation. Adding surface noise does not solve the fidelity gap.

The corrected door no longer overlaps the upstairs window. Front and side elevations are drafting views; the street preview is a 2D assembly study. None is an in-game screenshot or a volumetric building. Lamp bytes remain unchanged; the preview uses a nearest-neighbor half-size display. New components use a provisional 64 px/m; lamp source is retained at 128 px/m for a provisional two-meter display height. Density compatibility therefore remains a review item, not a passed uniform-density claim.

## Renderer finding and separately proposed improvement

Inspected `feat/render-001-three-scenes` commit `95cfab8de3b7840bb802a50d28b9f592e58b6bfd`:

- `src/engine001/outdoor.cpp`: height-aware grid DDA draws axis-aligned vertical columns from one ground plane, then billboards. There is no triangle/roof-surface path.
- `src/engine001/render.hpp`: fixed 64x64 material and prop arrays, with procedural construction through `makeEnvironmentArt()`.
- `src/engine001/environment_art.cpp`: existing material/prop pixels are constructed in code. Private external PNG loading and manifest binding are not implemented there.

Required for a truthful house proof: a bounded static textured geometry path with perspective-correct UVs, clipping, per-pixel depth shared with walls/ground/props, pitched roof planes, eave thickness/undersides and rectangular diagonal support volumes. Add a private asset-root loader with dimension/alpha checks; preserve nearest-neighbor sampling and avoid hardcoded production pixels. Keep existing renderer intact for A/B comparison. No gameplay, networking or broad renderer replacement.

Suggested comparison scope, **not implemented or authorized by this note**: one house, one lane, one supplied lantern; front/45-degree/side cameras and a short turn. Require actual executable captures and report geometry occlusion, native texture density, work-time/RSS measurements. Do not attach private images to public CI artifacts or PR comments. The roof and supports must remain fixed in world space while the camera turns.

## Tests not run / blockers

- Aseprite executable not found on PATH: native application CLI reopening, GUI editing, and re-export **NOT RUN**. No install/build attempt was made; availability beyond PATH is not established.
- New scene engine integration and screenshots **NOT RUN**, blocked by geometry and external-asset-loading dependencies.
- Windows/Android device, FPS/RSS/thermal tests **NOT RUN**.
- No owner acceptance, no production master certification, no public artwork authorization.

The task stops at the bounded asset feasibility result. Existing source art and engine branches are preserved. This PR contains only a general validator and this specification/report; the artwork, private manifest and authoring recipe are not committed.

## Run the read-only native check

`python tools/art/validate_aseprite_rgba.py /private/file.aseprite /private/file.png`

A PASS means this supported binary subset matches the PNG, not that Aseprite opened it or that the art is good.
