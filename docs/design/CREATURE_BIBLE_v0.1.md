# WILDSIGNAL — Creature Bible v0.1

**Status:** Proposed design, for review. **Date:** 2026-10-20  
**Project:** Original, commercial, cross-platform (Windows/Steam + Android/Google Play) 2.5D pixel-art multiplayer creature-collecting RPG.  
**Implementation:** Codex will implement only after design review; this document is *not* an instruction to immediately write the game.

## 0. Decision register

### Confirmed
- Working game title: **WILDSIGNAL** (trademark/marketplace availability **not checked**).
- Visual direction: first-person retro/pixel-art pseudo-3D, colorful yet mysterious, forest environments, ancient ruins, a sense of discovery. Blend cozy nature, mystical exploration, and restrained danger.
- Original creature-focused IP for eventual commercial distribution.
- User confirms the 10x10 creature sprite sheet was hand-created by them years ago (not AI-generated), they own it, and they authorize direct use of the artwork in WILDSIGNAL. Record this user-supplied provenance in the art ledger.
- Creatures are persistent companions: follow the player, react to the world, assist with exploration.
- **Turn-based** creature combat, with real-time exploration.
- Windows and Android targets, including low-end devices; C++ with lightweight cross-platform libraries rather than a heavyweight local game engine.
- Multiplayer and optional in-app purchases planned; gameplay should not be pay-to-win.
- Codex is the implementer; ChatGPT is the planning/specification collaborator.

### Proposed, not yet approved
- Exact names and final appearances of starters, affinities, rarity labels, encounter/bonding mechanics, evolution conditions, numerical battle rules, and initial roster size.
- Specific software renderer architecture, library selections, frame-rate/memory budgets, and engine pipeline, pending benchmarks.

## 1. Visual reference and intellectual property rule

The user shared a 10-column × 10-row sheet of original pixel-art creatures on 2026-10-20. On the next turn, the user **confirmed that they personally drew this artwork years ago, that it is not AI-generated, that they own it, and that the project can use it directly**. The earlier unverified-rights restriction is withdrawn. This sheet is WILDSIGNAL's **owned base creature artwork**; preserve the original designs rather than requiring unrelated redraws. Row/column (1-based) addresses refer to the user's source sprites. The original image should be retained in the project's asset ledger with creator, creation history (approximate if necessary), source filename, and rights note. Public-repo redistribution/commit of the raw sheet can be coordinated with the owner; inclusion in final game assets is authorized by the owner. This does not imply all future third-party contributions are cleared.

Qualities to preserve in existing owned sprites and any future original creature additions:
- Bold, readable silhouettes even at ~32–64 source pixels.
- Friendly, expressive faces; a few delightful asymmetries.
- Distinct creature families: forest, aquatic, ember, fungus, stone, spirit, insects.
- Strong, limited palettes; good readability against dark/detailed ruins.
- Diverse sizes and locomotion (hopping, gliding, crawling, floating, etc.).
- Each species feels like part of the local ecology and has at least one useful out-of-battle behavior.

## 2. World fiction and bonding premise

Players are **Wayfinders** carrying **Resonance Compasses**. Creatures emit or react to patterns called **signals**. The compass helps detect habitats, decode ruined machinery, find passages, and establish trust. 'Capture' is a gameplay analogue to encountering and befriending, but the final in-fiction mechanic needs its own design and iconography; do not copy familiar franchise balls or interfaces.

Companions follow and respond to stimuli: curiosity, comfort/fear, weather, sounds, nearby species, and ruins. Each playable species needs a visible idle behavior, a companion-follow reaction, a battle action, and a meaningful discovery ability. Idle NPC-like creatures should inhabit ecosystems rather than standing motionless waiting for battles.

## 3. Affinities (proposal)

Eight possible affinities:
1. **Flora** — growth, vines, fungi, adaptation.
2. **Ember** — heat, cinders, rhythm, ignition.
3. **Tide** — water, mist, flow, sensing.
4. **Gale** — wind, weather, speed, flight.
5. **Stone** — earth, geology, resonance with ruins.
6. **Gloom** — shadow, night, dreams, concealment.
7. **Lumen** — illumination, signals, protection.
8. **Wild** — primal, neutral and versatile behaviors.

**Prototype implementation:** Only Flora, Tide, Ember and neutral/no-affinity encounters initially; do not implement an 8×8 interaction matrix until playtesting. Introductory advantage loop: Flora > Tide > Ember > Flora. Affinity counters should be modest so team strategy remains relevant. Secondary affinity possible later, not in first vertical slice.

## 4. Turn-based battle framework (proposal)

