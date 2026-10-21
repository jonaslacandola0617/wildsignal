# WILDSIGNAL — The First Signal: Zone-by-Zone Production Blueprint v0.2

**Status:** Detailed *proposal* for review; blockout fixture is navigationally validated, not visually/technically approved.  
**Canonical grid fixture:** [FIRST_SIGNAL_BLOCKOUT_v0.2.json](maps/FIRST_SIGNAL_BLOCKOUT_v0.2.json)  
**Previous 15×11 document:** [LEVEL_MAPS_FIRST_SIGNAL_v0.1.md](LEVEL_MAPS_FIRST_SIGNAL_v0.1.md) is **superseded for exact tile coordinates** by v0.2, retained for design history.  
**Additional source of truth:** WORLD_BIBLE_v0.1.md, VISUAL_BIBLE_v0.1.md, NARRATIVE_QUESTS_v0.1.md, GAMEPLAY_SYSTEMS_v0.1.md.  
**No implementation authorized solely by drafting this document.**

## A. Tile space and coding contract

- All four graybox zones have **48 columns × 32 rows**. Origin at northwest (0,0); x east, y south; top = north. This is a **prototype topology**, not final geographic scale.
- Tile symbols: `#` = solid structural/foliage mass; `~` = solid water; `.` = walkable general floor; `=` = walkable principal path/pavement; `D` = walkable dock planks.
- Object state, NPCs, interactions, foliage decals and battle thresholds live in **separate metadata**, never overloaded on floor tiles. Full entries are in JSON.
- Camera `facing` is an authored cardinal convention only. Camera yaw/height/FOV, world units per tile, speeds, wall/floor/ceiling rendering and follower radius remain test-driven technical decisions.
- Every portal must be an explicit `interact` command; arrival cannot immediately retrigger the return portal. Portal checks can deny entry without teleportation. Use on-screen feedback such as "Your companion senses something missing" instead of silent failure.
- Grid flood-fill validation was completed when the fixture was authored: each zone's spawn, quest entity and portal is connected from its first spawn. Actual blocked-quest and first-person camera tests remain outstanding.
- Online versions may use the exact world template; offline story progression is entirely local and must never require a live online actor.

## B. Overall adventure graph and state lock

```
Z01 LANTERN QUAY  (start, save, compass, starter)
    | north; starter required
Z02 MOSSWAKE PATH (learn signals, marker, optional creature)
    | north; marker inspected
Z03 LILTWOOD FOREST (waystone, loop, companion veil puzzle)
    | northeast; veil gate opened
Z04 ECHO HOLLOW  (authored battle, sigils, pedestal revelation)
    | return portal; north-east side shortcut after completion
Z03 → Z02 → Z01  (always possible backtrack)
```

Back portals are never disabled after the player progresses; the final Echo Hollow shortcut is bonus convenience. Do **not** place a critical item behind an optional puzzle or reward source.

### Four visual story beats
1. **Comfort:** player's view from dock frames the golden beacon through colorful coastal structures.
2. **Curiosity:** a bent waystone under overhanging boughs emits a faint harmonic signal; a wild creature watches from a side clearing.
3. **Discovery:** a large Singing Root forms a unique silhouette in the forest; the companion acts like a genuine guide.
4. **Mystery:** russet masonry and a mint-lit pedestal frame the first revealed fragment of a buried signal network.

## C. Z01 Lantern Quay detailed layout

**Art kit:** `harbor_01` · warm harbor ivory, lantern gold and desaturated teal.  
**Navigation:** wide connected public space, intentional building mass silhouettes, no forced queues. Dock plank tiles at bottom lead from player spawn into beacon plaza. The town's four rectangular solid masses are **temporary collision proxies**, to be replaced by authored buildings/entrances after camera prototype. Do not turn the map into a boxed labyrinth.

| Stable ID | Coords | Purpose | Rule |
|---|---|---|---|
| Z01_SPAWN_SQUARE | (24,29) | initial dock arrival, facing north | always |
| NPC_001_MARA | (18,11) | Wayfinder guide | Q001 |
| NPC_002_FEN | (33,8) | workshop technician | Q001 |
| INTERACT_STARTER_ALTAR | (28,17) | confirm Glintle, Driftfin or Brumlet | Q002; atomic once-per-slot |
| LANDMARK_BEACON_TOWER | (23,11) | principal wayfinding silhouette | no collision until detailed art placed |
| LANDMARK_SOUTH_DOCK | (24,29) | harbor exit/arrival context | no quest condition |
| PORTAL_Z01_Z02 | (24,2) | path to Mosswake | FLAG_STARTER_SELECTED |

