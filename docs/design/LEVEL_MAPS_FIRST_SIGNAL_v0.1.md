**SUPERSEDED COORDINATES:** This early 15×11 test layout is kept for historical reference. For any new graybox use FIRST_SIGNAL_BLOCKOUT_v0.2.json and FIRST_SIGNAL_ZONE_BLUEPRINT_v0.2.md instead. Do not mix v0.1 trigger placement with v0.2.

# WILDSIGNAL — The First Signal: Level Maps v0.1

**Status:** FIRST DRAFT for creative approval, NOT final collision or artwork. **Mode:** fully playable offline; optional online instances use the same static maps. Coordinates use a simple normalized blockout grid; *do not treat as engine units or finalized scale*.

## 0. Common grid specification
- Coordinates: integer `(x,y)`, origin at top-left; x eastward, y southward. All grids below 15 columns x 11 rows. All rows are exactly 15 chars. For all zones, tile `#` means impassable solid, `.` means ground/walkable, `~` means nonwalkable water or decorative boundary, `G` means paired exit/entrance portal on walkable floor, `S` means zone spawn on walkable floor, `N` means NPC interaction tile (walkable for blockout), `I` means interactable marker (walkable trigger), `C` is a creature event (walkable), `B` is battle trigger (walkable), `P` is a puzzle anchor (walkable), `T` denotes narrative / story tile (walkable).
- Actor/player positions are centered at cells; interactions target adjacent cells or designated same-cell trigger; no inaccessible diagonal interaction. Final player/collider dimensions frozen after renderer tests.
- Boundary between zones occurs ONLY at explicit `G` portal objects with a transition identifier. Exits are blocked until prerequisite flags met, but an exit returning to previous zone must remain usable.
- North means negative y. Target walk time is a pacing objective, not guaranteed with these tiny schematic blockouts.
- All maps are closed rectangular grids and each cell has exactly one base symbol; decorative props, lighting and elevated faux details are attached by metadata layers, not encoded as collisions here.

## Z01: Lantern Quay / 15×11 (proto-grid)
```text
###############
#.....N.......#
#.............#
#...####......#
#...#..#......#
#...#..#..I...#
#...####......#
#.............#
#..S.........G#
#.............#
###############
```
**Z01** point definitions (0-based):
- Spawn `Z01_SPAWN_SQUARE` at (3,8); overlook town/beacon.
- NPC `NPC_001_MARA` at (6,1); onboarding guide, accessible by walking near NW.
- `Z01_COMPASS_WORKSHOP` and `STARTER_ALTAR` are metadata placed on `I` at (10,5); two interaction states in same workshop (combine UI, do not hide second objective).
- Exit `PORTAL_Z01_Z02` at (13,8) → `Z02_SPAWN_FROM_TOWN` at (1,1), requires `FLAG_STARTER_SELECTED`.
- Architecture blocks (4,3) through (7,6) with gap tile decorations represent one building; use exterior menu interaction at (10,5) as a simplified workshop façade.
- Walkable checkpoints for start, NPC, workshop, exit must be graph-connected; portals cannot be triggered through walls.
- Important correction for final map: town hall and workshop each deserve physically legible distinct entrances; prototype may combine services but art-ready map must distinguish both.

## Z02: Mosswake Path / 15×11
```text
###############
#S............#
#..#####......#
#..#...#......#
#..#...#..I...#
#..#...#......#
#..#####......#
#.............#
#..C.........G#
#.............#
###############
```
- Spawn `Z02_SPAWN_FROM_TOWN` at (1,1). 
- Signal marker `PROP_SIGNAL_MARKER` at (10,4): Q003 clue; sets `FLAG_MARKER_INSPECTED`.
- Creature event `C_MOSSWAKE_WILD` at (3,8): first nonhostile species observation, no forced combat.
- Exit `PORTAL_Z02_Z03` at (13,8) → `Z03_SPAWN_FOREST` at (1,1), requires `FLAG_MARKER_INSPECTED`.
- Backtrack `PORTAL_Z02_Z01_BACK`: metadata interactable at (1,1), returns to (12,8) in Z01. A single cell may be a spawn + return portal if it's marked in metadata; future art must visually explain it.