- **1 active companion per side**, with party capacity of **3** initially.
- Four equipable combat moves per companion.
- Roles include **Defender**, **Striker**, **Controller**, **Support**, and **Healer**. Species can overlap.
- Moves have: identifier, affinity, power/heal/effect, cost or limited charges (choose one after prototype), accuracy if applicable, priority, and target.
- Both participants select commands in a planning phase. Resolve using priority, then speed, then deterministic tie-breaking. The backend eventually validates commands and resolves online battles authoritatively.
- Target PvP turn duration and disconnect handling require later testing; never trust clients to calculate permanent rewards or inventory modifications.
- No real-money access to competitively exclusive moves or stronger creatures.
- Battle loop: encounter -> command selection -> animation/effects -> status update -> victory/retreat/befriending opportunity -> reward or bond growth.
- Starter balance: broadly equal power budget, contrasting strengths and weaknesses.

## 5. Rarity and evolution (proposal)

**Discovery rarity:** Common, Uncommon, Rare, Mythic. Encounter rarity measures ecological/discovery difficulty, **not raw combat superiority**. Mythic is primarily an achievement/lore classification.

**Growth:** experience plus bonding. Bond increases from exploration, helping, successful navigation, resting, and battles; avoid daily streak penalties or compulsive grinding.

**Evolution:** optional, species-specific, and story/ecology related; a small number of stages. Requirements might combine level, bond milestones, location, time-of-day, or completed discoveries. Players may keep original forms competitive. Avoid designing evolutions solely to inflate total creature count.

## 6. First starter trio — provisional concept studies

These names were used in earlier concept discussions and are retained for continuity. The cited sprite-sheet positions identify **owned source sprites that may be used directly**. Exact species names, roles and battle/exploration mechanics are still *proposals*. Preserve the source design when animating, making additional viewing angles, and designing evolutions.

### S01 — Glintle | Flora | "Lantern Guardian"
- **Visual reference:** R1C4, green-and-gold compact forest creature.
- **Art-production brief:** Preserve the owner's R1C4 sprite as Glintle's starting appearance, including its silhouette and palette. Develop additional idle, walking, follow, alert and battle frames matching that art; optional glow effects may be added without replacing the design.
- **Personality:** cautious, observant, affectionate once trusted; watches wall markings and pauses at hidden routes.
- **Combat role:** support/defender; protective growth, gradual recovery, field-control techniques.
- **Exploration behavior:** highlights hidden forest markings and reacts to secret paths.
- **Signature move concept:** **Lantern Grove** — protect against the next hit and restore a small amount of health afterward.
- **Evolution hook:** earns a new form after a bond milestone and restoring a lost forest beacon.
- **Species name, signature move, abilities and evolution stages:** pending creative approval; base sprite is owned project artwork.

### S02 — Driftfin | Tide | "Current Glider"
- **Visual reference:** R1C8, small bright-blue bubbly aquatic creature.
- **Art-production brief:** Preserve the owner's R1C8 small blue sprite as Driftfin's starting appearance. Extend it with animations for bouncing/gliding, puddle curiosity, and battle cues; any new fins or markings should be approved against the source sprite.
- **Personality:** curious, excitable, easily distracted by rain, puddles and moving water.
- **Combat role:** speed/controller; disrupts turn order and affects the opponent's momentum.
- **Exploration behavior:** senses underground waterways, hidden pools and stream currents.
- **Signature move concept:** **Slipstream** — moderate water damage with a small initiative advantage next turn.
- **Evolution hook:** discovers a flooded shrine during a storm.
- **Final anatomy, name and evolution stages:** pending art/legal review.

### S03 — Brumlet | Ember | "Ember Drummer"
- **Visual reference:** R5C10, round orange-red ember critter.
- **Art-production brief:** Preserve the owner's R5C10 little orange ember sprite as Brumlet's starting appearance. Add expressive rhythmic idle and attack frames, plus a subtle warm glow; do not impose an unrelated body redesign.
- **Personality:** boisterous, loyal, percussion-obsessed; reacts to machines and ancient bells.
- **Combat role:** offense/tempo; burst hits balanced by setup and defensive risk.
- **Exploration behavior:** lights dormant braziers, activates heat-driven mechanisms, responds to rhythmic puzzles.
- **Signature move concept:** **Cinder Beat** — builds rhythm and empowers the next Ember attack.
- **Evolution hook:** performs a signal rhythm at an ancient forge after sufficient bonding.
- **Final anatomy, name and evolution stages:** pending art/legal review.

**Starter cycle:** Glintle/Flora > Driftfin/Tide > Brumlet/Ember > Glintle/Flora. Prefer balanced counterplay over hard auto-wins.

## 7. Reference shortlist from the shared creature sheet

**These are positions in the owner's original pixel-art creature sheet.** The source sprites are permitted project assets; names, region placements and official species status are not yet approved.

