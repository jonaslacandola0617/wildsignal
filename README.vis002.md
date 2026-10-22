# VIS-002 texture sampling experiment

This is a reversible sampling comparison, not a finished house, art approval or a production scale contract. The existing three scenes and classic controls remain. No roof geometry is added. The calibration fixture is a five-unit-wide, four-unit-deep solid wall block with a one-unit-square texture band at z=1..2 in a neutral bounded test area. Materials repeat once per experimental unit on its backing and floor. This unit does **not** mean a metre or an approved facade module. UV u follows the wall grid coordinate; v is downward from the top of each square. All modes use identical geometry, FOV 75°, eye 1.6 existing engine units and unlit source colors. Window/door padding stays intact. The fixed prop is two experimental units tall at native aspect ratio in every mode, not a new approved lamp scale.

## Private import

Original PNGs/native masters remain private and unchanged. Use Python 3 and Pillow for offline import; the C++ executable adds no image-decoder dependency. The importer losslessly decodes PNGs, verifies source SHA256/dimensions/binary alpha, and generates pixel-center **nearest-neighbor copies** at 64, 128 and 256. The fixed prop's decoded native pixels are copied identically into each cache. Runtime uses dynamically sized images and validates bounds/content checksums. Runtime checksums detect corruption; source identity is verified by SHA256 at import, not by trusting a cache as a secure signed package.

Create a PRIVATE source JSON outside the repository:

```json
{
  "schema": "wildsignal-private-sampling-input-v1",
  "materials": [
    {"id":"your-material-id", "path":"source.png", "sha256":"actual-source-hash", "size":[256,256]}
  ],
  "fixedProp": {"id":"approved-prop-id", "path":"approved.png", "sha256":"actual-source-hash", "size":[96,256]}
}
```

The first material must be opaque (floor/backing); subsequent binary-alpha materials overlay it in the band. 1..16 materials are supported. The approved prop is independent of material sample resolution. Do not substitute another prop if the verified source is unavailable.

```powershell
python -m pip install Pillow
python tools/import_sampling_pack.py C:/private/sources.json C:/private/new-caches
```

Use a fresh output directory outside any Git checkout; the importer refuses overwriting/importing into source storage or Git. `.wst` contains private derivative pixels: never publish it, its private manifest, screenshots or artwork in public PR/CI artifacts. Public automated tests generate unrelated synthetic engineering patterns only.

## Windows x64 build and run

```powershell
cmake -S . -B build/win -G "Visual Studio 17 2022" -A x64 -T v143
cmake --build build/win --config Release --parallel 2
ctest --test-dir build/win -C Release --output-on-failure
.\build\win\Release\wildsignal_engine001.exe --sample-pack C:/private/new-caches/sampling_256.wst --resolution 480x270 --output C:/private/results/live_256
```

The original executable name and CMake/SDL3 pin are retained. `--sample-pack` selects the isolated calibration fixture; the default three-scene workflow is unchanged. W/S moves, A/D turns, F1 changes world resolution, F2 HUD, F12 actual display BMP. Actor stress is irrelevant to this fixture (zero actors, one approved prop). `--smoke-test` is for the retained three scenes; it is not the VIS-002 comparison protocol.

For repeatable comparison use a fresh output prefix every run and change only the cache filename. `--camera X Y YAW_DEGREES` sets a fixed pose. Suggested fixture poses (experimental coordinates): front `12 16.5 0`; corner `17 14.5 -45`; side `18 9 -90`; close `11.5 11.85 0`. No camera pitch is implemented. All use the same unlit renderer/material mapping, and headless frames have no camera motion unless explicitly requested.

```powershell
.\build\win\Release\wildsignal_engine001.exe --sample-pack C:/private/new-caches/sampling_128.wst --camera 12 16.5 0 --resolution 480x270 --fps 0 --warmup 60 --duration 180 --output C:/private/results/front_128 --screenshot C:/private/results/front_128.ppm
```

Headless executable accepts the same options; its results omit SDL upload/UI/presentation. JSON records sample resolution, exact external pixel cache bytes, combined fixed+external texture cache, frame/work timing and OS RSS. RSS includes the existing engine, framebuffer, telemetry and runtime overhead; cache byte counts alone are not process memory or minimum-system requirements. A Windows CI build with synthetic art is not private-art execution on the owner's Windows 11 PC. Win7/Android and sustained owner-device performance remain untested.

## Limits and next decision

64/128 caches are test derivatives, not replacement masters. Higher source resolution does not increase world-buffer resolution; distant/oblique detail still aliases with nearest sampling. Binary alpha is supported; translucent art is rejected. The source's baked upper-left shading remains in its pixels and does not rotate with a real light. The prop is a billboard and has no solid side volume. The block has no roof, eaves, interior or facade geometry contract; it is not a completed house. No final visual/renderer selection or commercial approval is implied.

Before a faithful house exterior, decide the numerical facade-module-to-world width/height and its relationship to door baseline, eye height and lamp scale, and supply the actual intended house assembly/roof geometry/material. A bounded future geometry proposal is fixed facade quads plus the referenced pitched roof/gable/eave surfaces with perspective-correct UVs and shared depth. Do not substitute roof cards or automatically implement another renderer. Private measured screenshots/results and artistic comparison remain outside this public repository.

Implementation/measurement summary: `docs/bench/VIS002_REPORT.md`. Private art comparisons and original sources are deliberately excluded.
