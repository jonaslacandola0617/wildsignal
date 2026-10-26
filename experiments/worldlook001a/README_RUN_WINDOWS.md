# Windows x64 grass review

Extract the entire private review ZIP to a writable folder. The executable uses static SDL/C++ runtime and a working OpenGL3.3 driver; no Python or Blender is needed to run the included scene. The code-only public CI artifact does not contain private artwork. The complete private package includes the original garden pack and the actual original grass atlas/manifest.

- `GRASS_OFF.cmd`, `GRASS_SPARSE.cmd`, `GRASS_MEDIUM.cmd`, `GRASS_DENSE.cmd`: same garden, camera14 and lighting,480x270/30cap with opt-in precise pacing. Add `--resolution 640 --cap 60` from a terminal for the higher-resolution comparison. These are candidate densities, not approved low-end presets.
- `GRASS_COMPARE.cmd`: render eight actual fixed-view native/presented PPM captures (both resolutions, all densities), then open their output folder. Existing review screenshots are supplied as lossless PNGs. Capture runs are not benchmarks.
- `RUN_INTEL_UHD_GRASS_BENCH.cmd`:24 noncapture cases:8 uncapped cost controls plus16 legacy/precise pacing comparisons, each600frames/120warmup. Allow roughly4–6minutes on the target PC. It verifies the private atlas SHA256, records all input hashes, OS/driver, process CPU, working set, draw/triangle/LOD/texture stats and raw timing CSVs. No fallback art or software rendering is allowed.

Optional suite controls:
```
RUN_INTEL_UHD_GRASS_BENCH.cmd -Suite cost
RUN_INTEL_UHD_GRASS_BENCH.cmd -Suite pacing
RUN_INTEL_UHD_GRASS_BENCH.cmd -Repeats 2
RUN_INTEL_UHD_GRASS_BENCH.cmd -Vsync
RUN_INTEL_UHD_GRASS_BENCH.cmd -FineTailUs 0
RUN_INTEL_UHD_GRASS_BENCH.cmd -TimerResolution 0
```

Return the newly created `runs/grass-TIMESTAMP-ID` folder privately. Keep the same display/power settings across comparisons. Do not record video during timing runs. Original strict loop AND API-return P95 targets remain <33.34ms at30/<16.67ms at60; a pass does not establish scanout/displayed FPS. Previous owner H4 precise results still missed those strict targets slightly. New grass Intel UHD performance remains NOT RUN until you execute this build.

WASD moves; mouse/arrow rotation and existing camera/render controls remain. F1 cycles internal resolution. P/N/V change pacer/cap/vsync; mixed controls invalidate a timed benchmark. `--pose 22` is the top-down approximation,23 eye-level and24 side;25 inspects one selected first tuft (requires kit). `--grass-facing view|cylindrical|crossed` compares card orientation. L/F8 sunlight toggles are disabled with active grass so shafts stayOFF. Existing prior renderer executables/branches retain their controls.

## Rebuild with Visual Studio2022 x64

Install Desktop development with C++, Git, CMake3.24+ and WindowsSDK. At the review source commit:
```
cmake -S experiments/worldlook001a -B build/worldlook -G "Visual Studio 17 2022" -A x64 -T v143
cmake --build build/worldlook --config Release --parallel 2
ctest --test-dir build/worldlook -C Release --output-on-failure
cmake --install build/worldlook --config Release --prefix out/worldlook001a-windows-x64
```

Install the exact private grass source commit pinned in data/grass_source.json using its `WORLDLOOK_001A/install.ps1`, and copy the verified garden pack into assets/. The private review ZIP already does this. No third-party source art enters the public engine repository.

CPU/GL tests, build instructions and exact commands are included. Mesa software captures validate actual rendering, not Intel UHD speed. Windows runtime on the owner's GPU, Win7, Android, energy/thermals and displayed FPS require separate testing. No merge or production-art approval is claimed.

**WORLDLOOK_001A_OWNER_VISUAL_AND_INTEL_UHD_REVIEW_PENDING**
