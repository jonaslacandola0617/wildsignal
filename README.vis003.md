# VIS-003 — bounded facade readability experiment

This task-specific branch and draft PR are stacked on VIS-002 (`a976310`), without merging or editing that branch. Original 256 masters, approved prop, experimental human scale (3.2/2.4), 1.6-unit eye, wall geometry, 75-degree perspective, source UV orientation and unlit lighting are preserved. Neither final scale nor final artistic/production approval is implied. No new art, roof, opening, side-depth geometry or gameplay is implemented.

## Select the controlled comparisons

Use the private caches, native-source anchor layout and treatment JSON supplied separately to the owner. From their private folder, with the executable location adjusted:

```powershell
$engine = 'C:/path/to/wildsignal_engine001.exe'
$review = 'C:/private/vis003'
$look = 'c' # a = exact corrected VIS-002 baseline, b = materials, c = shared post/layout
$stamp = Get-Date -Format 'yyyyMMdd_HHmmss'
& $engine --sample-pack "$review/caches/sampling_256.wst" --facade-layout "$review/facade_layout.json" --facade-scale human --facade-look $look --facade-treatment "$review/treatment.json" --facade-projection perspective --camera 12 16.5 0 --resolution 480x270 --warmup 0 --fps 30 --output "$review/local/${look}_$stamp"
```

A keeps the previous pixels/materials/framing/placement; it must reproduce the previous human-scale scene exactly. B changes wall material balance and ground contrast while retaining the original two framed panels and door/window anchors. C keeps B's material treatment and tests one center post, rectangular timber strips at their original aspect ratios, a painted base band and small documented placement changes. C has no additional physical depth or collision openings. Each step has separate screenshots; ground-only/plaster-only controls identify which material change helps. Original plaster A/B/C/D and two reversible contrast candidates are also compared under identical framing, ground, projection and cameras. Native A-D images are never overwritten.

Change `$look` between `a`, `b`, `c`. Changing only `sampling_256.wst` to `sampling_128.wst` or `sampling_64.wst` preserves sampling comparison. F1 cycles 320x180/400x225/480x270, F2 HUD, F12 actual displayed BMP. W/S moves, A/D turns, Escape exits; focus-reset behavior remains. The title/HUD identify the experiment. `--facade-projection orthographic` retains the eight-unit orthographic elevation. VIS-002 remains available by omitting `--facade-look` and `--facade-treatment`; the original band uses `--sample-pack` alone, and the original three scenes run without it.

Fixed perspective camera poses: close `(11.5,15,0)`, normal/front `(12,16.5,0)`, street `(12,21.5,0)`, corner `(17,14.5,-45)`, side `(18,9,-90)`. All candidates use the same poses, scale, field of view and lighting. At close distance the approved prop may leave the field of view; it is not moved to improve screenshots. Side views show a plain wall, not fake timber depth. The moving/turning evidence repeats the same collision-valid pose path for each candidate.

## Private material preparation

Existing v1 sampling manifests/caches stay supported. The opt-in `wildsignal-private-sampling-input-v2` importer adds three explicit, reversible offline operations, in this order:

- `opaqueRgb: true`: treat an almost-opaque material source (every alpha >=240) as opaque while retaining its RGB bytes. Transparent sprites and the fixed approved prop cannot use this operation. The supplied reference crops need this experimental interpretation; it does not approve them as paving.
- `contrast: {"numerator":1,"denominator":2,"center":[128,128,128]}`: synthetic example. RGB output is `(channel*n + center*(d-n) + d/2) // d`, with integer `d/2`. Private centers come from the verified material mean. Numerator/denominator are integers `1 <= n <= d <= 256`. Alpha is retained. No blur, new noise or replacement drawing is introduced. The half-contrast candidate retains authored variations rather than becoming a flat wall.
- `padTo: [256,256]`: copy a smaller rectangular source to the top left of a transparent square, without stretching its pixels. This permits supplied timber strips and small ground crops to enter the same 64/128/256 cache format. Explicit normalized UV regions and uniform world transforms preserve their original aspect. The padding is an import derivative, not a replacement native master. Pixel-center bounds keep lower-resolution sampling inside the declared region; alpha inside a timber silhouette remains intact.

