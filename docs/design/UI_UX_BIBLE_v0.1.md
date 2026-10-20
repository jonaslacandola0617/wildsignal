# WILDSIGNAL — UI/UX Design Bible v0.1

**Status:** PROPOSED UI architecture; requires creative review. **Date:** 2026-10-20.  
**Confirmed premises:** Offline-first adventure; optional online multiplayer; first-person retro pixel-art exploration; persistent visible companions; turn-based 1v1 creature combat; Windows Steam + Android Google Play; low-end device target.  
**Related sources:** GAMEPLAY_SYSTEMS_v0.1.md, BATTLE_BALANCE_BIBLE_v0.1.md, FIRST_NINE_CREATURES_v0.1.json, BONDING_ENCOUNTERS_v0.1.md, CHARACTER_NPC_BIBLE_v0.1.md, OFFLINE_ONLINE_SYSTEMS_v0.1.md.  
**Structured screens:** `UI_SCREEN_CONTRACTS_v0.1.json`; **Visual wireframes:** `UI_WIREFRAMES_v0.1.md`.  
**Rule for Codex:** Written + structured layouts are design drafts, not permission to invent missing systems or ship unapproved visual assets.

## 1. Experience principles

1. **The world remains the hero.** During exploration, HUD uses <15% of viewport area at rest as a *composition intention*, not a fixed technical number. Screen center stays clear of permanent hands, virtual sticks, popups and verbose quests. The Signal Compass appears when activated, not permanently as a Doom-style firearm.
2. **Pixel-world / readable UI.** Render the world at an experimental 320×180 / 400×225 / 480×270 internal resolution; draw UI separately at crisp device resolution. Pixel-inspired frame ornaments are fine; micro-pixel fonts are not.
3. **Parity, not identical inputs.** PC uses mouse + keyboard; mobile landscape uses left virtual stick and right look-drag plus contextual action cluster. All game actions have one canonical input ID independent of hardware.
4. **Offline by default.** The first UI action is **Continue Local Adventure** / **New Local Adventure**; no account prompt, online advertisement, storefront or network dependency. Online is a separate opt-in destination and never silently alters local progress.
5. **Low cognitive load.** One primary focus/interaction at a time. Important clues re-readable in Journal. In combat, all four moves and their energy cost visible; invalid actions explain why they cannot be used.
6. **Consistent original iconography.** Original user-owned creature pixel sprites; geometric signal marks, rings, 3-bar glyphs and lantern gold. No borrowed Pokémon-style capture balls, battle/menu layouts, Nintendo franchise visual motifs or weapon/ammo HUD.
7. **Accessible feedback.** Every information-bearing color has an icon/text, critical alerts have silent visual equivalents, battle animation never gates turn math, reduced motion and adjustable text scale included.

## 2. Mode and navigation model

### Main root
```
APP BOOT → LOCAL MAIN MENU
  ├─ Continue Local Adventure → (save validation) → Exploration
  ├─ New Local Adventure → Avatar Creator → Opening Q001 → Starter Choice → Exploration
  ├─ Save Slots → choose/backup/rename (destructive confirmation)
  ├─ Online Expedition → (optional account auth) → Online Home → Lobby → Online zone
  ├─ Options → Settings
  └─ Credits/Legal
```

### Local gameplay
```
Exploration HUD
  ├─ [Interact] → Dialogue / Object panel / Wild observation / Puzzle
  ├─ [Compass] → Compass overlay (hold/toggle configurable)
  ├─ [Companion] → Compact companion command overlay
  ├─ [Encounter] → Battle select → Battle resolution/results → Exploration
  ├─ [Journal] → Quest journal + Creature Fieldbook
  ├─ [Party] → Companion details / rearrange
  └─ [Pause] → Resume / Save / Party / Journal / Settings / Main Menu
```

### Online
```
Online Home → Friends/Join Code → Lobby → Shared Zone → Return Online Home
             ├─ later Trade UI [locked until backend]
             └─ later Cosmetic Store [locked until billing backend]
```
Offline and online have distinct saved ownership domains. The UI never says "Sync all offline creatures to ranked/trade", as this is not approved and would be insecure. It must visibly label **Local Adventure** vs **Online Expedition**.

## 3. Screen classification and source of truth

