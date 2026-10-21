# WILDSIGNAL — Character & NPC Design Bible v0.1

**Status:** PROPOSED CHARACTER DESIGN, **NOT approved character art**. Created 2026-10-20.  
**Known game decisions:** "Beautiful but mysterious"; first-person retro 2.5D; user-owned original creature pixel art; offline story + optional cross-platform online multiplayer; turn-based creature battle; low-end mobile + Windows PC; no heavyweight 3D art dependencies.  
**Related:** VISUAL_BIBLE_v0.1.md, NARRATIVE_QUESTS_v0.1.md, FIRST_SIGNAL_ZONE_BLUEPRINT_v0.2.md, maps/FIRST_SIGNAL_BLOCKOUT_v0.2.json, OFFLINE_ONLINE_SYSTEMS_v0.1.md.  
**Codex:** This is a creative design brief for review, not a request to implement portraits/customization/commerce yet.

## 1. Character North Star

**Wayfinders are curious field explorers, not soldiers.** The playable avatar should communicate warmth, resourcefulness and ecological curiosity while visually belonging to the player's colorful, hand-crafted creature artwork. A human should look believable at the side of a small odd-looking forest critter without either being visually overwhelmed.

Three character-language principles:
1. **Readable at 32–48-pixel visual heights on the screen.** Prioritize large silhouette zones, clear contrasting outfit blocks, asymmetrical accessories and a compact head/torso proportion. Fine facial pixels cannot be the only way to tell characters apart.
2. **Adventurer + conservationist:** maps, compasses, field notebooks, practical boots, utility bags, weathered fabric; no military armor silhouettes, bullets, gun-like tools or franchise-specific gear.
3. **Old-world warmth / signal mystery:** familiar stitched cotton, brass, wood and leather contrasted with unusual mint light, etched rings and harmonic light cues.

### Character scale relationship (proposal; confirm after renderer test)
- Avatar and NPC: human `height_units=1.0` nominal.
- Starting companions: Glintle about 0.35, Driftfin 0.30–0.4, Brumlet 0.4–0.45 of human height in apparent world scale. These ratios are NOT absolute dimensions and should be adjusted to honor owner's source artwork.
- NPC faces presented as clear portrait sprites in conversation; in-world detail is silhouette-first.
- All player/NPC sprite work uses pixel-consistent edges, no unapproved fractional rescaling, and no random AI reinterpretation of owned creature art.

## 2. Playable Wayfinder — avatar identity

### Player fantasy
The player is a new Wayfinder arriving by boat at Lantern Quay. Player selects a display name and avatar presentation, then bonds with one of three original starters. The protagonist is written without assumed gender; NPC dialogue never relies on a fixed canonical appearance.

### Avatar authoring style
- **Pose:** steady explorer stance, relaxed shoulders, facing outward while carrying a small resonance compass and satchel.
- **Outfit anatomy:** strong outer garment silhouette (jacket/poncho/vest), large readable belt/satchel, practical boots; scarf or band can identify their team color.
- **Shape variants:** proposed `frame_a`, `frame_b`, `frame_c` with different torso/shoulder silhouettes; all share the same gameplay collider and interact reach. Do NOT frame body variants as performance advantages.
- **Face/hair:** selectable face/eye and hair shape palettes with medium-high contrast. Preview on dark forest and warm town backgrounds.
- **Skin color:** inclusive set of palette ramps authored by an artist, not hue-shifting one skin pixel to create unnatural highlights; do not tie any palette to character class.
- **Pronouns:** optional profile field, not required for player-facing quest script; avoid forced identity labels or mechanical differences.
- **Starting customization:** body/frame, skin palette, hair shape and color, default jacket color, face detail, display name. Actual option counts to be approved against time to create sprites.
- **Starter choice:** separate from avatar appearance; equal progress opportunities.
- **First-person presence:** small compass hand in lower-right or lower-center edge when in use; otherwise allow minimal HUD. Do not permanently obscure wild creatures or puzzles. Hand palette matches skin choice and any visible sleeve matches outfit.
- **Self-view:** avatar edit/mirror view in Wayfinder Hall and inventory profile; third-person exploration is NOT planned for first playable.
- **Online presentation:** other players see a directional billboard sprite assembled/baked from their saved outfit preset; nearby name and interaction options subject to privacy/moderation settings.

