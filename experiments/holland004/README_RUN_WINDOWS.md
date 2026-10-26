# Windows x64 owner pacing review

Extract the entire **private** package to a writable folder. Do not run inside ZIP preview. The supplied executable uses static SDL/C++ runtime and requires a working OpenGL 3.3 driver. Do not bypass an antivirus alert without inspecting the package provenance/checksums. No administrator privilege or installation is required by the application. Windows 11 Intel UHD execution of this new build is **NOT RUN** until the owner tests it.

1. Run `GARDEN_480_CAP30_LEGACY.cmd` and `GARDEN_480_CAP30_PRECISE.cmd`, then the 640/cap60 pair. Compare the same moving camera manually with the arrow/WASD controls. P toggles pacing, N cap, V vsync; mixed controls invalidate a benchmark run.
2. Run the equivalent `SIMPLE_...` pairs. This loads the owner's exact five-object diagnostic scene (two blocks, two spheres, plane), without replacing any garden assets or implementing HOLLAND-005.
3. Close other optional applications, retain the same power/display settings and run `HARDWARE_PACING_SUITE.cmd`. Eight paced runs plus four uncapped controls take approximately 2minutes when the cap is met. Each run uses 600 frames/120 warmup, same deterministic strafe route,2x drawable, exact look/effects. Nothing is recorded during measurements.
4. Return the newly created `runs/pacing-TIMESTAMP` folder privately. `RESULTS.csv/json`, frame CSVs, process CPU, peak working set, driver/OS and model/fixture SHA256 inventory are included. Missing fixtures explicitly SKIP; execution errors stop the suite, never switch to software rendering.

Optional controlled comparisons (run from a terminal in this folder):
```
HARDWARE_PACING_SUITE.cmd -IncludeVsync
HARDWARE_PACING_SUITE.cmd -Repeats 2
HARDWARE_PACING_SUITE.cmd -WaitBackend sdl
HARDWARE_PACING_SUITE.cmd -FineTailUs 0
HARDWARE_PACING_SUITE.cmd -TimerResolution 0
```
These options apply identically to legacy and precise. The default inherited SDL policy requests 1ms timer resolution;0 disables that request for the experiment. Repeats reverse pacing order on odd repeats. Optional vsync stays clearly labeled. Uncapped is a throughput control, not displayed FPS.

The original strict loop AND API-return interval P95 gates are <33.34ms at 30 and <16.67ms at 60. A PASS is API pacing only; no scanout is measured. The original owner's legacy build failed these gates. Never infer new success from CI, short timer-only tests or Mesa software rendering.

## Rebuild with Visual Studio 2022 Build Tools

Install Desktop development with C++, CMake 3.24+, Git and a Windows SDK. In x64 Native Tools prompt, from the repository at the review source commit:
```
cmake -S experiments/holland004 -B build/h4 -G "Visual Studio 17 2022" -A x64 -T v143
cmake --build build/h4 --config Release --parallel 2
ctest --test-dir build/h4 -C Release --output-on-failure
cmake --install build/h4 --config Release --prefix out/holland004-windows-x64
```
Dependencies are fetched at pinned revisions; the install is **code-only**. Place the verified original garden pack under `assets/`, and the privately supplied simple fixture under `fixtures/simple/{assets,data,MANIFEST.json}`. Keep these out of public Git. The private review ZIP already contains them. Source Quaternius, Pixel Crawler, creatures and preceding experiments are untouched.

Timer-only diagnostic (no GPU or displayed frame rate):
```
holland004_timer_probe.exe --pacing precise --cap 60 --frames 180 --warmup 30 --output timer-check
```
Output names must be new. Inspect `backend` in timer_summary.json to see which timer actually ran. The capped renderer uses the same waiter. High-resolution timer creation is dynamically resolved; older Windows fallback paths exist but Win7 runtime compatibility is untested. Timers are closed and SDL releases its inherited timer request at shutdown. No thread-priority/power-plan change is made.

Status: **HOLLAND_004_OWNER_HARDWARE_PACING_REVIEW_PENDING**. HOLLAND-005 visual tuning is not part of this build.
