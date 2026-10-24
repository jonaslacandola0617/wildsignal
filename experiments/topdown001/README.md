# WILDSIGNAL TOPDOWN-001 — Genuine Pixel Crawler renderer proof

**The original libpng proof is retained below. A playable SDL3 extension is now documented in [README.playable.md](README.playable.md).** This is intentionally isolated from existing first-person src/engine001 and root CMake.

- Source art from the privately archived **Anokolisa Pixel Crawler Free Pack 2.11**; ZIP exists on `wildsignal-art` branch `art/pixelcrawler-freepack-2.11-intake-v0.1` in `third-party/pixel-crawler/free-pack-2.11/` (verified source SHA-256 `b7b228ca232da9958f191b01db87764dddb67610116151e06e8dd8ade6de94b8`).
- The private source PNGs are **not committed here**. The existing local proof consumes exactly seven sheet files named `floors.png`, `walls.png`, `roofs.png`, `trees.png`, `plants.png`, `farms.png`, `character.png` extracted without pixel edits from the original archive.
- Tested proof path: local Linux GCC C++20 + libpng, actual 320x180 PNG; source-only implementation is not a fully structured production engine, tilemap or playable demo.

## Reproduce headless proof

```bash
cmake -S experiments/topdown001 -B build/topdown001 -DCMAKE_BUILD_TYPE=Release
cmake --build build/topdown001
./build/topdown001/wildsignal_topdown001 --assets /absolute/private/pixelcrawler/sheets --output /absolute/private/result.png
```

A fresh directory of 7 unmodified source sheets is needed; do not use fake art. Source uses 16px tile crops directly, manual experiment placements, camera-targeted framebuffer, simple synthetic autonomous movement and basic collision. Colors/sprites are not redrawn. Original sprite license is preserved in the private archive.

## Original proof limits and the playable extension

The preserved `src/main.cpp` route has no SDL3 window, keyboard movement, directional animation, camera easing, accurate house atlas slicing, scene data, player interaction, or companion following. The new playable target supplies these renderer/movement features; interactions remain outside this prototype. The first frame is **not owner-approved Lantern Quay**, and old RENDER-002 isn't modified.

Execute `docs/architecture/TOPDOWN001_ENGINE_CONTRACT.md` and the new `docs/architecture/TOPDOWN001_CODEX_HANDOFF.md` from the parent branch to extend this into a true **SDL3 playable prototype**. Retain headless PNG capture as determinism/evidence route; properly identify atlas cells with a visual private atlas inspection instead of arbitrary row/column combinations. Implement proof on a fresh branch, show real native-window screenshots and animation captures, and stop for owner review.

**Privacy:** Never commit source PNGs, Aseprite masters, a derived atlas or screenshot containing Pixel Crawler art to this public repo or to public CI outputs. Source code and synthetic tests are okay.
