# WILDSIGNAL — Visual & Environmental Art Bible v0.1

**Status:** Proposed art-direction specification; visual mood is approved, numerical budgets and individual asset decisions are *pending validation*.  
**Audience:** Art production, level design, UI, engine/rendering, Codex. **Date:** 2026-10-20.
**North star:** "Beautiful but mysterious" — cute hand-pixeled creatures inhabit lush biomes and forgotten places; gameplay visually evokes first-person 1990s/retro 2.5D without relying on high-end 3D.

## A. Confirmed reference hierarchy
1. **PRIMARY ENVIRONMENT REFERENCE**: user's first-person retro dungeon screenshot, 2026-10-20: chunky textured brick, large arches and pillars, ceiling structures, luminous green ground, hanging vines, lantern focal points, bright playful 2D sprites and bold HUD. This is an aesthetic reference, not automatically a licensed asset.
2. **PRIMARY CREATURE SOURCE**: user's original, hand-drawn 10×10 pixel-art sheet (personally owned; owner approves use). Preserve the exact source species designs for characters the owner selects. It is not AI-generated.
3. **SECONDARY MOTIFS**: vibrant fantasy villages, enchanted forests, magical ruins, light mystery; avoid uniformly grim dungeons and avoid copying franchise UI/capture devices.
4. **TONE**: readable, welcoming, whimsical, intriguing; darker or threatening scenes are local exceptions.

### Brand tokens (WORKING)
Working title **WILDSIGNAL** (name clearance pending); Wayfinders (player role), Resonance Compass (world tool), Lantern Quay (start), Liltwood Forest, Echo Hollow.

### Mood balance
- Settlements: 70% warmth / 20% wonder / 10% unease.
- Forests: 50% lush wonder / 35% mystery / 15% danger.
- Ruins: 20% wonder / 50% mystery / 30% danger.
- Late regions: challenging and strange but not graphic horror.

## B. Design-language fundamentals

### Shape language
- **Creatures:** rounded masses, iconic ears/horns/leaves/fin silhouettes; a recognizable facial focal point. Preserve owned sprites and only design new animation frames consistent with them.
- **Nature:** asymmetry, curled roots, fronds, layered crowns, mushroom clusters; broad silhouettes read before microdetails.
- **Human architecture:** sturdy chunky masonry, timber, lantern brass, roof shapes; slight imperfections, repeated motifs for recognizable culture.
- **Ancient architecture:** geometric inlays, arch frames, stone rings, signal engravings, rhythm patterns; mysterious but learnable.
- **Interactables:** larger distinctive profiles and consistent icon family; don't hide objectives solely behind tiny color shifts.

### Proposed environmental palette tokens
These are semantic roles; pixel artists can shade within them. **Do not force source creature sprites into these colors.**

| Token | Hex | Role |
|---|---|---|
| Deep Tide | `#183B41` | darkness, depth fog, metal trims |
| Canopy Jade | `#367D62` | forest foliage, nature primary |
| Fern Glow | `#79B77C` | moss highlights |
| Signal Mint | `#74D6B1` | discoverable signal indicators |
| Lantern Gold | `#E8C878` | warmth, important focal lighting |
| Ember Coral | `#E38A70` | fires, forge, hazards |
| Harbor Ivory | `#F6EBD7` | labels, chalk, light plaster |
| Ruin Plum | `#4C3C5B` | mysterious shadows |
| Brick Russet | `#88594E` | ancient wall base |
| Tide Blue | `#4B9CC1` | water and coast |

Accessibility rule: color cues always have supplementary symbols, shapes, values, sound and/or animation.

### Lighting hierarchy (non-PBR for low end)
1. Ambient region tint and time-of-day palette LUT or grade.
2. Pre-shaded directional highlights painted into wall/floor/sprite textures.
3. Distance-based shade/fog for coherent depth and reduced far-scene cost.
4. Limited low-cost dynamic additive lantern or spell effect, benchmarked before adoption.
5. Emissive signal marks as simple animated sprite/texture overlays, NOT dynamic per-pixel lighting by default.

Scene readability: the player must immediately distinguish walkable floor, blocked structure, creature, and main interaction. Intensity levels follow attention priority: quest-critical > active companion > hostile encounter > decorative particles.

