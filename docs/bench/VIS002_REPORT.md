# VIS-002 sampling comparison — engineering review

Status: bounded texture experiment, **not final art approval, a completed house, or a numerical facade/world scale contract**. Draft PR targets the unchanged RENDER-001 branch. All private sources, derivatives, asset-specific manifests, screenshots and art assessment stay outside GitHub and public CI artifacts.

## Implemented

A private offline importer verifies source SHA256, RGBA dimensions and binary alpha, keeps 256 originals unchanged, and creates exact nearest-neighbor 64/128/256 caches. A separately verified prop stays at its native dimensions/pixels in every cache. Runtime textures carry width/height and bounded payloads; the executable has no new PNG/runtime dependency. It rejects bad headers, dimensions, alpha, checksums and trailing/truncated data. SHA256 establishes original identity at import; cache checksums are corruption checks, not cryptographic authentication.

The existing grid DDA is used for an isolated calibration block with square UVs and unlit source colors. Front/corner/side/close poses and three world resolutions remain fixed across modes. Framebuffer pixels and independent SDL HUD scaling remain unchanged. The one-unit texture band is an **explicit reversible test transform**, not a metre, story height or approved facade-module size. No roof, eaves, interior, geometric door/window opening or replacement artwork was added. The default three scenes retain their original path, fixtures and controls.

The current importer refuses output within actual Git checkouts; empty managed `.git` metadata mount directories are distinguished from repositories. `.wst` private caches are also ignored by Git. Public tests use unrelated synthetic engineering patterns only.

## Executed evidence

- Local GCC 14.2.0 Release and ASan/UBSan: original **200** checks, **479** RENDER checks (including 24 exact Echo goldens), **20,788** sampling invariants, and black-box import/export/offline/error checks passed. Local LSan remains disabled because of managed process-tracing restrictions; CI leak detection is separate.
- Sampling tests verify identical per-pixel depth, fixed-prop ownership masks and prop pixels across all three modes, cache byte accounting and no hot-frame heap allocation. Import tests compare exact derivative bytes and reject changed sources/corrupt caches/private output in checkouts.
- Windows CI builds x64 Release and exercises all three sample modes in SDL with synthetic data; Linux CI uses ASan/UBSan. No private art is available to either job. See PR checks/artifact for the build matching the desired source commit; build instructions are in `README.vis002.md`.
- Private execution produced **36 actual engine-world screenshots** (four poses × three framebuffers × three source sizes), plus three lossless 25-frame engine turn sequences. Captures use source `878d79fdb53bed1c0f946562f0579bd191c7ce7a`; the later instrumentation/import guard fix does not change raster output. The actual private packs additionally passed depth/prop-output invariance checks across every pose/resolution.
- These private captures are Linux **headless CPU engine** output, not painted concepts, SDL display screenshots, or private-art execution on the owner's Windows PC. Windows private-art/device visual/performance acceptance is **NOT RUN** here.

## Measured CPU screening

Source `e13d2682783af27f2e8be9a6039b8139e766fec5`; GCC `-O2 -DNDEBUG`; managed EPYC Linux container, not i3 hardware. Fixed front camera `(12,16.5,0°)`, 480×270, uncapped, order 64/128/256/256/128/64. Each run records 6,000 frames; independent analysis discards the first 600 and computes nearest-rank quantiles on the remaining 5,400. Raw telemetry correctly retains all 6,000 with warmupSeconds=0. This short screening is **not** the standard 60-second warmup/180-second capture/20-minute target-device acceptance.

Two-run ranges (observations, not confidence intervals):

| Source sampling | Median frame ms | P95 ms | P99 ms | Process peak RSS MiB | External pixel cache KiB |
|---|---:|---:|---:|---:|---:|
| 64 | 1.327–1.380 | 1.880–2.035 | 2.470–3.537 | 4.902–4.902 | 176 |
| 128 | 1.332–1.338 | 1.864–1.942 | 3.136–3.291 | 5.133–5.137 | 416 |
| 256 | 1.337–1.396 | 2.295–2.669 | 4.247–4.324 | 6.090–6.094 | 1,376 |

The selected cache has five materials plus a native fixed prop. The retained procedural renderer cache adds 640 KiB even though it is unused by this test; framebuffers, telemetry and code/OS overhead are additional. These are not minimum installed-RAM requirements or Android PSS. Frame times include previous sample I/O; component timings are in private raw CSV/JSON. No SDL UI/upload/present, GPU, input-latency or thermal measurement is included.

The former Linux `getrusage(ru_maxrss)` peak could retain the image-processing launcher's pre-exec high water. Linux now reports `/proc/self/status` VmHWM for the current executable image; repeated measured runs above use that metric. Windows retains GetProcessMemoryInfo. Preliminary launcher-affected peaks are excluded from the comparison, not interpreted as texture growth or a leak.

Medians overlap; the 256 runs have higher observed tails in this short shared-host sample. Do not infer equal cost, a final texture tier, minimum RAM or target-device FPS. Preserve originals and repeat balanced runs on the actual Windows 11 machine.

## Artistic and architecture findings

Private close views show progressively greater retention of joinery, lattice and foliage detail at 128/256. Distant/oblique source detail is increasingly limited by the world framebuffer and nearest-sampling aliasing. More source pixels do not correct texture repetition, value hierarchy, baked-light direction, prop scale, house silhouette or architecture. No new art acceptance is inferred.

The calibration block is not the approved harbor composition. Geometry is still axis-aligned walls on one floor; the prop is a billboard, binary alpha only. Door/window overlays are surface pixels, not openings. A square panel's source padding stays intact, but its test placement is not an approved facade/story/door baseline. There is no roof or gable/eave/support volume, and no roof billboard substitute.

Next: owner review of private near/oblique/turn evidence and Windows device measurement. Before a house proof, specify the square module's numerical world width/height, story assembly and door/eye/lamp relationship, and provide the intended house/roof contract. A bounded future static facade/roof/eave surface comparison with perspective-correct UVs and shared depth can then be proposed. No second renderer, building redesign, production renderer selection or unrelated gameplay is implemented or authorized by this report.


## Validated build

Source `e13d2682783af27f2e8be9a6039b8139e766fec5`: [successful Windows/Linux CI](https://github.com/jonaslacandola0617/wildsignal/actions/runs/39795433994). Windows x64 Release: **8/8 CTests** plus the standalone importer validation and SDL boots with all three synthetic sampling modes. The optional Pillow-dependent import CTest was not auto-registered by Windows CMake; the equivalent explicit Python step passed. Linux ASan/UBSan: **6/6 CTests**, including leak detection. [Code-only Windows artifact](https://github.com/jonaslacandola0617/wildsignal/actions/runs/39795433994/artifacts/12432278696), ZIP digest `sha256:553e977de3991b2407cb2603d2434908e243f04c02d6c80f0eff97435f31ebf3`; expires 2026-11-05. Private caches are supplied separately to the owner; they are never bundled in the public executable artifact. Later documentation-only commits do not change these executable sources.
