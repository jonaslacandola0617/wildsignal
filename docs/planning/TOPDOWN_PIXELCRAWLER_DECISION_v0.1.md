# WILDSIGNAL — Top-down pivot & Pixel Crawler environment-art decision v0.1

**Owner decision confirmed:** 2026-10-24  
**Status:** PRODUCT/ART DIRECTION CONFIRMED BY OWNER; ASSET BYTES NOT YET VERIFIED IN THIS BRANCH; TOP-DOWN IMPLEMENTATION NOT STARTED  
**Applies to:** upcoming top-down prototype / future environment art production.  
**Does not overwrite:** prior first-person RENDER-002 proof, existing PRs, approved original creature sprites, engine measurements, private Lantern Quay source art.

## Decision

1. **WILDSIGNAL pivots from the first-person 2.5D presentation to a charming three-quarter/top-down fantasy RPG presentation**, with clear readable tiles, bright cute fantasy, inviting villages, lush forest and mysterious underground places. This is a new presentation/renderer milestone, not permission to delete the prior engine.
2. **Primary/final environment-art foundation:** Anokolisa's **Pixel Crawler — Free Pack**, downloaded from [the official itch.io page](https://anokolisa.itch.io/free-pixel-art-asset-pack-topdown-tileset-rpg-16x16-sprites). The owner explicitly chooses this existing art family as the *production environment-art baseline* instead of commissioning/recreating the entire world asset library.
3. **Tile size:** treat the asset pack's **16×16 pixel logic tile grid** as the source format. Larger structures, roofs and scenery assemble from multiple tiles/sprites. Keep raw pixels and nearest-neighbor output; don't force 128/256 materials from the rejected first-person tests into the new 2D renderer.
4. **Zones:** Use the pack's appropriate forest, housing/interior, cave/underground and dungeon sources to support Lantern Quay, Liltwood and Echo Hollow, subject to the actual included inventory and each area's aesthetic suitability. Official free-pack web page currently describes Forest, Dungeon and *Houses — WIP / Cave — WIP*. Do **not** assume features shown on paid expansion pages are included.
5. **Not replacing original creatures:** user-owned original sprite masters remain canonical and private. Never automatically replace them with pack enemies, AI sprites or edited copies. Creature design and movement compatibility are a separate review.
6. **Main technical stack:** C++20 + SDL3 and low-end Windows/Android requirements remain; plan a separate minimal 2D tilemap prototype, retain old branches unmerged, and defer more elaborate systems until owner reviews actual top-down scene capture.

## License & provenance (based on owner-provided Anokolisa terms)

- The owner supplied the creator's explicit terms: **commercial-project use permitted**, **changes in shape/color/pattern permitted**, **attribution optional**, and **standalone sale/marketing of original or modified assets prohibited absent authorization**.
- Integrating sprites into a paid or free WILDSIGNAL executable as functional game assets differs from republishing them as a marketable artwork pack. A publicly browsable raw sprite atlas repository can be redistributed independently: keep original full-resolution sheets, animated sources, extracted atlas PNGs and private provenance **outside the public game GitHub repo** unless license-specific publication is explicitly reviewed and authorized.
- Preserve a private unmodified ZIP, creator's *actual terms file/PDF*, original itch.io URL, exact downloaded pack filename/version, hashes, and your own modification list. Public code may refer to private asset IDs and import steps, not embed Pixel Crawler sprite bytes.
- The exact **free pack** is the decided source; do not automatically extend this permission to independently sold Pixel Crawler expansions or unrelated creators' packs without checking their own terms.
- No requirement to attribute in game according to the owner-provided terms; optional credit can still be included for goodwill but not imply the author endorses or participates in WILDSIGNAL.

## Art & product constraints

- User wants a cheerful, cute, colorful classic handheld-RPG atmosphere **without copying Pokémon's proprietary assets or identity**.
- Retain "Beautiful but mysterious": surface scenes welcoming, later underground/ruins mysterious, not globally muddy and dark.
- Pixel Crawler sprites can be recolored/modified as needed **only with actual pixel/artifact review**; "pack chosen" does **not** mean every sprite must be used unedited.
- Previous Retro Diffusion, GameLabs, Flow and first-person test textures remain valuable historical experiments; they are *not automatically* part of the new visual production standard.

## Next actual production milestone (NOT EXECUTED)

`TOPDOWN-001`: a tiny playable Lantern Quay top-down fixture using the **real, privately mounted Pixel Crawler free pack**:

- Make a full private asset inventory: version, actual 16×16 grids, source sheets, dimensions, animation strips, collision tiles, alpha and licenses; no fabricated coverage.
- Simple C++ SDL3 camera and grid tilemap, nearest-neighbor integer upscale, stable layer ordering (ground → decals → objects → characters → canopy/foreground), 4-way movement, collision and one follow-behind test companion placeholder.
- One small cottage, grass/dirt, trees, fence, water or other terrain present in the *actual free pack* (substitute only if assets truly included). Prototype dimensions and controls are not canon until user review.
- Real headless/native framebuffer capture and short moving preview from the **actual running top-down executable**, not a mockup. Windows user hardware & Android checks remain NOT_RUN until run.
- Keep private sprites out of Git, measure RAM/frame times, do not change original creature art.
- Draft PR, no merge. Do not remove or rewrite RENDER-002 or other first-person branches.

## Decision boundaries

This records explicit owner project direction and license conditions. It is **not** a representation that we have downloaded/inspected the exact v2.11 ZIP, validated every WIP category, implemented the renderer or passed in-game artistic review. The user may choose a better/more recent free-pack version later; record the version when actual bytes are present.
