# WILDSIGNAL — Casual Co-op Companion Projection & Cosmetic-Only Commerce UX v0.1

**Status:** PRODUCT/UX PLANNING ONLY. D-020/021/023/024 are owner-approved directions. No multiplayer code, account backend, checkout or purchase integration has been created.
**Confirmed:** Free-to-play, optional **cosmetic-only** purchases; two-player Android↔Windows co-op at commercial launch; complete offline local adventure; independent server-verified online inventory; appearance of a familiar offline companion in casual co-op **when technically feasible**.
**Related:** `OFFLINE_ONLINE_SYSTEMS_v0.1.md`, `PRODUCT_VISION_RELEASE_SCOPE_v0.1.md`, `UI_UX_BIBLE_v0.1.md`, `FIRST_NINE_CREATURES_v0.1.json`.

## 1. Critical mental model

There are two *separate* progression stores. A **Local Adventure creature** is earned/kept locally, entirely offline. An **Online Expedition creature** is earned/verified by the server and can affect online rewards/gameplay. A **Projected Companion Look** is cosmetic visual data, *not* creature ownership or online attack/stat identity.

**Friendly UX vocabulary:**
- "Local Adventure" — offline character and companions.
- "Online Expedition" — verified co-op profile, battle stats and rewards.
- "Show my familiar companion's look" — cosmetic local appearance only, where permitted.
- "Online companion" — server-approved gameplay creature; its stats/moves are shown in party details.
Never write "import creature", "sync creature", "transfer progress" or "use your offline team online" without a specifically approved/implemented data model.

## 2. Proposed launch flow (two users, PC and Android)

```
Main Menu (no internet needed)
   ├─ Continue Local Adventure → local quests/battles/saves
   └─ Online Expedition (opt in) → account session verified
                                  ├─ Online Home (own server profile)
                                  ├─ Create/Join private 2-player room
                                  ├─ Set companion visual presentation
                                  │    ├─ Online verified companion look
                                  │    └─ Familiar offline cosmetic look [only if allowed]
                                  ├─ Both ready → online instance
                                  ├─ Explore / interact / optional co-op rewards
                                  ├─ Leave, disconnect or reconnect
                                  └─ Online Home → Main Menu → Local Adventure unchanged
```

Player A may host Android and Player B Windows, or vice versa. The room authority is the backend, not whichever client opened lobby. Launch baseline precisely **two** connected players, not a promise of huge public hubs.

## 3. Cosmetic-only local companion projection draft protocol (must be security reviewed)

- **Server authority:** all real online combat, health, cooldown, abilities, rewards, creature ownership IDs, stats, quest flags and tradable items come from authenticated server records. Never import modified local HP/moves/rarity into online state.
- **Input whitelist:** client MAY submit a *visual request* containing a catalog species art ID (`WS-###`), approved skin palette/outfit IDs and animation variant; server checks these identifiers against **currently published cosmetic projection whitelist**. Player source art origin (local save claim) alone is NOT proof of a verified creature or of a paid cosmetic entitlement.
- **No cosmetic economy bypass:** projected base companion appearance is allowed only within a **free, defined whitelist**. Purchased scarves/skins/companion accessories require independent verified entitlements. Requesting a paid appearance locally cannot spoof ownership.
- **Teammate clarity:** a small `Guest look` cosmetic badge or a companion label distinguishes appearance from actual verified online gameplay creature if the two differ; verified party panel always shows the true online creature. Do not mislead players about who is battling.
- **Scope:** only **casual noncompetitive co-op**; disable local visual projections for future ranked PvP, auctions, trading screens or competitive capture rewards unless later design explicitly defines trust model.
- **No local save upload:** do not upload an entire offline save or use its checksum as proof; the only cross-boundary data for this feature is a bounded visual ID request. If nickname/cosmetic user text is included later, apply chat/name moderation controls.
- **Fallback:** if requested species is not released/allowed for projection or has missing client sprites, display actual verified online creature; show concise message: "That companion look isn't available in Online Expedition yet. Your local companion is safe in Local Adventure." No secret stat reset or silent import.
- **Offline integrity:** returning to Local Adventure never modifies its creatures or quest flags due to online projection; local display choice may be stored as *device preference*, not ownership.

**Important unresolved experience detail:** if a player has no verified online creature yet, how do they start co-op? Recommended small default verified starter from server on first account creation, chosen once and independent of Local Adventure selection; requires explicit UX review and anti-duplication/provenance tests. Do not promise a fresh account can spawn with a projected creature as its gameplay entity.

## 4. The co-op reward and reconnect questions to resolve before implementation

