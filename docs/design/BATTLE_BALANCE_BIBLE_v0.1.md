# WILDSIGNAL — Battle & Balance Bible v0.1

**Status: PROPOSED — requires game-design approval and playtesting.** Turn-based combat is CONFIRMED, detailed numerical rules are NOT. Date 2026-10-20.
**Source roster:** `FIRST_NINE_CREATURES_v0.1.json` (nine owner-art sprites, 18 working moves).
**Test vectors:** `BATTLE_TEST_VECTORS_v0.1.json`; deterministic expected values are calculated and included.
**Design principle:** Accessible mobile 1v1 tactics, expressive creature silhouettes, predictable results, no paid stat advantages, fully offline, with possible reuse on authoritative online server.

## 0. Non-negotiable invariants
- Offline story/encounters run from local game data with no network.
- Online PvP/battles/trades require server authority; never upload untrusted local outcome to grant items or rank.
- Deterministic game state transition from **content version, battle state, input commands, and RNG seed**. No floating-point combat mathematics or wall-clock dependence.
- Four commands per creature; one active; up to three in party (proposal, as in Creature Bible); no timed offline command selection.
- Equivalent starter power budgets; novelty/rarity does not set base stat superiority.
- Each starter can complete Q001–Q005 and the first compulsory battle without a purchased item or online support.
- Enemy can recover from losing/retreating; no permanent starter loss in tutorial.

## 1. Stats, levels, combat resources

Stats: `hp` maximum hit points, `might` for physical attack, `guard` for physical defense, `pulse` for signal attack, `ward` for signal defense, `swift` for move order. All integers.

- Level range: 1..30 initial.
- Derived max HP: `base.hp + 4*(level-1)`.
- Derived other stats: `base.stat + (level-1)`.
- Starting starter level: **5**; first authored guardian `WS-024 Craglet`: **3**. Other wild encounters level 2–4.
- No IV/EV breeding bonuses, random trait/stat multipliers, equipment-derived power or critical-hit chance in initial prototype.
- Battle-only **Signal Energy**: 0–8 integer, 4 when battle begins; +2 at start of turns **2 onward** (capped at 8). Energy cannot be purchased. Costs are deducted only when an action actually executes successfully.
- One energy pool per active creature. On switching, a new creature enters at the energy it had when last active in this battle; first activation starts with 4. Energy never persists across separate battles.
- HP persists after battle in local story; safe rest points refill HP. Defeat in intro encounter restores starter and retries without penalty.

### Level-5 starter stats
| Species | HP | Might | Guard | Pulse | Ward | Swift | Strength |
|---|---:|---:|---:|---:|---:|---:|---|
| Glintle WS-004 | 64 | 15 | 19 | 18 | 19 | 14 | defensive signal/utility |
| Driftfin WS-008 | 60 | 15 | 14 | 18 | 16 | 22 | fast, controls tempo |
| Brumlet WS-050 | 62 | 20 | 16 | 18 | 14 | 16 | burst damage after setup |

These are balancing hypotheses, not final approved stats.

## 2. Turn sequence — normative proposed rules

```text
BATTLE_START -> TURN_START -> SELECT_COMMAND -> VALIDATE_AND_LOCK -> ORDER_ACTIONS
 -> EXECUTE_FIRST -> KO_CHECK -> EXECUTE_SECOND (if able) -> KO_CHECK
 -> END_PHASE_STATUSES -> KO_CHECK -> RESOLVE_FORCED_SWITCH_OR_END -> TURN_START
```

