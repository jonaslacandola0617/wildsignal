# GAMELOOP-001 — Offline Glade Playtest

Isolated playable first-person proof on the Petalyn/HOLLAND renderer. This playtest does not change the overall game camera decision. Requires the owner's private GAMELOOP_001 runtime assets; no model, texture or creature source is distributed in this public repository.

Launch the private portable package using **PLAY_GAME.cmd**. WASD moves, mouse looks, E interacts with the cottage shelter or wild Petalyn. 1 attacks, 2 defends, 3 befriends; Enter returns after the outcome. F5 saves, F9 loads, G changes grass density, R changes internal resolution. Escape pauses; Enter resumes; Q from pause saves and quits. Click the window to capture mouse again. Focus loss releases the mouse and pauses exploration.

Temporary battle rules: companion30HP, wild24HP. Attack deals6 and enemy reply8; Defend reduces reply to2 and raises trust. Befriend succeeds at12HP or less after defending once. A defeated/calm wild creature can also be recruited. Losing returns safely to the trail at30HP. The cottage restores health and records a visit; interior traversal is not implemented. Recruited Petalyn becomes a second following actor. No online systems or production balance is implemented.

Saves live in `saves/game.json` relative to the launcher. A versioned bounded JSON stores position, view, HP, recruitment and visit flags. Writes use a temporary file, validation, backup and atomic replacement. Invalid primary falls back to a validated backup; two invalid files start safely. Uncommitted temp files are ignored. Saving during battle is disabled; closing then preserves the last exploration checkpoint. Saving failures are shown and logged. Keep the entire extracted folder together and use a writable directory.

## Build

```sh
cmake -S experiments/gameloop001 -B build/gameloop -DCMAKE_BUILD_TYPE=Release
cmake --build build/gameloop --parallel 2
ctest --test-dir build/gameloop --output-on-failure
```

Windows x64: use `-G "Visual Studio 17 2022" -A x64`, then `--config Release` for build/install and `-C Release` for CTest. Optional `-DGAMELOOP_ASSETS=/private/runtime` enables genuine source mesh/skin tests. Existing Petalyn/HOLLAND targets remain unchanged and unmerged. Private art Issue35 supplies the required assets. Shader, save, collision and input implementations are source-only here.

Command help is available using `wildsignal_gameloop001 --help`. Default internal480×270,30cap,medium grass,512px cached static shadows, shafts OFF, individual grass shadows OFF. `--resolution640 --cap60` is a stretch profile, not a promise. `--megakit-house off`, `--grass-density off|low|medium|high`, `--render-mode original|toon` support matched comparisons. `original` means current-renderer unbanded flat-normal source geometry/materials, not the proprietary Quaternius example engine shader. Runtime textures are explicitly labelled256-budget derivatives unless an alternate private source pack is supplied.

Hardware launchers collect CPU/update/animation, submit, valid indexed GPU query samples, swap-completion intervals, pacing, draw/triangle counts and process working set. Capture runs include readback overhead and are not performance evidence. Mesa software rendering is diagnostic only; pass `--allow-software` explicitly. There is no verified IntelUHD/4GB FPS result until the owner runs the suite. Android and Windows7 are NOT RUN.

State remains **GAMELOOP001_DENSE_GLADE_MEGAKIT_OWNER_PLAYABILITY_AND_INTEL_REVIEW_PENDING** after implementation verification and owner package delivery. No merging or automatic expansion.