| Sheet cell | Visible motif | Potential ecology / role |
|---|---|---|
| R1C4 | Small green-gold forest sentinel | Flora starter direction |
| R1C8 | Round bright-blue aquatic buddy | Tide starter direction |
| R5C10 | Tiny orange ember-like creature | Ember starter direction |
| R2C4 | Green robed/mushroom-capped traveler | Forest healer NPC/species |
| R3C6 | Bright-green shell/leaf creature | Wetland gatherer |
| R4C3 | Brown round woodland animal | Early-route defender |
| R4C7 | Blue fast-moving creature | Aggressive water encounter |
| R5C5 | Long green serpentine creature | Rare forest guardian |
| R5C9 | White-blue little icy spirit | Mountain discovery |
| R6C3 | Small pale rounded creature | Nocturnal companion |
| R7C7 | Floating blue flame spirit | Ruin puzzle creature |
| R8C4 | Big-eyed moth-like creature | Gale scouting and night behavior |
| R9C1 | Red crab-like creature | Coastline defender |
| R10C5 | White fluffy horned creature | Highland herd member |
| R10C10 | Cloaked dark monolith figure | Ancient ruin mystery |

## 8. First playable slice scope (proposed)

**The First Signal:** target **15–20 minutes** of enjoyable content before scaling.
1. Lantern Quay: small hub and Wayfinder tutorial.
2. Liltwood Forest: short first-person route, environmental creature behavior and detection.
3. Befriend one of the three starters.
4. See companion physically follow you, react to a hidden-marking puzzle and reveal the entrance to ruins.
5. Resolve a 1v1 turn-based combat encounter.
6. Return to hub and demonstrate saved companion ownership.
7. Connect **two real players** to one small multiplayer zone during a separate networking milestone.

Production cap for initial vertical slice: **3 starters + 3–6 wild prototype species**, **1 small hub**, **1 forest zone**, **1 small ruin**. Do not produce the full 15-candidate roster, online economy, comprehensive crafting or monetization until this is fun.

## 9. Pixel-art and low-end technical constraints

- Reference art: retro pixel-art characters in a pseudo-3D first-person setting; high-contrast silhouettes.
- World: 2D-authored maps with perspective software rendering or other benchmarked lightweight 2.5D method. Doom-like aesthetics do **not** imply Doom's exact engine architecture.
- Tentative low-resolution internal framebuffer: 320×180; UI and text stay sharp/readable at platform display resolution. Benchmark before committing.
- Starter rendering: one strong front view for combat and three or four directional states for overworld as the art budget allows; sprite flipping is not appropriate if markings are asymmetric.
- Aim for small sprite atlases, compressed textures where useful, tight CPU budgets, minimal runtime allocations, and asset reuse. Performance goals are hypotheses until tested on real low-end Android and Windows hardware.
- Companion positioning should avoid camera clipping, pathfinding oscillations and screen obstruction.
- Preserve palette cohesion: vibrant greens and lantern gold; blue waterways; warm ember coral; deep teal ruin shadows.
- Accessibility: clear font and touch targets on phones, color-blind-friendly move/affinity markers, optional motion reduction.

## 10. Multiplayer + monetization design guardrails

- Cross-play target: Android ↔ Windows, shared account/entitlements subject to store policies.
- Multiplayer modes should start small: shared hub and instanced co-op; optional turn-based PvP.
- Server authoritative for battles, creature ownership, trading, currency, and purchase entitlements.
- Monetization concept: cosmetics, companion accessories, outfits and optional noncompetitive decoration. No pay-to-win stat upgrades or paid competitive exclusivity.
- Verify Google Play and Steam billing requirements separately before implementation.

## 11. Design questions awaiting approval

1. **Resolved:** The sheet consists of user-owned, hand-created pixel art, explicitly authorized for WILDSIGNAL use; not AI-generated.
2. Do you approve assigning the existing sprites R1C4, R1C8, and R5C10 to the working starter names Glintle, Driftfin, and Brumlet? Their original designs should be retained.
3. Should bonding replace traditional capturing, or should the game have both?
4. Should the first version offer 1v1 battles only, with PvP and multiplayer cooperative activities added later?
5. What minimum Android versions and actual test devices can we benchmark?

## 12. Suggested Codex handoff after approval

1. Add a machine-readable **species data schema** (not creature art) matching the approved concept fields.
2. Build a deterministic **turn-based combat simulation** with unit tests and no rendering dependencies.
3. Prototype one stationary first-person encounter using a temporary placeholder until the user's owned sprite file is included in the asset pipeline; then use the actual sprite.
4. Add companion-following behavior and one environmental interaction.
5. Test resource usage on Windows and Android before expanding art/content scope.

**Rights note:** The user has affirmed ownership of the original creature sheet and authorized its use in WILDSIGNAL. No AI-generated content disclosure is needed *because of this sheet*. Other assets and subsequent AI-generated content, if any, must be tracked independently.
