# WILDSIGNAL — First Playable Cross-System Consistency Gate v0.1

**Status:** Documentation audit only, with no game implementation, playtest, or actual save/battle execution. Updated 2026-10-20.
**Canonical plan sources:** `docs/design/maps/FIRST_SIGNAL_BLOCKOUT_v0.2.json`, `FIRST_SIGNAL_ZONE_BLUEPRINT_v0.2.md`, `NARRATIVE_QUESTS_v0.1.md`, `GAMEPLAY_SYSTEMS_v0.1.md`, `FIRST_NINE_CREATURES_v0.1.json`, `BATTLE_BALANCE_BIBLE_v0.1.md`, `BATTLE_TEST_VECTORS_v0.1.json`, `UI_SCREEN_CONTRACTS_v0.1.json`.
**Design approval:** provisional rules, entities and numbers remain proposals unless in `DECISION_REGISTER_v0.1.md` marked confirmed.

## 1. Primary spine — one consistent story path

```
Z01 Lantern Quay / Z01_SPAWN_SQUARE
  interact NPC_001_MARA + NPC_002_FEN -> FLAG_HAS_COMPASS
  interact INTERACT_STARTER_ALTAR -> FLAG_STARTER_SELECTED
  portal PORTAL_Z01_Z02 -> Z02_SPAWN_FROM_TOWN
Z02 Mosswake Path
  interact PROP_SIGNAL_MARKER -> FLAG_MARKER_INSPECTED
  portal PORTAL_Z02_Z03 -> Z03_SPAWN_FROM_PATH
Z03 Liltwood Forest
  optional NPC_003_IRI; interact LANDMARK_SINGING_ROOT -> FLAG_WAYSTONE_FOUND
  interact PROP_VEILED_WALL -> puzzle A->B->C -> FLAG_RUIN_GATE_OPEN
  portal PORTAL_Z03_Z04 -> Z04_SPAWN_FROM_FOREST
Z04 Echo Hollow
  TRIGGER_INTRO_BATTLE (guardian WS-024) -> FLAG_INTRO_BATTLE_WON
  sigils A,B,C toggle to ON/OFF/ON -> FLAG_SIGIL_PUZZLE_COMPLETE
  PEDESTAL_FIRST_SIGNAL -> FLAG_FIRST_SIGNAL_COMPLETE
  shortcut return to Z03, or backtrack through return portal
```
All these events are offline-local in the first slice. All three starter candidates must solve the same main objectives and have their own visual clue animations.

## 2. IDs in the first slice (no invented aliases)

| Purpose | Exact metadata identifier | Zone | Result/prerequisite |
|---|---|---|---|
| Guide | `NPC_001_MARA` | Z01 | Q001 dialogue/tutorial |
| Compass award | `NPC_002_FEN` | Z01 | `FLAG_HAS_COMPASS` (once) |
| Starter choice | `INTERACT_STARTER_ALTAR` | Z01 | `FLAG_STARTER_SELECTED` (atomic) |
| First gate | `PORTAL_Z01_Z02` | Z01 | requires starter selected |
| Signal marker | `PROP_SIGNAL_MARKER` | Z02 | `FLAG_MARKER_INSPECTED` |
| Forest portal | `PORTAL_Z02_Z03` | Z02 | requires marker inspected |
| Forest caretaker | `NPC_003_IRI` | Z03 | **optional** dialogue, no puzzle softlock |
| Singing Root | `LANDMARK_SINGING_ROOT` | Z03 | `FLAG_WAYSTONE_FOUND` |
| Veil wall | `PROP_VEILED_WALL` | Z03 | marker + root + companion; sets `FLAG_RUIN_GATE_OPEN` |
| Ruins portal | `PORTAL_Z03_Z04` | Z03 | requires ruin gate opened |
| Intro battle | `TRIGGER_INTRO_BATTLE` | Z04 | WS-024 Craglet candidate, sets `FLAG_INTRO_BATTLE_WON` on success only |
| After battle gate | `GATE_AFTER_BATTLE` | Z04 | blocks crossing until intro win |
| Three switches | `SIGIL_A`, `SIGIL_B`, `SIGIL_C` | Z04 | ON/OFF/ON puzzle, sets `FLAG_SIGIL_PUZZLE_COMPLETE` |
| Pedestal | `PEDESTAL_FIRST_SIGNAL` | Z04 | battle+sigils completed; sets `FLAG_FIRST_SIGNAL_COMPLETE` |
| Shortcut | `PORTAL_Z04_Z03_SHORTCUT` | Z04 | chapter complete |
| Return | `PORTAL_Z04_Z03_BACK` | Z04 | always usable |

**Do not rely on prose alias** `STARTER_ALTAR` (older working label) as a runtime identifier; `INTERACT_STARTER_ALTAR` is the current blockout ID. The old 15×11 coordinates are superseded.

## 3. Existing corrections to documentation in this milestone

1. **Q002 starter altar**: the narrative now uses exact map metadata ID `INTERACT_STARTER_ALTAR`.
2. **Iri**: ecological dialogue may help, but is optional to avoid an easily missed required NPC conversation.
3. **Veiled wall**: both `FLAG_MARKER_INSPECTED` and `FLAG_WAYSTONE_FOUND` are stated as the required Q004 prerequisites.
4. **Puzzle distinction**: forest = A→B→C order; Echo = three toggled switches ON/OFF/ON. The stale quest flag table wording was corrected, no "both puzzles same" implementation allowed.
5. **Battle exact speed tie**: the short Gameplay overview's old PRNG tie phrase was corrected to refer to detailed Battle Bible **alternating seat parity** proposal. Full combat implementation remains unapproved.
6. **Raw sprite labels**: stable `WS-###` source identities, not canonical species names, remain the only locked art IDs. Original 100-cell source image is user-owned and not AI-generated.
7. **Windows build**: x86_64-only Windows 7/8/8.1/10/11 target confirmed; x86/32-bit out of scope. Engine architecture and Win7 startup still require validation.

