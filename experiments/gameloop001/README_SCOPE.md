# GAMELOOP-001 — playable slice integration gate (scaffold only)

**Scope record:** The scaffold below records the authorized acceptance gate. A playable implementation now exists on this branch; see `README.md` and `docs/reports/GAMELOOP001_IMPLEMENTATION.md` for actual validation and remaining owner hardware review. This does not promote art or hardware performance to approved.

Owner authorized integration on 27 October 2026: core offline gameplay + visible Petalyn/Driftfin follower + existing R4/R5 foliage with increased real meadow coverage + Quaternius MegaKit authentic house test. All previous original branches/drafts stay unmerged.

- [Full public gameplay specification #47](https://github.com/jonaslacandola0617/wildsignal/issues/47)
- [Companion private art kit #35](https://github.com/jonaslacandola0617/wildsignal-art/issues/35)
- New branch `feat/gameloop-001-playable-glade` based on Petalyn runtime commit `27dcaffcd1ae9ff0edaf81d75ee8a6285dddbc1e` / [draft PR #46](https://github.com/jonaslacandola0617/wildsignal/pull/46).
- R5 engine comparison [PR #45](https://github.com/jonaslacandola0617/wildsignal/pull/45) and Quaternius [PIXEL3D PR #23](https://github.com/jonaslacandola0617/wildsignal/pull/23) live on DIFFERENT experimental branches. Resolve code dependencies carefully and add only necessary bounds-safe adaptations. No blanket merging of drafts.
- All rendered private art, canonical creature GLBs, Quaternius licensed meshes/textures and raw R5 grass source stay in `jonaslacandola0617/wildsignal-art`, never this public repository. Public here contains code, safe configs, manifests without restricted art.

## Game sequence — MUST run

1. Launch a genuine standalone Windows executable; camera/movement/collision works and density-modulated meadow looks grassy.
2. Observe visible animated Petalyn or Driftfin following without blocking camera.
3. Approach authentic Quaternius modular cottage, collide with walls, and interact with an entry-adjacent prompt (interior optional).
4. Approach wild creature, start turn-based encounter, Attack/Defend/Befriend, enemy response, win/recruit outcome, return to exploration.
5. Save to local file, quit EXE completely, relaunch and load position, recruitment and HP/flags without corruption.
6. Test a valid grass/asset inventory, performance baselines grass OFF/ON and house OFF/ON at 480/640, and proper fallback on missing private assets.

### Acceptance measurements
- Grass needs **>=75% visible ground coverage over designated meadow regions** at actual eye-height views; mask excludes roads, house, interaction clearing, rocks. Record >=5 camera positions and the method; visual art direction preserved, density/culling tuned, blade shadows OFF.
- 480x270 with 30fps render budget on real Intel UHD old 4GB target; 640x360 60fps stretch only if actual hardware can achieve it. Separate API loop/GPU elapsed, CPU animation, frame pacing (old Issue #33); i5/8GB earlier samples are different hardware.
- Quaternius standard kit CC0 genuine textured house/gable/windows/doors plus 2–4 factual matching props, actual loaded via present HOLLAND renderer. Include source/asset IDs/hashes, wall collision, first-person inspection; label whether style needs WILDSIGNAL bespoke assets after identical-camera A/B.
- No Android/Windows7/online/co-op claim in this slice. No full six zones, 100 production creature imports or monetization.

## Useful controls in review build

Proposed review flags: `--grass-density off|low|medium|high`, `--megakit-house on|off`, `--render 480|640`, `--shadow cached512`, `--benchmark`. Pick flags supported by actual implementation and document them accurately; **this README is not evidence these flags exist yet**.

Stop at `GAMELOOP001_DENSE_GLADE_MEGAKIT_OWNER_PLAYABILITY_AND_INTEL_REVIEW_PENDING`, provide fully functional private-art-backed portable ZIP and truthful Linux/Windows/hardware validation evidence, draft PR only.