### Texture/material language
- **Stone walls:** blocks of irregular but architecturally consistent size, cracks and erosion; avoid photo textures.
- **Ruins:** carved glyphs use WILDSIGNAL-specific signal vocabulary (rings, bars, wave angles), not appropriated real-world sacred writing.
- **Wood:** warm planks with clear plank rhythm and selective accent grain.
- **Dirt/path:** limited low-contrast repeated tiles with edge decals; not high-frequency noise.
- **Foliage:** sprite cards, silhouette clusters and wall trims; alpha sorting and overdraw tested on mobile.
- **Water:** palette-shifted floor animation / strips / sprites as an early approximation; no shader-dependent reflections in baseline.
- **Fire and lanterns:** sprite animation with localized warm color overlay if affordable.
- **Sky:** static/skewed sky backdrop or authored panorama; sky occlusion limits depend on renderer.

## C. Renderer-aware art rules

**Critical:** the supplied first-person screenshot *visually resembles* textured geometry with vertical look, complex arches and floors; it cannot be guaranteed by a basic axis-aligned raycaster. **Do not promise fidelity without prototype.**

Candidate renderers to benchmark:
1. Simple 2D grid raycaster with textured walls, floor/ceiling casting, billboard sprites and modest height illusion. Smallest scope, but limited geometry.
2. Sector/portal or column-based software renderer with authored wall segments, height differences and openings. More geometry freedom, more engineering risk.
3. Compact rasterization/path optimized for CPUs with no GPU assumptions. Evaluate feasibility, correctness and power draw.
Select based on proof of concept showing a Lantern Quay scene, a forest vista and one complex ruin room. A graphics-library backend and software approach may be mixed; a dedicated GPU must never be a hard requirement for minimum supported Windows hardware, and Android compatibility must be measured.

### Tentative display/raster targets (not final)
- Baseline internal world framebuffer: **320×180** for experimentation; also test 400×225 and 480×270.
- Pixel scaling: nearest-neighbor with clean integer scaling when possible; aspect ratio scaling handled deliberately.
- UI text and controls: draw at physical/display resolution or an independent high-resolution UI pass.
- FOV: test 65–75° horizontal; never freeze camera specifics from one screenshot.
- Camera height, player radius, tile scale, look up/down, floor/ceiling heights all require renderer/collision proof.
- Transparent sprite overdraw and alpha sorting are expected hotspots: impose data-driven limits after measurements.
- No assumptions about true 3D lighting, normal maps, free-flight, mirror surfaces, water refraction, soft shadows or vast vistas.

## D. Asset production standards (proposed)

### Source files
- Store artist-editable masters (e.g. Aseprite/LibreSprite layers or original pixel-editor format) when available.
- Store exported PNGs with alpha, no upscaled interpolation, consistent exact-origin/pivot data.
- Original user-owned sheet is **approved for inclusion**; record source filename, creator, provenance, approximate creation date and intentional transformations in an asset ledger. Do not claim specific license or exclusive legal status of future assets without review.
- Preserve original source sprites pixel for pixel in source archive; generate separated sprite outputs without destructively altering masters.
- Do not use image-removal methods that erase light edge pixels or introduce noisy halos; inspect transparency against both pale and dark checkerboard.

### Working pixel art targets (validation required)
- Creature source dimensions vary by cell; **do not force every sprite into exactly 64×64 source pixels**. Use padded transparent canvases and centerline pivots, preserving crisp native pixels.
- Wall/material textures: investigate 64×64 pixel source tile families, variants, overlays and trim strips.
- Animations: prototype 2–4 idle frames, 3–6 walking/floating frames, 2–4 alert frames, 3–6 battle-action frames; actual counts depend on source and art effort.
- For first person, sprite angles: front, back and 2 sides as minimum experimental set; 8-direction art only if gameplay/readability justifies cost. Do not silently mirror asymmetric details.
- Nearest-neighbor resizing for art previews. UI layout can use smooth vector text/icons where needed.
- Sprite atlas packing after character origins, widths, frames and view labels have been finalized.

### Naming convention
`assets/creatures/<species_id>/source/` — owned original files  
`assets/creatures/<species_id>/sprites/{front,back,left,right}/idle_00.png`  
`assets/creatures/<species_id>/sprites/.../walk_00.png`  
`assets/env/<biome_id>/materials/<material_id>_base.png`  
`assets/env/<biome_id>/props/<prop_id>_<state>_00.png`  
`assets/ui/icons/<name>.png`  
`assets/audio/<zone_id>/<cue_name>.ogg`

