# WILDSIGNAL — First Nine: Ecology, Encounters & Bonding v0.1

**Status:** Proposal for review. **Confirmed:** companions physically follow players, can assist exploration, 1v1 turn-based battles, offline story works fully, first game uses the user's owned original pixel sprites. **Proposed:** creature names, behaviors, recruitment/bond points, exact habitat positions and outcome numbers.

**Update 2026-11-01:** friendly spars are real-time fights (D-006). The owner confirmed that befriending stays free of dice and adds depth through species needs, wariness, temperament and a Reach Out fight command. Winning by knockout earns no trust; calming a Signari with gentle play and reaching out does. A frightened Signari refuses everything until it settles, and trust is never lost. The first playable test is `experiments/hearthgrove_emaceart/src/bond.cpp`, `src/wild_scene.cpp` and `data/wildlife_test.json`; numbers, placements and wording there are placeholders.

## 1. Philosophy

Unlike automatic/random capture, **creatures are encountered as wildlife and invited to join through trust**. The player earns trust by paying attention, improving creature habitat, optionally sparring, and respecting each species' cues. There is no projectile capture device and no paid shortcut to recruitment. Visual creature bodies remain the owner's original sprite designs (WS-IDs); new animations/multiple angles must be drawn to match.

A wild encounter has three independent components:
1. **Wildlife presentation**: animation, ecological role and curiosity/alert/flee behavior.
2. **Trust ledger**: encounter-specific once-only trust actions (offline locally, server-authoritative online).
3. **Optional spar**: staged turn-based fight; no permanent creature harm and no mandatory encounter for general recruitment.

## 2. Suggested trust model (v0.1 — test with players)

- A persistent wild creature interaction instance is identified by `worldInstanceId:zoneId:spawnId:speciesId`.
- Local trust starts at 0, caps at 5. **Observe** from a safe distance gives +1 **once per instance**; show a visible behavior clue. **Habitat Assist** succeeds only when species-specific environmental task completes and gives +2 **once**. **Friendly Spar** gives +1 after an eligible voluntary win, once; no trust for infinite repeat battles.
- **Invite** is enabled at trust ≥3, after the creature's quest-critical restrictions are satisfied. The creature joins local party or local reserve. If party full, add to reserve (requires reserve UI before shipping this feature); no force-replace/discard of owned creature.
- Invitation is deterministic at threshold, not chance-based. The creature can refuse dialogue-wise below threshold, but no hidden probability roll.
- Fleeing or losing a voluntary spar causes no trust penalty and no permanent disappearance. The encounter recovers/rests; replay-safe flags prevent repeated one-time rewards.
- Starter selection Q002 is a guaranteed narrative bonding event and bypasses wild encounter trust requirements. Each local save receives exactly one starter.
- Offline trust never enters a server-verified creature inventory automatically. Online encounter/trust/recruitment is server-authoritative and separately earned.
- Prevent exploit: all trust events recorded as idempotent instance actions; task item consumed/quest flag updated in one transaction; no paid item increases trust.
- Design risk: pure Observe + Assist path may make battles less useful. Prototype user testing must assess enjoyment before locking the feature; alternative trust/battle loop is allowed by explicit redesign.

## 3. All nine early roster candidates (art IDs are stable; all names proposed)

| Sprite ID | Name | Type | Place | Battle role | Environmental identity |
|---|---|---|---|---|---|
| WS-004 / R01C04 | Glintle | Flora | Lantern Quay starter | defender/support | reacts to hidden forest signal marks |
| WS-008 / R01C08 | Driftfin | Tide | Lantern Quay starter | speed/control | detects moisture and water channels |
| WS-050 / R05C10 | Brumlet | Ember | Lantern Quay starter | burst/tempo | resonates with warm stone and metal rhythms |
| WS-005 / R01C05 | Tumbletail | Wild | Mosswake | quick scout | gathers fallen nuts/seeds and hides behind rocks |
| WS-021 / R03C01 | Puddlefin | Tide | Mosswake rain-pool | defensive control | traces tiny ripple patterns; hides when startled |
| WS-012 / R02C02 | Mossmelt | Flora | Liltwood Fern Ring | recovery/support | cleans fallen leaves and helps fungi grow |
| WS-001 / R01C01 | Signalit | Lumen | Liltwood dusk / ruin fringe | fast signal scout | glows and gathers near old glyphs |
| WS-024 / R03C04 | Craglet | Stone | Echo Hollow | defensive guardian | guards broken seals and rests amid masonry |
| WS-067 / R07C07 | Hushflame | Lumen | Echo Hollow | pulse/support | responds to chimes and hovers by inscriptions |

These choices cover six early wild species + three starters using the exact owner's existing artwork. No other cells in the 100-sprite catalog are deleted or converted into evolutions. Naming and habitat tags remain provisional.

## 4. Per-species authored activities

