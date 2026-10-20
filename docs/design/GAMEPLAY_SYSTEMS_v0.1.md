# WILDSIGNAL — Gameplay Systems & State v0.1

**Status:** MIXED. Confirmed: 1st-person exploration; turn-based battles; persistent companion; fully offline solo story and online optional. Numbers are proposed; not approved to implement.

## Core loop
Town preparations → travel → observe creature/ecological clues → discover signal → befriend or battle → companion helps solve optional or main-path interaction → gain story/creature/resource knowledge → save and return.

## Player and input
- World: first-person real-time walk; right/left turn (mouse / right touch drag), movement (WASD / left virtual joystick), interact (E / touch icon), compass (Q / touch icon), companion command (C / touch icon), menu (Esc / gear).
- Controller bindings optional follow-up, not a requirement of first technical demo; rebindable keyboard and adjustable joystick sensitivity planned.
- On phones use landscape, safe areas/notch handling; no tiny tap targets.
- Interact only with one target nearest camera center and within a fixed tunable reach, with a named icon/text prompt to avoid touch precision puzzles.
- Limit repeated interactions via debouncing; never require fast twitch timing.

## Companion following finite state machine
`IDLE`, `FOLLOW`, `APPROACH`, `INVESTIGATE`, `ALERT`, `ASSIST`, `REST`, `IN_BATTLE`, `UNAVAILABLE`.
- Follow local player smoothly while preserving collision and line of sight; if separated by a doorway, navigate existing walkable grid to catch up.
- If beyond tether distance and safely out of camera, respawn beside player as explicit **recovery fallback** (not exploitable in puzzles/battles).
- No object interaction when out of reach or blocked by authoritative nav/collider rules.
- Companion ALERT near signal clue; audio/icon cue and orientation toward landmark.
- Rest when stationary, excited after successful finding, frightened but not immobilizing near danger.
- All starters can solve the story puzzle by affinity-specific animation; never gate critical quest on class pick.
- Track `bond_points` and `bond_events_seen`; no daily time-gating or compulsive loss; exact bond threshold TBD.

## Wild species / encounters
- Ecological behavior set per species: habitat, wander radius, active times, alert distances, curiosity reactions, social group tendency.
- Encounter approach via observing and interacting; no mandatory random battles in tutorial.
- Modes of gaining creatures (noncombat trust/battle+trust/both) remain unapproved.
- Spawn seeded locally offline, server-authoritative online. Save story-relevant encounters to prevent duplication.

## 1v1 turn-based combat (proposal)
- Each party up to 3; 1 active; 4 moves.
- Phases: PREPARE → SELECT_ACTION → LOCK_COMMANDS → ORDER → EXECUTE → STATUS → KO_CHECK → NEXT_TURN / END.
- Available actions: one of 4 moves, swap, item, retreat (if permitted).
- Order: move priority first, speed second, deterministic PRNG tie-break; all calculated by single simulation module with explicit seed.
- Encounter uses arena view preserving surrounding zone palette; battle UI at separate high-resolution layer.
- **Numerical proposal now drafted:** see `BATTLE_BALANCE_BIBLE_v0.1.md`, `FIRST_NINE_CREATURES_v0.1.json`, and `BATTLE_TEST_VECTORS_v0.1.json`. The numbers are not yet approved. Do not code an alternative damage formula from this abbreviated section.
- Pause/local menu behavior: offline battle paused immediately; online battle follows server timing/disconnect policy to define.
- KO recovery: no loss of owned starter in tutorial; retriable; clear return-to-checkpoint.
- PvP: eventual server-resolved deterministic simulator; online XP/creature transactions server-owned.

## Puzzle / interaction spec
- `Veiled Wall` requires inspect-marker + sing-root + companion present.
- Puzzle sequence A→B→C; wrong selection resets progress and informs player.
- Each starter reveals sequence through own sensory affordance; no gameplay advantage/disadvantage in story completion.
- No purchases, internet, rare drops, co-op partners or specific starter required to complete a critical story route.
- Keep triggers idempotent to avoid repeated reward exploits.

## Inventory / items
- `RESONANCE_COMPASS` is a quest tool and cannot be discarded or permanently sold.
- Recovery consumable placeholder exists; precise economics, maximum stacks and crafting TBD.
- Rarity categories reflect discoverability not battle power.
- Quest flags separate from items, to prevent inventory bugs from breaking progression.

## Save/load and session ownership
- Offline local save owns all single-player world/quest/party state.
- Online mode uses separate server-authoritative ownership model **proposed**, not yet approved. Never trust file import into trade or PvP.
- On portal: atomic save checkpoint after successfully entering destination and validating spawn.
- Loading older save version requires migration path or readable error with preserved backup.
- Online sign-out returns to local main menu with local slot untouched.

## Acceptance test examples
1. From airplane mode, begin new game, choose any starter, complete puzzle + battle, save, reboot offline, load completed state.
2. Companion follows through corner corridor, avoids camera overlap and walls, uses recovery tether on distance.
3. Quest gate remains completable with any starter, even after wrong puzzle order or losing intro battle.
4. Input maps same interaction semantics across Windows keyboard and Android touch.
5. Offline combat results reproduce exactly from state + chosen commands + seed.
6. Network outage during optional online join never damages local save.
7. Every interaction has a stable ID and cannot award the same unique quest item twice.

## Approvals pending
Battle formulas/affinities, starter species art assignments, bond acquisition, combat effect list, save slot cap, player inventory limits, healing economy, online shared profile plan, player defeat handling.


## Detailed design dependency (2026-10-20)

The first nine species and optional creature invitations are proposed in `BONDING_ENCOUNTERS_v0.1.md` and `FIRST_NINE_CREATURES_v0.1.json`. The low-level battle status, energy, damage and turn-order rules are in `BATTLE_BALANCE_BIBLE_v0.1.md`. **When there is a contradiction, treat the detailed v0.1 Battle Bible/typed move definitions as the current *proposal*, not an approved rule.** Player ownership, friendly spar and all story encounters remain playable offline. The first compulsory guardian is WS-024 Craglet at level 3, with no automatic invitation until a distinct post-chapter encounter.


## UI binding contract (2026-10-20)

The UI draft in `UI_UX_BIBLE_v0.1.md` and `UI_SCREEN_CONTRACTS_v0.1.json` is a read-only view of gameplay/battle/quest state. It sends typed action IDs to local simulation (offline) or server (online) and never directly modifies HP, creature ownership, battle status, quest flags, trade or inventory. Four battle move slots must retain the move order in FIRST_NINE_CREATURES_v0.1.json; all costs and effects render from data. Mobile taps and keyboard keys map to the same action IDs. `UI_ACCEPTANCE_TESTS_v0.1.json` supplies acceptance cases before release. UI routes and dimensions remain proposed, not yet code.
