# Wildsignal development backlog

Updated November 1, 2026 (Asia/Shanghai).

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

## Recommended next gameplay milestone — first complete encounter

Status: **proposed next milestone**, not permission to expand production scope or replace approved design/art.

Make one short, understandable Tumbletail encounter work from a fresh save through a satisfying conclusion using the existing corner, field book and bonding systems. The current encounter data and wording are explicitly first-draft pilot material.

1. Give the player a clear immediate objective through the existing field book/prompts.
2. Let the player discover Tumbletail's behavior by watching, then help with its scattered seeds.
3. Clearly communicate wariness, recovery, trust and invitation availability without requiring debug knowledge.
4. Let the invitation succeed, show the companion following, and record the discovery/joining in Mara's book.
5. Confirm that rest, save/quit and reopen preserve the result and let the player continue normally.

Acceptance: a fresh player can complete the encounter without a developer explaining controls or hidden conditions. Frightening or being refused must have an understandable recovery path. The peaceful observation/help/invitation route works without mandatory combat. Existing Reach Out and friendly fights remain available for a separate focused playtest. No new creature pipeline, larger region, economy or networking is part of this milestone.

After this loop passes a hands-on playtest, connect companion friendship to one small exploration objective from the existing quest design. Then expand toward a short first playable slice, adding content in increments that can be tested end to end.

## Current evidence

The first optimization pass is preserved on `codex/optimize-low-end-20261101` (engine 26a9ea2). Uploaded Intel UHD runs recorded ~42.5 FPS uncapped for the optimized benchmark and ~29.8 FPS during capped normal play. The reference and optimized hardware runs had slightly different live-input camera paths; they are not a perfectly controlled pair. These short runs do not certify the exact i3 configuration, S10+, legacy Windows or thermal endurance.