### Starter WS-004 Glintle
- **Idle:** adjusts posture to face faint signal pulses; bright face flash at interaction.
- **Follow:** short hopping gait with occasional inspection pause; camera occlusion-safe trailing position.
- **Exploration:** detects Fern Ring's strange inscriptions and can reveal the Veiled Wall's symbols via luminous outline; *not a mandatory starter for critical path*.
- **Battle:** LANTERN_GROVE guards and heals gradually; SPROUT_DART and ROOT_THREAD attack/control.
- **Bond event:** one-time +1 on Q003 signal-marker and root discovery, not a repeatable grind.

### Starter WS-008 Driftfin
- **Idle:** bobbing/hovering, tiny wet-blue droplets (max few 2D particles).
- **Follow:** small float/bounce, never crosses blocked water or clips wall.
- **Exploration:** responds to seepage channels, same forest puzzle solution as other starters.
- **Battle:** SLIPSTREAM quick Tide signal move; MIST_HUSH slows and QUICK_DART gives +1 priority.
- **Bond event:** one-time +1 on uncovering wet seal near Veiled Wall.

### Starter WS-050 Brumlet
- **Idle:** shakes with a rhythmic 2–3 frame motion and a warm glow.
- **Follow:** short springy steps, bell-like clink only when significant interaction.
- **Exploration:** senses active forged inlays and warms Veiled Wall marks; same puzzle output as others.
- **Battle:** CINDER_BEAT sets next-hit FOCUS, EMBER_KNOCK and COAL_FLICK for damage/status.
- **Bond event:** one-time +1 at Q004 successful gate restoration.

### Wild WS-005 Tumbletail — Mosswake Path
- **Initial spawn:** `C_MOSSWAKE_WILD` existing entity at Z02 (36,20); species ID assignment **proposed**.
- **Loop:** forage 2–3 nearby cells → spot player → pause → scoot to rock → emerge.
- **Observe:** study where it gathers food, +1 trust.
- **Assist:** restore disturbed seed cache under an existing nearby optional prop, +2 trust. Implement as a short 1-button interaction once, not a crafting dependency.
- **Friendly Spar:** optional, +1 if won.
- **Invite:** after Observe+Assist or eligible combo, no consumable used.
- **Combat:** fast lightly guarded Wild physical techniques, BRACE/ROOT_THREAD.
- **Quest guard:** never occupies portal tile or blocks intro Q003.

### Wild WS-021 Puddlefin — Mosswake rain pool
- **Proposed spawn:** `C_MOSSWAKE_POOL` as a new metadata entity near an authored shallow puddle; **not yet in existing 48×32 fixture**. Suggested vicinity Z02 (33,21); inspect geometry before committing.
- **Loop:** emerges from ripple → rotates → retreats below surface.
- **Observe:** wait for still water, study bubble pattern, +1.
- **Assist:** dislodge leaf from tiny rain-pool runnel, +2 (free interaction, no inventory).
- **Spar:** optional 1v1.
- **Invite:** deterministic at trust ≥3.
- **Battle:** Tide control, guard bubble, no water simulation required. Sprite puddle prop can fake movement.
- **Quest guard:** pool branch is optional and marked, no swimming mechanic.

### Wild WS-012 Mossmelt — Liltwood Fern Ring
- **Spawn:** existing `LANDMARK_FERN_RING` entity at Z03 (11,18) remains a landmark. Add separate `C_FERN_RING` encounter metadata so the landmark never becomes the creature itself.
- **Loop:** slow crawl under mushroom/fern → pause while absorbing moisture → rise when sunlight enters.
- **Observe:** notice trail and eating pattern, +1.
- **Assist:** remove fallen debris from mushroom bed; no combat required, +2.
- **Invite:** after sufficient trust; possible first healing-oriented wild.
- **Battle:** MOSS_MEND healing, SPROUT_DART and ROOT_THREAD.
- **Quest guard:** assistance uses optional reachable pocket, not linked to Q004 essential gate.

### Wild WS-001 Signalit — Liltwood/Echo outskirts
- **Spawn proposal:** `C_LILTWOOD_SIGNAL_DUSK` near Liltwood north-east approach **requires new entity metadata** after layout approval; alternative in an Echo Hollow corridor, never both as identical persistent instance.
- **Loop:** light pulses → glances at sigil → moves toward shadow → returns to glow.
- **Observe:** identify repeating pulse interval, +1.
- **Assist:** align a decorative signal prism, +2 (single simple puzzle without blocking story).
- **Spar:** optional and nonlethal.
- **Battle:** Lumen SOFT_GLOW, ECHO_WISP speed control, QUICK_DART.
- **Quest guard:** optional, never a prerequisite to the mandatory gate.

