# TOPDOWN-001 — Current implementation state (2026-10-24)

**Owner:** Top-down cute fantasy is the new game direction. **Pixel Crawler Free Pack 2.11** is the selected commercial environment-art source (Anokolisa, included `Terms.txt`).

## What has actually been done

- Original uploaded ZIP verified (486 zip entries, 3,100,525 total uncompressed bytes), SHA-256 `b7b228ca232da9958f191b01db87764dddb67610116151e06e8dd8ade6de94b8`.
- Original terms found at `Pixel Crawler - Free Pack/Terms.txt`; commercial game use and modifications permitted, attribution optional, standalone repack/resale prohibited.
- Local independent C++20 + libpng proof source generated and **built successfully** with GCC/CMake on Linux. Reads original Pixel Crawler PNG sheets, composites their pixels into a **320×180 RGBA actual CPU output**, preserves nearest-neighbor sprites, tests tile layer and simple source-sheet character. Local source and private test assets are furnished as conversation attachments, **not automatically committed to this public GitHub branch**.
- The prototype's current PNG is a **real headless-engine frame**, not yet owner-accepted visual art. The scene is crude and not visually representative of planned final Lantern Quay.
- An isolated source-only ZIP, and a separate **private** runnable-input ZIP have been created; original art must remain privately stored.

## Still required

- **Private art repository upload:** complete ZIP is **NOT VERIFIED UPLOADED** to `jonaslacandola0617/wildsignal-art` in this task. Do not claim upload. Preserve ZIP/private source in the user's connected file storage if needed until successfully imported.
- **Public game repo source import:** locally tested source exists in the attached source-only ZIP, but not yet verifiably landed as GitHub source files. Codex should import/reconcile on a fresh feature branch and run actual build/tests.
- SDL3 interactive controls/window, polished layer organization, real map design, animation, companion follow behavior, collisions for trees/objects, real Pixel Crawler atlas selection and owner approval are still pending.
- Windows 11 Intel i3/4 GB, Android, Windows 7 tests: NOT RUN.

## Strict preservation

The previous first-person RENDER-002, VIS branches and source art are historical R&D and remain untouched. Never ship or republish the third-party raw ZIP from public game GitHub. User original creature art remains canonical and protected.

See `docs/planning/TOPDOWN_PIXELCRAWLER_DECISION_v0.1.md` for current direction and rights.
