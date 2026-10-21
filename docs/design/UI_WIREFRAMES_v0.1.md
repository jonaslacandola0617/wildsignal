# WILDSIGNAL — Annotated UI Wireframes v0.1

**Status:** DESIGN DRAFTS, not finished art. **Built from:** `UI_SCREEN_CONTRACTS_v0.1.json` using normalized 1000×562 **safe-content** coordinates as screen intent, NOT absolute device pixel counts.  
**Core visual direction:** retro first-person textured worlds (owner's environment reference) with crisp pixel-era panels and user-owned creature pixel sprites. The UI below intentionally does not copy weapon/ammo HUDs or creature-battle screens from another franchise.

## 1. Main menu — screenshot target UI-01

```text
┌──────────────────────────────────────────────────────────────┐
│                                                              │
│                         W I L D S I G N A L                  │
│                   Hear the wild. Discover the unknown.       │
│                                                              │
│                  ┌─────────────────────────┐                 │
│                  │ CONTINUE LOCAL ADVENTURE│ (if save valid) │
│                  └─────────────────────────┘                 │
│                  ┌─────────────────────────┐                 │
│                  │   NEW LOCAL ADVENTURE   │                 │
│                  └─────────────────────────┘                 │
│                  ┌─────────────────────────┐                 │
│                  │        SAVE SLOTS       │                 │
│                  └─────────────────────────┘                 │
│                  ┌─────────────────────────┐                 │
│                  │   ONLINE EXPEDITION     │ (opt-in)        │
│                  └─────────────────────────┘                 │
│                      OPTIONS      CREDITS                   │
└──────────────────────────────────────────────────────────────┘
```

**Hero background:** tranquil dusk harbor vignette with gentle lantern lights, subtle painterly/pixel panels; could be low-cost static background if renderer load is expensive. Background decorative only. No online sign-in, ad, or shop on entry. With no save: main action New Local Adventure; Continue visibly disabled or hidden with no false action. Online disabled when network unavailable; offline story still works. Every menu action keyboard-focused on PC and large enough for touch.

## 2. Landscape first-person exploration — screenshot target UI-02

```text
┌──────────────────────────────────────────────────────────────┐
│ LILTWOOD — Follow the strange signal          [Glintle ♡] ⚙│
│                                                              │
│                [WORLD: FOREST / ROOTS / RUINS]               │
│                        (reticle +)                          │
│                                                              │
│                 ... Singing Root ...                         │
│                                                              │
│        [STICK]                          [companion] [interact]│
│       MOVEMENT                             [COMPASS]         │
└──────────────────────────────────────────────────────────────┘
```

A creature companion is a **world sprite behind/to the side of the camera**, not a permanently fixed HUD mascot blocking view. When no valid target is ahead, the central interaction prompt disappears. Mobile stick is bottom-left; look-drag uses uncluttered right half; context/compass/companion buttons are bottom-right. On Windows these touch-only controls hide and an `E` prompt appears near reticle. Pause remains visible/tappable. Floating objectives collapse after an interaction; opening Journal expands details. Do not draw a large minimap by default. First-person compass appears only when in use.

## 3. Main interaction dialogue — screenshot target UI-03
```text
┌──────────────────────────────────────────────────────────────┐
│                                  X Close                     │
│                BACKGROUND WORLD (dimmed)                      │
│                                                              │
│                                                              │
│         ┌──────────┐ ┌────────────────────────────────────┐   │
│         │  MARA    │ │ Mara Venn                          │   │
│         │ portrait │ │ "The forest has been quiet..."    │   │
│         └──────────┘ │                    [Continue ▶]  │   │
│                      └────────────────────────────────────┘   │
└──────────────────────────────────────────────────────────────┘
```
Speaker art and text separate layers; crisp and large enough on mobile. Esc/Android back closes safely; conversation remains replayable. "Continue" does not award quests until model returns completion.

## 4. Starter selection — screenshot target UI-04

```text
┌──────────────────────────────────────────────────────────────┐
│ Choose Your First Companion                                   │
│                                                              │
│  ┌────────────────┐ ┌────────────────┐ ┌────────────────┐    │
│  │ owned sprite   │ │ owned sprite   │ │ owned sprite   │    │
│  │ GLINTLE        │ │ DRIFTFIN       │ │ BRUMLET        │    │
│  │ Flora          │ │ Tide           │ │ Ember          │    │
│  │ Guardian       │ │ Fast control   │ │ Rhythmic burst │    │
│  └────────────────┘ └────────────────┘ └────────────────┘    │
│                                                              │
│  Selected companion's ecological skill...  [INVITE / CONFIRM]│
└──────────────────────────────────────────────────────────────┘
```
Cards reference owner-owned WS-004, WS-008, WS-050 art. Selection reveals details then confirmation modal `MODAL_CONFIRM` requires deliberate "Yes, choose <name>" and states once per local save. No RNG, money or online step.

## 5. Battle action — screenshot target UI-05

```text
┌──────────────────────────────────────────────────────────────┐
│ [Craglet  LV3  HP ▰▰▰▰▱]                                      │
│                                 ┌─────────────┐              │
│             RUIN WALL / SIGILS  │  OPPONENT   │              │
│                                 │  OWNED ART  │              │
│                                 └─────────────┘              │
│   ┌─────────┐                                                 │
│   │ COMPAN. │               [1 Sprout Dart] [2 Lantern Grove]│
│   └─────────┘               [3 Root Thread] [4 Tap]           │
│ [Glintle  HP 64/64]          Energy 4/8                      │
│ [Status icons+labels]       Party · Item · Retreat · Help     │
└──────────────────────────────────────────────────────────────┘
```

NOTE: four moves are shown in the **species data's slot order**, not necessarily the illustrative order in this hand sketch. E.g. Glintle's actual proposed slots in JSON: `TAP`, `SPROUT_DART`, `LANTERN_GROVE`, `ROOT_THREAD`. Cost/typed effect shown on card, with disabled/unaffordable state and focus outline. On entering `PLAYBACK` remove move input, animate from deterministic event list, allow `Skip FX` without changing HP, then show battle result. Battle is not time-limited offline. Dynamic status/battle formulas must come from battle simulator, never animation or UI. When all moves unaffordable, TAP remains free and usable.

## 6. Compass and signal puzzle — screenshot target UI-06

```text
┌──────────────────────────────────────────────────────────────┐
│ Signal: [▰▰▰▰▱]     The stone responds to your companion      │
│                                                              │
│                        ◇     △     ║                         │
│                      [Sigil A][Sigil B][Sigil C]             │
│                                                              │
│                  Your clue: A → B → C                          │
│                                                              │
│                         [Return to world]                    │
└──────────────────────────────────────────────────────────────┘
```

There are **two different** Chapter 0 signal puzzles. Liltwood forest Veiled Wall uses A→B→C **sequence**; Echo Hollow uses toggles to match **ON/OFF/ON**. The screen switches interaction style and descriptive clue accordingly. Never make a player press tiny objects on a complex first-person texture to solve it—use large buttons and clear icons. Companion changes the in-world reveal animation, not the underlying sequence or critical solution.

## 7. Party, Fieldbook and Journal — screenshot target UI-07

```text
┌──────────────────────────────────────────────────────────────┐
│ JOURNAL / FIELD BOOK / PARTY                                 │
│                                                              │
│ ┌─────────────────┐ ┌──────────────────────────────────────┐ │
│ │ Left: quest,    │ │ Right: selected quest's clue text,  │ │
│ │ creature list,  │ │ ecology, stats, bond status or       │ │
│ │ or party slots  │ │ illustrated sprite description.     │ │
│ │                 │ │                                      │ │
│ └─────────────────┘ └──────────────────────────────────────┘ │
│                                [Return]                      │
└──────────────────────────────────────────────────────────────┘
```

First Fieldbook screens only reveal species discovered by the player; do NOT advertise unreleased entries from all 100 source artworks. Party supports 1 active / 3 carried candidate limit. Reserve needed when a wild invitation could overflow party, otherwise don't enable capture/recruitment with nowhere safe to store new creature.

## 8. Save / options — screenshot target UI-08
```text
┌──────────────────────────────────────────────────────────────┐
│ PAUSE                                                        │
│                                                              │
│                       Resume                                 │
│                       Companions                              │
│                       Journal                                 │
│                       Backpack                                │
│                       Save locally                            │
│                    Options   Main menu                        │
│                                                              │
│              [Saved locally • timestamp]                     │
└──────────────────────────────────────────────────────────────┘
```

A local save ack is shown only after disk write success and the engine verifies it. No "cloud synced" wording without an authenticated server response. Offline pause always stops local battle/world while online UI can stop local input but cannot pause the authoritative server.

## 9. Optional online home and lobby — screenshot target UI-09

```text
┌──────────────────────────────────────────────────────────────┐
│ ONLINE EXPEDITION                                            │
│                                                              │
│        Account: [signed out / server verified]               │
│                                                              │
│        [Friends]      [Join Code]       [Host Lobby]          │
│                                                              │
│        Offline companions are in LOCAL ADVENTURE.            │
│        Only server-verified online creatures can be traded.  │
│                                                              │
│                                     [Back to local menu]     │
└──────────────────────────────────────────────────────────────┘
```

This design is **tentative** because online/offline progression sharing is not yet approved. Show informative status in a non-alarming tone. Future Trade and Store screens may exist as design-only concepts; no billing or item transfer until backend and product policies are approved.

## 10. Per-screen blueprint index (26 screen contracts)

| Phase | Screens | Scope |
|---|---|---|
| Vertical slice | BOOT, MAIN_MENU, SAVE_SLOTS, AVATAR_CREATE, STARTER_PICK | local onboarding |
| Vertical slice | EXPLORE, COMPASS, DIALOGUE, PUZZLE, OBSERVE, BOND | world interactions |
| Vertical slice | BATTLE_SELECT, BATTLE_PLAYBACK, BATTLE_RESULT | deterministic combat |
| Vertical slice | JOURNAL, FIELD_BOOK, PARTY, INVENTORY, WORLD_MAP, PAUSE, SETTINGS, CREDITS | player menus |
| Online proof | ONLINE_HOME, ONLINE_LOBBY | opt-in cross-player connection |
| Later | TRADE, COSMETIC_STORE | only with server/store implementation |

## 11. Screen review checklist

- For each root screen, verify complete task from keyboard/mouse and touchscreen; no input path hidden behind mouse hover.
- Test actual landscape devices at 16:9 and ~20:9 with display cutouts and gestures. A 1000×562 safe reference layout is not sufficient to assert 48dp controls.
- Verify 125% and 150% text without clipping and keyboard focus through every overlay.
- Default exploration has minimal obstruction; inventory and dialogue can occupy more of viewport intentionally.
- Large creature sprites are not cropped at native pixel borders and alpha background is clean.
- All buttons have disabled/loading/error states and distinct action IDs in UI_SCREEN_CONTRACTS_v0.1.json.
- Review starter pick with the **real source artwork** after transparency processing; conceptual wireframes are not finished pixel art.
- Verify loss/retry, save-corruption recovery, reconnect/disconnect and no-network boot; do not call UI fully specified while trust realm policy, online API and renderer geometry remain unresolved.