**Player orientation and teaching:** spawn with a clear view toward beacon tower; lead naturally via Mara's position, Fen's workshop frontage, starter altar, then north forest gate. All three must be distinguishable using geometry, color, signage and short compass guidance. Mara's conversation can open quest tracker; Fen awards compass. Starter choice panel includes species portraits, affinity, battle role and exploration specialty; confirmation warned irreversible on this local slot.

**Environment props:** four building kit silhouettes; lookout tower, market tent and ropes; flower troughs; lantern line; pier bollards; a few animated wind flags; looped ambient tide strips outside dock collision. Harbor sky is backdrop, not expensive ocean geometry.

**Offline presence:** 2–4 idle town NPC sprites and dock wildlife, independently of any server. **Online overlay:** nearby player billboard sprites and companion visuals can appear but must not block Mara/Fen/altar or portal.

**Hero camera shots to approve:**
- Q-01 `(24,29)` facing north: dock to beacon, workshop and warm path.
- Q-02 `(18,16)` facing east: market-like plaza perspective and starter altar.
- Q-03 `(24,6)` facing north: forest gate and first threshold to wilderness.

**Audio:** buoy wood creak, distant bells, wind chimes, harbor fauna; low-intensity melodic loop; compass signal audible within approach to Fen.

## D. Z02 Mosswake Path detailed layout

**Art kit:** `forest_edge_01` · fern green with clear golden ground path and darker understory.

| Stable ID | Coords | Purpose | Rule |
|---|---|---|---|
| Z02_SPAWN_FROM_TOWN | (24,29) | entry from Quay | always |
| LANDMARK_BENT_WAYSTONE | (19,21) | memorable curvature landmark | passive |
| C_MOSSWAKE_WILD | (36,20) | nonhostile wild creature observation | can observe, no mandatory combat |
| ITEM_OPTIONAL_CACHE | (37,23) | first optional collectible | once per save, never gates quest |
| PROP_SIGNAL_MARKER | (29,9) | teach investigate compass signals | sets FLAG_MARKER_INSPECTED |
| PORTAL_Z02_Z03 | (24,2) | northern forest opening | requires marker inspected |
| PORTAL_Z02_Z01_BACK | (24,29) | back to town | always, explicit interact |

**Geometry:** the main path snakes north with a safe 5-cell approximate cutout width, while a side track loops east to a creature clearing. West water cells are out-of-route ambience; they do not imply required swimming. The existing fixture intentionally has no forced random encounter.

**Visual contrast:** underbrush `#` is an opaque movement blocker, layered sprite fronds become nonblocking foreground props. Forest-edge trees can rise as textured columns; *not* geometry requiring realtime overhang physics. Signal marker uses consistent mint triangle + 3-bar symbol and increasing sound pulse, readable for color-blind players.

**Hero shots:** P-01 from (24,27) north along curved trail; P-02 from (19,21) northeast with bent waystone; P-03 (30,10) north with the signal marker and forest threshold.

**NPC/creatures:** one owned sheet sprite selected after catalog is approved. Creature should investigate when approached and turn its gaze to the trail; battle is optional. No species ID invented until catalog assignment.

## E. Z03 Liltwood Forest detailed layout

**Art kit:** `liltwood_01` · primary lush jade, bark-brown, signal mint, lantern gold.  
**Topology:** first forest feels like an ecosystem rather than a hallway: main path passes through a western clearing to the Singing Root, an eastern shortcut loop returns toward checkpoint, and a side spur leads to a secret cache. A bright, unmistakable ruin gate is seen from the northeast.

