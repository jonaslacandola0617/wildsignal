# Chapter 6 — Art, audio and assets

**Status:** DRAFT 2026-11-02. Drafted by Claude from chapters 1–5 and an inventory of `D:\wildsignal\assets`, the review builds and `wildsignal-art`. Choices are in 6.8.
**Built on:** chapters 1–4 (locked) and chapter 5 (functional parts locked; UI look OPEN).

## 6.1 Art direction — the look we already have

The pilot village already has a look players respond to; 1.0 keeps it and applies it everywhere:

- **Raised 3D camera** over a small, dense world; characters read clearly against the ground.
- **Low-resolution render, crisp pixels:** the world renders at 480, 640 or 1280 wide (F6) and is scaled up with whole, sharp pixels.
- **Stylised lighting:** three calm light tones per surface, soft sun-edge highlights (no leaf rims), clean outlines, real shadows, optional sun shafts.
- **Living foliage:** leaf cards that sway in the wind (with the jitter fix), dense meadow grass.
- **Selective focus (F8):** the tilt-shift band through the player, off in fights.
- **Creatures:** soft, rounded, glossy, big readable eyes (the approved WS-001 to WS-020 style).
- **Palette by mood:** golden greens in Hearthgrove; jade and cyan glow in Liltwood; turquoise and sand on the coast; rust and ember at Ashbell; violet mist and lantern gold in Gloamfen; black rock, magma and crystal blue in the Caldera.

**Consistency rule:** models from different packs must pass through the same lighting, palette and outline settings so they look like one world. Any new pack is tested in the real game next to existing models before it's approved.

## 6.2 Asset sources and licences

Every asset in the game needs a record: source, licence, download date, file hash and any changes. Files bought or downloaded stay in the private art repo; the public engine repo holds code only.

| Source | Used for | Licence | Status |
|---|---|---|---|
| Quaternius — Medieval Village MegaKit (free) | Village buildings, props | CC0 | ✅ in use |
| KayKit — Adventurers 2.0 (free) | Wayfinder base character | CC0 | ✅ in use |
| EmaceArt — Free Fantasy Medieval Houses and Props / Slavic World Free | Houses, trees, rocks, grass, terrain | Free commercial use; no resale or redistribution of the files | ✅ in use |
| Anokolisa — Pixel Crawler Free Pack 2.11 | WS_HOUSE_001 texture atlas | Commercial use and edits allowed; no resale | ✅ in use |
| Humble Pixel — Paper UI System | Placeholder UI | Commercial use and edits allowed; no redistribution | ⚠️ placeholder (chapter 5) |
| Daniel Linssen — m5x7, m6x11 fonts | UI text | m5x7 CC0; m6x11 needs credit, commercial terms unconfirmed | ⚠️ confirm before release |
| Owner-generated — Worldline packs, WS_HOUSE_001 model, creature designs | Props, house, Signari | Owner's own | ✅ |
| `medieval_house_pack_pixel_art.glb`, `cemetry-blend.zip` | — | **No licence on record** | ⛔ do not use until the source and licence are found |
| Pokémon models in `assets/samples` | Measurement and proportion study only | Third-party, not licensed for use | ⛔ never in the game |

## 6.3 What 1.0 needs — the asset list

| Category | What | Amount | Have now | Made by |
|---|---|---|---|---|
| **Signari** | Models, 6 shared skeletons, 12 animations each, lower-detail versions | 50 | 20 models, 1 rigged | Claude (chapter 4 pipeline) |
| **Wayfinder** | Player model, 4–6 looks, animations (walk, run, sneak, roll, hurt, Tune In, compass pulse, interact, cheer) | 1 | Test model from KayKit | Claude, from the KayKit CC0 base |
| **Named characters** | Mara, Old Fen, Iri, Juno, Sol | 5 | — | Claude, from KayKit base + accessories |
| **Other people** | Chorus members (2 looks), villagers (8–10 variants), shopkeepers | ~14 | — | Claude, kitbashed from KayKit |
| **Region kits** | Ground, paths, trees and plants, rocks, buildings, props, water per region | 6 regions | Region 1 mostly done | Packs + Claude assembly; new kits per region (6.4) |
| **Stations and ruins** | Resonance station ruins, Echo Hollow, drowned station, forge, marsh station, Deep Station | 6 sets | — | Claude, built to match the region kits |
| **Homestead** | Plot ground, farm plots, 6–8 crops with growth stages, barn, greenhouse, pond, kiln, fences, paths, ~30 decorations | ~50 pieces | WS_HOUSE_001 | Claude + packs |
| **Items** | Icons for healing items, materials, tools, key items, seeds | ~60 icons | — | Code-drawn icons first; final style with the UI look |
| **Type and skill icons** | 8 type icons, skill icon shapes per role | ~40 | — | Same |
| **VFX** | Hits per type; skill effects (a shared library per type); ultimates; Tune In ring; compass pulse; rain, mist, fireflies; signal glows | ~80 effects | Ground marks, smoke, sun shafts | Claude, code- and particle-driven |
| **UI** | Panels, buttons, bars, frames | 1 set | Placeholder | **OPEN** (chapter 5) |
| **Music** | Title; Hearthgrove day and night; one theme per region (5); homestead; wild fight; guardian fight; rival; Chorus; finale; credits | ~14 tracks | — | See 6.5 |
| **Sound effects** | Footsteps per surface, UI, compass, Tune In, per-type attacks, hits, faint, ambient loops per region, weather | ~150 | — | See 6.5 |
| **Signari cries** | One short cry per species | 50 | — | See 6.5 |

