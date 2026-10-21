# WILDSIGNAL RENDER-001

Multi-environment renderer experiment extending the accepted ENGINE-001 prototype.
Original programmer art only. No production gameplay, private creature sheets or online systems.

## Windows 11 x64

Install Git, CMake 3.24+, Visual Studio 2022 Desktop development with C++ (v143 + Windows SDK), and Python 3 for export tests. Network is needed once to fetch the pinned SDL 3.2.10 source. The packaged executable runs offline.

```powershell
git checkout feat/render-001-three-scenes
cmake -S . -B build/win -G "Visual Studio 17 2022" -A x64 -T v143
cmake --build build/win --config Release --parallel 2
ctest --test-dir build/win -C Release --output-on-failure
cmake --install build/win --config Release --prefix out/render001-windows-x64
cd out/render001-windows-x64
.\wildsignal_engine001.exe --scene quay
.\wildsignal_engine001.exe --scene forest
.\wildsignal_engine001.exe --scene echo
```

Keep `data/bench` beside the executable. CLI also accepts explicit fixture paths. The executable name is retained for baseline continuity. Build uses static SDL/CRT; no SDL DLL installation is needed. This is an unsigned development build. Windows 7 and Android remain unvalidated.

W/S move, A/D turn, F1 cycles 320×180 / 400×225 / 480×270, F2 toggles HUD, F3 toggles 1/8 actor stress, F12 saves the actual display BMP, Esc exits. Mouse controls remain deferred to Issue #5. `--sprites 24` selects the larger stress population. Scenery is additional to actor count; telemetry reports submitted and final visible counts separately.

```powershell
.\wildsignal_engine001.exe --scene forest --sprites 24 --resolution 480x270 --warmup 60 --duration 180 --fps 30 --walkthrough --output bench/local/forest_device_001
```

Use a new output prefix each run. JSON/CSV export on clean exit. Run each scene/resolution/population separately for device comparison. `--screenshot world.ppm` saves the internal world buffer; F12 includes HUD and scaling. Native defaults: 60-second warmup, 30 FPS cap. Headless defaults: 300 frames, no warmup or cap. An explicit duration without `--frames` overrides the headless frame default.

## Reproducible CPU-only validation

```sh
cmake -S . -B build/core -DENGINE001_BUILD_SDL=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build/core --parallel 2
ctest --test-dir build/core --output-on-failure
python3 tools/capture_render001.py build/core/wildsignal_engine001_headless bench/local/matrix --standard
```

The last command runs 27 short matrix cases, followed by a 60-second warmup and 180-second capture per scene at 480×270 / 24 submitted actors with scripted walking. CPU-only results exclude presentation and do not establish performance on the owner's PC. See `docs/bench/RENDER001_REPORT.md` for evidence and limitations.