| Stable ID | Coords | Purpose | Rule |
|---|---|---|---|
| Z03_SPAWN_FROM_PATH | (24,29) | forest arrival | always |
| NPC_003_IRI | (21,25) | caretaker with ecology hints | Q003 |
| LANDMARK_LANTERN_STUMP | (24,24) | rest/checkpoint | available after story introduction |
| LANDMARK_FERN_RING | (11,18) | companion/environment observation clearing | optional encounter |
| LANDMARK_SINGING_ROOT | (29,14) | investigate missing waystone | sets FLAG_WAYSTONE_FOUND |
| PROP_VEILED_WALL | (36,10) | companion-driven forest puzzle | inspected marker + root found |
| PROP_OPTIONAL_FOREST_CACHE | (8,8) | secret side branch | once per save |
| LANDMARK_RUIN_GATE | (42,3) | stable portal silhouette | locked until veil puzzle solved |
| PORTAL_Z03_Z04 | (42,3) | enter Echo Hollow | FLAG_RUIN_GATE_OPEN |
| PORTAL_Z03_Z02_BACK | (24,29) | return to Mosswake | always |

**Companion puzzle v0.2 design:** After Q003 and inspecting root, at `PROP_VEILED_WALL`, the companion reveals three signal symbols **A, B, C**; user selects A→B→C in an overlay with no time limit. Glintle illuminates markings, Driftfin traces seepage channels, Brumlet warms an engraved line. Each companion grants exactly the same actionable clues/quest outcome. Puzzle overlay is keyboard/controller/touch accessible; on mistake signal resets but no resources are lost. On completion, FLAG_RUIN_GATE_OPEN = true and the northeast portal visually awakens.

**Ecology:** wild creature at Fern Ring follows a short roam/forage/notice loop and does not spawn on the gate. Companion reacts to root from ~3 abstract tiles, player controls remain active; don't force a 30-second canned animation.

**Hero shots:** F-01 (24,26) looking NW to lantern stump; F-02 (22,16) looking NE to Singing Root, framed by overgrown stones; F-03 (34,11) looking NE to Veiled Wall; F-04 (40,6) looking north toward opened Ruin Gate.

**Lighting:** gentle canopy mottling and background foliage; dusk tint enters as quest reaches forest; one pulse-effect system can illuminate signal symbols, but no expensive dynamic shadows. Ambient wildlife quiets briefly near puzzle to make discovery noticeable.

## F. Z04 Echo Hollow detailed layout

**Art kit:** `echo_hollow_01` · reddish brick archwork, violet stone shadows, cyan/mint ancient signals, warm lanterns.  
**Topology:** southern antechamber → narrow corridor with mandatory introduction battle boundary → central chamber → gated north threshold → sigil room and pedestal → optional shortcut return.

| Stable ID | Coords | Purpose | Rule |
|---|---|---|---|
| Z04_SPAWN_FROM_FOREST | (24,29) | southern arrival, faces north | always |
| TRIGGER_INTRO_BATTLE | (24,21), crossing x22..26 y19..22 | guaranteed first approach through corridor | only until FLAG_INTRO_BATTLE_WON |
| LANDMARK_BROKEN_SEAL | (24,19) | story clue after battle | nonblocking decoration |
| GATE_AFTER_BATTLE | (24,10) | lock north chamber until battle won | FLAG_INTRO_BATTLE_WON |
| SIGIL_A | (17,6) | first interactable sigil | puzzle state |
| SIGIL_B | (24,6) | central sigil | puzzle state |
| SIGIL_C | (31,6) | third sigil | puzzle state |
| PROP_OPTIONAL_RELIC | (12,7) | optional archaeological object | once per save |
| PEDESTAL_FIRST_SIGNAL | (24,4) | story completion | battle + sigil puzzle complete |
| PORTAL_Z04_Z03_SHORTCUT | (34,7) | return forest without backtracking | FLAG_FIRST_SIGNAL_COMPLETE |
| PORTAL_Z04_Z03_BACK | (24,29) | southern return | always |

**Battle design:** entering corridor boundary triggers one mandatory tutorial battle once per save; battle overlay pauses offline world. If lost, player recovers near entrance with safe HP and can retry. Re-entering cannot claim battle reward twice. The gated doorway checks `FLAG_INTRO_BATTLE_WON` on any attempt to cross; moving to its side cannot bypass the check.

**Distinct second puzzle proposal (not a repeat A→B→C):** after battle the three sigils are toggle switches. Initial states `A=0, B=1, C=0`; etched mural indicates target `A=1, B=0, C=1`. Players toggle A, B and C in any order, each event persisting locally. When state matches target, set `FLAG_SIGIL_PUZZLE_COMPLETE`, sound 3-part harmonic and allow pedestal. Each sigil has visible symbol + icon/number in addition to color; no rapid timing.

