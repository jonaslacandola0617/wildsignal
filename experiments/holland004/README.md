# HOLLAND-004 — isolated pacing proof

Status: **HOLLAND_004_OWNER_HARDWARE_PACING_REVIEW_PENDING**. Implements Issue #33 from PR #32 commit `95d50b3afaaddd4c01cc0ace9ffe1e251c9d2d57`. Issue #34 visual tuning and Issue #30 leaves are deferred. No merge, gameplay, renderer rewrite, or art changes.

The owner recorded 34 actual Windows 11 / i3-10110U / Intel UHD runs. All12 capped cases failed the original API loop/interval P95 gates. Raw archives contain11 uncapped cases per resolution (22 total), correcting the issue's count of14. Median capped loop excess was0.84–1.07ms; cumulative API phase drift across480 measured intervals was0.43–0.80seconds. GPU queries were short and uncapped work had substantial headroom. Old telemetry did not isolate sleep, so these observations support a pacing diagnosis without proving all stalls came from the timer.

## Implementation

`--pacing legacy|precise` defaults to **legacy** until owner acceptance. Legacy preserves PR32's relative `sleep_until(frameBegin+1/cap)` AFTER swap. Precise waits BEFORE swap, using an integer rational, absolute30/60Hz phase. Ordinary wake errors are absorbed by the next deadline instead of accumulating. More than half a frame late rebases to a fresh interval, avoiding catch-up bursts. Cap0 takes neither wait path and creates no native timer. Live P switches pacing; N cycles cap0/30/60; V toggles vsync and resets phase. These controls do not alter render inputs.

The precise default waiter dynamically requests a Windows high-resolution waitable timer. Unsupported creation falls back to a regular waitable timer, then SDL_DelayNS on failure. Handles are RAII-owned/closed. A bounded500us fine tail uses CPU pause instructions; configurable0–1000us, once per deadline. After tail exhaustion it sleeps rather than repeatedly spinning. `--wait-backend sdl` compares SDL's sleep backend. We do not change thread priority, power plans, or call timeBeginPeriod directly.

**Inherited timer policy matters:** pinned SDL3.2.10 already requests1ms Windows timer resolution by default. It pairs requests/releases on hint change and SDL_Quit. Both pacers inherit that same policy; `--timer-resolution default|0|1` permits explicit A/B tests. Default is not a claim that no global timer policy is active. Timer resolution and fine waits may increase CPU/energy use. Per-process CPU time and active-wait wall time are reported; battery, thermals, energy and long-session behavior require device testing. A thread's active wall time includes preemption and is not equivalent to CPU time.

GPU sources, shaders, mesh import, Wordline models, material textures, toon bands, convex highlights, shadows, experimental volumetric controls and camera math are reused unchanged from HOLLAND-003/HOLLAND-001. The owned fixture's BLOCK/SPHERE/PLANE use compatibility model aliases and are **not Wordline artwork**. Original supplied GLBs are packaged privately without regeneration.

## Measurement contract

`frames.csv` retains legacy columns and adds monotonic begin/work-ready/swap-call/after-swap/completion timestamps, absolute target, blocked-call and active-tail wall durations, direct wait error, CPU work wall time, process CPU delta, phase recovery, cap/pacer/swap interval. `pacing.json` reports P50/P95/P99 and CPU utilization (one full core=100%; normalized system value also given). CPU work is wall time excluding pacing waits; driver blocking remains included. GPU elapsed queries remain asynchronous; missing samples remain missing. `swap_ms` now brackets only SDL_GL_SwapWindow, excluding pre-swap pacing and capture. Timing launchers disable capture/readback.

The precise deadline targets **swap invocation**, not guaranteed API return or display scanout. The separately recorded API-return and completion intervals reveal work/swap jitter; neither proves displayed FPS. `display_interval_ms=-1`/null and `scanout_verified=false` are intentional. Vsync may block in the driver; cap+vsync is an explicit experiment, not assumed equivalent to uncapped vsync.

Original strict gates remain loop P95 AND swap-return interval P95 **<33.34ms at30**, **<16.67ms at60** on actual Intel UHD. Small jitter alone can violate these tight thresholds; do not relabel them as passes or substitute median/query throughput. New precise pacing remains unverified on the owner's device, even if CI or software tests improve.

**Drawable correction:** the H3 report said hidden timing windows were3x. H3 source actually defaults hidden windows to1x unless overridden; owner raw logs used explicit2x. H4 suites force `--present-scale2` for consistent comparison. Historical measured results are not rewritten.

## Build and tests

```
cmake -S experiments/holland004 -B build/h4 -DCMAKE_BUILD_TYPE=Release
cmake --build build/h4 --parallel 2
ctest --test-dir build/h4 --output-on-failure
```

Dependencies are pinned SDL3, cgltf, stb. No additional runtime dependency. For software GL diagnostics on Linux add `-DPIXEL3D_TEST_GL=ON -DSDL_UNIX_CONSOLE_BUILD=ON -DSDL_X11=OFF -DSDL_WAYLAND=OFF` and run CTest with `SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1`. Pillow/NumPy are needed for GL image assertions. `-DPIXEL3D_BUILD_GL=OFF -DPIXEL3D_SANITIZE=ON` builds CPU-only ASan/UBSan tests.

Tests cover rational deadlines across millions of slots, drift, stalls/recovery, cap transitions, overflow/invalid clocks, actual six-way cap/pacer rendering/camera equality and no cap0 waits; all inherited H3 tests remain enabled. Timing speed is not a flaky CI assertion. `holland004_timer_probe` compares Windows wait APIs without a GL context, using explicitly synthetic2ms work; this is not hardware graphics/scanout validation.

Private proof (real-model inputs required):
```
python experiments/holland004/tools/proof.py EXE GARDEN_PACK_JSON ORIGINAL_SIMPLE_ROOT NEW_OUTPUT --baseline PR32_EXE --benchmark --allow-software
python experiments/holland004/tools/analyze.py NEW_OUTPUT NEW_ANALYSIS --plots
```
Omit `--allow-software` on a hardware run. `proof.py` writes genuine native and integer-presented captures, checks exact pixels/camera rows against PR32, and runs separate noncapture600/120 benchmarks. Charts use all recorded post-warmup samples. A fixed-frame camera replay cannot establish pacing; use visible paired launchers and raw timestamps for owner review. `analyze.py` handles both old and new schemas and never treats Mesa timing as Intel validation.

Public CI contains code-only executables and synthetic tests. Model bytes, owner raw logs, captures and packaged private fixtures stay outside GitHub. See README_RUN_WINDOWS.md for reproducible Windows instructions and review commands. Win7, Android, scanout, physical GPU VRAM and thermal compatibility/performance are **NOT RUN**.