1. **TURN_START:** increment `turnIndex` (1-based), regenerate +2 energy for active living creatures if turnIndex > 1. Newly introduced creatures do **not** get double regeneration when they first switch in. Statuses `SLOW` affect the current turn from previous turns. Combat state remains paused in offline menus.
2. **SELECT_COMMAND:** each side chooses exactly one legal move, `SWITCH`, `ITEM`, or `RETREAT` when encounter mode permits. For initial PvP, `ITEM` and `RETREAT` are disabled. No real-time timer offline; server input deadline TBD for PvP.
3. **VALIDATE_AND_LOCK:** verify creature alive, move belongs to it, energy >= cost at the start of action sequence, target valid, swaps permitted, inventory available and encounter mode permits item/retreat. Invalid command becomes `TAP` for automated opponent only; humans must reselect offline, server must reject or apply published timeout default. Never let a client invent a move ID.
4. **ORDER_ACTIONS:** priority descending, then effective swift descending, then deterministic seat alternate: on odd turns seat 0 wins exact tie; on even turns seat 1 wins. `SWITCH` priority +3, `ITEM` +2, `RETREAT` +4, individual moves use declared `priority` in JSON (e.g. BRACE +1). `SLOW`: `effectiveSwift=max(1,floor(swift*75/100))`, applies to following 2 full rounds after receipt.
5. **EXECUTE:** actor must still be alive and be the active creature; otherwise action skipped, **without energy deduction**. Swap changes active creature (and clears that creature's GUARD). For a legal move, deduct cost, check accuracy (see RNG), apply damage/healing/status, emit visual event keyed to same simulation result. If target fainted, it cannot perform pending move.
6. **KO_CHECK:** immediately after each action. If one team has no living creature left, end battle; if reserve creatures remain, forced switch occurs at start of the next turn, before regular selection. Forced switch has no energy cost and cannot be used as an extra attack. Never choose a KO'd creature as the forced replacement.
7. **END_PHASE_STATUSES:** for each living creature in seat order, BURN deals 3 fixed HP then REGEN heals 3 HP; a creature reduced to 0 HP by BURN does not heal and is KO'd. Tick applied BURN/REGEN on this end phase if just applied; expire after exactly two end phases. GUARD expires at the end of the same turn if no hit consumed it. SLOW and FOCUS duration counters do not decrement on the turn they are applied, but decrement on subsequent full turn ends. Trigger KO_CHECK again.
8. **BATTLE_END:** server/local simulation returns an explicit result plus event list, never direct UI mutations. Local world resumes from a stable saved checkpoint. Award-once IDs are recorded transactionally.

**Switch/retreat design simplification:** swapping does not itself attack. Enemy cannot switch before the initial authored boss combat. PvP item policy and disconnect default remain open decisions.

## 3. Accuracy & seeded PRNG

- Move `accuracy` integer from 0 to 10000 (10000 always hits). Avoid floats/percentage samples.
- When `accuracy < 10000`, take **one** unsigned integer from a battle-owned `xorshift32` state and compute `roll = next() % 10000`; hit iff `roll < accuracy`. No draw for 100% accurate moves. Tie-breaking does not use RNG.
- `xorshift32` norm: uint32_t `x`; `x ^= x << 13; x ^= x >> 17; x ^= x << 5;` with shifts/casts in uint32_t; return `x`. Do not allow seed zero (normalize to 1 when loading).
- For seed=1, first PRNG outputs: `270369 (roll 369)`, `67634689 (roll 4689)`, `2647435461 (roll 5461)`, `307599695 (roll 9695)`.
- Save/replay initial seed plus deterministic draw order and action log; message sequence numbers prevent duplicate application online.
- Initial 18 move candidates are accuracy 10000, by deliberate early-prototype design; accuracy RNG tests anticipate future moves and must not accidentally consume RNG in v0.1.

## 4. Affinity interactions

**Early prototype: one directional cycle only.** Flora > Tide, Tide > Ember, Ember > Flora. In a reverse matchup disadvantage multiplier 80%; advantage 125%; otherwise neutral 100%. All pairs involving Stone/Lumen/Wild or same affinity are neutral in this first prototype. Later Gale/Gloom and complete 8×8 table are out of scope.

No same-type bonus, crits, weather damage or damage variance in initial battle tests.

## 5. Damage calculation — integer-only, exact order

For `physical` attacks choose `off=attacker.might`, `def=target.guard`; for `signal` choose `off=attacker.pulse`, `def=target.ward`. All stats >=1.

```
raw = max(1, floor(power * off / (2 * max(1,def))) + floor(attackerLevel/4))
afterAffinity = max(1, floor(raw * affinityPct / 100))  // 80, 100, or 125
afterFocus = max(1, floor(afterAffinity * (FOCUS?125:100) / 100))
finalDamage = max(1, floor(afterFocus * (GUARD?50:100) / 100))
damageApplied = min(targetHP, finalDamage)
```

- `power` is positive for attacks and zero for support. Attacker level is 1..30. Integer intermediates must use sufficiently wide integers (64-bit before division), then clamp to safe stat/HP limits.
- `FOCUS` only consumed on the owner's next **landed damaging** move; moves that miss do not consume it.
- `GUARD` halves the **next incoming landed damaging move**, then clears; it expires at end of the current turn if unused. It does not prevent BURN tick.
- If target HP already 0, don't attack and do not spend cost.
- Combat log records raw, modifiers, finalDamage, actual HP delta; turn order must not depend on animation timing.

Examples (all level 5 unless noted):
- Glintle SPROUT_DART (power 18, Pulse 18) vs Driftfin Ward 16: `raw=11`, Flora>Tide ⇒ `13` HP damage.
- Driftfin SLIPSTREAM (power 20, Pulse 18) vs Brumlet Ward 14: `raw=13`, Tide>Ember ⇒ `16`.
- Brumlet EMBER_KNOCK (power 20, Might 20) vs Glintle Guard 19: `raw=11`, Ember>Flora ⇒ `13`.
- Glintle SPROUT_DART vs Craglet level 3 Ward 18: `10` damage (neutral matchup).
- Brumlet EMBER_KNOCK vs guarding Glintle: `13 ⇒ 6` damage.
- Glintle with FOCUS vs Driftfin: `13 ⇒ 16` damage.

## 6. Initial statuses and support actions

- `GUARD`: one-hit 50% reduction, until end of current turn. Refreshing same status replaces earlier instance; cannot stack.
- `REGEN`: +3 HP to owner at each of next 2 end phases (the application round counts as first); no heal beyond max HP. Renewing refreshes two ticks, no stacks.
- `BURN`: 3 fixed HP loss on target at each of next 2 end phases, including application round; KO possible. Refresh, not stack.
- `SLOW`: effective swift reduced to 75% for next two complete turns, NOT used to reorder already locked actions; refresh not stack.
- `FOCUS`: next landed damaging move gets ×125% after affinity, before guard; lasts current round plus 2 subsequent rounds, then expires if unused; refresh not stack.
- All statuses must support `sourceId`, `appliedTurn`, `remainingTicks`, `endsAt` where appropriate; use integer IDs and deterministic resolution order.
- All statuses expire when battle ends; persistent negative statuses and cures are future design topics.
- Healing actions happen at execution, not deferred except REGEN. `HEAL 3/8` clamps at max HP.
- Status move success is 100% in v0.1 unless it fails due to illegal target; damaging moves with status apply status only when hit and target survives.
- One outgoing status per targeted creature per status type; buffs cannot compound.

## 7. Starter and first-wild species move library
All IDs, energy costs and power values are defined in `FIRST_NINE_CREATURES_v0.1.json`. 18 distinct reusable moves. Explicit creature move slots:
- Glintle: TAP; SPROUT_DART; LANTERN_GROVE; ROOT_THREAD.
- Driftfin: TAP; SLIPSTREAM; MIST_HUSH; QUICK_DART.
- Brumlet: TAP; EMBER_KNOCK; CINDER_BEAT; COAL_FLICK.
- Tumbletail: TAP; QUICK_DART; BRACE; ROOT_THREAD.
- Puddlefin: TAP; MIST_HUSH; SLIPSTREAM; CURRENT_SHIELD.
- Mossmelt: TAP; SPROUT_DART; MOSS_MEND; ROOT_THREAD.
- Signalit: TAP; SOFT_GLOW; QUICK_DART; ECHO_WISP.
- Craglet: TAP; BURROW_BUMP; STONE_SHELL; BRACE.
- Hushflame: TAP; SOFT_GLOW; ECHO_WISP; RESTORING_GLOW.

### Ability examples
- Glintle LANTERN_GROVE: +1 priority is **proposed correction** so GUARD can protect in same round; 3 energy; gives GUARD and 2 ticks of REGEN.
- Brumlet CINDER_BEAT: 2 energy; sets FOCUS 125% next landed damaging attack.
- Driftfin SLIPSTREAM: 2 energy; straightforward Tide signal power 20. Its "fast" role derives from high Swift and QUICK_DART priority; there is **no unimplemented bonus initiative** attached to SLIPSTREAM.
- Boss Craglet in Q005 can only use TAP, BURROW_BUMP and once-per-battle STONE_SHELL; restrict healing in tutorial to avoid stall, at the same cost as normal move.

For any change to move definitions, update JSON and deterministic fixtures atomically. No stats/abilities should be coded from prose only.

## 8. Battle AI and first-battle fairness (proposal)

**First tutorial battle:** WS-024 Craglet Lv3 vs starter Lv5. The guardian's intent is to test player and protect a ruin rather than depict cruelty. Enemy selects TAP or BURROW_BUMP depending on energy; can use one STONE_SHELL at ≤50% HP, once per battle. No crits and no unpredictable damage variance. If balancing shows its 63 HP lasts too long, tune encounter-specific HP scalar or set a fixed scripted HP state in enemy data, never silently alter base species stats.

- No KO permanently removes owned creature.
- Intro tutorial allows unlimited retries, free recovery and clear explanation of type matchups when relevant; all starter selections must be tested.
- Standard encounters: learn elemental relations with in-world hints not mandatory memorization. Optional sparring increases bonds.
- Mandatory boss cannot randomly flee, permanently disappear or be recruited until after story; allow future separate discovery of its species if desired.
- PvP: no consumable item effects in ranked mode; turn timer, ranking and sanctions remain unresolved.

## 9. UI and animation/feedback requirements
- Battle view uses retro 2.5D background of current biome with readable larger front creature sprite. Sprite ownership source tied to WS-IDs.
- Always show 4 actions, each with energy cost, affinity icon/name and one-line tooltip. Disallow unpayable moves with visible explanation.
- Turn preview indicates priority/speed qualitative order only if determinable; avoid hidden-information leaks in PvP.
- Display HP numeric + bar, signal energy 0..8, status icons with text accessible on tap, opponent tell (animation/portrait).
- UI remains sharp above low-resolution world render. Keyboard and Android touchscreen select same action IDs.
- Effects are event-list-driven, skippable/accelerable; no flashing required to understand outcomes.

## 10. Automated validation and property tests
1. For each starter, initialize Lv5 and verify HP, Might, Guard, Pulse, Ward and Swift vectors.
2. Exact damage examples above match `BATTLE_TEST_VECTORS_v0.1.json`.
3. No floating point or negative damage; finalDamage positive for successful attacks even with GUARD.
4. Energy never below 0 or above 8; cannot execute unpayable moves.
5. BRACE +1 priority executes before ordinary 0; switch +3 before skills; exact ties resolve alternating by turn parity.
6. Burn then regen order correct and deterministic; KO from burn cancels its regen.
7. Slow has no mid-turn reorder; focus consumed only on landed hit; no status stacking.
8. An offline paused battle waits indefinitely for player command; no imposed turn timer.
9. Battle replay from same contentVersion/state/commands/seed generates byte-equivalent state and events on Windows and Android.
10. Online server rejects invalid/duplicated/out-of-sequence commands and grants awards at most once.
11. All three starter variants defeat intro boss with at least one understandable strategy and no unique scarce consumables (playtest, not currently verified).
12. Battle exit/rest checkpoints retain no phantom modifiers, retained swap flag or battle-owned energy.

## 11. Design review blockers
- Is "Signal Energy" more interesting than traditional limited uses? This document proposes energy but needs a playtest.
- Should wild encounters allow direct friendly invitations after aid, battle, or both? Bonding Bible proposes both.
- Should ranked PvP eventually normalize level and stat totals?
- Exact end-phase status edge cases, switch status retention and enemy AI tuning must be implemented against tests and reviewed before content expansion.
- No competitive rewards derived from untrusted offline save files; offline vs verified online profiles still require owner's decision.

**Codex implementation gate:** create a standalone platform-independent C++ combat library only after schema and tests are reviewed; no rendering dependencies, stores or networking introduced in that task.
