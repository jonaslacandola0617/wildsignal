# HOLLAND-003 private Windows owner review

Extract the ENTIRE ZIP, open `windows/`. Keep exe/assets/data/tools together.
Double-click RUN_OWNER_TOON_CONTROL.cmd first: exact supplied strong toon profile,
shadow OFF/shaft OFF, same640 pose14/control. Compare RUN_HIGHLIGHTS_ONLY.cmd,
RUN_COMBINED_EDGES.cmd, RUN_SHADOW_512.cmd and RUN_SHADOW_1024.cmd. All retain the
owner's material/sun/ambient/toon settings. New shadows and contacts are explicit.

RUN_SUNLIGHT_LOW.cmd and RUN_SUNLIGHT_STANDARD.cmd start at sun-facing pose20,
using the unchanged palm/barn scene. RUN_HAZE_ONLY.cmd is a separate ordinary-haze
control, not beams. RUN_SUNLIGHT_AUTO.cmd tests the automatic fallback. LOW480 is
a separately labeled sparse fallback, not the controlled full-scene benchmark.
Each launcher chooses a new output folder under runs/. No output is overwritten.

WASD/click+mouse, Esc releases/exits. H highlights; J legacy/off/only/combined;
K old/new shadow proof; L8-step quarter shafts; M bounded contacts. O dark outlines,
B old toon bands; G original OFF/RAW/COMP camera; F1 resolution; F3 poses0..21;
F7 shadow mode; F8 shaft method; F9 hard/soft; F10 half/quarter; F12 capture.
The new camera reference math and source art remain unchanged. No leaf experiment.

Actual test matrix, native/presented stills, crops and movies are under evidence/.
Edge movies: top-left OFF, top-right PR28 B1, bottom-left highlight-only,
bottom-right combined. Shadow movie: old512/new512/new1024. Sunlight movie:
OFF/8-quarter/12-half. Final movie: owner toon control/optional full combination.
Movie replay10fps is not measured displayed FPS. Native PNGs are lossless actual
framebuffer pixels; H264 previews can add compression artifacts. See raw CSV/JSON.

Run RUN_HARDWARE_TESTS.cmd without interacting with its visible GL windows.
Default34 runs:12 paced forward/reverse controls and22 uncapped effect costs,
600 steps per run,120 warmup excluded. Both480 and640 use the SAME full garden.
Windows PowerShell5+ is built in; the script needs no Python/Blender/network.
Optional shorter matrix from PowerShell in windows/:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/hardware.ps1 -SkipEffects
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/hardware.ps1 -Frames 1200 -Warmup 240 -Vsync
```

Return the new runs/hardware-* folder. It contains SYSTEM.json (OS/CPU/driver/RAM),
RESULTS.csv/JSON and every raw frame/summary/command. CPU submit timings are wall
clock durations that can include driver blocking. Async elapsed queries cover
the whole draw/presentation, not isolated per-pass GPU time. Shadow/shaft pass
CPU submission costs are also recorded. Total view and shadow triangle counts
are distinct. Texture byte counts are logical, not measured VRAM. Process peak
working set uses actual Windows APIs. Map cache startup is recorded in frame0,
excluded from steady summaries after warmup. Captures are disabled in benchmarks.

Software GL is rejected. A hardware pacing pass requires observed Intel UHD on
Windows,>=120 timed frames, BOTH loop and swap-return P95 strictly below33.34ms
at480, or16.67ms for optional640/60 stretch. Check CPU/RAM against owner's i3/4GB.
Cap30 has nominal33.333ms spacing: OS scheduling can still fail33.34ms; exact
violations must be reported. API swap return is NOT scanout/displayed FPS.
Mesa software results never prove this hardware gate. CI PowerShell syntax and
Windows compilation do not establish real on-device launcher/graphics success.

No game account, Python, Blender, Visual Studio or external DLL distribution is
needed to run. Static SDL/static MSVC CRT, x64 exe; OpenGL3.3-capable Windows
64-bit driver required. MSVC2022 build instructions are in README.md. Windows7
and Android compatibility are unverified. Original/converted assets stay private;
no public GitHub upload. Owner-declared Wordline test assets are not final game
art. No production gameplay, creature modifications or direction change.

HOLLAND_003_OWNER_VISUAL_AND_HARDWARE_REVIEW_PENDING