### Core equipment motif: Resonance Compass
- Bronze/brass irregular circle, etched triangular signal-glyph, mint pulse ring. Practical "field study" tool, not a weapon.
- `COMPASS_REST`, `COMPASS_SCAN`, `COMPASS_SIGNAL_ALERT`, `COMPASS_INTERACT` visual states.
- Each state produces an identifiable icon + audio + motion response, not color alone.
- Offline quest-given and unlosable, never purchased.

## 3. Visual branding and costume families

Proposed outfit families fit the landscape and allow future paid cosmetics without altering stats:
1. **Harbor Fieldwear:** warm cream tunic, sea-teal vest, brown straps, sturdy boots; default and always available.
2. **Woodland Surveyor:** olive poncho with gold lining, knee boots, lantern pin; Liltwood narrative reward concept.
3. **Ruins Archivist:** charcoal coat with teal geometric trim, ink-stained gloves, satchel; Echo Hollow reward concept.
4. **Storm Runner:** slate rain cape, copper buckles, cyan scarf; future Stormwatch region.
5. **Festival Wayfinder:** coral/gold stitched jacket and headscarf; seasonal cosmetic proposal, not approved SKU.

The same silhouette language should fit all frame variants. Paid cosmetics (future) are strictly visual; no movement bonuses, secret access, combat modifiers or stat advantages. Players retain a legible free outfit set.

### Palette rules
Human clothing uses **Deep Tide** `#183B41`, **Harbor Ivory** `#F6EBD7`, **Lantern Gold** `#E8C878`, teal and coral accents as defined in Visual Bible. NPC costume accent swatches below are **provisional**. Important: preserve the full coloring and personality of original creature sprites; characters support creatures visually rather than repaint creatures to match humans.

## 4. NPC cast: four authored designs

### NPC_001 — Mara Venn | Wayfinder guide
**Narrative:** Clear-minded, warm but cautious leader who invites the newly arrived Wayfinder to investigate the silent forest signal. Prefers asking questions to giving commands. No authoritarian/competition-heavy imagery.  
**Silhouette:** Medium-height, asymmetrical shoulder scarf / short travel cloak, strong diagonal satchel strap, prominent brass compass. Side-swept hair or tied wrap, one shoulder larger than the other to read from the back.  
**Colors:** Deep tide coat `#183B41`, warm coral scarf `#E38A70`, small lantern gold badge `#E8C878`; ivory shirt.  
**Iconic gesture:** Raises compact field map; then points to north Forest Gate.  
**Location:** `Z01` `NPC_001_MARA` coordinate (18,11) in draft map; NPC quest table ID `NPC_001`.  
**Quest:** Initiates Q001, after compass directs to starter choice, after Q005 celebrates player discovery without overwriting player's emotions.  
**Dialogue keys (proposed):**
- `npc.mara.greeting`: "A Wayfinder? Good. The forest has been quiet in all the wrong places."
- `npc.mara.why_signal`: "When the lanterns called, the woods used to answer. We need to learn why they stopped."
- `npc.mara.after_compass`: "Find a companion willing to explore with you. You'll need each other's instincts."
- `npc.mara.chapter_complete`: "You found an answer—and another question. That's how this work begins."
**Reaction sprites:** welcome, concerned, pointing, quiet-smile.

