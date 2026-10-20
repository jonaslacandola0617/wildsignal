# WILDSIGNAL — Offline/Online Architecture v0.1

**CONFIRMED:** Offline story adventure with local saving and no mandatory login; optional online multiplayer. **PROPOSED:** Data-trust policy and two-profile model.

## Player modes
- **Local Adventure (offline):** from a cold start with airplane mode: create/choose local save, explore all story-critical regions, choose starter, befriend creatures, turn-based battles, solve puzzles, unlock chapters, return to save; no network calls required for game logic. Multiple local save slots are a later scope decision.
- **Online Expedition (opt-in):** explicit sign-in/account; join a server-authoritative hub or an instance with friends; presence, co-op, PvP and trading as optional activities. Network outage returns an intelligible error and lets player return to local adventure without corrupting any save.
- **Shared client code:** deterministic battle/world simulation and content data may be shared, but local simulation and server simulation have different authority boundaries.

## Crucial unresolved product decision: how offline companions interact with online progression
### PROPOSAL A (RECOMMENDED for secure trading): Two trust realms
- Local save has local story flags, inventory, earned creatures and local economy.
- Online account has server-authored online character, tradable creatures, currency, cosmetics, matchmaking rank and online reward history.
- Players may choose visually matching avatars and starters, but this is not a direct import of local items.
- Casual co-op can optionally project local companion *appearances* as guest-only cosmetics while battle inventory and rewards come from server-validated profile. Requires design work and unambiguous labels.
- Online story progress can be separate; alternatively share noncompetitive quest completion cosmetics only after server verification.

### PROPOSAL B: Limited attested import
- An offline creature could enter casual online co-op marked **unverified/non-tradable**. Never enter trading, ranking, global market, or competitive PvP.
- Server limits unverified inventories and never grants tradable rewards from their stats. Hard to explain and defend; more implementation complexity. Do not implement without approval.

### Not acceptable
- Blind upload of local save data as trusted online creatures/items/currency.
- Client-reported PvP wins or receipt ownership.
- Treating file hashes, encryption or client-side signatures as sufficient to establish legitimacy of offline progression.

## Local save contract (proposed)
- Atomic save: write new temp data, fsync as platform permits, validate checksum and schema, rotate prior backup, then replace current slot.
- Header: `save_schema_version`, `game_content_version`, `slot_id`, `created_at`, `updated_at`, integrity checksum for corruption detection (NOT anticheat).
- Data: `player_id_local`, `avatar`, `zone_id`, `spawn_id`, `quest_flags`, `inventory_local`, `creatures_local`, `party`, `bond_states`, `puzzle_states`, `rng_seed`, `options`.
- Use stable IDs not unversioned positional list indices. Never save raw pointers or internal addresses; avoid device-dependent serialization.
- Cloud backup is optional and distinct from multiplayer authoritative progression; unresolved store/platform backup and conflict policy.
- On corrupted primary load, offer newest intact backup and show what was restored, without overwriting automatically.
- Save at safe milestone and explicit menu action; auto-save only after durable transaction boundaries.

## Network/server spec (proposal)
- Authoritative online service controls player movement bounds, creature ownership, battle turns, trade atomicity, inventories, currency, time-sensitive shared-world changes and purchases.
- Online battle: seed and turn order live on server; command window with server-owned timeout; disconnect means pause/forfeit policy to approve; don't let client send results as truth.
- Movement uses limited-rate snapshots/input commands, interpolation for nearby players, no client-side teleport trust.
- Region channels: `join_zone`, `leave_zone`, `player_state`, `creature_visual_state`, `battle_command`, `battle_result`, `puzzle_interact`, `quest_event`, `trade_offer`, `trade_confirm`, `entitlement_update`.
- Each channel requires versioned schema, authentication, replay resistance, validation and rate limits.
- Online player cosmetics acquired through storefront undergo server receipt verification; never persist secrets or signing keys in Git.

## Test gates
1. Boot device in airplane mode, from cold start; start new local game, finish first quest and save/load.
2. Disable network at every screen/transition: no crash, no local progress lost, no silent upload.
3. If online auth fails, local game remains playable.
4. Tampered local save cannot yield transferable/tradable competitive assets.
5. Online item trades atomic and idempotent, including disconnect/retry.
6. Online backend outage affects optional online screens only.
7. Android app reinstall/cloud restore and Windows save migration tested separately.
8. Permission/privacy and entitlement checks validated for each storefront.

**APPROVAL NEEDED:** A or B; online progression and co-op guest companion policy; offline save slots, migration and backup UX; disconnect behavior.
