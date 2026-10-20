# WILDSIGNAL — Offline/Online Architecture v0.1

**CONFIRMED (user, 2026-10-20):** Offline adventure with no mandatory account; optional two-player Android↔Windows co-op at commercial launch; independent local offline and server-verified online progression; familiar local companions may be displayed cosmetically in casual co-op *where feasible*. Exact implementation, account recovery and presentation fallbacks are still proposed.

## Player modes
- **Local Adventure (offline):** from a cold start with airplane mode: create/choose local save, explore all story-critical regions, choose starter, befriend creatures, turn-based battles, solve puzzles, unlock chapters, return to save; no network calls required for game logic. Multiple local save slots are a later scope decision.
- **Online Expedition (opt-in):** explicit sign-in/account; join a server-authoritative hub or 2-player cross-platform co-op instance at launch. Trading and ranked PvP are **postlaunch plans**, not minimum initial online features. An outage permits exit to local adventure without corrupting its save.
- **Shared client code:** deterministic battle/world simulation and content data may be shared, but local simulation and server simulation have different authority boundaries.

## Approved trust boundary and open cosmetic projection details
### OWNER-APPROVED PLANNING DECISION: Two trust realms
- Local save has local story flags, inventory, earned creatures and local economy.
- Online account has server-authored online character, tradable creatures, currency, cosmetics, matchmaking rank and online reward history.
- Players may choose visually matching avatars and starters, but this is not a direct import of local items.
- **Approved product direction, feasibility-gated:** casual co-op should display the *appearance* of the familiar offline companion where technically feasible, but this is an untrusted, noncompetitive **visual projection**. The server verifies any allowed cosmetic species/shape ID and controls all battle stats, rewards, inventories and entitlements. A cosmetic projection is **not a purchased cosmetic SKU** or transferable companion. If secure/clear presentation is not feasible, show verified online companion instead and inform the player; owner review required for UX fallback.
- Online story progress can be separate; alternatively share noncompetitive quest completion cosmetics only after server verification.

### REJECTED AS CURRENT PLAN: Limited attested import
- An offline creature could theoretically enter casual online co-op marked **unverified/non-tradable**, but this is **not the selected planning model**. Do not implement it unless separately approved. Never enter trading, ranking, global market, or competitive PvP.
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

**APPROVED:** Separate local and server-verified online progression; desired *visual-only* local-companion representation in casual co-op when feasible; two-player Android↔Windows co-op at launch. **APPROVAL/VALIDATION STILL NEEDED:** secure companion visual projection allowlist and fallback behavior; avatar selection and account linkage; online quest persistence; offline save slots/migration/backup UX; disconnect and co-op quest ownership; exact server deployment, multiplayer security, trading and PvP postlaunch.


## 2026-10-20 Trust-domain projection acceptance requirements (new)

1. An offline local creature `WS-004` may inspire a matching **visual** appearance in a casual lobby only when that appearance passes the server's allowed-species/cosmetic whitelist. This does not create online possession of `WS-004`, upgrade an online creature, or entitle trading.
2. Display two distinct labels if both are shown in UI: **Local Adventure companion** and **Online Expedition companion**. Avoid implying an online character's stats/identity are imported.
3. Online combat and co-op rewards depend exclusively on server-authored online creature ID, stats and event outcomes. Offline SaveData edits change no server results.
4. The projected appearance must not mislead opponents or teammates about competitive species if PvP is eventually added; keep projection limited to casual modes, or display a separate verified identity/stat label.
5. When cosmetic projection unavailable (unreleased species, missing asset, incompatible client version or security rule), show a known available verified cosmetic or online companion, tell player the difference, and do not silently copy creature ownership.
6. Each online instance supports exactly two player slots for launch baseline, with Windows/Android clients able to join either slot. Larger rooms only after measured test and approval.
7. Offline story can always start, load and finish if account or online service fails; online-only UX is clearly labeled.
8. No online inventory/trade/receipt system treats local checksum or encrypted local file as proof of legitimate provenance.

**Implementation remains unauthorized until a separate approved task and acceptance test.**
