# Build the fresh journey

Use the engine checkout's current-linux preset. It requires C++20, CMake 3.24+, Ninja, X11/OpenGL development headers and the pinned FetchContent dependencies. The cloud onboarding helper in `/workspace/shared/wildsignal-setup/install.sh` activates this task's prepared tools and local headers.

```sh
cmake --preset current-linux
cmake --build build/current-linux --target wildsignal_game wildsignal_hd2d_test journey_session_tests --parallel 4
ctest --preset current-linux --output-on-failure
```

Prepare a **new private output directory**, outside the engine checkout. Supply the verified Pixel Crawler ZIP, the private art checkout containing house 023, and the converted original creature folder containing WS004/WS005/WS008 GLBs and their external PNGs. The helper never overwrites an existing runtime.

```sh
python tools/prepare_journey.py --pixel-crawler /private/pixel-crawler.zip --art /workspace/wildsignal-art --creatures /private/original-creatures --output /private/journey
build/current-linux/current/wildsignal_game --play /private/journey --house WS_HOUSE_023 --output /private/runs/journey
```

Controls: N new, L continue, 1–3 save slot on title; WASD movement; E talk/advance; 1–3 dialogue choices; F starts a nearby wild fight; 1 basic / 2–4 skills / R ultimate / C Signari dodge / Space Wayfinder roll / Tab swap / hold F Tune In. B book, I bag, M archipelago overview, Q compass hint, P pause, F5 save, Esc exit. Rest at the arrival path (E) heals and saves. Soft Moss can be gathered near the shaded wall at the west side of the meadow. No autoload or overwrite of the old pilot's saves: this build uses `<play>/saves/slot1.json` and companions.

For automated evidence only, use the documented cloud environment variables `SDL_VIDEODRIVER=offscreen EGL_PLATFORM=surfaceless LIBGL_ALWAYS_SOFTWARE=1 LP_NUM_THREADS=2 XDG_CACHE_HOME=/workspace/shared/wildsignal-setup/cache`. Mesa/llvmpipe is software rendering; its timings are not Intel UHD or Android performance. Add `--hidden --frames N --capture-every N --replay FILE` to record actual frames. Replay format is `frame down key` / `frame up key`; use `v` for F5 and `p` for pause. Choose a fresh output directory for every run.

`journey.json` records final story step, team count, encounter state and position. The rules regression test covers arrival → Fen → selected starter, menu movement gating, save/continue and missing-art rejection. It does not claim a finished story campaign. Native Signari are currently drawn from their original meshes in the rest pose; animation integration remains unfinished.

This is a reviewable foundation. See `HD2D_GAME_DIRECTION.md` for the remaining release scope and reference influences. The legacy pilot and standalone look-test targets remain available.
