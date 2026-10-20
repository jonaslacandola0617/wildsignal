# WILDSIGNAL — First Codex Technical Proof Task Packets v0.1

**Status:** READY FOR OWNER REVIEW, **NOT AUTHORIZATION TO IMPLEMENT FULL GAME**. These are precise small tasks intended *only* once the user explicitly initiates implementation.  
**Approved user intent:** ChatGPT plans, Codex writes code.  
**Dependencies:** ENGINE_ARCHITECTURE_ADR_v0.1.md, RENDERING_BENCHMARK_PLAN_v0.1.md, CANONICAL_SPEC_INDEX_v0.1.md and DECISION_REGISTER_v0.1.json. Replace branch names with approved immutable design commit SHA before Codex starts.

## CODEX-000 — Documentation/handoff consistency
**Outcome:** Before any technical code, enumerate conflicting specifications, select explicit draft baseline, and create tasks without inventing gameplay.

**In:** list docs and statuses, compile stable IDs, report missing source sprites/actual hardware and any conflicting map coordinates.  
**Out:** no 100-creature implementation, no store, no server, no reliance on unapproved numerical balance.  
**Acceptance:** show table of exactly which approved docs/commit apply to ENGINE-001; mark 15×11 map superseded; no changes to `main` without review.  
**Block:** owner has not approved source revision.

## ENGINE-001 — Minimal SDL3 native shell
**Outcome:** A small C++20+SDL3 window/input application builds for Windows and Android with a single CMake-driven core.

**In:** robust SDL initialization/cleanup and error paths, blank 320×180 framebuffer, window output, keyboard/mouse and Android touch event logging, graceful suspend/resume, safe-area values, explicit save-path query, diagnostics overlay, versioned pinned dependencies.
**Out:** battle, real creatures, game content, online service, IAP, native graphics framework beyond SDL3.
**Platform targets:** Windows x64 executable, Android arm64-v8a installable test APK; Google Play target API 36+ in Android configuration per policy checked 2026-10 (minSdk and pinned NDK after test).
**Tests:** actual Win/Android launches; 10 open-close cycles; Android background/resume, activity recreation if reproducible; log contains device and screen size; no crashes; no stray invalid pointers.
**Evidence:** CI build URL, install/run instructions, screenshot each platform, logs, real-device specs and git commit SHA.
**Unknowns to record:** exact Windows compiler, SDL3 version, NDK/Gradle versions and minimum Android OS.

## RENDER-001 — Grid raycast baseline
**Outcome:** software framebuffer first-person walkthrough of equivalent basic town/forest/ruin test scenes.

**In:** 2D map loading, tile collision, mouse/touch camera, textured vertical walls, floor/ceiling illusion if supported, depth-correct transparent billboard, fixed scene seed, 320×180 and 480×270 framebuffer, crisp UI overlay, audio optional.
**Out:** level polish, real online, game economy, quests, infinite terrain.
**Tests:** screenshot fidelity, stable rays at wall corners, no out-of-bounds tile access, sprite sorts properly behind obstacle, framerate/memory measured under same path.
**Evidence:** 3 screenshots/30s video, .csv raw frame traces, .json benchmark metadata using RENDERING_BENCHMARK_PLAN schema; report unsupported arch/pitch honestly.
**Failure:** if architecture cannot represent required arch/height, document limitation before expanding art.

## RENDER-002 — Sector/portal feasibility comparator
**Outcome:** equivalent camera paths for benchmark scene with at least one true doorway/arch/height-offset demonstration if feasible.

**In:** minimal sector portal drawing and clipping, floor/ceiling geometry, variable sector heights, billboards/occlusion.
**Out:** universal renderer, multistory open world, network, polished maps.
**Tests:** identical scene settings/texture inventory as RENDER-001, no infinite portal recursion, no slicing/clipping artifacts in 3 mandatory hero views, comparable Windows/Android performance traces.
**Evidence:** same capture format with side-by-side differences; list issues and extra code/scope.
**Fallback:** if infeasible under time box, record technical evidence, do not mark as completed/passed.

## PRESENT-001 — Display path comparison
**Outcome:** contrast direct CPU window surface vs SDL streaming texture presentations using the same CPU-rendered world.

**In:** nearest-neighbor scaling; text at native/display resolution; resizing, Android cutout handling, capture p95 world render vs presentation time separately.
**Out:** GPU 3D world renderer; assume standard OS display compositor / optional integrated GPU allowed.
**Tests:** fonts remain crisp at 1280×720 and Android native landscape; no excess memory uploads; frame pacing and thermal metrics compared.
**Evidence:** logs, crisp UI screenshots, each platform's presentation backend documented.

## PERF-001 — Representative hardware runs
**Outcome:** benchmark R1/R2 plus presentation alternatives on actual low-end target devices with identical conditions.

**In:** 3 scene fixtures, 3 internal resolutions, baseline 16/32/64 sprite-density settings for forest, three 60s passes + 5-minute navigation + 20-minute Android thermal run.
**Out:** unsupported "runs on any hardware" claim.
**Provisional pass criteria:** 30 delivered FPS and p95 ≤33.3ms; memory target steady ≤250MiB peak ≤350MiB; **not yet measured or guaranteed**.
**Tests:** actual installed-app execution, not simulator-only, attach device CPU/RAM/OS, raw traces, screenshots and known visual omissions.
**Decision output:** ADR-005_RENDERER_FINAL.md with selected technique and explicit environment art implications, or documented no-go.

## USER SIGNOFF NEEDED BEFORE CODING
- Confirm that a **technical-only experiment** is authorized and whether Codex should use a separate feature branch/PR.
- Identify available low-end PC and Android test hardware (or plan access).
- Clarify whether the current repo can hold unfinished code publicly.
- Approve dependency pinning/prototype SDL3 path. None of the above creates an artificial deadline for the full game.
