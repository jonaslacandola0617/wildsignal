# RENDER-001 measured evidence

See [the technical report](../RENDER001_REPORT.md) for interpretation, limitations and pending gates.

- `*_windows_sdl.png`: actual SDL software readbacks on hosted Windows x64, including HUD and resize/letterbox. Source `b83b06b5872ada6d1851fdd67e9ef73a4333d0ab`. Matching `*_windows_sdl.json` and `*_windows_sdl_frames.csv` cover complete 36-frame lifecycle runs. These are short CI smoke results with a dummy video driver, not a physical display or owner-device benchmark. See `windows_context.json`.
- `linux_matrix.csv`: summary of 27 stationary, 60-frame uncapped runs. Exact per-run JSON reports are in `linux_matrix/`; all original raw frame streams are compressed in `linux_matrix_frames.tar.gz`.
- `*_standard.json` / `*_standard_frames.csv.gz`: separate sequential 60-second warmup + 180-second CPU-only captures at 480×270, 24 submitted actors, a 30 FPS cap and scripted walk/turn. Full visible counts vary with view and collision. These are not 24-continuously-visible sprite benchmarks.
- Linux performance source is `b3444c54442c7bd181b0ee4e68d026bd2c712fd5`, built clean with GCC 14.2.0 `-O2 -DNDEBUG`. The following source commit only changes explicit numeric conversions, the actor/visible HUD text and duration tests. `linux_context.json` records host limits, build/capture commands and fixture hashes. Host CPU is AMD EPYC, not the owner's i3.
- `SHA256SUMS.txt` identifies the recorded files. Statistics were checked against raw CSV with nearest-rank quantiles; original JSON reports were copied without relabeling their build SHA or measured values.

Extract on Windows with Python 3 (also works on Linux):

```powershell
python -m tarfile -e linux_matrix_frames.tar.gz extracted_matrix
python -m gzip -d echo_standard_frames.csv.gz
python -m gzip -d quay_standard_frames.csv.gz
python -m gzip -d forest_standard_frames.csv.gz
```

The committed gzip files remain available in Git if a local decompression removes one. The Windows CI artifact contains uncompressed native and CPU matrix results plus the runnable package, import report and package checksums. GitHub artifact retention is limited; this committed evidence remains available afterward.

`standardTimingProtocolMet` checks elapsed duration only. It does not certify stable 30 FPS, a real device, all actors visible, minimum RAM, thermal behavior or architectural/visual acceptance. RSS is process resident memory, not installed RAM or Android PSS. Long captures are on a shared managed host without exclusive CPU scheduling. Warmup filtering uses frame completion time; its boundary may be straddled by the first retained frame.