### NPC_002 — Old Fen | Compass technician
**Narrative:** An eccentric but capable craftsperson who repairs old resonance instruments. Protects the safety of creatures and believes they should not be treated as equipment.  
**Silhouette:** Short and broad, round split goggles, rolled oversized sleeves, a belt full of tools, angular lamp gloves. No frailty caricature.  
**Colors:** Brass `#B99B5F`, muted brown `#6F5D4B`, rust copper `#A7684C`, light blue lens glint `#82C9D4`.  
**Iconic gesture:** Taps a compass case twice and tilts it toward the player; floating signal-glyph appears for an instant.  
**Location:** `Z01` `NPC_002_FEN` at (33,8) in draft map; `NPC_002` in quest table.  
**Quest:** Awards COMPASS during Q001; may provide safe repeat tutorial on compass with no duplicate item grant.  
**Dialogue keys:**
- `npc.fen.greeting`: "Mind the springs. These little things hear more than we do."
- `npc.fen.award_compass`: "A compass doesn't order the wild about. It helps you listen."
- `npc.fen.reminder`: "Watch the ring and listen for its note. The closer you are, the clearer it gets."
- `npc.fen.after_q005`: "That wasn't supposed to wake. Tell me exactly what you heard."
**Reactions:** examine lens, adjust tool, hold compass forward, focused nod.

### NPC_003 — Iri | Forest caretaker
**Narrative:** Patient, observant keeper of Liltwood trails and animal habitats. Recognizes that the creatures are responding to new signals. Does not function as an intrusive tutorial voice.  
**Silhouette:** Narrow leafy rain-poncho, distinct rust-orange shoulder satchel, staff/walking reed (nonweapon), a single large leaf at the hood.  
**Colors:** Moss jade `#367D62`, fern glow `#79B77C`, warm russet `#BA704E`, sage cream `#D9D8A7`.  
**Iconic gesture:** Kneels and gestures to a small creature trail, then toward Singing Root.  
**Location:** `Z03` `NPC_003_IRI` at (21,25); does NOT spawn in Z02 Mosswake in initial blockout. Quest brief may call her "at the forest entrance."  
**Quest:** Q003 optional conversation gives ecology clues; Q004 puzzle solution never gated behind speaking to Iri alone if player has observed signal markers and root.  
**Dialogue keys:**
- `npc.iri.greeting`: "Even the smallest ones are listening. See how they all turned toward the root?"
- `npc.iri.hint`: "Follow the sound, not the footprints. Your companion knows something I don't."
- `npc.iri.after_gate`: "The way opened for you together. Remember that."
**Reactions:** kneel, watch companion, point, relieved.

### NPC_004 — Bellkeeper Sol | Echo Hollow archivist (voice/memory only in Chapter 0)
**Narrative:** Keeper of a long-forgotten signal archive. Presence is ambiguous during the opening; no actual embodied NPC appears in Echo Hollow until later chapter to preserve mystery and limit art work.  
**Silhouette for future full appearance:** Lean tall robe with broad squared collar, angular silver ring fasteners, trailing sash; symmetrical ancient-ruin geometry. Avoid generic hooded villain tropes.  
**Colors:** Ruin plum `#4C3C5B`, cool silver `#B9C4CF`, faded mint `#74D6B1`.  
**Chapter 0 implementation:** a static, original mural silhouette / record-echo overlay near the pedestal. This is a noninteractive story memory; map fixture intentionally contains NO physical `NPC_004` entity. The only required physical map actors are Mara, Fen and Iri.  
**Quest:** flavor teaser post-Q005, no quest blocking.  
**Dialogue key:** `npc.sol.echo`: "A signal is not a command. It is a question." (from earlier opening script, presented as an inscription; attribution remains deliberately uncertain).
**Future design scope:** true identity, role, motivations, in-person dialogue and animations to be established after narrative review.

## 5. Quest-dialogue and behavioral state model

| NPC | Spawn/location | Pre-quest | Mid-quest | Quest complete | Cannot block |
|---|---|---|---|---|---|
| Mara / NPC_001 | Z01 (18,11) | Offers Q001 | Reminds compass→starter→gate | Chapter acknowledgement | If player skips talk initially, compass workshop grants navigation hint and no quest deadlock |
| Fen / NPC_002 | Z01 (33,8) | Identifies compass | Awards unlosable COMPASS exactly once | Explains compass use | Repeated interaction idempotent |
| Iri / NPC_003 | Z03 (21,25) | Shows forest ecology | Optional Root clue | Reacts to Ruin Gate | Essential puzzle clue available without her dialogue |
| Sol / NPC_004 | Echo Hollow inscription | Not physically present | No combat/interaction | Lore teaser | No body sprite or mandatory conversation |