| Topic | Current safe planning default | Still needs approval/testing |
|---|---|---|
| Instance capacity | exactly 2 player slots | spectators, future 4-player |
| Cross-play | Windows x64 and Android arm64 supported clients | legacy Windows online TLS/security |
| Exploration progress | server issues online rewards | shared quest grants, per-account flags |
| Server creature inventory | server-origin creatures only | initial starter, reserve migration |
| Local companion look | free approved visual species whitelist | art variant list, UI disclosure/fallback |
| Reconnect | grace period with server-side session state | duration, duplicate sessions, host loss |
| Session leave | preserve server saved progress | boss reset / rewards if one player leaves |
| Purchasing cosmetics | server verifies store-specific receipts | recovery/refunds/revocation, SKU prices |

For offline Chapter 0, network failures never enter this flow. No shared local story state is implied by casual co-op.

## 5. Cosmetic-only launch design constraints

**Approved monetization invariant:** F2P app, optional cosmetics only, zero competitive power:
- Allowed candidates: outfit/scarf color palettes, nonstat companion ribbons/badges, compass casings, portrait decorations, themed emotes. Small number of free items must remain attractive.
- Forbidden: stronger creatures, exclusive competitive moves, XP multipliers, encounter-bond probabilities, catch/battle energy refills, real-money materials required to pass puzzle, premium faster travel, paywalled story.
- First-person cosmetics need a **visible value proposition**: small sleeve/compass overlay during solo adventure, customizable avatar seen by co-op partner, profile preview and optional celebration poses; do not promise a full third-person camera.
- To limit workload, first cosmetics are **palette or accessory variants** on approved human/companion source sprites. Every new combination increases authored frame count; measure art budget before pricing.
- Never ship placeholder paywall screens in first slice. In demo, disabled store option may be omitted entirely rather than teasing unreleased checkout.

### Purchase authority and platform separation (future only)
- Android version uses Google Play Billing integration where required; Windows Steam version follows Steam transaction mechanisms/policies. **Do not assume** one platform receipt automatically grants cross-store entitlement. Account-binding/entitlement portability must be designed and vetted.
- Backend must verify receipts/entitlements and handle acknowledgements, replayed purchases, refund/chargeback/revocation and delayed platform responses. UI says `Owned` only on trusted confirmed state.
- A direct standalone legacy Windows 7/8/8.1 release may have **no checkout** until a compliant and secure store path is chosen; offline core must remain playable regardless. Do not circumvent store policies.
- Privacy/age/social content and regional currency/tax obligations should be reviewed with current policies nearer launch. We are not conducting legal compliance approval merely by planning.

## 6. Future acceptance cases (not executed)

1. On Android airplane mode, first local play has no account prompt and no disabled story-critical action.
2. New verified online account receives a legal companion for casual co-op without importing local creature inventory (proposed server starter contract).
3. A PC player and Android player join either host arrangement, see only 2 slots and synchronize authorized positions/companion looks.
4. Editing offline creature HP or rarity never changes server online creature stats/rewards/tradeable stock.
5. Selecting locally owned WS-004's **appearance** in casual co-op changes only the visible billboard/look if allowlisted; online verified creature remains unchanged.
6. Requesting arbitrary `WS-999` or unreleased/premium variant is rejected to safe cosmetic fallback, no crash.
7. Two simultaneous join/leave/reconnect attempts cannot duplicate creature rewards or items.
8. Offline save checksum or client-signed badge is never accepted as a backend legitimacy proof.
9. Closing online session and launching Local Adventure returns exactly unchanged HP/items/quest flags except deliberate local changes made while offline.
10. User can tell the difference between Local Adventure / Online Expedition and visually projected guest look in under one tutorial tooltip (qualitative playtest).
11. Purchases have no gameplay modifiers and verified entitlement + refund behavior is covered by backend tests before checkout.
12. Win7/8/8.1 standalone offline client never requires Steam or checkout login; multiplayer support there remains security-gated and may be excluded with explicit owner review.

## 7. Out of scope for this planning cycle
- Implementing network authority, matchmaking, TLS, social moderation backends, Steam/Google store billing, IAP SKU prices, receipt verification or a working player-avatar projection.
- Marking unverified legacy Windows online as supported.
- Changing source creature art, generating cosmetics, actual paid-asset storefront mockups.
- Declaring cross-store purchased cosmetic entitlement portability.

**Owner review still required:** exact online onboarding starter, acceptable label for guest appearance, which species looks can be projected at launch, co-op rewards ownership and old-Windows-online support boundaries.
