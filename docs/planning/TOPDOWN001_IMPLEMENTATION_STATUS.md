# TOPDOWN-001 — Current implementation state (2026-10-24)

**Owner:** Top-down cute fantasy is the new game direction. **Pixel Crawler Free Pack 2.11** is the selected commercial environment-art source (Anokolisa, included `Terms.txt`).

## What has actually been done

- Original uploaded ZIP verified (486 zip entries, 3,100,525 total uncompressed bytes), SHA-256 `b7b228ca232da9958f191b01db87764dddb67610116151e06e8dd8ade6de94b8`.
- Original terms found at `Pixel Crawler - Free Pack/Terms.txt`; commercial game use and modifications permitted, attribution optional, standalone repack/resale prohibited.
- Local independent C++20 + libpng proof source generated and **built successfully** with GCC/CMake on Linux. Reads original Pixel Crawler PNG sheets, composites their pixels into a **320×180 RGBA actual CPU output**, preserves nearest-neighbor sprites, tests tile layer and simple source-sheet character. Local source and private test assets are furnished as conversation attachments, **not automatically committed to this public GitHub branch**.
- The prototype's current PNG is a **real headless-engine frame**, not yet owner-accepted visual art. The scene is crude and not visually representative of planned final Lantern Quay.
- An isolated source-only ZIP, and a separate **private** runnable-input ZIP have been created; original art must remain privately stored.

## Still required

- **Private art repository upload:** complete ZIP is **VERIFIED UPLOADED** to `jonaslacandola0617/wildsignal-art` in this task: `third-party/pixel-crawler/free-pack-2.11/Pixel Crawler - Free Pack 2.11.zip` on `art/pixelcrawler-freepack-2.11-intake-v0.1`, 2,447,514 bytes, Git blob `a5fb408858c8fdaffd2820160ef178597b5ebe6f`; local original SHA-256 matches.
- **Public game repo source import:** original headless proof source and CMake are now committed on `feat/topdown-001-pixelcrawler-sdl-proof`, verified **byte-for-byte by Git blob SHA** against the locally built and executed code. `experiments/topdown001/src/main.cpp` blob `ef6b3991f29052bcd4503b83a922a25791c4a977`. Local headless Linux run PASS; a fresh GitHub/Windows build after import is **NOT RUN**. Next: SDL3 playable implementation and testing from this branch.
- SDL3 interactive controls/window, polished layer organization, real map design, animation, companion follow behavior, collisions for trees/objects, real Pixel Crawler atlas selection and owner approval are still pending.
- Windows 11 Intel i3/4 GB, Android, Windows 7 tests: NOT RUN.

## Strict preservation

The previous first-person RENDER-002, VIS branches and source art are historical R&D and remain untouched. Never ship or republish the third-party raw ZIP from public game GitHub. User original creature art remains canonical and protected.

See `docs/planning/TOPDOWN_PIXELCRAWLER_DECISION_v0.1.md` for current direction and rights.
