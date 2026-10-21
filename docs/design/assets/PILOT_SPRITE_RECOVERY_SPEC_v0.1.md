# WILDSIGNAL — Four-Pilot Sprite Reconstruction & Animation Contract v0.1

**Status:** PLANNING ONLY. Creator artwork and source-pixel appearance inspected on 2026-10-20. No creature sprite has been cropped, cleaned, animated, repainted or committed in this milestone.
**Source manifest:** `docs/design/assets/ORIGINAL_SPRITE_SHEET_MANIFEST_v0.1.json` contains all 100 source-cell records plus reproducible candidate rectangles, SHA256 and pilot list.
**Original owner:** User personally drew the 100 creatures; it is NOT AI-generated. Historical editable Aseprite file was on a broken desktop and is unavailable; reconstructed editable masters can be created from the surviving flattened PNG without asking user to redraw.
**Privacy:** Original sprite-sheet bytes remain in the user's Library; neither source PNG nor sprite export may be published into a public GitHub repository without owner's explicit consent.

## 1. Verified source inspection and coordinate convention

- Original file `image(20261020-050620).png` is **850×832 RGB PNG without alpha**; SHA-256 `d49b383f1df8a2d42823c9e184b734df0c4d026a1a9d60448e6932bfa64c3fbe`; visually 10 rows and 10 columns, lime/yellow-green backgrounds separated by thin near-white grid lines.
- White gridlines fall near **x ≈ [4, 88, 172, 256, 340, 424, 508, 592, 676, 760, 844]**, **y ≈ [4, 86, 168, 250, 332, 414, 496, 578, 660, 742, 824]**. Coordinates are approximate separator centerlines; trim boxes deliberately exclude separators.
- Candidate cell crop for 0-based `col=c`, `row=r`: `x=[6+84c, 88+84c)`, `y=[6+82r,86+82r)`, i.e. initial **82×80** pixels. These were measured from source separator patterns, **not** guaranteed creature opaque bounds. A sprite touching crop edges triggers manual adjustment and review.
- Source background is not a single exact hex color: many slightly different lime shades and near-white separator pixels appear. Therefore pure RGB equality and global flood-fill/keying are unsafe; thin highlights and disconnected accessories may be lost.
- The source seems to include separate dark grounding shadow marks under some creatures; do **not** automatically classify these as body pixels. Artist must decide whether they are world-contact shadows, part of original design, or purely sheet decoration.
- **No final world scale / atlas dimensions have been approved.** First normalize silhouettes visually and preserve genuine shape differences; a 40-pixel tall sprite and a 60-pixel tall sprite should not be forced into identical distorted dimensions.

## 2. Four pilot records (original visual observation; species identity remains proposed)

| Stable sprite ID | Row/column | Candidate crop (left,top,right,bottom-exclusive) | Original visible appearance | Primary cleanup risk |
|---|---|---|---|---|
| `WS-004` | R01C04 | `[258,6,340,86)` | small upright green/yellow/gold insect/woodland creature, pointy appendages | tips of antennae/horns, tiny feet |
| `WS-008` | R01C08 | `[594,6,676,86)` | bright blue round-bodied waterlike creature with floating blue bubbles above | detached bubbles vs background |
| `WS-050` | R05C10 | `[762,334,844,414)` | orange-red compact glowing creature with detached ember specks above | stray sparks and light highlights |
| `WS-024` | R03C04 | `[258,170,340,250)` | chunky brown-red rocky creature with irregular face/claws | jagged stone edge and ground contact |

These are descriptions of the *existing* drawings, not authorization to modify their anatomy or final naming. Glintle/Driftfin/Brumlet/Craglet remain working names.

## 3. Future alpha reconstruction: manual QA workflow

1. Read surviving PNG bytes; hash-check against manifest to avoid silently using altered/resized screenshot.
2. Preserve original file separately and immutable. Create staging crop **copies** from manifest bounding boxes; no resampling, sharpening or palette reduction. Maintain source-pixel coordinate mapping.
3. Inspect crop at 1× and nearest-neighbor 8× on checkerboard and on actual Quay/Fern/Echo backgrounds. Identify visible body, disconnected sparks/bubbles, cast-shadow marks, and line remnants.
4. Segment background as a **proposed mask**, with seed colors/contiguous flood fill + edge conservatism. Never blindly mark all light-green colors transparent. Save **candidate alpha mask as separate editable layer** so artist can restore accidentally removed outlines/speckles.
5. Keep intentional detached parts—bubbles, embers, antennae—within an expanded canvas or separate frame FX layers when appropriate. Do not delete because they fail to touch main body.
6. Scan 4 edge directions and corners for gridline remnants, one-pixel lime halos, white stripes, and torn creature outlines. Identify soft RGB spill created by existing flattened antialiasing/compression, and explicitly decide artist-reviewed cleanup.
7. Separate world-contact/shadow as optional **shadow layer** if this improves visual depth. Moving sprite body alone must not drag its shadow around unnatural locations.
8. Author **reconstructed source master** with explicit `source`, `outline_restore`, `body`, `detached_fx`, `contact_shadow`, `animation` layers if used; clearly state **these are new reconstructed files, not the lost original Aseprite masters**.
9. Export lossless transparent `WS-###_front_idle_00.png` with 1:1 pixel fidelity, atlas guard pixels and metadata. Keep a QA sheet showing side-by-side *original cell* vs alpha result on 4 contrasting backgrounds; produce checksum/diff log for all edited pixels.
10. Artist/owner accepts original silhouette, palette, face, detached objects and intentional shading at native scale **before** adding sides/back/animations or uploading deliverables to public repo.