The separate structured contract lists `screenId`, phase (`vertical_slice`/`online_proof`/`later`), state requirements, normalized component rectangles and transition IDs. Names must be used consistently:
- `SCR_BOOT`, `SCR_MAIN_MENU`, `SCR_SAVE_SLOTS`, `SCR_AVATAR_CREATE`, `SCR_STARTER_PICK`.
- `SCR_EXPLORE`, `SCR_COMPASS`, `SCR_DIALOGUE`, `SCR_PUZZLE`, `SCR_OBSERVE`, `SCR_BOND`.
- `SCR_BATTLE_SELECT`, `SCR_BATTLE_PLAYBACK`, `SCR_BATTLE_RESULT`.
- `SCR_JOURNAL`, `SCR_FIELD_BOOK`, `SCR_PARTY`, `SCR_INVENTORY`, `SCR_WORLD_MAP`, `SCR_PAUSE`, `SCR_SETTINGS`.
- `SCR_ONLINE_HOME`, `SCR_ONLINE_LOBBY`, `SCR_TRADE`, `SCR_COSMETIC_STORE` (phased).
- `MODAL_SAVE_WARNING`, `MODAL_DISCONNECT`, `MODAL_ERROR`, `MODAL_CONFIRM`, `TOAST_AUTOSAVE` (overlay components, not root screens).

**State-driven UI:** no root screen invents game truth. The UI reads an immutable view model of local battle/world or server-authoritative online session state, submits typed action IDs, and waits for transition / validation results. The UI cannot award species, alter HP, modify trade currency, change save flags, or mark purchases owned by itself.

## 4. Layout and responsive design contract

- Layout reference space: **1000×562 normalized canvas** as a 16:9 **safe-content region** (not literal pixel resolution). Rect values `[x,y,w,h]` are integer logical units in this canvas. Renderer scales and re-anchors to actual screen's inset-safe rectangle. Avoid direct hard-coded screen pixel offsets.
- Target layout profiles:
  - PC wide landscape 16:9; mouse pointer, keyboard focus ring, optional controller later.
  - Android landscape 16:9 to 20:9; account for notches, rounded corners, system gesture insets; use device safe area, not whole physical width.
  - Very narrow window / small phones: central UI panels become scrollable vertically and reduce decorative ornaments before shrinking interaction controls. Gameplay orientation recommendation: **landscape**.
- UI minimum touch target: design around **48×48 Android dp** for primary controls, with extra gap around neighboring targets. Normalized mockups must be validated at actual density: unit rectangles do not guarantee dp min.
- Primary body text: aim for **16 sp equivalent** at default, scaling to 125%/150%; smaller decorative text cannot carry critical information. Pixel art aesthetics must not sacrifice legibility.
- Text reflow/localization: support 30–50% length expansion in buttons/labels and non-English names; avoid baked-in text textures.
- Safe-content bottom corners are reserved for touch motion/look controls in exploration only. Fullscreen modals disable analog motion and lock look input unless explicitly stated.
- PC gamepad mapping not locked for first engineering proof; future controller navigation should work through same focus/event abstraction.
- HUD transparency/opacity adjustable. Keep center 40%×45% world image unobscured at rest (design check, except context prompt when aimed at object).

## 5. Visual tokens (proposal)

Palette reuse, per Visual Bible:
- `ui.bg.deep` #183B41, `ui.panel` #243B40 (approx), `ui.text` #F6EBD7, `ui.focus` #E8C878, `ui.signal` #74D6B1, `ui.danger` #E38A70, `ui.water` #4B9CC1, `ui.muted` #93A8A2.
- Small box trims echo chiseled stone and brass joints; no skeuomorphic literal pokéball circles.
- Typography: one bold heading face and one highly legible body face, with documented redistribution/commercial embedding license. Candidate selection **not made**.
- Motion: 100–220ms open/close fades/slide as proposals; reduced-motion removes scale/pulse. Save feedback short and unobtrusive; no endless spinners in offline menus.
- Icons: 16–24 source pixel symbols for affinity + independent shape/label; UI export may scale them. Affinity colors alone never indicate relationship.
- Button states: `normal, hovered, focused, pressed, disabled, loading`; enabled/destructive action must be clearly distinguished. Online-only controls never appear usable when offline.

## 6. Exploration HUD — layouts and behavior

