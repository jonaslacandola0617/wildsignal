> **2026-10-24 DIRECTION SUPERSESSION NOTICE — CURRENT DESIGN**
> Previous first-person hero camera targets are archived reference. New milestone TOPDOWN-001: small playable top-down Lantern Quay world using actual Pixel Crawler Free Pack tiles, with capture and owner review. Forest/Liltwood and underground/Echo Hollow targets to follow after first scene validation.
> See [Top-down + Pixel Crawler decision](../planning/TOPDOWN_PIXELCRAWLER_DECISION_v0.1.md) for the governing details. Historical material below is preserved unchanged.

---

# WILDSIGNAL — Three First-Person Hero Scene Targets v0.1

**Status:** CREATIVE PLANNING SPEC, not generated images, not production art and not tested in a renderer.
**Canonical prototype map:** `docs/design/maps/FIRST_SIGNAL_BLOCKOUT_v0.2.json`; tiles are 48×32 and **NOT** an engine unit scale. Use its stable zone/entity IDs.  
**Art Bible:** `VISUAL_BIBLE_v0.1.md`, `FIRST_SIGNAL_ZONE_BLUEPRINT_v0.2.md`, `UI_UX_BIBLE_v0.1.md`.  
**Creatures:** Original owner-owned sprites in `ORIGINAL_SPRITE_SHEET_MANIFEST_v0.1.json` (not uploaded as code assets).  
**Production gate:** owner should approve layouts/sketches before Codex implements renderer test and before an environment artist makes hundreds of textures.

## A. Cross-zone composition contract

1. **Beautiful but mysterious**: warm, inviting foreground with one compelling signal/ruin anomaly. Avoid horror gore, military FPS combat framing, heavy black fog masking all art, or imitation of a specific commercial game's textures.
2. **First-person pixel world:** low-res, deliberately stepped wall/floor texture shading plus crisp, lively companion billboards; allow deep outdoor visibility. Camera placement should feel like a human Wayfinder walking, not a flying camera or orthographic tile map.
3. **One clear object hierarchy** per image: route/landmark > companion/reactive element > foliage/ambient props. Each scene readable with HUD and without HUD.
4. **Original ecological sprites** retain their palette, proportions and identifying parts. User-owned creature artwork is source of truth, not generic stock art. Use carefully labeled test placeholders only until four pilot sprites have been restored.
5. **Lighting tricks:** authored texture shading, simple color ramps and 2D signal sprites preferred for first renderer proof. This document does NOT require realtime GPU shadows, 3D mesh shaders, water reflection, volumetric fog or dynamic GI.
6. **Reuse:** three material families: harbor timber/plaster/brass; Liltwood bark/stone/ferns; Echo masonry/brass/mint sigils. Shared mint "signal" glyph shape/signature and readable UX prompts.
7. **Composition comparison:** benchmark candidates must target same location, nominal height, direction, FOV, world resolution and sprite count. If geometry can't reproduce arches or pitched ceilings, mark unsupported and prepare a silhouette-based trick, not a dishonest side-by-side.

### Proposed visual-validation camera settings (NOT engine locks)
- Start with horizontal FOV `75°` and eye position/collision centered in nominated map tile; adjust after owner's pixel reference is reviewed and mobile motion-comfort tested. Use level-plane pitch `0°` for baseline; separately report ±10° look pitch if supported.
- Native world buffer candidates 320×180, 400×225 and 480×270, nearest-neighbor scaling. HUD composites independently at screen resolution.
- Qualitative sprite distance targets: main companion 6–12% of visible frame height when standing ~1–2 abstract tiles behind/side, enemy creature 20–35% frame height at a staged battle distance. **These are framing targets only**; camera/world scale not approved.
- Outdoor forest image should preserve open walkable path (clearly visible floor), not simply texture-filled wall cards. Echo ruin should have clearly distinguishable floor, ceiling and tall arch silhouette, though arch may be billboard/façade if renderer cannot do true openings.

## B. Scene target HERO-Z01 — Lantern Quay, welcoming departure

**Zone / kit:** Z01 / `harbor_01` 
**Draft camera location:** `(24,29)`, north-facing (the zone spawn `Z01_SPAWN_SQUARE`). Secondary reference `(18,16)` east-facing, and `(24,6)` north.
**Narrative beat:** arriving by boat in a living town at golden hour and seeing the beacon pointing to a stranger forest.

