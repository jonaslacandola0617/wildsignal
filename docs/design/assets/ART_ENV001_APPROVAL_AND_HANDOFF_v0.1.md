# WILDSIGNAL — ART-ENV-001 Approved Visual References & ART-ENV-002 Handoff v0.1

**Owner decision: 2026-10-21.** User reviewed the generated Environment Art Pack and explicitly replied **“yes these are good.”** This approves the **visual style/atmosphere** of the two environment reference scenes: **Lantern Quay** and **Liltwood Forest**. It does NOT approve the individual 27 texture patches / 27 extracted prop candidates as seamless, production-ready or free of halo artifacts, does NOT approve new creature designs, and does NOT authorize public publication of the art.

## Canonical reference asset package

**Private, conversation-attached artifact** (NOT stored in public GitHub): `WILDSIGNAL_Environment_Art_Pack_v0.2_Approved_References.zip`.

- ZIP SHA-256: `c66eb81a689f46ca7de67cf632a65fe646692a63bf9c86f499a1f1b1ea5b57fd` (for verifying exact attached bytes after transfer).
- Source generated environment atlas: `a_wide_detailed_pixel_art_concept_asset_sheet_and.png`, 1536×1024, SHA-256 `9c9690f75aec00792406017bd4a2533611c4d5506334dfbcef17d1dcbe2d0226`.
- Original contents: 62 extracted image records including **27 candidate texture patches, 27 candidate prop cutouts, 5 background crops and 3 scene reference illustrations**.
- The v0.2 archive adds `OWNER_VISUAL_APPROVAL.md` and updates `ASSET_MANIFEST.json`: only `LANTERN_QUAY_CONCEPT_NOT_GAMEPLAY` and `LILTWOOD_FOREST_CONCEPT_NOT_GAMEPLAY` have `ownerVisualApproved=true`. All production sprite/texture QA is pending. There are **no newly redrawn or pixel-altered images** in this administrative version.
- An AI-generated **environment art** source atlas is not the same as the **owner's original hand-drawn 100-creature sheet**; the latter is not embedded, altered or released here.
- No environment or creature image files are committed to public GitHub without explicit separate distribution permission. The project is intended for commercial release, so check any third-party assets, fonts, and licenses in subsequent art authoring.

### Reference file IDs and visual signoff

| Asset ID in archive | Purpose | Owner visual style status | Shipping/game integration status |
|---|---|---|---|
| `LANTERN_QUAY_CONCEPT_NOT_GAMEPLAY` | Handcrafted-looking harbor town reference (scene illustration) | **APPROVED 2026-10-21** | Illustration only; NOT a first-person game scene |
| `LILTWOOD_FOREST_CONCEPT_NOT_GAMEPLAY` | Lush enchanted woodland/Singing Root reference | **APPROVED 2026-10-21** | Illustration only; NOT a game scene |
| Echo Hollow scene concept | Bonus appearance reference | No explicit new final signoff | Concept, not game art |
| Each extracted material/prop/backdrop crop | Candidate production asset | NOT INDIVIDUALLY APPROVED | UNFINISHED; cleanup, tiling/alpha/geometry testing required |

## What Codex and art production must do NEXT

**ART-ENV-002 (first actionable deliverable):** create a *small* real, reusable game-art subset from the approved generated environmental reference and crop candidates:
- **Lantern Quay:** seamless paving stone; timber/plaster wall material; roof-shingle source with proper side/roof volume; one window/door trim; one lantern on transparent RGBA; one skyline/horizon element. Do not require a city block yet.
- **Liltwood:** seamless dirt/path/grass material; 2 distinct bark/trunk treatments; 2 foliage/fern alpha sprites and a root/stone special silhouette at the Singing Root; one mid/far canopy background layer. Realize layered spatial depth and believable irregular plant forms, not uniformly spaced barrel columns.
- **Materials:** choose documented source/pixel scale (e.g. 64×64 for tiling *candidates*, subject to genuine art review); prove seam repeat both axes on a 3×3 tiled preview. Fix isolated crop seams manually as needed, not by stretching one illustration into a tile.
- **Transparent props:** hand-correct source dark matte cutouts, check on light/dark/bright backgrounds, restore outlines and holes, preserve detached leaves and light halos where intentional. A threshold-generated alpha PNG is not a completed sprite.
- **World presentation:** provide stable material/prop IDs and expected transform/scale, including which are genuine geometry, billboards, parallax panoramas and how roof forms should appear when the camera turns. Avoid hardcoded procedural substitute drawings.
- **Visual checks:** use the original two *approved reference illustrations* for aesthetic comparison plus a **separate game-engine screenshot** at corresponding approximate composition. Screenshots and references must always be labeled correctly.
- **Testing:** check exact SHA256, size, RGBA mode, tileability and alpha quality, expected pivot/world dimensions; add small parser/load/export tests where code integration is authorized. Keep performance 320×180 and 480×270 / 1 and 24 actors bounded on owner Win11 i3/4GB.
- **Art delivery:** new original environment PNG files in a PRIVATE revisioned art-workspace directory; manifest `assets-production-v0.1.json`; 3×3 tile previews; side-by-side source→export contact sheet; in-engine geometry feasibility memo.
- **Gate:** OWNER reviews the **actual production assets** and later the moving in-engine screenshots before any scene art/renderer ADR is marked accepted. Generated scene illustrations alone are NOT the required engine proof.

## Binding Codex rules

The canonical root `AGENTS.md` added to planning branch on this decision is a safeguard only **when included in the actual task branch**. Codex must fetch or merge it before art integration. The art archive is **NOT** automatically available to Codex from a ChatGPT sandbox link: user must supply/upload the ZIP or an appropriate owner-controlled private location accessible to that execution. **If Codex lacks actual image files or technical QA approval, it must stop rather than invent a replacement.**

## Outside approval

No authorization to merge PR #4/#6, publish private source artwork, change original creature designs, claim Windows 7/Android compatibility, implement the game loop/battle/networking, or declare the visual style achieved in a playable build. Existing RENDER-001 procedural artwork remains an engineering placeholder; no retroactive creative approval.

**Status:** visual targets approved; individual game-ready assets and visual fidelity proof **not yet finished**.