**Source preservation metrics (planning QA, not test results):** no pixels outside body/FX remain bright lime or white grid except where original sprite intentionally includes those tones; no missing antenna/bubble/spark; source bitmap remains byte-for-byte unchanged; crop remaps to correct WS-ID; every manual retouch has metadata. Pixel-by-pixel equal RGB cannot be required on antialiased edge cleanup, so use an annotated visual diff.

## 4. Future minimal animation budget and collider contracts

No fixed animation resolution until alpha pilot accepted. Proposed first-slice sprite action set per 4 pilots:
- **Static:** `front_idle_00` as immediate basic billboard in world/battle.
- **Minimal motion:** front idle 2–3 frames; walk/follow 2–4 frames for *starter* overworld; non-player wildlife may initially remain on one sprite plus safe bob/flip if approved.
- **Battle:** neutral, anticipation, attack, hurt, faint (some may reuse one source pose with small offset effect in first pass, pending art approval).
- **Directional views:** front, back, left, right are aspirational for visible co-op; initial first-person companion can use billboard-facing prototype, provided fake rotations do not flip horns/face in misleading ways. Do not pretend a single front sprite is true 360° art.
- **Special FX:** detached blue water bubbles on WS-008, ember dots on WS-050 can be authored separate alpha cards; should not add high overdraw or battery drain.
- **Foot/pivot:** ground walkers anchor near central contact point; hovering units have body-center plus authored world hover offset. Each creature's `pivotPx` and opaque box must be measured from finished alpha art. Collision uses authored *physics profile*, not opaque texture pixels.
- **Battle particles:** each move references typed move ID (e.g., `SPROUT_DART`), never character bitmap-specific damage logic.

Proposed export metadata per species:
```json
{
  "speciesId": "WS-004",
  "sourceAssetId": "SRC_CREATURE_SHEET_001",
  "sourceCell": "R01C04",
  "sourceSha256": "d49b383f1df8a2d42823c9e184b734df0c4d026a1a9d60448e6932bfa64c3fbe",
  "reconstructedMaster": null,
  "opaqueBoxPx": null,
  "pivotPx": null,
  "worldScale": null,
  "hasAuthoredBack": false,
  "hasAuthoredSide": false,
  "animations": [],
  "qualityStatus": "NOT_CREATED",
  "publicDistributionApproved": false
}
```
Actual pilot assets/frames may be stored in owner-approved private art storage and only reviewed game exports committed later with explicit consent.

## 5. Character/art consistency tests (future, not performed)

- **SIL-001:** 4 creatures match original visible silhouettes on dark stone, warm harbor, bright forest and black backgrounds. No lost detached bubble/spark.
- **SIL-002:** NPC/creature apparent size ratios from Character Bible remain visually credible without resampling corruption.
- **SIL-003:** Contact ground/hover pivot consistent in first-person forest at close, medium and far range; no sprite goes behind floor/wall incorrectly.
- **SIL-004:** First battle portrait shows the original owner's art with non-blurry nearest scaling and a clear boundary around alpha.
- **SIL-005:** 1 / 8 / 24 visible animated sprites use bounded atlas memory on actual S10+ and i3/4GB; no untested 2GB compatibility claims.
- **SIL-006:** Animations preserve recognizable identifying features of WS-004/008/050/024, and no scene requires unavailable editable Aseprite historical file.
- **SIL-007:** Source PNG/public-repo privacy approvals honored and asset provenance displayed in maintainers' ledger.

## 6. Owner review checkpoints

**Needed before editing original creature pixels in a subsequent authorized step:** confirm whether cleaned/derived art can be committed to public GitHub, or should remain private; sign off starter mappings/working names; decide if original contact shadows are kept as ground FX. No new source master required to proceed.

**No art files modified as part of this document.**


## 7. Owner approved four candidate recovery previews (2026-10-21)

The owner explicitly approved the four pilot recovery **candidates** after reviewing the recovery presentation: WS-004, WS-008, WS-050 and WS-024. See `FOUR_PILOT_APPROVAL_v0.1.md`. This permits continuation of private sprite edge cleanup and reconstructed editable masters, while preserving original designs. Alpha/ground-shadow manual QA remains pending; final production ready and public sharing have NOT been approved. ENGINE-001 implementation remains separately gated.
