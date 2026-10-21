# WILDSIGNAL — ENGINE-001 Echo Hollow prototype

Native C++20 CPU grid raycaster + SDL3, one original 24×24 benchmark scene.
**Prototype for review; real Windows 11 i3/4GB acceptance is still required.**
No production gameplay or online systems are included. No owner creature artwork
is embedded or published. See [dependency/art ledger](third_party/README.md).

## Build and run on Windows 11 x64

Install Visual Studio 2022 **Build Tools**, Desktop development with C++ (v143,
x64), CMake >=3.24 and Git. A full Visual Studio editor is unnecessary. Use an
**x64 Native Tools Command Prompt for VS 2022**. Ninja is optional for the commands
below (required only for the supplied presets). Limit local builds to two jobs on
the owner's 4GB machine.

```powershell
git clone --branch feat/engine-001-echo-renderer https://github.com/jonaslacandola0617/wildsignal.git
cd wildsignal
cmake -S . -B build/win -G "Visual Studio 17 2022" -A x64 -T v143
cmake --build build/win --config Release --parallel 2
ctest --test-dir build/win -C Release --output-on-failure
cmake --install build/win --config Release --prefix out/engine001-windows-x64
cd out/engine001-windows-x64
.\wildsignal_engine001.exe
```

CMake retrieves the pinned SDL source at **build time**; the packaged executable
does not access the network. Static SDL and static MSVC CRT are selected, so no
SDL3.dll is required. Keep the installed `data` and `licenses` folders with the
EXE. Launch from a writable extracted folder; default reports go under
`bench/local` relative to the working directory. To write elsewhere, pass
`--output <writable-directory>/<unique-run-name>`.

With Ninja and the x64 compiler in PATH, `cmake --preset windows-local-release`,
`cmake --build --preset windows-local-release` and `ctest --preset
windows-local-release` are equivalent single-config commands. Debug preset is
also provided. For an offline build, acquire that exact SDL commit beforehand
and add `-DFETCHCONTENT_SOURCE_DIR_SDL3=C:/path/to/verified/SDL` to configure.
No global SDL installation or engine editor is needed.

GitHub Actions also builds/tests Windows x64 Release and uploads a portable
artifact plus import/toolchain records. CI execution is not owner hardware testing.
To use that ZIP without installing build tools, extract it, open
`out/engine001-windows-x64` and launch `wildsignal_engine001.exe`, retaining its
data/license folders. The [draft PR](https://github.com/jonaslacandola0617/wildsignal/pull/4)
links the latest artifact and measured evidence. Artifacts expire after 14 days.
The core-only Linux/direct commands and actual results are in
[the implementation report](docs/bench/ENGINE001_IMPLEMENTATION_REPORT.md).

## Controls and measurements

| Key | Action |
|---|---|
| W / S | Forward / backward, 2 tiles/s |
| A / D | Turn 90 degrees/s |
| Esc | Exit and finish telemetry |
| F1 | 320×180 → 400×225 → 480×270 |
| F2 | Toggle separate display-resolution HUD |
| F3 | One / eight generic creature billboards |
| F12 | Save current display, including HUD, as BMP beside this run's CSV |

Focus loss releases all keys and freezes movement. Minimize pauses capture.
Resolution changes atomically replace CPU/SDL texture resources. World output
uses nearest-neighbor and integer scaling when possible, centered in a black
letterbox. Small windows use nearest-neighbor fractional downscaling; pixel
widths can be uneven. Resize does not stretch the world aspect ratio. HUD uses
SDL's pixel debug font independently of the world buffer (1× or 2× physical
pixels). Use at least a 640-pixel-wide window for the full debug legend; smaller
windows can clip text. No mouse capture/pitch is implemented.