**At-rest composition:** top-left compact location + quest title (single line or two), top-right companion portrait, health and bond alert; near-bottom-right compass/interact action *only when relevant*; reticle at center subtle; minimal mini-map by default (optional compass strip/waypoints). Keyboard FPS lock cursor while exploration is active; tab/pause releases cursor.

### Input (proposed baseline)
| Action ID | PC default | Android landscape | State |
|---|---|---|---|
| MOVE | WASD | bottom-left virtual joystick | explore only |
| LOOK | mouse drag/mouse look | drag right half excluding buttons | explore only |
| INTERACT | E | context action at lower-right | when valid target |
| COMPASS | Q | right edge compass icon | exploration / puzzle |
| COMPANION_CMD | C | right companion icon | exploration |
| JOURNAL | J | top-left objective panel / pause | explore/pause |
| PARTY | P | companion portrait / pause | explore/pause |
| MAP | M | pause > map | explore/pause, after map unlocked |
| PAUSE | Esc | top-right gear/pause | all local scenes except noninterruptible atomic save |
| MENU_ACCEPT | Enter/Space/LMB | tap focused | menus |
| MENU_BACK | Esc/RMB | visible back or Android back | menus |
| BATTLE_MOVES | keys 1–4 or arrows + Enter | large four move cards | battle select |

No mandatory double-tap, swipe-only, tiny drag handle, long-hold or precise timing inputs; all essential actions always have tap and keyboard equivalents.

### Focus/interaction
- Ray/target selection only when in range and unobstructed; show a context label `[E] Speak to Mara` / `[tap] Inspect Singing Root`.
- If multiple nearby targets, choose closest to center, then by distance, then stable ID; do not randomly change selection when standing still.
- Pointer sensitivity, invert Y and motion reduction adjustable.
- When dialogue opens: stop player motion, zero joystick vector, release mouse capture on PC, and show an explicit close/back action.
- Q001 player cannot scan before receiving Compass; button disabled with "Receive a compass from Fen" hint, not a misleading functional icon.
- When no companion is assigned: party icon shows "No companion yet" without breaking the menu.

## 7. Starter choice, fieldbook and party UX

### Starter pick
- Three cards with **the user's approved source sprites**, not generated approximations; names Glintle / Driftfin / Brumlet proposed, IDs WS-004/008/050 stable.
- Each card includes affinity icon+name, a concise personality, battle identity and exploration assistance. No hidden numerical advantage claims.
- Selection opens detailed preview; `Confirm companion` requires a second explicit confirmation stating this choice is permanent **for this local save**. No purchases or online account required.
- Atomic save transaction grants exactly one starter and Q002 flag; interrupted transaction recovers without duplicating or losing a starter.
- For a phone, allow horizontal cards with a visible selected card and no microscopic side text; switch cards through tabs/swipe + arrow buttons.

### Creature Fieldbook
- Unseen species: silhouette/unknown entry with habitat clue only if unlocked, not reveal entire secret roster. Observed: sprite, ecology and behavior. Befriended: full entry, stats and companion relationship.
- All 100 art IDs can appear in data, but visible species list may be filtered to released/encounterable set. Do not show unfinished 91 species as unlockable placeholders in first playable.
- Fieldbook separate from owned Party. Search and affinity filters are later optional features.

### Party + reserve
- Active companion slot one; party max three **proposed**. If invite succeeds when party full, send creature to reserve with clear confirmation. Do not silently discard/release.
- Show HP current/max, bond state, level, affinity, four equipped moves, current active status; swapping in world does not cost real money.
- For first playable with only one selected starter, reserve UI can be stubbed if no recruitable wild creatures are implemented yet; invitation cannot be enabled without reserve storage working.

## 8. Battle UX — no inherited franchise menu

