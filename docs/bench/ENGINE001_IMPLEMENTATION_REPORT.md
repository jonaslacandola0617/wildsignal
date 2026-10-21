# ENGINE-001 implementation evidence

Status: **prototype implemented; acceptance pending owner hardware/visual tests**.
Source baseline: existing `feat/engine-001-echo-renderer` at
`40bed2de1325aa385b4d573122145beb7124a94a`, descended from the preproduction branch.
Issue: https://github.com/jonaslacandola0617/wildsignal/issues/3.
No original planning specification has been edited. Main/preproduction refs have
not been changed. Implementation permission is the owner's explicit ENGINE-001
instruction and Issue #3, superseding the older planning-only authorization text.

## Implemented

C++20, CMake, static SDL3 3.2.10; separate platform-neutral renderer/test runner
and SDL composition root. The exact 24×24 fixture is loaded and validated,
including actor IDs, closed borders, camera ranges and floor reachability.
W/S/A/D camera, bounded circle/wall collision, F1 resolution cycle, F2 UI toggle,
F3 one/eight creatures, focus reset, resize/letterboxing and F12 display capture.
CPU textured walls/floor/ceiling, pillar depth, cutout placeholder sprites,
decorative arch silhouette and emissive-looking signal cue. Crisp SDL debug HUD
is rendered independently of the ARGB8888 world buffer. Buffers are 230,400,
360,000 and 518,400 bytes. Six original 64×64 textures occupy 98,304 bytes.

Telemetry streams CSV and emits JSON on clean exit with nearest-rank p50/p95/p99,
FPS, simulation/world/sprite/UI/upload/present timing, build ID, configuration
groups, events and platform RSS/peak working-set methods. Unknown measurements
remain unknown. CPU-only headless timings explicitly exclude SDL/display costs.

## Local checks actually executed

Execution host: Linux x86_64, GCC **14.2.0** (Debian 14.2.0-19).
No Windows OS, Windows cross-compiler, SDL3 installation or CMake binary was
available locally. The shell's network proxy was unreachable. The existing
repository was imported via the GitHub connector as an exact shallow checkout;
all blob/tree/commit hashes were verified against the existing branch, including
its original +0800 commit timestamps. No unrelated repository baseline was used.

Run all core checks with:

```sh
bash tools/check_engine001.sh
```

This compiles Release (`-O2 -DNDEBUG -std=c++20`, warnings as errors), runs the
core tests and black-box export checks, then compiles/runs ASan+UBSan. Leak
detection defaults on; this managed host prevents LeakSanitizer's thread scan
(reported as a ptrace restriction). Here use `ASAN_OPTIONS=detect_leaks=0 bash
tools/check_engine001.sh`; leak detection must remain enabled on an unrestricted
host/CI. It uses no downloaded dependency and is not a substitute for CMake or
the native SDL build. Standard CMake core-only route on a suitably equipped host:

```sh
cmake -S . -B build/core -DENGINE001_BUILD_SDL=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build/core --parallel 2
ctest --test-dir build/core --output-on-failure
```

| Check | Local result |
|---|---|
| T001 fixture dimensions/bounds/coordinates/reachability | PASS |
| T002 cardinal/diagonal/swept collision, corner contacts, time-step cap | PASS |
| T003 actual pillar full/partial occlusion, front visibility, invalid projection | PASS |
| T004 exact allocations and initialized bounded pixels/depth at all three sizes and 24 yaws | PASS |
| T005 100 CPU buffer cycles, failed resize preservation, viewport math | PASS (SDL lifecycle separately gated) |
| T006 focus loss/resume, opposite-key cancellation, repeatable camera | PASS for typed input |
| T007 malformed/deep/oversized JSON, wrong geometry, missing fixture | PASS; art is generated so no missing image file dependency |
| T008 JSON/CSV counts, nearest-rank quantiles, unknown fields, overwrite protection | PASS |
| T009 offline headless boot | PASS with both HTTP proxies deliberately dead; no runtime network code exists |
| Hot render allocation audit | PASS: zero heap allocations across a warmed 360-degree stress sweep |
| SDL3 3.2.10 header/API syntax check | PASS with `g++ -fsyntax-only`, not linked or run |
| T010 Windows EXE/import inspection and SDL smoke | Awaiting Windows CI execution; legacy runtime still untested |

The core suite has **200 assertions**. Sanitizer and final CI evidence is recorded
in the follow-up evidence update; no passing Windows or device result is inferred
from this table. Short Linux headless rendering has executed and produced an
actual world image. Final screenshots and measured run files will be added after
the source commits so their build identifiers identify reviewable source.

## Limits and recommendation

Walls have one height (3.2 fixture units), level floor/ceiling and zero pitch.
The arch is a camera-facing cutout trim at (12,12.5), not an opening in wall
geometry; it can look flat or turn unnaturally from the side. Binary alpha cards
are sorted by camera depth and clipped against wall columns. No intersecting
nonparallel transparent planes, translucent blending, stacked rooms, slopes,
partial-height walls, real dynamic lights, animation assets or audio are provided.
Telemetry excludes minimized intervals, but focus-only background rendering can
remain in a capture and is annotated with events. A forced process kill may
leave incomplete CSV without JSON. The screenshot key overwrites the previous
display BMP within the same run; use distinct run names for a screenshot set.

No actual owner PC, Windows 7/8/8.1/10/11 interactive session or Android device
was tested locally. No 30 FPS target-hardware or minimum-RAM claim is established.
The Windows CI job builds x64 Release, runs dummy-video SDL lifecycle smoke and
records PE imports/toolchain; it cannot prove interactive visuals, old-OS startup
or integrated-GPU performance. Import review and real legacy OS testing remain
required even with `_WIN32_WINNT=0x0601` and static SDL/CRT.

Recommendation: obtain the Win11 i3/4GB walkthrough and visual review before
expanding to town/forest or choosing a production renderer. If tall arch/side-view
fidelity fails owner review, compare a small sector renderer using the same scene;
do not hide grid-raycaster limits by adding more game systems. This is an
engineering recommendation, **not** a ratified renderer ADR.

Windows instructions, controls and packaging are in [README.engine001.md](../../README.engine001.md).
No production gameplay, multiplayer, accounts, battles, full AI, inventory,
quests, saves, progression, payments, Steamworks, Play services, additional
regions, owner art or creature catalog was added. Issue #3 must remain open until
the prescribed real-device and visual gate is satisfied.