Metadata must define `species_id`, directional frame count, pivot, physical bounds, lighting policy, and relevant asset ownership notes; avoid hardcoding file paths in the engine.

### Map art kits
A biome kit should have:
- 6–10 common wall/floor/ground texture variants (target, not mandate).
- 3–5 landmark shapes/props with unique silhouette.
- Consistent modular trim sets and corners.
- 2–4 ambient props; 1–2 ambient wildlife reactions.
- Interactive object states: inactive, available, targeted, solved.
- Simplified LOD / distant representations as needed.
- Art-only tiles NEVER define collision or gameplay; gameplay map data provides authoritative material/collider tags.

## E. Detailed biome kits

### Lantern Quay
- Day palette: harbor ivory, cedar/coral wood, warm lantern gold, blue-teal roofs.
- Roofline silhouette: varied heights, hanging pennants, beacon tower; avoid flat repetitive buildings.
- Materials: painted clapboard, wood beams, stone lane, brass signal devices.
- Key props: lamp posts, handcart, rope coils, signboards, baskets, explorer backpack, shop canopy, tide bell.
- Landmark: central beacon tower, broad square with view to Forest Gate.
- NPC/avatars: multiple player styles readable at billboard scale. Never block main navigation with crowds.
- Weather: initially dry; optional gentle rain later.
- Ambient motion: hanging banners, lamp flames, occasional shore bird.
- HUD: calm, small quest direction indicators; minimal combat overlay.

### Mosswake Path and Liltwood Forest
- Main hues: fern jade, glow mint, dark bark brown, golden motes.
- Geometry: root arches, broad tree trunks as wall volumes, boulders as obstacle silhouettes, leaf walls, clear path edges.
- Props: mushrooms, cut stumps, vines, wooden signs, carved waystones, ferns, seed pods, luminous roots.
- Landmarks: singing root, signal markers with standard symbol code, veil wall.
- Signals: luminous geometric bands with a distinct audible harmonic; hidden clues visible via companion awareness state.
- Sprites: layered distant canopy backdrop; near and mid foliage separated to control occlusion and avoid jagged alpha overload.
- Unsafe design: dense leaf walls masking exits, ambiguous repeated T-intersections, low-contrast creatures on identical green backgrounds.

### Echo Hollow ruins
- Brickwork: reddish russet on plum/taupe shadows, mortared chunky stone, green moss and drips.
- Shapes: arches, pier bases, lintels, broken stairs as *non-traversable environmental forms unless renderer supports elevation*.
- Props: suspended lantern, metal grate, carved pedestal, signal sigil, stone wheel, broken jars, nest and wall vines.
- Interactive language: signal panel = triangular luminous inset + 3 bars + short chime; ready = rhythmic pulse; used = stable light; inactive = muted stone.
- Reused lighting: warm light above a cool violet-shadowed chamber with jade moss accents.
- Scene anchor: pedestal centered past the one onboarding battle.
- Performance: avoid huge open double-height rooms until renderer proven; fake unreachable upper galleries with background/art.

### Tideglass Inlet
- Hues: turquoise, coral ochre, beige sand, teal shadow.
- Forms: curved rock, cave mouths, waterline edge decals, coastal roots.
- Use simple animated texture strips for tidal activity, avoiding full 3D water.

### Ashbell Works
- Hues: ember orange, charcoal blue-gray, worn copper.
- Forms: cylindrical bells, pipes as static decor, blocked furnace mouths, rhythmic glowing inlays.
- Animated props: slow mechanism cycles, 2D sparks with budget cap.

### Advanced biomes
- Stormwatch Cliffs: cool slate, cyan-grey fog, sparse grass, signal towers; constrained visibility over cliffs.
- Gloamfen: muted violet, peat green, aqua bioluminescence; no gameplay relying on being too dark to see.
- Emberglass Caldera: basalt and molten orange with limited luminous cracks; animation not particle flood.
- Highlands: pale amber grasses, cool blue stone, wind-shaped silhouettes and observatories.

## F. Characters, companions and combat scene