```powershell
# Standard manual capture: 60s warmup, then 300s walkthrough on the real PC.
.\wildsignal_engine001.exe --warmup 60 --duration 300 --fps 30 --output bench/local/w11-320-one
# Repeat with --resolution 400x225 or 480x270, --stress, --fps 0, --software.
# Automatic repeatable collision-aware walk/turn sequence (not manual acceptance):
.\wildsignal_engine001.exe --walkthrough --warmup 60 --duration 180 --output bench/local/w11-scripted
```

`--help` lists all options. `--frames N` bounds a smoke test. `--camera X Y YAW`
starts at a validated fixture position, yaw clockwise from north.
The optional scripted sequence is a collision stress path and can stop against
walls; use manual input for a representative five-minute scene walkthrough.
`--screenshot world.ppm` captures the last raw world buffer, without UI; F12
captures the composited display. Warmup defaults to 60 seconds in SDL, zero in
the headless tool. Duration means **capture time after warmup**. Exit with Esc
for final JSON; terminating/killing the process may leave only an incomplete CSV.
An existing report prefix is rejected to preserve evidence.

JSON uses `wildsignal-render-bench-v0.1`; CSV contains actual per-frame timings,
configuration and camera. Quantiles use nearest rank. Frame times are monotonic
completion-to-completion, including cap wait and prior logging I/O. CPU wall/floor,
sprites, input/update, texture upload, UI and SDL present are timed separately;
SDL may defer draw work into present. Mixed configurations are grouped in JSON
and identifiable in CSV. Logging retains at most 120,000 samples (reserved once),
then reports dropped count. Events are bounded to 4,096. Rendering allocates no
heap memory per frame; resize/export may allocate. RSS is sampled once per second,
peak working set/RSS queried at export; allocated framebuffer bytes are not RSS.
Unknown hardware, thermals, input latency and Android PSS remain `null`/unknown.
Headless UI/upload/present metrics are `null`. No run automatically becomes a
device acceptance PASS. A warmup-only run exports `NOT_MEASURED` and null quantiles.

## Supported geometry and limits

The exact source fixture is copied to build/package data without changing the
planning document. Closed fixed-height walls (3.2 tile units tall), floor/ceiling
casting, wall depth per screen column, and back-to-front cutout billboards are
supported. Art uses russet/plum masonry, muted stone, moss accents and a small
mint triangular signal. The signal pulse and floor patch are authored effects.
Actors are visual markers, with no actor collision, companion AI or battles.

The arch is a **camera-facing alpha trim at (12,12.5)**, a decorative silhouette
over the existing pillar layout. It can look flat/rotate while viewed from the
side; it is not transparent wall geometry or a collision portal. Billboard
sorting is valid for parallel camera-facing cards inside the fixed wall height;
arbitrary intersecting planes, partial-height walls and translucent blending are
unsupported. True arches, stacked rooms, slopes, variable heights, look pitch,
dynamic lighting, audio and Android packaging are absent. Large hitches discard
simulation time above 50ms; bounded collision substeps prevent tunneling.

Windows 7–11 x64 remains a **target**. `_WIN32_WINNT=0x0601` and Win7-era memory
APIs avoid an unnecessary newer API floor, but SDL/compiler/CRT imports and real
execution need separate verification on every OS. No Windows 7, Android, 2GB RAM,
30 FPS baseline or visual approval claim follows from Linux checks or cloud CI.

## Scope and acceptance

No networking, accounts, multiplayer, combat, full AI, inventory, quests, saves,
payments, Steamworks, Play services, additional regions or creature catalog was
implemented. Existing preproduction specifications remain unchanged.

Before acceptance: run Release on the owner's Win11 i3/4GB machine for at least
five minutes, review screenshots at all three resolutions (near/far/occluded
marker, HUD on/off), exercise F1/F2/F3, resize, minimize, Alt-Tab and quit, and
retain JSON/CSV with actual CPU/GPU/OS/display/power details recorded separately
without personal information. Use a one-minute warmup and at least three-minute
capture per configuration. The broader 20-minute thermal/Android renderer
comparison is a later milestone. Do not close Issue #3 as done before its gate.
