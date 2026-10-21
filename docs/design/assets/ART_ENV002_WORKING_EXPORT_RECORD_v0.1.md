# WILDSIGNAL — ART-ENV-002 Working Asset Export Record v0.1

**Owner action:** User said “approve” after ART-ENV-001 reference approval and ART-ENV-002 game-asset next-step discussion. Treat this as authorization to **begin private asset production**, NOT production-art final signoff, public Github upload, or renderer implementation.

**Artifact generated privately on 2026-10-21:** `WILDSIGNAL_ART_ENV002_Working_Assets_v0.1.zip`.
- ZIP SHA-256: `4852493ddc1e61dd47d3aa082a847f2e99edb44243c1374559d40a7ddd98ba7d`.
- Source: previously owner-approved `WILDSIGNAL_Environment_Art_Pack_v0.2_Approved_References.zip` (private ChatGPT artifact, **not** tracked in this public repo).
- Contains 7 candidate 64×64 RGBA repeatable materials: `QUAY_STONE_PAVING_01`, `QUAY_TIMBER_PLASTER_01`, `QUAY_IVORY_PLASTER_01`, `QUAY_RUSSET_ROOF_01`, `FOREST_GRASS_01`, `FOREST_PATH_01`, `FOREST_UNDERGROWTH_01`.
- Contains 6 conservatively prepped props: `QUAY_LANTERN_01`, `QUAY_HARBOR_BANNER_01`, `FOREST_FERN_01`, `FOREST_FERN_02`, `FOREST_TREE_BROAD_02`, `FOREST_SINGING_ROOT_01`.
- `ASSET_MANIFEST.json` gives source/export SHA256, dimensions, stable IDs, source pixel lineage, conservative alpha preservation and **pending** statuses.
- `04_TILE_REPEAT_PREVIEWS` contains 3×3 repeated texture previews; `03_REVIEW_CONTACT_SHEET.png` compares these with padded transparent props.

## Actually verified

- Asset file checksums and manifest dimensions consistent across all 13 images.
- Tile pixel-identity checks: top row equals bottom row and left column equals right column for all 7 PNG materials (no mechanical boundary mismatch; 64×64 RGBA).
- 6 prop canvases include transparent outer borders; existing visible pixels and alpha preserved and copied, not artist-refined.
- ZIP CRC integrity checked successfully.

## NOT verified / known defects

- **Artist-quality seamlessness is NOT proven.** Opposite-edge color parity can still create obviously repeated patterns; several 3×3 previews have noticeable tiling repetition and visual bands.
- **Sprite outline quality is NOT proven.** Earlier dark-background-key cutouts could contain residual matte pixels, tiny detached pieces and lost faint outlines. Human pixel review remains necessary.
- All pivots/world scales and roof/tree/root **three-dimensional geometry** are unassigned. Neither roof nor the Singing Root becomes 3D because a 2D PNG exists.
- Game engine load/render/near/oblique views not attempted. Owner has approved **visual direction**, not these 13 finished assets.
- Public distribution/shipping and commercial art-export approvals remain separate.
- This export was built by a local image-processing script from user-approved references, **not** a Codex program and **not** a scene screenshot.
- No source image bytes from user-owned hand-drawn 100-creature sheet are present in this pack.

## Implementation boundary

The private ZIP and its image bytes are not automatically accessible from GitHub. Codex must receive the actual archive via owner-approved shared access before any future code integration. `AGENTS.md` on the planning branch bans inventing substitute artwork. Next: manually retouch/author actual tile and prop quality, give geometry contracts, provide a small in-engine fidelity test only after that art passes approval.

**Record status:** 13 mechanical image checks passed; **visual art QUALITY PENDING**, **production asset acceptance PENDING**, **engine integration NOT STARTED**, **art held PRIVATE**.