**Five visible anchors**:
1. Central **Beacon Tower** rising above roofs near `LANDMARK_BEACON_TOWER` (23,11).
2. **Wayfinder Square** with pale stone/timber path framing player toward Mara's side and workshop approach.
3. Fen's workshop visual motif: giant brass compass rim/lens and tools, approachable (entity `NPC_002_FEN` Z01 33,8).
4. Warm lanterns with small mint signal flecks foreshadowing the forest mystery.
5. Harbor edge/plank strips and simple distant horizon backdrop: outdoors should not read as four gray walls.

**Palette swatches for art briefing** (not the only allowed colors):
- Warm ivory `#F6EBD7`, golden lamplight `#E8C878`, timber `#805C43`, deep tide `#183B41`, foggy teal `#5A8E8E`, signal mint `#74D6B1`.
**Texture direction**: repeating timber plank seams and damp bolts, soft plaster façades, chunky paving with dark joints, canvas flag/banner elements, low-cost fog/horizon rather than render-distance to infinity.
**Actor budget for future benchmark**: 2 named NPC billboards (Mara/Fen) + 2–4 ambient sprites visible when looking into square; one companion sprite only after Q002. 3–8 noninteractive small props in each view. Counts are test setups, NOT final production limits.
**Readability**: horizon/beacon must remain visible at both 320×180 and 480×270. Gold NPC/quest focus cannot be confused with mandatory green mystery signal. Compass lower-screen animation remains optional, not gun-like HUD.
**Avoid**: Doom shotgun HUD, flat corridor maze, overbright bloom, NPC hidden by a fixed front billboard or obstructed by round roof props.
**Accept:** owner can navigate visually between Mara, Fen, starter altar, north gate without requiring a permanent minimap; FPS and alpha renderer tests not yet run.

## C. Scene target HERO-Z03 — Liltwood Singing Root discovery

**Zone / kit:** Z03 / `liltwood_01`
**Draft camera:** `(22,16)`, northeast toward `LANDMARK_SINGING_ROOT` (29,14). Secondary `(34,11)` northeast toward `PROP_VEILED_WALL` (36,10), `(40,6)` north/east toward ruin gate (42,3). Exact yaw subject to actual geometry and occlusion.
**Narrative:** a living forest full of curious species with a single ancient root that emits something unnatural.

**Five anchors**:
1. Irregular **Singing Root** great trunk/root coil, nearly centered at a clearing focal point; a signature silhouette readable at low res.
2. Clear moss/earth walkable **looping path**, 2–3 tiled bands visible, not dead straight corridor.
3. Fern, mushrooms, fronds as **nonblocking alpha billboard groups** with limited depth/overdraw; blocked foliage volumes remain authored collision.
4. One original user-owned companion in midground reacts toward tree, 1 optional roaming original wildlife species near Fern Ring (not over the ruin gate).
5. Faint **mint triangular signal marks** under tree bark, paired with audible motif and text/icon alternative.

**Palette**: deep bark `#4B4634`, forest jade `#367D62`, mist moss `#79B77C`, muted earth `#A38B62`, golden waymarker `#E8C878`, signal mint `#74D6B1`.
**Lighting**: dappled painted foliage, atmospheric forest fade using cheap distance shading, signal pulsation as texture-strip swap or simple sprite. No per-pixel realtime leaf shadows required.
**Alpha stress case**: render 1, 8, 24 sprites with mixed tree fronds/wildlife; log distinct draw order and sorting limitations. 10–20 foliage cards is a separate test level, not a required final scene count.
**Readability**: root glyphs must be distinguishable from natural bright greens by shape and interaction prompt; forest route not obscured by nearest billboard greenery. Every starter reveals same quest-critical clue via different reaction animation.
**Avoid**: neon green glowing everywhere, full-screen alpha overdraw, unclear route, borrowed creature designs.
**Accept**: first glance gives place mystery, open navigation, and companion life; owner recognizes original creature silhouette; exact shadow/fog mix to be tested.

## D. Scene target HERO-Z04 — Echo Hollow arch & pedestal

