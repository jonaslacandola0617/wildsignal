# Chapter 5 — UI and controls

**Status:** DRAFT 2026-11-02. Drafted by Claude from chapters 1–4 and the existing pilot UI. Choices are in 5.9.
**Built on:** chapters 1–4 (locked). Existing pieces: the paper HUD skin, Mara's animated field book, the pause menu, fonts m5x7 and m6x11, 640 × 360 UI layout in `experiments/hearthgrove_emaceart`.

## 5.1 Principles

1. **The world is the star.** While exploring, the HUD stays small and appears only when relevant. No minimap on screen; the Resonance Compass and landmarks guide you.
2. **A Wayfinder's field kit.** Menus look like Mara's paper journal and a traveller's kit: paper panels, ink, stamps, bookmarks. It's cozy and already partly built.
3. **Crisp at any size.** UI is laid out on a 640 × 360 grid and scaled in whole pixels to the screen, separately from the world's render size (F6).
4. **Read at a glance in fights.** Big shapes, colour plus icon for every type (never colour alone), cooldowns shown as a sweep on the button.
5. **Short words.** Sentence case, short lines, verbs on buttons ("Rest", "Tune in", "Build").

## 5.2 Visual style

- **Panels and buttons:** Humble Pixel *Paper UI System* (commercial use allowed, modification allowed, no redistribution: the files stay in the private art repo; credit "@Humblepixel" given anyway).
- **Fonts:** m5x7 for body text and m6x11 for titles, by Daniel Linssen (credit required; confirm commercial use with the author before release — see 5.9).
- **Type icons:** eight simple shapes, each with its own colour (leaf, drop, flame, rock, swirl, star, crescent, paw), used everywhere a type appears.
- **Palette:** warm paper, ink brown, lantern gold for highlights and the recruit line, coral for danger, teal for calm.

## 5.3 Screens in 1.0

| Screen | Purpose |
|---|---|
| Title | Continue, new game, settings, credits |
| Save slots | 3 slots, each showing Wayfinder name, region, team and play time |
| Wayfinder creation | Name, a few looks, pronouns for dialogue (he / she / they) |
| Exploration HUD | See 5.4 |
| Fight HUD | See 5.5 |
| Tune In | The compass ring closing around the Signari, then the "joined" ribbon and stamp |
| Dialogue | Speaker name, portrait, two-line box, tone choices for the Wayfinder |
| Field book (B) | Signari log (seen / fought / recruited, habitat, type, Mara's notes), quests, station echoes |
| Team (Tab outside fights) | The 3 team Signari and the reserve; swap at rest spots |
| Signari detail | Stats, type, kit with skill ranks, level, field ability |
| Bag (I) | Healing items, materials, tools, key items |
| Map (M) | Region map with discovered places, rest spots, homestead sites, gates; no live minimap |
| Shop | Buy and sell; one per village |
| Rest spot | Heal, change team, save, move the homestead (at homestead sites) |
| Homestead build mode | Place, move and remove buildings, plots and decorations on the homestead grid |
| Crafting bench | Recipes, materials, craft |
| Pause and settings | Continue, settings, controls, save and quit |
| Settings | Display (render size, focus, window), audio, controls (rebinding), accessibility (5.7), language |
| Credits | |

## 5.4 Exploration HUD

- **Top left, briefly:** region name when you enter a region.
- **Bottom left:** small portraits of the 3 team Signari with health rings.
- **Bottom right:** the current prompt ("E Rest", "F Fight", "Q Pulse") and the compass icon with its cooldown.
- **Centre bottom, briefly:** notes and toasts (one at a time).
- **Over wild Signari:** name and level when close; the "!" when frightened.
- Opening any menu hides the HUD.

## 5.5 Fight HUD

- **Top right:** the wild Signari's plate: name, level, type icon, health bar with the **recruit line**, and its calm meter.
- **Bottom left:** your active Signari's plate, the Wayfinder's composure, and the two Signari waiting to swap in.
- **Bottom centre:** the skill bar: basic, skill 1–3 (each with a cooldown sweep), and the ultimate, ringed by the **Resonance meter**.
- **On the ground:** attack telegraphs filling up during wind-ups; the 9.5 m ring.
- **Floating:** damage numbers, "Strong!" / "Resisted", "Worn down".
- **Tune In:** appears as a big prompt when the Signari is worn down and you're close enough.

## 5.6 Controls

The same action always sits on the same key across exploring and fighting where possible. All keys can be rebound.

| Action | Keyboard and mouse | Controller | Touch (Android) |
|---|---|---|---|
| Move | WASD | Left stick | Left virtual stick |
| Camera | Mouse | Right stick | Drag on the right half |
| Interact / talk | E | A | Context button |
| Sneak | Hold Shift | Hold L3 | Toggle button |
| Compass pulse | Q | Y | Compass button |
| Start a fight | F | X | Context button |
| Field book | B | View | Menu |
| Team | Tab | D-pad up | Menu |
| Bag | I | D-pad left | Menu |
| Map | M | D-pad down | Menu |
| Pause | Esc | Menu | Pause button |
| **In fights:** basic attack | Left click or 1 | X | Big attack button |
| Skill 1 / 2 / 3 | 2 / 3 / 4 (or Q / E / R style via rebinding) | Y / B / RB | Three skill buttons |
| Ultimate | R | RT | Ultimate button (glows when ready) |
| Signari dodge | C | LB | Dodge button |
| Wayfinder roll | Space | A | Roll button |
| Swap Signari | Tab, or click a portrait | D-pad left / right | Tap a portrait |
| Tune In | Hold F | Hold X | Hold the Tune In button |

This replaces the pilot's fight keys (1–5 moves, Q dodge, R reach out). In fights Q moves from Signari dodge to unused, so the compass key never fires by accident.

## 5.7 Accessibility

- Text size: normal, large, larger.
- Assist options from chapter 2: slower fights, longer telegraphs, auto-dodge for the Wayfinder.
- Hold or toggle for every held action (sneak, Tune In).
- Reduced motion (book animations, screen shake, focus blur off).
- Colour-blind safe: every type has its own shape; danger is shown by shape and flashing, not only red.
- Full key and button rebinding.

## 5.8 Localisation

1.0 ships in **English**. All text lives in string tables from the start, so other languages can be added later without code changes. The pixel fonts need replacing or extending for non-Latin scripts.

## 5.9 Owner choices

| # | Question | Recommendation |
|---|---|---|
| U1 | The field-kit paper look for menus and HUD (5.1–5.2)? | Yes |
| U2 | The screen list for 1.0 (5.3)? | As listed |
| U3 | The control scheme (5.6)? | As listed, fully rebindable |
| U4 | No minimap; a full map screen only (5.1, 5.3)? | Yes |
| U5 | English only at launch, built ready for translation (5.8)? | Yes |
| U6 | Controller support at 1.0 and Android touch controls (5.6)? | Yes, both |
| U7 | Before release, confirm m6x11's commercial terms with Daniel Linssen, or swap fonts? | Confirm with the author |

## Change log

- 2026-11-02 — First draft.