### Wild WS-024 Craglet — Echo Hollow guardian
- **First sighting:** `TRIGGER_INTRO_BATTLE` existing authored entity at Z04 (24,21), mandatory story battle against Lv3 guardian, one-time flag `FLAG_INTRO_BATTLE_WON`.
- **Loop:** quietly guards ancient threshold, becomes alert when a signal pulses.
- **During Q005:** mandatory friendly/nonlethal guardian trial; **cannot be recruited as battle reward immediately** because this would unexpectedly interrupt the story/party.
- **After Q005:** optional rest encounter `C_CRAGLET_REST` back in Echo Hollow, distinct once-only instance. Observe its worn stone marks +1; re-seat loose brick without consumable +2; then invite if trust ≥3, giving a path to own all nine candidates in later local play.
- **Battle:** Stone tank, two attack moves and one limited defensive action.
- **Quest guard:** intro guardian cannot be farmed for duplicate rewards; loss returns to checkpoint with healed companion.

### Wild WS-067 Hushflame — Echo Hollow chamber
- **Proposed spawn:** `C_ECHO_HUSHFLAME` after Q005 on a safe, optional altar alcove. Requires new metadata after blueprint review.
- **Loop:** faint floating light at quiet inscriptions → glows when nearby signal sounds.
- **Observe:** listen/watch recurring 3-note pulse +1 (also show visual cue).
- **Assist:** align two noncritical resonant chimes so its hiding place settles, +2.
- **Battle:** SOFT_GLOW, ECHO_WISP, RESTORING_GLOW support.
- **Quest guard:** never required for core puzzle; added after first 20-min chapter.

## 5. Habitat data and world authoring

### Proposed earliest encounter placements
```text
Z01 Lantern Quay: choose exactly one of WS-004 / WS-008 / WS-050 as starter
Z02 Mosswake Path: WS-005 Tumbletail (existing C_MOSSWAKE_WILD); WS-021 Puddlefin (add rain-pool encounter)
Z03 Liltwood Forest: WS-012 Mossmelt (add C_FERN_RING metadata); WS-001 Signalit (add dusk signal encounter)
Z04 Echo Hollow: WS-024 Craglet required trial; WS-067 Hushflame optional after chapter; WS-024 optional recruitment in distinct encounter after chapter
```

Every roaming creature must have an authored position/area, obstacle-aware bound, specific idle + alert animation and single meaningful habitat action. Existing map fixture only carries some generic landmark/event IDs; **do not say all six encounter placements are already implemented in map JSON**.

### Encounters versus population
- Ambient silhouette wildlife may appear without recruitment/interactions, but don't reuse a quest-critical creature instance as extra random spawns.
- Prefer 1–2 living creatures visibly moving in an early clearing rather than ten stationary sprites; prevent unnecessary alpha overdraw on mobile.
- Optional wild species can be encountered repeatedly *visually*, but each invitation is a logged unique creature instance with deterministic persistence.
- Original creature art retains all original colors; do not apply unauthorized shape redesign to force artificial habitats.

## 6. Evolution and long-term growth (not finalized)

- No evolution family links are authored for the first nine, even if artwork resembles a later stage. The owner must choose whether a second art-cell corresponds to evolution or a new species.
- Proposed: optional, bond + discovery/location gate, with all original forms still viable competitively. Evolution should never delete source sprite.
- No generated AI sprites, no paid evolution-only item and no online requirement for story evolutions.
- Prototype growth may use levels 1–30 stat rule; experience curve, evolution threshold and additional learnable moves require later approval. Level 5 starter and Level 3 guardian chosen for first encounter feasibility, not assumed final balance.

## 7. Offline vs online ownership and restoration
- Local wild recruits persist to offline save only; creature ownership data includes source species ID, encounter instance ID, bond history hash for debugging (not security), obtained flag, nickname, level, HP and move set.
- Reinvite after save reload must return existing creature, not spawn a duplicate ownership transaction.
- Optional online co-op uses separate server-trusted progression as proposed, with inventory bridge explicitly unapproved. Offline tampering cannot grant server-tradable or PvP-eligible creatures.
- Do not make online login mandatory to unlock the Chapter 0 companion.

## 8. Tests before authorizing Codex gameplay implementation
1. User can choose each of three starters exactly once per local save without softlock.
2. All six wild species have observable ecological action and an optional route to invite, including boss species only after chapter completion.
3. Trust cannot be farmed by repeating Observe/Assist/Spar flags.
4. Never require a specific starter affinity for a critical forest puzzle.
5. If party 3 is full, capture/invitation succeeds to reserve without replacing a companion (requires reserve UI spec/implementation first).
6. Airplane-mode saves maintain trust, habitats, encounter outcomes and companion ownership.
7. Friend/foe dialog is consent/bond fiction, not borrowed capture-device imagery.
8. Revisit/backtrack preserves wild instance identities and resolved environmental assistance.
9. Enemies do not reappear on exits as duplicate unique inventory sources.
10. Owner approves final names, habitats, evolution status and art pipeline before committing nine species as canonical.

**Production boundary:** The user asked for detailed planning; this document does not imply the sprites have been extracted or game code implemented.