These optional fields are rejected by v1 and by the native fixed prop. Source SHA256, native RGBA dimensions and alpha are checked before preparation, cache checksums after loading, and source hashes again afterward. Prepared background must be opaque. Output must be fresh and outside Git/source directories. No new runtime image library is added. Cache dimensions remain the selected mode; a padded small ground crop does **not** gain new detail when represented by a 256-square cache. This is stated in the private import ledger.

```powershell
python -m pip install Pillow
python tools/import_sampling_pack.py C:/private/sources.json C:/private/new-caches
```

## Treatment schema and transforms

Private `wildsignal-facade-treatment-v1` JSON fields: `plasterMaterial` (zero-based slot, or -1 for ground-only control), `ground`, `vertical`, `horizontal`, `frameBorder`, `groundRepeat`, `baseHeight`, `doorX`, `windowX`, `windowZ`. Each region is `{material,u0,v0,u1,v1}` in the padded square. Store exact native source bounds once, then reuse them for every sampling mode. The loader bounds indices, field values, material opacity, native vertical/horizontal aspect, nonempty UV/pixel-center regions and ground opacity. The fixed prop is independent of all these fields.

World band/panel scale stays 2.5 square units and total wall height 3.2; B's selected plaster uses one repeat per 2.5 units, including the upper/side areas. Frame-border UV fractions identify the existing module's timber edges, retaining their actual pixels/UVs. This is an explicit geometric mask/composite; no original module PNG is painted over. C uses two horizontal strips per rail and three vertical strips total; every individual strip has uniform world scaling derived from its native UV aspect. The shared center post replaces the previous doubled supports. The private layout records the experimental 0.2-unit base, balanced door/window centers and window elevation. These are experimental surface transforms, not a building/world authoring contract.

Ground repeats preserve the supplied UV crop's native aspect. Only an explicitly declared interior UV region is used to exclude atlas border contamination. No seamless-edge repair or new paving is invented. The source crops are not approved production textures and remain visibly repetitive/nonseamless. A new approved seamless paving material is still required for production.

## Build, capture and limitations

```powershell
cmake -S . -B build/win -G "Visual Studio 17 2022" -A x64 -T v143
cmake --build build/win --config Release --parallel 2
ctest --test-dir build/win -C Release --output-on-failure
python tools/capture_vis003.py --exe build/win/Release/wildsignal_engine001_headless.exe --packs C:/private/caches --layout C:/private/facade_layout.json --treatment C:/private/treatment.json --study C:/private/study.json --output C:/private/new-evidence
```

The public Windows artifact contains both executables and no private art/caches. The capture helper requires Pillow and the headless executable. It refuses existing output or Git checkouts, records/rechecks input hashes, and saves actual PPMs plus lossless PNG conversions. It captures A/B/C at six views, two world resolutions, three sample modes, material/ground controls and identical 25-frame moving/turning paths. Sheets are labelled assemblies of genuine frames, not generated targets. All private bytes remain outside Git and public CI. CI uses unrelated synthetic fixtures only.

CPU/RSS screening runs A/B/C/C/B/A at fixed normal/front, 480x270 and sampling 256: 3,000 uncapped frames per process, first 500 omitted only in the summary. Raw telemetry retains all frames. This is short shared-host Linux headless work, not SDL/UI/presentation or Windows-owner/i3/4GB measurements, not the sustained device benchmark and not compatibility certification. Local fallback: `ASAN_OPTIONS=detect_leaks=0 bash tools/check_engine001.sh` when managed tracing prevents LSan; CI leak detection is separate. Local CMake/SDL absence must not be called a native Windows run.

Geometry remains the dominant next question once the material/layout comparison is reviewed: flat trims, no recess/doorway/window volume, no roof/gable/eaves and a billboard prop. Source plaster relief/repetition and ground tiling remain art limitations. No amount of 2D contrast adjustment completes the approved coastal architecture. Owner review determines whether the visual result is acceptable; any static facade/recess/roof geometry comparison requires a separately bounded next assignment.