## 4. Remaining *real* consistency risks before Codex can implement quests

| Issue ID | Risk and specific fix needed | Severity / when |
|---|---|---|
| GATE-001 | A flood-fill reachable entity does not prove a **line of sight** or that the camera sees the landmark; check three hero shots in actual graybox renderer before texture lock. | High / renderer proof |
| GATE-002 | `GATE_AFTER_BATTLE` is currently a **single metadata entity**, not a full collision barrier across corridor x22..26. Define a gate volume spanning ALL traversable width, or a state-driven collision trigger that blocks movement without bypass. | High / before Q005 |
| GATE-003 | Mandatory `TRIGGER_INTRO_BATTLE` volume spans a proposed chokepoint, but must test diagonal collision, teleport/companion shortcuts and save reload beyond it. | High / before battle production |
| GATE-004 | The 48×32 map data includes placeholders/landmarks, not all six wild encounter spawn entities proposed by Bonding Bible. Add only those approved for actual first playable. | Medium / recruitment phase |
| GATE-005 | Starter's irreversible choice must be atomic across local save recovery. Need versioned save transaction and one-time award ID; source maps alone do not prove persistence. | High / before first play |
| GATE-006 | Tutorial boss defeat recovery, burn/faint, retry and HP refill require distinct sim/save behavior; cannot be implemented as merely a UI button. | High / battle proof |
| GATE-007 | Quest A→B→C forest puzzle vs ON/OFF/ON ruin puzzle must have separate puzzle schema and persisted states. UI's single `SCR_PUZZLE` screen must be parameterized without mixing success conditions. | High / before Q004/Q005 |
| GATE-008 | `PORTAL_Z03_Z04` and return portals may point to spawn tiles near another return portal. Portals are **explicit interact + cooldown**, never automatically triggered on teleport. | High / before zone switch |
| GATE-009 | Battle data is PROPOSED, not playtested. Existing arithmetic vectors prove only example math, not overall win balance, softlock-free all-starter gameplay or server determinism. | Medium / before balancing lock |
| GATE-010 | Companion follows player via proposed FSM, but exact navigation radius/speed/teleport fallback remains unknown until renderer/collision world units are chosen. | Medium / physics proof |
| GATE-011 | First-person HUD and UI screen contract reference normalized rectangle coordinates; actual 48dp touch target and notches require Android device testing on S10+. | Medium / mobile proof |
| GATE-012 | Offline companions may project visually online without stats; the online character still requires a server-verified actual creature. Onboarding flow is undecided. | High / co-op implementation |
| GATE-013 | Only `WS-004`, `WS-008`, `WS-050`, `WS-024` need source alpha assets for initial proof. Original image has no transparency or editable layers; art export contract must be approved. | High / before owned sprite test |
| GATE-014 | User's 2GB Android RAM goal cannot be confirmed by testing Galaxy S10+; actual low-memory hardware baseline needed before minimum specs. | Medium / release claim |

## 5. Planned test evidence, not yet executed

- **NAV-001:** BFS/map connectivity from each spawn to each required metadata node, with type-safe entity IDs and valid portal destinations. Preliminary draft-grid connectivity was checked earlier; future build must run its own tests.
- **NAV-002:** Quest-state graph: from fresh save, traverse Q001–Q005 with each starter, all 8 relevant flags in order, no unearned re-awards on revisit.
- **NAV-003:** Mandatory guardian cannot be bypassed with wall corner sliding, jumping (if later included), backtracking or scene reload.
- **BATTLE-001:** Deterministic replay from same level/species/move/seed, including exact tie and status-edge tests; every chosen starter can overcome Lv3 guardian.
- **SAVE-001:** Kill/interrupt app on every portal/starter/puzzle/reward atomic checkpoint, recover exactly once and with correct local creature count.
- **UI-001:** Android native 16:9/20:9 camera/look, text scaling, touch buttons and notched display; Windows keyboard/mouse and Alt+Tab.
- **ART-001:** Source 850×832 file SHA preserved; extracted alpha sprites align with 100 WS IDs; no lime/white line edge contamination in four pilots.
- **PLAT-001:** Win11 owner i3/4GB, S10+ actual, then x64 Win7/8/8.1/10 compatibility checks and a real 2GB Android if claimed.
- **NET-001:** Android↔Windows co-op, authoritative stats, no local save stat import, optional appearance-only projection when authorized.

All test outcomes are **NOT RUN** and not approvals to implement them today.

## 6. Review gate and next specification tasks

1. Approve exact four pilot-creature source appearance/cropping contract.
2. Review three HERO-Z01/Z03/Z04 environmental compositions using a future owned-art contact sheet / sketches; no external reference copy.
3. Confirm device test availability/legacy distribution; record whether Win7/8 x64 VM/licenses can be accessed.
4. Close GATE-002, -003, -005, -007 and -008 as formal quest/collision/save task spec after renderer geometry decision.
5. Freeze one baseline content manifest revision for future ENGINE-001, without changing any core rules while benchmark is running.
6. Owner explicitly approves a **small** coding experiment separately when planning checks are met.

**No production game code, sprite extraction, engine test or multiplayer backend has been executed in this milestone.**
