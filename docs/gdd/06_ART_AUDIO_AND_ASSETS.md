# Chapter 6 — Art, audio and assets

**Status:** LOCKED 2026-11-02 by the owner (A1–A7). The **style guide** stays OPEN until look development (LOOKDEV-001) picks a direction.
**Built on:** chapters 1–4 (locked) and chapter 5 (functional parts locked; UI look OPEN).

**Two owner rules shape this chapter (2026-11-02):**
1. **We make our own assets.** None of the current 3D assets is approved; they're all experiments. Buildings, vegetation, props and the rest of the world are built for Wildsignal, so we decide exactly how it looks.
2. **Zero budget.** No paid packs, no commissions. Everything is made by Claude (scripted Blender models, code-drawn UI and effects, synthesised audio) or comes from free sources whose licence allows commercial use.

## 6.1 Art direction — decided through look development

The final look isn't chosen yet. The rendering features in the pilot are kept as a **starting point**, not a commitment: raised camera, low-resolution render with crisp pixels (F6), three-tone lighting, outlines, shadows, swaying foliage, selective focus (F8). The creatures' soft, rounded, glossy style (WS-001 to WS-020) is approved and the world must suit it.

**Look development (LOOKDEV-001), the first art task:** Claude builds one small test scene in the real game, a cottage, a tree, a bush, rocks, a fence, a path and grass, in **three style directions**, all made from scratch:

| Direction | Idea | Feels like |
|---|---|---|
| **A — Pixel-textured low-poly** | Simple, chunky shapes with small pixel-art textures and visible texels (like WS_HOUSE_001) | Classic, retro, handmade |
| **B — Soft toy** | Rounded, bevelled, slightly bulgy shapes in flat colours with gentle gradients, matching the glossy Signari | Cozy, modern, toy-like |
| **C — Painted gradient** | Clean low-poly shapes coloured with soft hand-painted gradients and vertex colour, no textures | Calm, storybook, very light on memory |

The owner compares fixed screenshots and a moving preview of all three and picks one (or a blend). The result becomes the **style guide** in this chapter: shape language, proportions, texel density or colour rules, palette per region, outline and lighting settings. Only then is the chapter fully locked.

**Palette by mood** (applies to any direction): golden greens in Hearthgrove; jade and cyan glow in Liltwood; turquoise and sand on the coast; rust and ember at Ashbell; violet mist and lantern gold in Gloamfen; black rock, magma and crystal blue in the Caldera.

## 6.2 How we make things

| Category | Method |
|---|---|
| Signari | Scripted Blender pipeline (chapter 4) |
| Buildings, vegetation, rocks, props | **Scripted Blender pipeline, same approach as the Signari:** one script per asset family, built from shared shape and paint helpers, so every model follows the style guide and can be rebuilt or restyled in one pass |
| Wayfinder and people | Built by Claude in the same pipeline, matching the chosen style (KayKit stays a temporary stand-in until then) |
| Terrain and ground | Generated in code and Blender: height maps, painted ground types, paths |
| Effects (VFX) | Code- and particle-driven in the engine |
| UI | Code-drawn by the engine (chapter 5); pixel-art UI sprites drawn by script if the owner wants a textured look later |
| Icons | Drawn by script on a pixel grid |
| Audio | Synthesised by Claude, plus free libraries (6.6) |

Every scripted asset keeps its source script in the private art repo, so changing the style later means changing helpers and rebuilding, not redrawing by hand.

## 6.3 Current assets — experimental, not approved

| Source | Where it's used now | Licence | Role from now on |
|---|---|---|---|
| Quaternius — Medieval Village MegaKit (free) | Village buildings, props | CC0 | Placeholder until our own replaces it |
| EmaceArt — Free Fantasy Medieval Houses and Props / Slavic World Free | Houses, trees, rocks, grass, terrain | Free commercial use; no resale or redistribution | Placeholder |
| KayKit — Adventurers 2.0 (free) | Wayfinder | CC0 | Placeholder |
| Anokolisa — Pixel Crawler Free Pack 2.11 | WS_HOUSE_001 texture | Commercial use and edits allowed; no resale | Reference for direction A |
| Humble Pixel — Paper UI System | UI | Commercial use and edits allowed; no redistribution | Placeholder (chapter 5) |
| Daniel Linssen — m5x7, m6x11 | UI fonts | m5x7 CC0; m6x11 needs credit, commercial terms unconfirmed | Keep m5x7; confirm or replace m6x11 |
| Owner's own — creature designs, WS-001–020 models, WS_HOUSE_001, Worldline packs | Signari, house, props | Owner's own | Kept |
| `medieval_house_pack_pixel_art.glb`, `cemetry-blend.zip` | — | No licence on record | ⛔ do not use |
| Pokémon models in `assets/samples` | Measurement only | Not licensed | ⛔ never in the game |

**Placeholder rule:** any third-party file still in the game at release must have a recorded licence that allows it; the goal is that none remain except fonts and free audio.

## 6.4 What 1.0 needs — the asset list