### Main battle composition
- Background references the current zone's environment through a simplified, low-cost view; enemy creature front sprite reads large in upper-middle/right. Player companion front/side sprite visible near lower-left or as a card; no need for high-poly models.
- Enemy HP/name/status in upper-left or near enemy. Player HP/name/status in lower-mid-left. Signal Energy displayed as **0–8 bar AND number**, move costs obvious.
- Bottom dock has four large move cards in a 2×2 grid for landscape. Each shows move, affinity, cost, power/effect and state. Card select does not execute immediately if confirmation is needed for risky actions; regular move one tap + clear active processing is acceptable once playtested.
- Secondary actions: Party/Swap, Item (offline only if rules allow), Retreat (wild/offline only), Help and Pause. Disable by mode without misleading error.
- `SELECT_ACTION` no timed pressure in offline game. `LOCK/PLAYBACK` disables cards, resolves deterministic engine events, supports speed-up and reduced motion; cannot double-submit same command.
- Field results show faint/heal/status and HP updates driven entirely from simulation output. Combat math occurs once in engine/server; animation doesn't influence timing.
- `BATTLE_RESULT`: success/failure, bond event and one-time reward/quest progression; return to local exact checkpoint; accessible Retry if Q005 guardian defeated player.

### UI state ↔ engine ownership
| UI state | Engine source | Legal actions | Forbidden |
|---|---|---|---|
| select | local battle view model/server turn notification | inspect move, submit valid command | local HP writes |
| locked | command accepted | inspect status; wait | resubmit command |
| playback | deterministic event stream | skip cosmetic playback | manual damage |
| result | signed/authoritative outcome or local simulation outcome | continue, inspect log, retry if permitted | award twice |
| error | command rejected or network issue | choose again where safe; return menu | pretend success |

Combat costs and status durations come from typed FIRST_NINE_CREATURES.v0.1 data and Battle Bible, not UI literals. e.g. Glintle Lantern Grove energy cost 3, +1 move priority, guard/regeneration; Driftfin Slipstream 2; Brumlet Cinder Beat 2. These are proposals, not final balance.

## 9. Wild observe / bonding and puzzle UI

- Wild encounter: small observation card overlays world without forcing battle immediately; choices **Observe**, **Help**, **Friendly Spar** and **Leave**. Locked actions display explicit reason. Trust meter e.g. 0–5 *proposed*, must not show automatic invitation before actual trustworthy state update.
- Inviting after trust threshold asks `Invite <species> to join?`; success gives local-only companion and a reserve/party destination. Item and unique award must be idempotent.
- Companion puzzle panel: 3 distinct glyph controls A, B, C with geometric symbols and numbered labels; clue available through caption, icon and gentle sound, independent of starter selected. Wrong order resets local puzzle progress with nonpunitive feedback; no timed taps.
- Echo Hollow second puzzle (proposed): three toggle sigils target ON/OFF/ON, with distinct pressed/latched states. Do not reuse the forest's A→B→C sequencing as if the puzzles are the same.
- Quest-critical hints stored in Journal under **Seen Clues** so players can resume days later without guessing.

## 10. Local saves, connectivity and main menu

- Boot / local main menu never blocks waiting for a sign-in handshake. Initial menu must show `Continue Local Adventure` only if valid local save; otherwise primary action `New Local Adventure`.
- `Save Slots`: proposed 3 local slots, but number is not approved and should remain configurable. List created/last played, location, chapter; require typed/held confirmation for deleting, and support backup restore after corruption. Do not silently overwrite.
- `SAVE` feedback: short “Saved locally” label; don't claim “Cloud synced” without backend acknowledgement. UI reflects saving/busy/failed/recovered states.
- `Online Expedition` requires internet and opt-in sign-in. Offline state disables button with `Connect to play online` explanatory copy, but does not disable local menu.
- **Separate trustworthy progression** is a proposed architecture; UI must not promise copy/upload of offline creatures to ranked/trading. Online inventory, rewards and entitlements come from backend. Online cosmetic marketplace is not present in prototype and cannot be a placeholder that routes to an actual payment screen without store compliance.
- On disconnection: show retry/exit online options; local adventure remains intact, never silently convert server progress into local save. Leave online screen to local menu only on explicit command.

## 11. Inventory, journal, map and settings

### Inventory
- Tabs: Field Tools (Resonance Compass and quest items), Supplies (future recovery), Key Findings (nonconsumable clues). No purchasable currency until economy spec; quantity formatting human-readable.
- Tooltips include description, whether item can be used/offered/discarded, and source if appropriate. Quest Compass cannot be discarded.

### Journal
- Active objective first with one clear next step. Completed quests and Seen Clues tabs. NPC dialogue remains revisit-able. Quest flags are engine-owned, read-only here.
- User can hide quest tracker while exploring; journal itself always accessible.

