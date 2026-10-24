# Codex execution packet — WILDSIGNAL TOPDOWN-001 (playable Lantern Quay)

**Current direction: USER-APPROVED cute top-down fantasy / Pixel Crawler Free Pack 2.11 environment art.**  
**Coding repository:** `jonaslacandola0617/wildsignal`  
**Start baseline branch:** `feat/topdown-001-pixelcrawler-sdl-proof` (stacked on the unmerged documentation transition)  
**Private art repository:** `jonaslacandola0617/wildsignal-art`  
**Existing proof:** `experiments/topdown001/` has **exact** locally built and executed C++20/libpng headless PNG capture source; Git blob `ef6b3991f29052bcd4503b83a922a25791c4a977`. It is not SDL3 or production quality.

## 0 — Restore, audit, protect

Read `AGENTS.md`, `docs/planning/TOPDOWN_PIXELCRAWLER_DECISION_v0.1.md`, `docs/architecture/TOPDOWN001_ENGINE_CONTRACT.md`, this handoff, `experiments/topdown001/README.md`, and `experiments/topdown001/PRIVATE_SOURCE_MAP.md` **before coding**.

Verify parent branches and changes before writing; create a **new task branch** from the top-down proof baseline, open a DRAFT PR, never merge or force-push. Existing first-person RENDER-002, all old experiments, decision register, original creature art and old PRs must remain untouched.

Resolve the original Pixel Crawler ZIP from the **private art repo** path documented in PRIVATE_SOURCE_MAP. If authorized tool access is unavailable, clearly state that access is blocked; request the source ZIP from the owner rather than download an unrelated version or invent art. Verify the SHA-256/Terms.txt, unzip into a **private non-git directory**. Do not commit any source PNG, extracted atlas, Aseprite, screenshots of licensed art, caches or screenshots as a public GitHub artifact; deliver visual evidence privately.

## 1 — Inspect before composing

- Inventory the exact free-pack sheets useful for a small **Lantern Quay village**, not content from paid expansions.
- Create private, large nearest-neighbor previews of `Floors_Tiles.png`, `Walls.png`, `Roofs.png`, `Vegetation.png`, trees, fences, props, doors, signs, and `Entities/Characters/Body_A` directional idle/walk animations.
- Build a small semantic atlas coordinate catalog (ground grass, paths, water, hedge/tree, cottage wall/roof, entry, fence, decor). Every coordinate must be visually confirmed. Map must **not** be assembled by cycling arbitrary tiles based on modulo math as in old proof.
- Record collision/occlusion masks and sprite anchor feet for actual assets; large art sprites are not one 16×16 tile.
- Describe allowed terms: commercial game use, edits allowed, attribution not mandatory, no unapproved standalone art redistribution.

## 2 — Implement genuinely playable SDL3 top-down prototype

Keep C++20, SDL3 (use existing pinned SDL3 source/cross-project toolchain where sensible), CMake and CPU/memory discipline. No web/JS/heavy game engine. Existing experimental libpng headless proof should remain runnable for comparison; do **not** replace the old renderer under `src/engine001`.

Required:
- A resizable real **SDL3 game window** showing a crisp logical **320×180** internal world buffer; nearest-neighbor integer upscale when possible (letterbox on odd aspect ratios). Avoid implicit linear interpolation or blurry subpixel scrolling.
- Four-way WASD and arrow movement; maintain stable speeds using delta time. Move/collide in a 2D world; no classic first-person turn-left/forward control.
- Simple but actual tilemap loaded from a small editable map/fixture (JSON/CSV acceptable) and a separate collision layer. At least one beautiful, coherent cottage, grass, path, trees, flowers, water/fences only if in the real free pack. Tiny scene / no sprawling map.
- Camera follows player smoothly **without breaking pixel alignment**. No forced 256px environment texels; use source-native 16px tile artwork. Test zoom at nearest scale.
- Directional idle and walk animations from actual Pixel Crawler sheets (validate frame layout); sprite feet Y-sort relative to props.
- A clearly **temporary** following companion (can use a licensed pack mob or neutral test sprite; explicitly NOT the user's canonical WILDSIGNAL creature). Basic collision and follow distance; don't implement bonding/combat yet.
- Layered render order ground → path/decal → building walls/props → actors/foreground trees → HUD, with correct tree occlusion via anchor/Y sorting; no flat tile overdraw obscuring player unexpectedly.
- ESC exit, resolution/scale toggle and camera debug overlay if useful. Windows-first interaction but preserve Android feasibility.

## 3 — Test and produce evidence

- Build and run local Linux, Windows x64 if permitted; validate absence of private sprite bytes in public Git tree and CI artifacts.
- Retain a **headless deterministic framebuffer route**, including 320×180 PNG/PPM captures from real executable, 3 camera positions, one moving 3–10s animated preview and recorded frame count. A mockup is not evidence.
- Tests: tile index bounds; file-not-found handling; grid alignment; sprite transparent pixels; collision, camera bounds, walk-frame cycling and companion-follow distance; memory budget, no per-frame PNG decoding, frame time data (median/P95; honest environment details).
- Use existing game source without adding proprietary assets to public repository. If generated CI artifacts contain third-party licensed pixels, keep CI synthetic-only.
- Document limitations: win7/Android/i3 4GB not tested until actually run. Don't claim release readiness.

## 4 — STOP and report

Deliver:
1. Draft implementation PR URL/commit and feature branch, no merge.
2. Private actual in-engine screenshots and motion preview.
3. Windows executable/package if successfully compiled; otherwise honest blocker.
4. One private art/atlas manifest and user-acceptance questions: readability, cute-fantasy feeling, appropriate tile scale, camera/movement quality, companion visibility.
5. Exact outcome table: implemented/verified/incomplete/blocked.

**Prioritize a genuinely charming playable screenshot over feature count**. This is a minimal visual gameplay slice, not a full MMO, battle system or all six zones.
