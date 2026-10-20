# WILDSIGNAL — First Signal Narrative & Quest Spec v0.1

**Status:** Proposed. All named NPCs, dialogue lines, objectives and quest numbering require creative approval. **Constraint confirmed:** fully completable offline with any starter.

## Story premise
An unexplained signal pulses beneath the archipelago after a long silence. Wayfinders study creature behavior and the ruins without treating creatures as disposable equipment. The inciting incident is a missing guiding resonance from Liltwood that leaves woodland travelers disoriented. A newly arrived Wayfinder is asked to investigate. The ancient signal network is neither inherently evil nor safe; the meaning remains mystery.

## Key characters
| ID | Name (working) | Role | Visual silhouette | Motivation | First location |
|---|---|---|---|---|---|
| NPC_001 | Mara Venn | Wayfinder guide | weathered explorer scarf, brass compass | recover missing signal without alarming town | Lantern Quay Wayfinder Hall |
| NPC_002 | Old Fen | compass technician | short figure, round lenses, lamp gloves | repair signal receiver; protect creatures | Compass Workshop |
| NPC_003 | Iri | forest caretaker | layered moss cape, orange satchel | restore waystones and forest routes | Liltwood entrance |
| NPC_004 | Bellkeeper Sol | ruins archivist (tease only) | dark indigo robe, silver ring motifs | learn truth of Echo Hollow | Echo Hollow memory inscription |

Voice acting is NOT assumed. Text/localizable dialogue, emotes, portrait and sound blips suffice.

## Chapter 0: "The First Signal" — goal 15–20 minutes (to validate)
### Q001: The Wayfinder's Compass
- Trigger: `new_game` in `Z01_SPAWN_SQUARE`; initial state `Q001_ACTIVE`.
- Objective: approach NPC_001, interact, visit NPC_002.
- Sample dialogue:
  - Mara: "The forest used to answer our lanterns. Tonight, something else is calling."
  - Fen: "Take this compass. It won't tell you where to go. It will tell you when something is listening."
- Reward: `ITEM_RESONANCE_COMPASS`; set `FLAG_HAS_COMPASS=1`.
- Fail-safe: compass can be reclaimed at Workshop if inventory is empty; quest never depends on purchased items.

### Q002: A Friend at the Gate
- Trigger: `FLAG_HAS_COMPASS` and interaction with `STARTER_ALTAR`.
- Objective: choose one of starter entries S01/S02/S03; companion obtained and enters `FOLLOW` state.
- Sample: Mara: "You don't choose a tool. You invite a friend."
- Reward: one local story starter; `FLAG_STARTER_SELECTED`. Selection is persistent, one-time per local slot; UI explicitly warns before confirm.
- Fail-safe: if save interrupts award, reconcile starter ownership and flag transactionally.

### Q003: The Missing Waystone
- Trigger: exit Quay north through `PORTAL_Z01_Z02` with starter.
- Objective: follow Mosswake route, inspect `PROP_SIGNAL_MARKER`, witness companion ALERT, talk to Iri, find `LANDMARK_SINGING_ROOT`.
- World beat: wildlife becomes still as compass hum rises near the missing stone.
- Checkpoint: `Z03_SPAWN_LANTERN_STUMP`.
- Reward: local quest progress + companion bond event (once per quest per save).

### Q004: Beyond the Veil
- Trigger: interact with `PROP_VEILED_WALL` while starter follows and `FLAG_MARKER_INSPECTED`.
- Three visually different **equivalent** solutions:
  - Glintle: luminous markings brighten, revealing sequence.
  - Driftfin: damp channel reveals the same sequence.
  - Brumlet: heated stone inlay reveals the same sequence.
- No creature-dependent main quest softlock. Interact on 3 symbol markers `A,B,C` in order; incorrect order resets only puzzle, not quest.
- Reward: `FLAG_RUIN_GATE_OPEN=1`; `PORTAL_Z03_Z04` unlocked.
- Recovery: clue accessible again any time and no single-use consumable.

### Q005: The Chamber's Answer
- Trigger: enter Echo Hollow and cross `TRIGGER_INTRO_BATTLE`.
- Encounter: one authored intro 1v1 vs nontradeable NPC-owned wild/guardian creature, creature spec TBD; player allowed retry after healing, no item-loss softlock.
- Post-battle: solve a three-switch pattern puzzle (initial A=off/B=on/C=off; target A=on/B=off/C=on, toggle any order). Set FLAG_SIGIL_PUZZLE_COMPLETE, then activate pedestal; no online partner required.
- Sample inscription: "A signal is not a command. It is a question."
- Reward: `FLAG_FIRST_SIGNAL_COMPLETE=1`; unlock fast return/return portal to Liltwood and save at Lantern Quay. Ending reveal: deeper network visible but inaccessible.
- Failure: losing battle returns to nearest checkpoint, restores companion to safe HP and preserves Q001–Q004 completion.

## Quest event truth table
| Flag | Initial | Set by | Used by |
|---|---|---|---|
| FLAG_HAS_COMPASS | false | Q001 Workshop award | Q002 unlock |
| FLAG_STARTER_SELECTED | false | atomic Q002 choice | Forest gate |
| FLAG_MARKER_INSPECTED | false | Z02 clue interaction | Q004 |
| FLAG_WAYSTONE_FOUND | false | Singing Root interaction | Q004 |
| FLAG_RUIN_GATE_OPEN | false | Q004 puzzle success | Echo Hollow portal |
| FLAG_INTRO_BATTLE_WON | false | deterministic combat result | pedestal |
| FLAG_SIGIL_PUZZLE_COMPLETE | false | Z04 post-battle A→B→C puzzle success | pedestal activation |
| FLAG_FIRST_SIGNAL_COMPLETE | false | pedestal activation (requires sigil puzzle complete) | chapter completion |

If `FLAG_WAYSTONE_FOUND` is needed by Q004, require it explicitly in the gate checker. All flags must be persisted; interactions after completion return idempotent responses.

## Story pacing and safety rules
- One brief line of dialogue per initial beat, skip/fast-forward support, no unskippable cutscene >10 seconds proposed.
- No online login or other player required in Q001–Q005.
- Save on starter acquisition, entering a new zone, opening gate and completing chapter.
- Different visual clue for each starter but exact same critical solution and equal rewards.
- Weather/time transitions are scripted atmosphere, not required to wait in real time.
- Avoid appropriating real-world sacred scripts; invent clear WILDSIGNAL sigil language.

## Pending decisions
Main-character naming/avatar choice, official NPC designs, actual quest text, introductory antagonist vs environmental mystery, starter trust vs capture acquisition mechanics, first enemy sprite from owner's sheet, healer/checkpoint system, lore reveal strength.
