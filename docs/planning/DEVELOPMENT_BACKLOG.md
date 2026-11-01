# Wildsignal development backlog

Updated November 1, 2026 (Asia/Shanghai); milestones replaced November 2, 2026.

Owner direction: park further performance optimization and resume gameplay development. Keep the completed shadow-cache optimization. The minimum-device targets remain the Intel i3 10th-gen / 4 GB Windows PC and Galaxy S10+; deferring optimization does not change those targets.

## Deferred engineering work

These are backlog entries, not active implementation tasks. Preserve the existing assets and visual direction.

| ID | Work | When to revisit | Evidence of completion |
| --- | --- | --- | --- |
| PERF-001 | Isolate replay input from live mouse/keyboard and record anonymous device/run identity | Before the next comparative performance benchmark | Repeated runs follow the same camera/player path; replay still permits a safe quit; source, assets, settings and device metadata are recorded |
| PERF-002 | Batch/instance repeated trees, bushes and compatible static props | After scene population expands or measured rendering cost threatens the 30 FPS goal | Reduced submissions on the target PC, equivalent controlled images/motion, preserved ownership IDs, transparency, culling and dissolve |
| PERF-003 | Profile foliage overdraw, pixel shading, outlines and remaining GPU passes; simplify measured hotspots | Before changing effect quality or when a representative busy view exceeds budget | Per-pass measurements and comparable target-device frame times; no unsupported speedup claim |
| PERF-004 | Supply and integrate useful distant vegetation/grass mesh LODs | When representative scene scale warrants them | Actual LOD assets with provenance, measured triangle reduction, stable silhouettes/transitions and owner visual review |
| PERF-005 | Review elapsed-time clamping and world simulation under stalls | Before longer gameplay acceptance tests; promote immediately if slow-motion or lost-time bugs are reproduced | Movement/timers remain correct across controlled stalls without breaking deterministic fight rules |
| PLATFORM-001 | Build the current pilot for Android: GLES/backend, packaging, touch and lifecycle support, then S10+ profiling | At the first mobile platform milestone, before claiming Android support | A real APK works on the owner's S10+; lifecycle/input checks and a sustained thermal run are recorded |

Memory reduction is lower priority while representative process memory remains within the working budget. Monitor growth; no leak-free or whole-device RAM claim follows from the short current samples.

Reopen optimization when representative content causes sustained misses of the ~30 FPS goal, visible frame spikes, memory pressure or measured mobile thermal problems. Keep lightweight performance checks at gameplay milestones; do not wait for a large rewrite to discover a regression. Windows legacy-OS verification remains a separate existing release requirement.

## Next gameplay milestones — fights first, connected regions

Updated November 2, 2026. The owner withdrew the "first complete Tumbletail encounter" milestone and set a new direction: creature fights are the core loop, wild Signari are recruited by fighting then reaching out, and the world is connected regions streamed in chunks (D-070 to D-074). Full rationale and proposed rules: [Gameplay direction v0.2](../design/GAMEPLAY_DIRECTION_v0.2.md).

| ID | Work | Evidence of completion |
|---|---|---|
| FIGHT-RECRUIT-001 | Reach Out opens only when the wild Signari is worn down (proposed ≤40% health) and calm; HUD shows the recruit marker and when Reach Out is open | Deterministic fight tests for the new gate, knockout still ends the chance, a hands-on fight in the corner recruits a Signari |
| REGION-001 | Region and chunk data format plus a streaming loader, proven by chunking the existing Hearthgrove corner | Same look as today, memory and frame times measured on the i3/4 GB PC, chunk loads cause no visible hitch |
| SPAWN-001 | Wild Signari placed from per-chunk spawn tables, roaming, starting fights on contact or F | Spawns survive chunk unload/reload, no duplicates of recruited Signari |
| REGION-002 | First real region blockout, larger than the corner, with one gate to a second area | Walkable end to end with saves, owner visual review |
| TEAM-001 | Levels, experience and new moves | Owner-approved design first, then implementation with tests |

Do one milestone per branch, in this order. Numbers in the proposed rules need owner approval before they become production logic.

## Current evidence

The first optimization pass is preserved on `codex/optimize-low-end-20261101` (engine 26a9ea2). Uploaded Intel UHD runs recorded ~42.5 FPS uncapped for the optimized benchmark and ~29.8 FPS during capped normal play. The reference and optimized hardware runs had slightly different live-input camera paths; they are not a perfectly controlled pair. These short runs do not certify the exact i3 configuration, S10+, legacy Windows or thermal endurance.