**Narrative framing:** reveal a huge old signal network diagram (texture/sprite mural, *not* real multilayer geometry), a bright point awakening beneath the archipelago, and one resonant inscription. Leave the mystery unsolved. Bellkeeper Sol is optional lore reference, not required NPC.

**Hero shots:** E-01 (24,28) north to lamp-lit corridor; E-02 (24,17) north under large arch with stone pillars; E-03 (24,8) north looking at three sigils and pedestal. The geometry may use flat painted arches and billboards if sector-style height rendering isn't yet feasible.

## G. Environment kit / props / encounter authoring boundaries

Create three distinct material families `harbor_01`, `liltwood_01`, `echo_hollow_01` plus transition kit `forest_edge_01`. Reuse flora and signal trim textures across zones; different hierarchy and palette make them identifiable.

- **Fixed collision** is map data, not imported from sprite transparency.
- **Functional interactables** have 4 visual states: dormant, available, focused, resolved (puzzles need extra individual states).
- **Creature positions** require 1–2 cell free space and pathing test around local geometry. Spawn conditions are data-driven, seeded locally.
- **Camera composition** needs two approved screenshots per location minimum from specified hero cameras; highlight landmarks, avoid featureless repeated walls.
- **Audio occlusion** is optional; one ambient layer and 2–3 event stingers per zone can support first prototype.
- **Persist** all unique rewards, puzzles, starter ownership and checkpoint before zone switch.
- **Mobile readability**: avoid interactive targets that rely on identifying small 2-pixel marks or tiny precise taps. Interaction prompt snaps to the nearest centered target.
- **Performance**: the grid size is storage geometry, NOT permission to render every prop or actor every frame. Occlusion, fog, visibility culling and sprite counts are benchmarked on a real low-end phone.

## H. Cross-document quest/flag rules (proposed but consistent)

| Quest | Required state on entry | Persistent outcome |
|---|---|---|
| Q001 Compass | new local save | FLAG_HAS_COMPASS |
| Q002 First companion | FLAG_HAS_COMPASS | FLAG_STARTER_SELECTED, species ID |
| Q003 Missing waystone | FLAG_STARTER_SELECTED | FLAG_MARKER_INSPECTED and FLAG_WAYSTONE_FOUND |
| Q004 Veiled Wall | marker inspected + root found + companion present | FLAG_RUIN_GATE_OPEN |
| Q005 Echo Hollow | ruin gate open | FLAG_INTRO_BATTLE_WON; FLAG_SIGIL_PUZZLE_COMPLETE; FLAG_FIRST_SIGNAL_COMPLETE |

Never implicitly infer Q005 completion simply from opening gate; each flag represents distinct trigger. No cloud sync or login needed to complete and locally save any quest.

## I. Implementation acceptance and outstanding design gates

**Automated from fixture**: 4 zones, 48×32 rectangular grids, blocked outer boundary, all named entities and spawn/portal tiles reachable from zone primary spawn, all portal destination IDs valid. (Recorded checks: Z01 856 reachable, Z02 376, Z03 524, Z04 498.)

**Before authoring final art**:
1. Validate front-facing experience from all hero camera points with actual low-end renderer.
2. Rewrite collider and decorative geometry where hero shots look like corridors or walls dominate.
3. Test puzzle and battle locks with sequences including loss, wrong sigil state, saving mid-quest, backing out and returning.
4. Ensure no alternate route bypasses Z04 battle due to incorrect trigger checks.
5. Ensure save/resume never spawns inside blocker or automatically retriggers portal.
6. Confirm the player can understand where to travel without a minimap during first quest.
7. Confirm *offline* playthrough with all three starters, 30 FPS goal on test hardware (not guaranteed).
8. Approve actual room sizes / tile scale, geometry shapes and art budget before tagging maps `APPROVED_FOR_PRODUCTION`.

**Still not finalized:** actual 3D illusion renderer, game-space units/FOV/collision radius, polished town floorplan, detailed forest loops at larger scale, species-to-habitat mappings, full NPC scripts, first foe identity, soundtrack, online session appearance and server persistence.
