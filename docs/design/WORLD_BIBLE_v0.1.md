# WILDSIGNAL — World & Environment Bible v0.1

**Status: PROPOSAL. Not approved to implement.**  
**Owner:** Game design / world design. **Date:** 2026-10-20.  
**Related documents:** Creature Bible v0.1 (draft PR #1); Visual Bible v0.1; Master Pre-production Plan v0.1.
**Rule:** Codex must not invent unspecified geography or imply proposed location names are final; create test/blockout maps only until this spec is approved.

## 1. World vision and pillars

An original first-person, retro pixel-art, creature-companion RPG set in a tropical fantasy archipelago. At a glance the world feels welcoming, lush and bright; each region contains older structures and ecological mysteries; advanced areas become ominous without relying on horror or gore.

Pillars:
1. **Beautiful but mysterious:** curiosity before fear; inviting silhouettes and warm safe zones contrasted with ancient geometric ruins and unusual light.
2. **Readable in first person:** paths, exits, creatures and usable surfaces must be identifiable without a top-down camera.
3. **Creatures belong here:** every species has habitat, behavior, time-of-day tendencies and discoverable interactions.
4. **Companions change exploration:** physical follower, reactions, signals and optional shortcut access; no starter choice blocks the critical story.
5. **Small rich zones rather than empty open world:** reusable tilesets, intentional landmarks, manageable sightlines, separate loadable zones to respect low-end memory.
6. **Multiplayer-friendly boundaries:** safe social hubs, readable nearby avatars and instanced activities where needed.

### World-fiction premise (proposal)
The Wayfinders explore the scattered isles of the **Signal Archipelago** (world name provisional). Ancient "resonance" stations once stabilized the relationship between inhabitants and creatures. Strange signals are resurfacing from below the ruins, changing weather, migration and creature behavior. Wayfinders carry Resonance Compasses to investigate, build trust and open forgotten passages. Avoid copying names/iconography/capture devices from other franchises.

### Time, weather and state
- Vertical slice: one scripted transition from afternoon to dusk; time does not require a real-time clock.
- Later: optional regional day/night phases and simple weather states (clear, rain, fog, storm). Network server owns shared state; clients render a synchronized approximation.
- No expensive fully dynamic global illumination. Lighting variations come from authored color grading, distance fog, pre-shaded art and a small number of dynamic effects.
- A quest-critical species and route must remain discoverable even if weather/time would otherwise hide its normal spawn.

## 2. Geographic structure: region graph (NOT cartographically finalized)

Central **Lantern Quay** on the south coast. Routes radiate north to **Liltwood**, east along the cliffs to **Ashbell Works**, and west to **Tideglass Inlet**. An old network of ruins links them.

```text
                                [STORMWATCH CLIFFS] ---- [HIGH HIGHLANDS — future]
                                         |
           [GLOAMFEN MARSH — future] -- [ECHO HOLLOW RUINS]
                    |                    /       \
           [LILTWOOD FOREST] -- [MOSSWAKE PATH]  [ASHBELL WORKS]
                    |                    |           |
                    +-------------- [LANTERN QUAY] --+
                                         |
                                  [TIDEGLASS INLET]
                                         |
                              [ISLAND PASSAGE — future]
                                         |
                             [EMBERGLASS CALDERA — future]
```

This is a **travel graph**, not a scale-accurate map. For an initial release, isolate each outdoor/indoor location as a loadable zone with short transitions. Zone links must remain consistent with the official world overview once approved.

### Region hierarchy and planned release scope

| ID | Name | Tier | Role | Affinities / ecology | Progress |
|---|---|---|---|---|---|
| Z01 | Lantern Quay | Slice | Starting settlement / shared safe hub | Starter selection; friendly ambient creatures | Available at start |
| Z02 | Mosswake Path | Slice | Tutorial trail between hub and forest | Wild + introductory encounters | Early |
| Z03 | Liltwood Forest | Slice | Main first wilderness | Flora, Wild; hidden paths | Early |
| Z04 | Echo Hollow — Antechamber | Slice | First small ruin / story reveal | Lumen, Stone, Gloom hints | Post-bond trial |
| Z05 | Tideglass Inlet | Later | Shoreline and tidal caves | Tide, Gale | Early optional |
| Z06 | Ashbell Works | Later | Abandoned forge and mechanisms | Ember, Stone | Early optional |
| Z07 | Stormwatch Cliffs | Later | Co-op ascent / storms | Gale, Stone | Mid |
| Z08 | Gloamfen Marsh | Later | Night discoveries and mist | Gloom, Flora | Mid |
| Z09 | Emberglass Caldera | Later | Late-game high-risk underground | Ember, Stone | Late |
| Z10 | Highlands | Later | Cold upland habitats and old towers | Gale, Lumen | Late |

**Proposed shipping plan** is not the same as the first prototype. Zone count for a commercial release remains TBD after prototype benchmarks and workload assessment.

## 3. Place designs and level intent

### Z01 — Lantern Quay | Home base
- **Mood:** golden-hour comfort; wooden docks, indigo rooflines, teal-painted window trims, plants spilling from stone walls; sea horizon.
- **Landmarks:** Beacon Square (spawn), Wayfinder Hall (tutorial/companion services), Compass Workshop (first quest), South Dock (later boat travel), Market Lane (future cosmetics/shops), Forest Gate.
- **Navigational loop:** Square → Hall → Workshop → Gate → Square. No dead-end maze in first town. Use silhouette of central lantern tower visible from several approach directions.
- **Gameplay:** player introduction, controls, choose one starter, equip following companion, first save, talk to 1–2 NPCs.
- **Multiplayer:** safe-zone social space; no combat; cap nearby displayed avatars using level-of-detail sprite policies; important conversation panels cannot be obstructed.
- **Performance:** limited water animation; distant coastline billboard/backdrop; do not show hundreds of NPCs.
- **Audio:** gentle harbor ambience, wood creaks, gull-like original fauna, distant bell.
- **Lighting:** warm orange lanterns contrasted against muted twilight teal; environmental readability prioritized.
- **Exits:** north to Mosswake, west to future Tideglass, east to future Ashbell.

### Z02 — Mosswake Path | Tutorial threshold
- **Mood:** open green path narrowing into fern-lined trail; a clearly readable portal between comfort and wilderness.
- **Landmarks:** milestone obelisk, fallen wooden footbridge (decorative, not mandatory), small signal marker, forest arch.
- **Gameplay:** teach interaction, inspect one habitat clue, see one nonhostile wild species, demonstrate companion following and "alert" behavior.
- **Flow:** no forks until key controls learned; one optional alcove for a collectible.
- **Audio:** footsteps change from wooden planks to soil; birds and rustling leaves; subtle signal chimes.
- **Lighting:** bright daytime with soft canopy shadows; keep critical interactable marker brighter than nearby clutter.

### Z03 — Liltwood Forest | Wonder and discovery
- **Mood:** oversized roots, jade canopy, cyan luminescent mushrooms, amber fireflies and ancient broken markers.
- **Landmarks:** Singing Root (central landmark), Lantern Stump (rest/checkpoint), Fern Ring (wild encounter clearing), Veiled Wall (companion discovery), Ruin Gate.
- **Path hierarchy:** golden main route, two visible return loops, one optional pocket. Every fork has a distinctive landmark or sign; no undifferentiated repeated corridors.
- **Wildlife:** Flora and Wild small creatures forage or sleep; alert/react loops rather than idle standstill. Starter species can notice environmental clues.
- **Puzzle:** read signal marking → activate one of three equivalent environmental interactions; ALL starters must be able to achieve progression. Glintle highlights hidden markings; Driftfin reveals seepage channel; Brumlet activates warmth-sensitive inscription. Each solution opens the same Ruin Gate, with alternative animation/dialogue.
- **Optionality:** alternate paths provide collectibles/creature encounters, not mandatory progression locks.
- **Audio:** insects and leaves, reactive resonant tones near secrets, calming wildlife when companions are near.
- **Lighting:** mossy foregrounds, readable midtones, bluish distance fog to avoid long CPU sightlines.

### Z04 — Echo Hollow (Antechamber) | First mystery
- **Mood:** weathered reddish brick, square block arches, interlocking signal sigils, dripping moss and a single warm lantern focal point.
- **Landmarks:** broken seal at entrance, 3-sigil chamber, inactive pedestal, overlook onto unreachable deeper ruins (future tease), return portal.
- **Gameplay:** first 1v1 tutorial battle, light environmental interaction, "First Signal" story reveal, optional treasure, exit to forest.
- **Encounter:** intentional single battle at easy difficulty, no surprise unavoidable chain battles.
- **Puzzle:** one starter-specific discovery animation, one shared three-step sequence. The puzzle and rewards work for all starters.
- **Narrative:** player learns that the signals are caused by a waking ruin network; mystery remains unexplained.
- **Visual:** torch/lantern colors contrast with violet-blue shadows; symbols have shape cues in addition to hue for accessibility.
- **Audio:** quiet resonance pulse increasing near the pedestal, stone movement, one signature motif for the mystery.
- **Performance:** avoid transparent overdraw across the entire room; special light effects triggered sparsely.

### Z05 — Tideglass Inlet | Later, optional shoreline
- **Mood:** luminous water, coral-toned rock and tidal pools, kelp or coastal grasses.
- **Landmarks:** tidal arch, tide gauge, grotto entrance and a ruined pier.
- **Gameplay:** timing- and observation-based water ecology; hidden pools; rare coastal signals.
- **Constraint:** initial renderer need not simulate actual water surfaces, refraction or flowing physics; animated floor textures and sprite sheets can suggest water safely.

### Z06 — Ashbell Works | Later, ruin industry
- **Mood:** blackened charcoal walls, copper fittings, warm forge interiors with hanging ropes and bells.
- **Landmarks:** furnace heart, resonance bell, cooling cistern, conveyor remnants.
- **Gameplay:** rhythmic environmental interactions and heat switches; Ember/Stone ecology.
- **Constraint:** use authored simple animation for machines rather than simulated complex physics.

### Z07 — Stormwatch Cliffs | Midgame
- **Mood:** overcast teal sky, high wind, jutting rock strata, remote signal towers.
- **Gameplay:** two-player cooperative route and powerful gale species.
- **Constraint:** first-person safety and traversal visibility need testing; slopes/overhangs require upgraded renderer or segmented elevations.

### Z08 — Gloamfen Marsh | Midgame
- **Mood:** dusky violet mist, dark reflective puddles, luminous reeds, moving shadow silhouettes.
- **Gameplay:** sound-based tracking, night-focused encounters, difficult navigation with unmistakable landmarks.
- **Constraint:** fog cannot replace functional navigation; keep obvious return route.

### Z09 — Emberglass Caldera | Late
- **Mood:** volcanic basalt, glowing fractures, geometric chambers and intense heat.
- **Gameplay:** advanced combat, rare Ember/Stone creatures, environmental hazards.
- **Constraint:** no volumetric particle simulation required; palette animation + modest particles.

### Z10 — Highlands | Late
- **Mood:** cold grass, alpine stone, pale sky and abandoned observatories.
- **Gameplay:** tracking migrations and unusual signal/weather relationships.
- **Constraint:** no real-time terrain tessellation; geometry must match rendering capabilities.

## 4. Example blockout: first playable map connectivity

**These are abstract playable cells / room nodes, not authoritative collision geometry.** Codex can prototype the flow but final tile-by-tile maps require review. Intended player direction: Lantern Quay > Mosswake > Liltwood > Echo Hollow > Return.

```text
 [Lantern Quay]
    Beacon Square -- Wayfinder Hall
        |                 |
   Compass Workshop -- Forest Gate
                           |
                     [Mosswake Path]
                       Milestone
                           |
                     Signal Marker -- Optional Cache
                           |
                      Forest Arch
                           |
                   [Liltwood Forest]
                      Lantern Stump
                      /           \
                 Fern Ring     Singing Root
                      \           /
                      Veiled Wall / Companion Solution
                           |
                        Ruin Gate
                           |
                   [Echo Hollow]
                   Entry / Checkpoint
                           |
                  Intro Creature Battle
                           |
                    3-Sigil Chamber
                           |
                  Pedestal / Story Beat
                           |
                        Return
```

### First-session beat timings (design target, test with players)
- 0–3 min: arrival, immediate control, warm view of Quay.
- 3–6 min: choose companion, learn following and interact.
- 6–9 min: Mosswake tutorial with first creature behavior.
- 9–14 min: Liltwood looping trail and companion-specific reveal.
- 14–18 min: Echo Hollow first 1v1 battle and mystery.
- 18–20 min: quest closure, travel back / save. No excessive dialogue or long forced cutscenes.
- These targets are hypotheses; playtests determine actual pacing.

## 5. Zone technical and gameplay specification format

Every map authored or implemented needs a checked-in spec containing:
- `zone_id`, `display_name`, `release_phase`, `design_status`, `narrative_purpose`.
- `tile_dimensions`, `world_unit_scale`, `floor_layers`, `colliders`, `navigation_graph`, `occlusion` (to be frozen only after renderer prototype).
- Entry and exit IDs; destinations; spawn points; checkpoints; safe spawn radius; return route; battle permitted flag.
- Primary route graph and estimated walking time; optional loops and secrets; readable landmarks.
- Biome palette and materials; texture family; ambient light, fog rules and time/weather variants.
- Creature encounter table by habitat, conditions, rarity, and behavior; deterministic seed plan if applicable.
- Interactables with IDs, triggers, requirements, fallback paths, reward IDs, story flags.
- NPCs with IDs, dialogue keys, shop IDs and daily schedules if any.
- Soundscape events with distances and priorities.
- Multiplayer policy: shared hub vs player instances; visibility cap, encounter instancing, state authority.
- Save/reload state: puzzle flags, defeated/befriended states and player progress.
- Device budgets: target peak memory for the zone, visible sprite count and worst-case CPU/frame timings measured on actual low-end devices.
- Acceptance tests: navigation without getting lost, no blocked quests for any starter, fallback interaction always works, save/restore correctly reproduces state, no collision leaks.

## 6. World and art consistency constraints

- 2D-created terrain data + depth-illusion presentation. A simple raycaster does not automatically support vertical looking, outdoor height variation, bridges or true multi-floor structures. The screenshot's complex architecture is *reference aesthetics*, NOT proof that simple raycasting will reproduce it.
- Do not commit to slopes, stacks of floors, true sky occlusion, mirrors or free-flying camera without a rendering prototype and documented cost.
- Early scenes: mostly planar navigation, authored portals, sector transitions, sprite trees/foliage and distant background planes. Complex layers can be staged via level transitions or advanced renderer later.
- Landmark silhouettes must look distinct; avoid repeating identical green forest turns.
- Color is not the only cue for switches, exits or hazards: also use shape/sound/icon.
- Geometry density follows purpose: sparse enough for smooth mobile navigation and clear companion trailing, detailed enough to feel authored.
- Source-of-truth environment atlas, material rules, collision layers and lighting settings are documented in VISUAL_BIBLE_v0.1.md.
- Map behavior must be stable during network reconciliation. Quest-critical interaction checks are server-authoritative for online play.
- Support both touchscreen and keyboard navigation with identical reachable geometry.

## 7. Unresolved decisions and explicit review gates

1. Confirm or rename Signal Archipelago / Lantern Quay / Liltwood / Echo Hollow / other biomes.
2. Decide starter-specific solutions versus universally available exploration tools. This proposal guarantees *no main-story softlocks*.
3. Decide whether open social hubs are persistent or room-based and choose soft limits on nearby players.
4. Confirm travel: seamless portals vs small loading transition; no fast travel at first except checkpoints.
5. Freeze actual tile scale, player collision, camera height and FOV only after prototype testing.
6. Decide death/defeat/retreat handling for exploration and battle; protect player progress.
7. Approve ecology tables and zone-by-zone creature distribution after the creature catalog is signed off.
8. Approve minimal story quest script, tutorial wording, map signage and HUD wayfinding.
9. Define multiplayer quest ownership and puzzle state (per-player vs shared session).
10. Benchmark low-end Android and Windows devices before deciding size/number of draw calls, visible actors or render effects.

## 8. Codex instruction

**Do not treat proposed map list as permission to build all regions.** After approval, first implement a graybox test (Quay → Mosswake → Liltwood → Antechamber) with place-holder textures and owned creature art when provided. It must pass keyboard and touchscreen navigation, following, one encounter, a companion puzzle with no starter softlock, save/load, and performance logging. Build the renderer/collision systems to support data-authored future locations without baking one-off rules into code.