### Map
- Early version: authored stylized schematic rather than expensive live 3D minimap; mark visited entrances, exits and checkpoints without revealing unexplored secret spaces. Orientation uses north arrow and current zone. Accessible from pause via key M.

### Settings
- Graphics: render scale / pixel filtering (nearest default), fps cap (30/60 proposal), brightness, fog reduction if available; avoid quality options promising non-supported renderer features.
- Input: sensitivity, left/right joystick sizes, virtual button reposition toggle (optional after prototype), mouse invert Y, keyboard remap.
- Audio: music/SFX/ambience sliders and mute.
- Accessibility: text scale, contrast, subtitle/text blips, reduced motion, screen shake off, hold-to-toggle compass, color-blind legibility.
- Account/privacy: online-only; no sign-in required to access local settings.
- Confirm settings on Apply and support Restore Defaults. Store per-device (not authoritative online character state).

## 12. UI resource/memory discipline

- Load root screens from shared icon atlases, not a unique high-resolution background per menu.
- UI transition logic is data-driven; no dozens of heavy native screens hidden in memory.
- UI should not decompress 100 creature sprites at game boot. Load active species and visible Fieldbook entries on demand; unload distant screens when memory constrained.
- Text input, IME and Android back handling are platform-adapted but feed the same UI action contract.
- Avoid animation of entire screen surfaces on each frame; animate only changed transforms/opacity layers where feasible.
- GPU-optional renderer is undecided; UI composition path must be prototyped on CPU-only PC and Android native targets before choosing font-rendering strategy.
- Record frame time, memory, UI input latency, touch missed taps and dialog reflow on real low-end devices; no unbenchmarked 2GB compatibility claim.

## 13. Accessibility and privacy rules

- Touch target minimum approximate 48 dp, with 8 dp proposed gap; test device-specific safe areas.
- Fully keyboard-navigable menus with visible focus and meaningful tab order. No swipe-only story step.
- High contrast in ruins: critical text never blended into glowing green environmental background. Always support text size increases.
- Action feedback via visible labels/icons, sound and optional vibration (off possible).
- Offline gameplay sends no telemetry without clear opt-in; online friend names/IDs filtered for moderation and privacy before launch. User-generated names need a report/block tool in multiplayer before public distribution.
- Notches, status bars, Android back button and Windows Alt+Tab must not trigger accidental combat commands.
- Online trade confirmation must spell out item ownership and cannot be bypassed by rapid double-tap.

## 14. Explicit questions requiring owner signoff

1. Approve **minimal exploration HUD**, compass emphasis and no default minimap.
2. Confirm landscape-only mobile gameplay (currently recommended but not final).
3. Choose **full offline story + separate verified online progression** or limited guest offline companions (architectural decision still open).
4. Approve **3 creature party**, 4 moves and 0–8 Signal Energy for initial game.
5. Approve binding/accessibility defaults and art-font candidates after visual tests.
6. Decide whether wild encounters use **trust invitation + optional spar** or traditional battle/capture mix.
7. Specify first release multiplayer: online lobby only vs visible roaming players/duels/trading; no premature storefront implementation.
8. Approve a maximum of **3** local save slots (proposed), and separate device settings profiles.
9. Finalize user-facing terminology for statuses and affinities.
10. Approve exact UI geometry and actual screenshot comps after prototype target-device validation.

## 15. Codex-ready task boundaries (after approval)

- `UI-001` platform-neutral UI state router and typed event bus; unit-test screen transitions in offline mode.
- `UI-002` standardized safe-area layout engine and touch + mouse input mapping; verify at 16:9 and 20:9; no gameplay simulation.
- `UI-003` exploration HUD, focus/interact prompt, compass overlay; uses read-only world model and fake fixture.
- `UI-004` dialogue, journal, starter choice and accessibility settings; no save sync to online.
- `UI-005` battle select/resolution/result driven by deterministic battle fixture; rejects unaffordable or duplicated commands.
- `UI-006` party, Fieldbook, observation and invite flow only after local creature ownership/bond mechanics are approved.
- `UI-007` optional online UI states; must be gated by tested backend and separate entitlement model.

**Task packet mandatory:** exact actions, accessibility checks, visual screenshot on both platforms, offline airplane-mode test, performance evidence and regression test logs. No storefront purchasing in first playable.
