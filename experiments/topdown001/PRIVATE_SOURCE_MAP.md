# TOPDOWN-001 — Private Pixel Crawler source mapping (public metadata only)

**Source ZIP:** `Pixel Crawler - Free Pack 2.11.zip`  
**Immutable ZIP SHA-256:** `b7b228ca232da9958f191b01db87764dddb67610116151e06e8dd8ade6de94b8`  
**Private GitHub source:** `jonaslacandola0617/wildsignal-art`, branch `art/pixelcrawler-freepack-2.11-intake-v0.1`, at `third-party/pixel-crawler/free-pack-2.11/Pixel Crawler - Free Pack 2.11.zip`.  
**No artwork bytes included in public game repository.**

Local proof source expects a private directory containing these **unmodified copied/renamed ZIP entries**:

| Local basename | Original ZIP member under `Pixel Crawler - Free Pack/` | Size (px) | SHA-256 |
|---|---|---:|---|
| floors.png | `Environment/Tilesets/Floors_Tiles.png` | 400×416 | `fa4f849cbf835f5d3c558a73a084b1200e57571b13beef402f6977516de33528` |
| walls.png | `Environment/Structures/Buildings/Walls.png` | 672×800 | `8be7e349c23548104b2bf7cf7b4194779795832c857dc010110a7468e5c843c7` |
| roofs.png | `Environment/Structures/Buildings/Roofs.png` | 400×400 | `c164bc96ed8988dd9d8e3c4dae1e1d7838cc0f27abdcddd38890c548065d06af` |
| trees.png | `Environment/Props/Static/Trees/Model_01/Size_02.png` | 128×96 | `286fc573beb3ab53c63e90cd62e13019c14975c38cb948b6178e1790c1827363` |
| plants.png | `Environment/Props/Static/Vegetation.png` | 400×432 | `d9ae59ea28a6a8b22300b140a9453776f88456270368f68736a396edf74b3797` |
| farms.png | `Environment/Props/Static/Farm.png` | 400×400 | `2f22c3e4aea21630977262bbba80bcf218d5c0c739123fefc33a08ab7396539d` |
| character.png | `Entities/Characters/Body_A/Animations/Idle_Base/Idle_Down-Sheet.png` | 256×64 | `4c71421bfa775e670925111b7cf7f0cf3949c2be100eb8a1d753b5a8071f6ea6` |

**Important:** Artist sprites in the source ZIP are often atlases bigger than one 16×16 tile. The proof's arbitrary (u,v) atlas coordinates can draw visually incoherent arrangements. The SDL3 prototype must build a documented atlas-cell catalogue by direct visual inspection of the real sheets. Never claim these source slices are semantic IDs before verifying.

For 4-direction character movement, separately inspect the actual `Idle_Base/Idle_{Up,Down,Side}-Sheet.png` and `Walk_Base/Walk_{Up,Down,Side}-Sheet.png` in the same private archive. Verify their frame sizes and directions instead of guessing. A companion demonstration must use a clearly designated temporary placeholder, not modify/replace user-owned original creatures.

**License:** Anokolisa's `Terms.txt` is inside the private ZIP. Commercial game use and edits permitted; no resale/distribution as a standalone art product. Public code repositories and CI logs/artifacts must contain only non-art code/metadata or synthetic pixels; avoid posting actual sprites/captures publicly.