**Zone / kit:** Z04 / `echo_hollow_01`
**Draft camera:** `(24,17)`, north to large chamber and `GATE_AFTER_BATTLE` (24,10). Secondary `(24,8)` north toward `SIGIL_A/B/C` near (17,6)/(24,6)/(31,6) and `PEDESTAL_FIRST_SIGNAL` (24,4).
**Narrative:** old warmth replaced by solemn echo—complex architecture, a silent guardian trial, then reveal a huge dormant signal network.

**Five anchors**:
1. **Tall masonry arch silhouette** and thick pillars with high contrast dark recess, rusty brick joints.
2. Visible **floor AND ceiling treatment**; older pseudo-3D style without impossible real 3D requirements.
3. Torches/brass frame warm amber along perimeter; lavender shade deepens inner chamber.
4. **Three independent mint sigils**, geometric icon/text identity clearly distinguishable with/without color, with ON/OFF/ON postbattle target pattern (proposal).
5. A bright, small **Signal Pedestal** at focal depth, plus original owner-art Craglet guardian sprite before victory; no required full living Bellkeeper Sol actor in opening.

**Palette**: brick rust `#91533D`, stone dark `#342F42`, ruin plum `#4C3C5B`, torch `#D9A45E`, polished brass `#B99B5F`, signal mint `#74D6B1`.
**Renderer limitation checks (mandatory honesty)**: grid raycast may fake arches with opaque masks/sprite trims but may not support see-through under-arch geometry, true vertical stacking, sloping vaulted ceiling, collision on elevated ledges or unrestricted pitch; sector/portal alternative should explicitly compare these features.
**Readability**: battle corridor remains physically legible and cannot bypass mandatory `TRIGGER_INTRO_BATTLE` simply because player walks around an overlay trigger. Architectural drama must not hide route to sigil room/return portal.
**Avoid**: an empty square dungeon corridor masquerading as architecture, dynamic lights used as performance crutch, arcane lettering too small for phone.
**Accept**: user sees a believable special place through first person, distinct from forest and harbor; 3 sigils and pedestal discoverable with subtitles/icon prompts; screen-level masks documented as tricks if used.

## E. Production comparisons and signoff

| Proposed shot | Benchmark question | Visual signoff | Tech signoff |
|---|---|---|---|
| HERO-Z01 primary | Can an outdoor plaza feel broad with visible beacon at 320×180? | Owner screenshot comparison | FPS/memory + collision/line of sight |
| HERO-Z03 primary | Can forest foliage feel lush without green-mush or overdraw? | Owner recognizes companion and root | depth/order/fog on Android |
| HERO-Z04 primary | Can tall arches and distinct chamber feel like reference? | Owner approves ruins mood | supports/doesn't support heights/pitch |
| HERO-Z03 puzzle | Are symbols readable with all starter animations? | No color-only clue | touch/interaction hit testing |
| HERO-Z04 post-battle | Are three sigils clear and not too small? | Ruin wonder preserved | event state tied to quest flags |

**Potential collision/visibility finding to check before production:** The 48×32 maps' solid `#` blocks are **proxies**, so waypoint/reachability tests do NOT guarantee line-of-sight to Beacon Tower from dock or that realistic archways exist in the geometry. These screenshots must be blockout-tested before final meshes/textures. Do not assert that hero views are already geometrically valid.

## F. Source art & renderer selection rules

- World art may be sketched from scratch in the WILDSIGNAL style; do not copy the third-party retro dungeon reference textures as shipped material.
- Preserve user's original creature pixels. This spec does not create, generate or edit images.
- Renderer proposals are A: CPU grid raycast; B: CPU sector/portal geometry; C: minimal CPU raster fallback only if necessary. No selection before real Win11 i3/4GB and Galaxy S10+ tests, plus additional legacy x64 Windows functional tests.
- All three hero views should be documented in a single side-by-side review sheet **when image production is authorized**. Use exactly the same subject, angle and relative sizes for comparison, not unrelated concept art.
- Hero views and palettes are *briefs*, not locked final camera/FOV/asset density values.

**Next proof gate:** owner approves scene silhouettes from annotated art (future deliverable) before Codex benchmarks renderer candidate. No code authorized by this design note.