### Player characters
- First-person exploration: character presence through optional hands/compass, footsteps, interaction animation.
- Multiplayer: require readable outward-facing humanoid sprites with at least directional turns and outfit-overlay plan. Pixel overlays for cosmetics must align to base frames; check scope before making hundreds.
- NPCs: unique hero silhouette per role (Wayfinder leader, workshop keeper, guard, merchant), simple readable facial/garment identity.
- Character costumes: boots, scarves, jackets, goggles, backpacks, regional motifs; no copyrighted franchise patterns.
- Avatar creation and skin tones/hair variety are accessibility/inclusion requirements, but actual combinations are planned after avatar art-pipeline proof.

### Creature companion
- Preserve the owner's exact base sprite designs (sheet cells cited in Creature Bible). Original appearance is canonical once individual species assignments are approved.
- World scale: physically present, generally behind/side of player; must not obscure crosshair or important targets.
- Reactions: follow, stop, look toward clue, excited, frightened, rest.
- Battle: larger staged sprite display and a distinct move-readability animation state; combat scene uses the same world mood.
- Follower should cast a *cheap* ground-contact cue/shadow to read position; no complex physically accurate shadows required.

### Battle UI
- 1v1 turn-based core. Strong large creature sprites; four moves readable with icons + text + affinity shapes; health/condition bars clear on small screens.
- Do not duplicate the provided reference game's hand/weapon/ammo UI; adapt its bold, expressive pixel style to companion/battle functions.
- Portraits and creature sprite details remain legible in a 16:9 phone layout. Test touch button minimums and localization text.
- Animate move intent and status clearly; avoid gratuitous flashing and screen shake. Provide reduced motion.

## G. Audio-visual consistency
- Music: warm melodic instruments and soft rhythmic mystery. Zones share a recognizable "signal" three-note motif with variations.
- Biome soundscapes: location-specific bird/bug/wind/water loops plus original companion calls; expose separate music/effect volume.
- Signal discovery: one cross-game audio identity, escalating from faint to clear near secrets.
- Night/ruins cannot become unreadable for hearing-impaired players; use visual pulses/icons.
- No externally sourced music/images without recorded commercial licensing and attribution requirements.

## H. Environment and art approval checklist
Every map concept must supply:
1. Top-down annotated geometry/blockout with entry/exit, safe return and critical path.
2. First-person mood board, 2–3 camera-view composition studies, scene landmarks and material swatches.
3. Palette tokens plus light/fog/profile in daytime and alternate state.
4. Asset list keyed to existing modular kit; new asset list and estimated art hours.
5. Creature encounters and animation visibility expectations, companion path and interaction spot checks.
6. Touchscreen and keyboard legibility check; no dependence on color alone.
7. CPU/render benchmark target with actual test evidence when validated.
8. Owner signoff and version before production implementation.

## I. Open art decisions
- Exact canonical native pixel scale from original sprite sheet, original transparent/master files, and source revision history.
- Renderer selection: raycaster vs sector/portal vs another lightweight method; vertical geometry scope.
- Camera field-of-view, player hands/compass placement and optional third-person / social portrait display.
- Final logo and typography; name/trademark clearance.
- Character body shapes and custom outfit constraints.
- Pixel font locale coverage, UI minimum physical tap sizes and CRT/filter toggles (default OFF).
- Landscape-only vs responsive orientation (initial recommendation: landscape).
- Quantitative sprite/texture/particle/lighting budgets, based on device tests not guesses.

**Instruction to Codex:** Keep all art parameters configurable; build ONE representative room for each of town, forest, ruin before authoring many maps. Never replace the owner's creature sprites with generic AI art or synthesize new species without approval.


## J. Player/NPC bible cross-reference

Playable avatar appearances, NPC silhouettes, costumes, first-person equipment, user-customizable palettes, sprite layer rules and related visual acceptance tests are authored in `CHARACTER_NPC_BIBLE_v0.1.md`. Map-binding and sprite-data metadata are in `CHARACTER_ASSET_SCHEMA_v0.1.json`. These are proposed production defaults; user-owned creature sprites remain canonical creature art. The opening chapter requires Mara, Old Fen and Iri as visible NPCs, and Bellkeeper Sol as a **nonphysical inscription**. Character palette/canvas/timing targets must be verified against the low-end rendering prototype before freezing.