| Category | What | Amount | Have now (approved) |
|---|---|---|---|
| **Signari** | Models, 6 shared skeletons, 12 animations each, lower-detail versions | 50 | 20 models |
| **Wayfinder** | Player model, 4–6 looks, animations (walk, run, sneak, roll, hurt, Tune In, compass pulse, interact, cheer) | 1 | — |
| **Named characters** | Mara, Old Fen, Iri, Juno, Sol | 5 | — |
| **Other people** | Chorus members (2 looks), villagers (8–10), shopkeepers | ~14 | — |
| **Region kits** | Ground, paths, trees and plants, rocks, buildings, props, water — per region | 6 regions | — |
| **Stations and ruins** | Echo Hollow, drowned station, forge, marsh station, Deep Station, plus small ruins | 6 sets | — |
| **Homestead** | Plot ground, farm plots, 6–8 crops with growth stages, barn, greenhouse, pond, kiln, fences, paths, ~30 decorations | ~50 pieces | — |
| **Item icons** | Healing items, materials, tools, key items, seeds | ~60 | — |
| **Type and skill icons** | 8 type icons, skill icon shapes per role | ~40 | — |
| **VFX** | Hits per type, skill effects, ultimates, Tune In ring, compass pulse, rain, mist, fireflies, signal glows | ~80 | — |
| **UI** | Panels, buttons, bars, frames | 1 set | — (code-drawn interim) |
| **Music** | Title; Hearthgrove day and night; one theme per region; homestead; wild fight; guardian fight; rival; Chorus; finale; credits | ~14 tracks | — |
| **Sound effects** | Footsteps per surface, UI, compass, Tune In, per-type attacks, hits, faint, ambience per region, weather | ~150 | — |
| **Signari cries** | One per species | 50 | — |

## 6.5 Region kits

Each region gets its own kit, built by script in the chosen style: a ground set, 3–5 tree and plant families with variations, rocks, 4–8 buildings, 15–30 props, water if needed, and its station. Kits share helpers so the whole game stays consistent. Region 1 (Hearthgrove) comes first, straight after look development, and replaces the current placeholder village.

## 6.6 Audio — free only

| Audio | Approach |
|---|---|
| **Music** | **Gathered and generated by the owner (decided 2026-11-02):** free royalty-free music found online, and instrumental tracks generated with AI music tools such as Google's. Small, warm instrumentation suits the cozy mood: soft piano, plucked strings, flute, light percussion. Before any track goes in, its terms are recorded and must allow commercial use in a game; AI tools' terms vary and can change, so the terms at the time of generation are saved with the track. |
| **Sound effects** | The easiest part (owner): free royalty-free and **CC0** libraries (for example Kenney's CC0 audio and other public-domain collections), edited to fit, plus sounds Claude can synthesise (UI clicks, compass pulse, Tune In tones, magic and signal effects). |
| **Signari cries** | Synthesised and processed per species (pitch, formants, layered textures), so all 50 are unique and owned. |

Only licences that allow commercial use are accepted, and every audio file goes into the same manifest as art.

## 6.7 Production rules

- One private manifest lists every asset: ID, file, size, triangle count, texture size, source script or source, licence, version, review status.
- Review always happens in the real game: fixed screenshots **and** a moving preview. Passing tests isn't art approval.
- Budgets: Signari as in chapter 4; buildings up to ~5,000 triangles; props up to ~1,500; textures up to 512 × 512 (1024 for large terrain); nearest-neighbour sampling if the chosen style uses textures.
- Region memory budgets are set in chapter 7 and checked on the i3 / 4 GB PC.

## 6.8 The UI look (open from chapter 5)

With no budget for UI packs, the choice is between:
1. **The clean code-drawn style** (already the interim plan), refined over time.
2. **Pixel-art UI sprites drawn by script** to match the chosen world style (panels, buttons, frames, icons), once LOOKDEV-001 has picked the world style, so UI and world match.

Recommendation: decide after look development; whichever world style wins, the UI follows it.

## 6.9 Owner choices

| # | Question | Decision (owner, 2026-11-02) |
|---|---|---|
| A1 | Build all our own assets with a zero budget; current packs are placeholders only (6.2–6.3)? | Yes |
| A2 | Look development first: three style directions in one test scene, owner picks (6.1)? | Yes |
| A3 | The three directions to test: pixel-textured low-poly, soft toy, painted gradient (6.1)? | Yes |
| A4 | Music | **DECIDED:** free royalty-free music plus AI-generated instrumentals, terms recorded per track |
| A5 | Sound effects from CC0 libraries plus synthesis; cries synthesised (6.6)? | Yes |
| A6 | UI look decided after look development, following the world style (6.8)? | Yes |
| A7 | The licence rule and the do-not-use list (6.3)? | Yes |

## Change log

- 2026-11-02 — First draft.
- 2026-11-02 — Owner: no current 3D asset is approved, we build our own (especially buildings and vegetation), and there is no budget. Reworked around own assets, look development and free audio.
- 2026-11-02 — Owner accepted A1–A7; music from free royalty-free sources and AI instrumentals. Locked; style guide pending LOOKDEV-001.