### Dialogue system contract
- Stable localized keys: `npc.<id>.<state>` and branch conditions `quest_flags`.
- A single interaction is explicitly triggered by input, opens dialogue in a readable separate overlay, halts local movement (offline), can be dismissed, re-read, and never forces control loss longer than approved animation.
- Dialogue choices are optional in first playable. When added, each choice must have documented flag/reward effects; no silently invented relationship system.
- Each character has 3–4 facial/gesture icons for the first art milestone; final frame counts determined after native pixel scale is known.
- Essential mission text presented with UI text + direction icon, not sound or subtle sprite expression alone.
- No speech voice asset required; optional distinct simple UI blip, volume adjustable.

## 6. Character art pipeline and asset contracts

**Preferred authoring source:** editable pixel file format with layers (Aseprite/LibreSprite or comparable), exported lossless PNG sprite atlas + metadata.  
**All pixel sizes below are experimental art production targets, not exact engine constraints.**

### Sprite sets (proposed starter scope)
- `avatar_frame_a`: 48×64 transparent source canvas, pivot at (24,60) in artist pixels; standardized across frames.
- Player 4 facing directions `front, right, back, left`; idle 2 frames and walk 4 frames per direction; 24 directional source frames per base avatar.
- One consistent direction convention (relative to camera yaw) for every humanoid; mirrored rights/lefts only for accessories that remain semantically correct.
- NPC body frames: front/back/sides 1 idle each + two dialogue/emote frames for visible tutorial NPCs; optionally walk cycle only when their authored scene actually requires motion.
- NPC portrait: 48×48 or 64×64 face/shoulder pixel art; high contrast silhouette; do not scale source by bilinear interpolation. Final dimensions after UI prototype.
- First-person arms/compass: small anchored overlay frames with skin/sleeve palette consistent with avatar; overlay never required for tasks other than using compass.
- Player avatar nameplates, targeting outlines, quest symbols and interaction feedback are UI art, not baked into character sheet.
- Keep character collisions uniform among avatar variants; cosmetics never change collisions/hitboxes.

### Suggested asset storage
```text
assets/characters/
  player/
    source/{frame_a,frame_b,frame_c}.aseprite
    presets/<preset_id>.json
    sprites/<variant>/<direction>_<action>_<frame>.png
    first_person/<skin_palette>/<sleeve_id>_<action>_<frame>.png
  npc_001_mara/source/...
  npc_001_mara/sprites/front_idle_00.png
  npc_001_mara/portraits/neutral.png
  npc_002_fen/...
  npc_003_iri/...
  npc_004_sol/inscription/...
assets/ui/character/<icon_id>.png
data/characters/<character_id>.json
```
**Caution:** This is a naming plan only. Original art and the sprite-sheet files are not currently committed to the GitHub PR. Codex must not assume that any listed binary file already exists.

### Asset metadata
For each character, supply: stable `character_id`; display-name localization key; `model_role` ("player"/"npc"/"memory"); `sprite_canvas_px`, `pivot_px`, `world_scale` (to be measured), `facing_convention`; `animation_sets` with frames and frame durations; `palette_set`, `outfit_slots`; `voice_profile` (optional); `collision_profile`; `interact_radius`; `npc_dialogue_id`; `source_asset_provenance`.
Separate image metadata from gameplay state and do not introduce dynamic runtime file reads for each individual frame.

### Pixel performance budget (illustrative only)
At 48×64 uncompressed RGBA each frame is 12,288 bytes. A 24-frame base atlas is roughly 288 KiB of raw pixel memory before overhead/compression. Outfits multiply frames quickly, so use palette-indexed layers or pre-baked combinations in a limited number of canonical presets. Benchmark both CPU palette processing and texture/upload paths before choosing dynamic blending vs offline atlas baking.

## 7. Customization and economy separation

