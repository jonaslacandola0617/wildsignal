# HOLLAND-001 — Windows private review package

Extract the entire private review ZIP. Open `windows/` and double-click:

- `RUN_B0_BASELINE.cmd`: Wordline with the inherited PIXEL3D-004 C treatment.
- `RUN_B1_REFINED.cmd`: selective outlines/highlights, camera snapping OFF.
- `RUN_B2_OFF.cmd`, `RUN_B2_RAW.cmd`, `RUN_B2_COMP.cmd`: identical refined
  renderer, comparing continuous camera, legacy raw snap and fractional display
  compensation. Start with COMP, then press G to cycle the three camera modes.
- `RUN_LOW_480.cmd`: 480×270, sparse vegetation, cached512 shadows, shafts OFF,
  30FPS application cap. This is a proposed fallback, not a verified hardware pass.
- `RUN_HARDWARE_TESTS.cmd`: automated actual-machine benchmark suite. Leave its
  visible GL windows running; do not interact with them during measurements.

The `.exe`, `assets/`, `data/` and launchers must remain together. No Blender,
Python, Visual Studio, account, network login or game-service runtime is needed
to run this portable package. It contains static SDL3 and static MSVC CRT.
An actual Windows x64 OpenGL3.3-capable graphics driver is required. This is not
Android or Windows7 certification. Do not move the executable out of its folder.

Controls: click the window for mouse-look, WASD to move, Esc releases mouse then
quits. F11 cycles B0/B1/B2 outline studies; G cycles OFF/RAW/COMP and selects B2.
F1 changes resolution; F3 cycles viewpoints (13 front,14 corner,12 foliage,
1 medium); O toggles outline, F7 shadow mode, F8 shafts, F9 shadow softness,
F12 captures. Existing controls/CLI are retained. Camera position/yaw/pitch stay
continuous; only the effective render camera and final presentation are adjusted.

Native `final.ppm` / `frame_*.ppm` are internal geometry/post framebuffer pixels.
`final_presented.ppm` / `presented_*.ppm` are actual default back-buffer captures
BEFORE swap, including compensation. COMP and RAW intentionally have identical
native pixels for the same camera. Their displayed pixel boundaries differ at
integer upscale. No material filtering/blur is used. Borders clamp one source
texel; perspective parallax, occlusion, free-look and arbitrary depths are not
pixel-perfect. Reference depth is an experimental10 world units, not world scale.

Each launcher creates a new folder under `windows/runs/`. Do not reuse output
paths; the executable refuses to overwrite an earlier review. Capture files and
all included Wordline sources/derivatives remain private; do not upload them to
public WILDSIGNAL GitHub. Assets are owner-declared Wordline test models, not
final approved game art. No creature sprites are included or modified.

## Hardware measurements

The default suite has34 runs:20 paced controls (five modes, reverse-order repeat,
two profiles) and14 uncapped effect cost runs.600 frames/run,120 warmup excluded.
Optional from PowerShell in `windows/`:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/hardware.ps1 -Frames 1200 -Warmup 240
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/hardware.ps1 -SkipEffects
```

`-Vsync` requests swap interval1; default requests0. Both requested and observed
states are logged. Low480 uses sparse scene/30cap; balanced640 uses full small
garden/60cap. Compare within a profile: every B0/B1/OFF/RAW/COMP run uses identical
geometry, route, sun and effects. Effects use cap0 and independently change only
shadow, outline, shafts or vegetation visibility. Vegetation is actual opaque
triangle geometry, not alpha cards; its toggle measures geometry/fill cost,
not isolated transparent overdraw. Cached-map warmup cost is separately visible.

Results under `runs/hardware-*`: `SYSTEM.json` (OS build/CPU/GPU/driver/RAM),
`RESULTS.csv/JSON`, each run's exact command, `frames.csv`, `summary.json` and
logs. Raw frames include CPU update/cull/submit, asynchronous query associations,
draw calls/triangles/culling, process working set, swap-return intervals, camera
positions/offsets and cap state. Peaks use Windows process working-set APIs,
not guessed texture bytes or Linux RSS. No per-frame glFinish is used.

Low target: P95 loop AND complete swap-return interval<=33.3ms. Balanced stretch:
<=16.7ms. Exact violations are reported. A PASS denotes API frame pacing on an
observed Intel UHD/Windows run, NOT independently verified scanout/displayed FPS.
OS/CPU/RAM must also be checked against the intended i3/4GB machine. Offscreen or
Mesa numbers never establish Intel UHD performance. Software GL is rejected by
these launchers. No target-hardware success is implied by Windows CI compilation.

Stop at **HOLLAND_001_OWNER_VISUAL_AND_HARDWARE_REVIEW_PENDING**.