## Z03: Liltwood Forest / 15×11
```text
###############
#S............#
#..#####......#
#..#...#......#
#..#...#..P...#
#..#...#......#
#..#####......#
#......I......#
#..C.........G#
#.............#
###############
```
- Spawn `Z03_SPAWN_FOREST` at (1,1), with metadata back portal at (1,1) → Z02 (12,8).
- `LANDMARK_SINGING_ROOT` at (7,7), marked `I`; sets `FLAG_WAYSTONE_FOUND` and unlocks puzzle attempt.
- `PROP_VEILED_WALL` at (10,4), `P`; inspect first marker + singing root then solve A→B→C mini-interactions in a local overlay; open gate as a **quest-state transition**, no need to physically delete wall for prototype.
- `C_FERN_RING` at (3,8): optional wild encounter.
- `PORTAL_Z03_Z04` at (13,8) → Z04 (1,1); requires `FLAG_RUIN_GATE_OPEN`.
- Optional pocket for cache: art prop near (9,1), not a required route.
- Final map **must add explicit spatial loops/clearings** not evident in schematic: Singing Root ↔ Fern Ring paths, distinctive canopy landmarks and a clear visible Ruin Gate. Do not take this 15×11 test grid as an art-ready forest.

## Z04: Echo Hollow Antechamber / 15×11
```text
###############
#S............#
#..#####......#
#..#...#......#
#..#...#..P...#
#..#...#......#
#..#####......#
#......T......#
#..B.........G#
#.............#
###############
```
- Spawn `Z04_SPAWN_RUIN` at (1,1), metadata back portal to Z03 (12,8).
- `TRIGGER_INTRO_BATTLE` at (3,8); set `FLAG_INTRO_BATTLE_WON` only after deterministic success. After defeat, respawn Z04 (1,1) with restored starter, no lost progression.
- `SIGIL_PUZZLE` at (10,4), `P`: A→B→C interactions in overlay; prerequisite `FLAG_INTRO_BATTLE_WON`. Story-level puzzle uniqueness TBD.
- `PEDESTAL_FIRST_SIGNAL` at (7,7), `T`; chapter reward if puzzle completed, sets `FLAG_FIRST_SIGNAL_COMPLETE`. **Additional required save flag** `FLAG_SIGIL_PUZZLE_COMPLETE` introduced here, and should be added to narrative/quest event data.
- `G` at (13,8) is **return-only shortcut** to forest after chapter completion. It does not lead to new zone.
- Important: story pacing and geometry should put battle before sigil/altar in visually obvious order. Current schematic is a connected navigation/trigger feasibility proof only, not designed to force encounter on all walkable paths. Trigger or gate placement must be rewritten in final level art map to prevent skipping the required battle.

## Overlay and portal table
| Transition ID | Source | Destination | Trigger |
|---|---|---|---|
| PORTAL_Z01_Z02 | Z01 (13,8) | Z02 (1,1) | starter acquired |
| PORTAL_Z02_Z01_BACK | Z02 (1,1) | Z01 (12,8) | always |
| PORTAL_Z02_Z03 | Z02 (13,8) | Z03 (1,1) | marker inspected |
| PORTAL_Z03_Z02_BACK | Z03 (1,1) | Z02 (12,8) | always |
| PORTAL_Z03_Z04 | Z03 (13,8) | Z04 (1,1) | forest gate open |
| PORTAL_Z04_Z03_BACK | Z04 (1,1) | Z03 (12,8) | always |
| PORTAL_Z04_Z03_SHORTCUT | Z04 (13,8) | Z03 (12,8) | first signal complete |

## Authoring contract for next revision
- Maps become **multi-layer data**, e.g. `base_ground`, `solid_walls`, `scenery`, `interactions`, `spawns`, `portals`, `audio_zones`, `creature_spawns`, `lighting`, `navigation`.
- Each walkable tile must be graph-reachable from spawn or explicitly marked decoration-only. Perform BFS validation at CI/load time.
- Quest triggers cannot be bypassed by movement around one cell; check interaction state at gates instead of relying only on geometric chokepoints.
- A portal must pair with one destination spawn, in stable IDs; no recursive portal triggering on loading.
- Add player/companion collision test in narrow paths, doorways and each interacting sprite.
- All encounter/quest data valid offline; online authoritative mode reuses map layout but NEVER trusts client-submitted unique rewards.
- Next deliverable after owner approval: **30–80-cell-scale room/area blueprints** with real navigational loops, door widths, room vistas and camera viewpoints, generated from the approved renderer dimensions. Do not promise an exact final grid before engine feasibility tests.

## Acceptance tests for this conceptual blockout
1. Script parses each grid as 15×11, asserts boundary wall (except portals inside), and validates legend.
2. BFS over walkable cells connects starter spawn, required interactions, quest entrance and destination portal (subject to state flags).
3. Test both entrance and exit portal directions, every zone.
4. Finish offline Q001–Q005 for each of 3 starters without map softlock.
5. Interrupted save at every portal, game resumes at valid cell (no loop).
6. Companion navigates around central obstacle and does not clip or teleport onscreen.
7. Revisit after chapter complete does not regrant items or repeat forced battle.