**First playable:** create one character with frame choice, a handful of authored hair/skin palette presets, jacket color, name and free starter outfit. Save locally. Character customization must not require login.

**Later online launch:** server-side cosmetic entitlements; let online avatar share *appearance settings* where licensing/ownership permits, not trust offline inventory as paid ownership. Offline item edits cannot grant purchased online cosmetic licenses. Free cosmetics are permanently available even without multiplayer servers.

**Cosmetic slots proposed:** hairstyle, headwear, outerwear, scarf/neck, backpack, companion accessory. Do not commit to all six until layer/atlas cost is validated. Exclude any creature-stat or traversal modifiers.

**Store-specific:** Google Play Billing and Steam transactions verified server-side; do not assume transfer of purchases between storefronts or permanent entitlement offline without offline verification scheme and platform/legal review. Cosmetics are optional; never block story/creatures for lack of paid cosmetics.

## 8. Scene-level art briefs — four critical hero shots

1. **CH-01 / Beacon Square**: from `Z01_SPAWN_SQUARE (24,29)`, first-person player sees warm tower and Mara silhouette near Wayfinder Hall; little bit of hand-held compass glints when activated. Character sprites should occupy clear area, not be overwhelmed by environment.
2. **CH-02 / Fen's Workshop**: from Quay approach toward Fen (33,8), warm interior-esque frontage, heavy bronze metal, old lenses and field tools, mint instrument pulse; high readability of Fen's short/wide silhouette.
3. **CH-03 / Forest Guide**: from Liltwood `(24,26)` toward Iri (21,25), overgrown foliage, orange bag separates Iri from green background; owner's original starter sprite reacts near player's feet or side, not covering NPC.
4. **CH-04 / Echo Inscription**: at Echo Hollow pedestal after battle, dark plum wall with a symmetrical ring-marked silhouette/inscription; *no full living Sol sprite* required yet, preserving mystery.

Each shot must be tested at 320×180 world buffer upscaled and on typical Android landscape screen; verify NPC/outfit silhouette, target feedback and legible UI text. 320×180 is a test candidate, not an approved fixed renderer.

## 9. Acceptance criteria for character art/design approval

1. Three NPCs distinguishable in grayscale, at reduced first-person sprite scale and without relying on tiny face pixels.
2. Player frame and cosmetic presets share alignments across front, side and rear states; no tearing at neck/hand/sleeve/hat when animated.
3. In first-person view hand/compass matches avatar skin/outfit and does not obstruct center reticle, companion or readable puzzle elements.
4. Mara at Z01 (18,11), Fen at Z01 (33,8), Iri at Z03 (21,25) agree exactly with map fixture and quest scripts; Sol is not spawned as a physical actor in Chapter 0.
5. Dialogue can be read, skipped, reopened and localized; important hints remain available after interruption.
6. Portrait palette clearly distinguishes dialogue speaker on a low-brightness mobile screen, with and without color perception.
7. Moving players and their companions render with proper depth and occlusion; no visibility of other players in an offline game.
8. Starting cosmetics and character creation available offline with no server call; an edited local profile cannot unlock online paid entitlements.
9. Source assets and rights ledger tracked for created NPC/player art; user-owned creature sprites never replaced by unrelated filler art without signoff.
10. Compare CPU time and memory of sprite batching against target low-end hardware. Real FPS claim only after testing.

## 10. Creative choices needing owner approval
- Wayfinder proportions, facial style, default appearance and how elaborate character creation should be in version 1.
- Final costume motifs and name "Wayfinder" vs alternatives.
- Mara/Fen/Iri/Sol designs, character relationships, presentation of Sol as nonphysical memory.
- Confirm NPC initial line-by-line dialogue, whether to offer portrait conversations or in-world speech bubbles.
- Paid cosmetic roadmap vs first slice scope.
- Player name character limit, social moderation and privacy defaults; additional platform policies to check nearer release.

**Default for review:** Keep an inclusive but deliberately **small** player-customization set; use 3 static tutorial NPCs + one inscription only; visually emphasize your already-owned creature sprite art rather than competing with it.