## 6.4 Region kits — where each region's look comes from

| Region | Base | Still needed |
|---|---|---|
| Hearthgrove Vale | EmaceArt + Quaternius village, WS_HOUSE_001 | Harbour and docks (Worldline harbour props are a start), Wayfinder Hall, Compass Workshop |
| Liltwood Forest | EmaceArt trees and plants | Giant roots, glowing mushrooms, Echo Hollow ruins |
| Tideglass Coast | — | Sand, tide pools, cliffs, sea caves, boats, fishing town, the drowned station |
| Ashbell Works | — | Red rock, forge buildings, chains and bells, chimneys |
| Gloamfen Marsh | EmaceArt water and reeds | Boardwalks, lanterns, marsh houses, mist, the marsh station |
| Emberglass Caldera | — | Volcanic rock, magma, crystals, the Deep Station |

For regions 3–6: Claude first searches for **free or affordable, commercially licensed** packs that pass the consistency rule (6.1) and lists them for the owner, then builds what's missing in Blender. Nothing is used until its licence is recorded.

## 6.5 Audio

Audio hasn't been planned before, and the game has none yet. It matters a lot for "cozy" and "mystery".

**CHOICE A4 — where music comes from:**

| Option | What | Cost |
|---|---|---|
| A | Royalty-free music libraries and packs (some free, some paid), picked to fit | Low to medium; less unique |
| **B (recommended)** | **Commission a composer** for the core themes (title, Hearthgrove, fights, finale), libraries for the rest | Medium; gives Wildsignal its own identity |
| C | Commission everything | Highest |

**Sound effects:** recommended from **CC0 and royalty-free libraries** (for example Kenney's CC0 audio, free game-audio bundles), edited to fit, plus sounds Claude can synthesise (UI clicks, compass pulse, Tune In tones).
**Signari cries:** recommended **synthesised and processed** sounds (pitched, layered voice and instrument samples) designed per species, so all 50 are unique and owned.

## 6.6 Production rules

- One private manifest lists every asset: ID, file, size, triangle count, texture size, source, licence, hash, version, review status.
- Review always happens in the real game: fixed screenshots **and** a moving preview. Passing tests isn't art approval.
- Budgets: Signari as in chapter 4; buildings up to ~5,000 triangles; props up to ~1,500; textures up to 512 × 512 (1024 for large terrain); nearest-neighbour sampling.
- Region memory budgets are set in chapter 7 and checked on the i3 / 4 GB PC.

## 6.7 The UI look (open from chapter 5)

Until the owner chooses, the game uses a **clean code-drawn UI** (simple rounded panels, soft colours, crisp text, simple icons). Options to settle later, in any order:
1. Claude shortlists 3–5 paid UI packs (typically $5–30) that fit a cozy adventure, with licences checked, and the owner picks by eye.
2. Commission the core UI pieces from an artist once the screen list (chapter 5) is final.
3. Keep refining the code-drawn style if it grows on the owner.

## 6.8 Owner choices

| # | Question | Recommendation |
|---|---|---|
| A1 | Keep the current look as the art direction (6.1)? | Yes |
| A2 | The licence rule and the "do not use" list (6.2)? | Yes |
| A3 | People (Wayfinder, NPCs, villagers) built from the KayKit CC0 base (6.3)? | Yes |
| A4 | Music: libraries, a composer for core themes, or full commission (6.5)? | Composer for core themes + libraries |
| A5 | Sound effects from CC0/royalty-free libraries plus synthesis; cries synthesised (6.5)? | Yes |
| A6 | For regions 3–6, Claude searches for fitting packs first, then builds the rest (6.4)? | Yes |
| A7 | UI look: shortlist paid packs now, or decide later (6.7)? | Shortlist now, decide when you see them |

## Change log

- 2026-11-02 — First draft.
