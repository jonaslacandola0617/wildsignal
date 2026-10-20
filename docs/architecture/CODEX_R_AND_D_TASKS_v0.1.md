# WILDSIGNAL — Codex R&D Task Packets v0.1

**Status:** Planning **only**; not assigned, not approved for implementation. Required readings: DECISION_REGISTER_v0.1.md, PRODUCT_VISION_RELEASE_SCOPE_v0.1.md, ENGINE_ARCHITECTURE_v0.1.md, RENDER_BENCHMARK_PLAN_v0.1.md, VISUAL_BIBLE_v0.1.md and first-slice benchmark scenes.
**Goal:** Verify native multi-platform and the lightest viable renderer BEFORE authoring the whole game. Codex should ask about unresolved assumptions rather than inventing fantasy rules.

## ENGINE-001 — Native startup / display proof

**Outcome:** A very small native C++20 + SDL3 application builds and runs on Windows x64 and Android arm64 with a low-resolution software image buffer, simple text overlay and keyboard/touch input.

**Dependencies:** platform-neutral CMake and pinned toolchain decision; original sprite optional, not necessary. No game server, no login, no payment.
**In scope:** CMake project, SDL3 window creation/input, software test framebuffer, nearest-neighbor output, resize handling, runtime FPS/memory output, safe shutdown, minimal Android JNI/Activity wrapper. A single static checkerboard graphic is permitted as a clearly labeled placeholder; this does not claim WILDSIGNAL art finished.
**Out of scope:** quests, creature battle, final renderer, multiplayer, save schema migration, music, social accounts, Steamworks/Play Billing.
**Constraints:** local build must be possible with text editor and modest Windows compiler; Android build may run in cloud runner; final debug APK must be installable on real Android device. CLI steps must be reproducible in README with dependency versions.

**Acceptance:**
1. Build Windows native executable using pinned CMake/SDL3; record compiler version and CPU/OS.
2. Produce Android debug APK with arm64 native library via pinned NDK/Gradle and install it.
3. Start in airplane mode and display moving checkerboard for 5 minutes.
4. Keyboard MOVE/LOOK and Android virtual joystick/look drag report correct typed actions without gameplay state.
5. Resize/windowed/fullscreen/Android app pause and resume do not crash.
6. Frame time split for `update/draw/present`; memory metrics use actual OS instrumentation and don't claim unknowns.
7. World image buffer 320×180 and 480×270 toggle; pixel art stays sharp.
8. No secrets or signing keys committed.

**Evidence:** `ENGINE-001_build_commands.md`, test screenshots PC + Android, console logs, basic performance JSON, known limitation list. No false '2GB supported' conclusion.

## RENDER-001 — Grid raycast candidate

**Outcome:** One visually coherent first-person textured raycast scene with floor/ceiling treatment, static prop billboard and movable camera/collision.

**In scope:** authored 2D test fixture subset, wall texture sampling, sorted sprite depth, near/far shading or fog, frame buffer; one owned sprite if cleaned alpha version available. 1 scene each: Quay, Liltwood, Echo Hollow (basic proxies permitted); log unsupported decorative architecture.
**Out of scope:** full NPC AI/creature catching, detailed story, complex multilevel stairs, final assets.
**Acceptance:** no wall ghosting, no visible occlusion inversion, no sprite halo, camera look constraints documented, collision stable and input feels reasonable with touch. Same camera locations in benchmarks; bench log includes draw_world/display time and memory.
**Evidence:** scene screenshots/short video, profiler JSON and specific list of visual gaps vs art reference.

## RENDER-002 — Sector/portal candidate

**Outcome:** A sector/portal room test achieving stronger architectural depth where raycaster fails, with matching first-person controls and as similar texture/sprite load as practical.

**In scope:** minimum sector wall segments, portal visibility, variable wall heights/faux arch treatments with limited primitives, clipping and sprite depth; no full multi-floor world. Use three benchmark scenes where feasible.
**Acceptance:** unsupported features identified explicitly; measurable comparison against RENDER-001 using equal world resolution and actor counts; no collision/portal clipping failures. Resource/time costs documented, not guessed.
**Evidence:** visual comparison matrix + logs. If this alternative is too expensive, mark failed without forcing it into game architecture.

## BENCH-001 — Head-to-head decision

**Outcome:** Recommendation for ADR-002/003/004 made from real hardware evidence and approved visual direction.

**Required matrix:** Windows integrated GPU/no discrete graphics + low-end Android device; 320×180/400×225/480×270; Quay, forest, ruins scenes; 1/8/24 sprites; 20min Android thermal soak.
**Acceptance:** full device/OS/CPU/RAM info recorded, frame p50/p95/p99, peak RSS/PSS, 2–3 screenshots per scene, unsupported features table, 20 minute uninterrupted run, independent gameplay readability review.
**Decision:** score per RENDER_BENCHMARK_PLAN_v0.1.md; if neither renderer meets visual and 30 FPS goal, revise level design or hardware target transparently. Do not silently adopt a heavy GPU game engine.
**Evidence:** repository issue summary / ADR proposal, raw result artifacts and known tradeoff summary. Never claim POC runs on every 2GB phone without real testing.

## CI-001 — Repeatable hosted platform builds

**Outcome:** GitHub Actions configured to build and run unit/smoke tests, produce versioned Windows and Android debug artifacts without installing SDKs on creator's low-end PC.

**In scope:** Linux tests + sanitizers where supported; Windows x64 compile; Android arm64 NDK/Gradle debug APK, caching and pinned versions, provenance/third-party license ledger; no privileged release signing in first pass.
**Acceptance:** run on new pull request from clean checkout without relying on developer machine paths; artifacts downloadable and tagged with commit SHA; build failure clearly identifies toolchain phase. Secrets absent.
**Evidence:** workflow run links and artifacts, docs for how owner retrieves APK/exe and reports FPS/RAM from device.

## Shared Codex task policy

**No broad "build all WILDSIGNAL" prompt.** Every future coding issue references: exact signed-off docs commit, in-scope/out-of-scope behaviors, stable IDs, tests, test device and evidence. If a proposed design detail affects permanent save IDs, commerce, network authority or art identity, ask for approval. Implementation should be incremental and code-reviewed.

**One potential development strategy:** ENGINE-001 → RENDER-001 → BENCH-001 for feasibility; if aesthetics fail, RENDER-002 then BENCH-001 again; CI-001 can run in parallel after platform bootstrap. This is not an instruction to start these today.


## Legacy Windows requirements to add when ENGINE-001/CI-001 are authorized

**D-012 confirmed target:** Owner builds/develops on Win11 i3/4GB; game compatibility must be evaluated on Win7 SP1, Win8, Win8.1, Win10, Win11. Add `WINLEG-001..010` from `WINDOWS_7_11_COMPATIBILITY_v0.1.md`. ENGINE-001 must not assume Win10-only runtime calls, Steam client access for Win7/8.1, or the existence of Win7 support merely because SDL3 builds successfully. Cloud CI can compile legacy-compatible binaries; **real old-OS start-up testing still needed**. Win7 SP1 x64 is a proposed floor, 32-bit coverage unresolved, account/co-op support on legacy OS security-gated. These are task descriptions for later approval, **not commands to implement now**.
