# WILDSIGNAL — Real Device & Legacy Windows Validation Matrix v0.1

**Status:** Test PLAN only. Nothing below has been executed, and no support claim should be advertised until results are recorded. Owner date 2026-10-20.
**Primary goal:** identify which first-person renderer and packaging strategy is viable on the owner's actual Windows/i3 4GB and Galaxy S10+, and prove native **x86_64 only** legacy Windows functionality before claiming support back to Win7.
**Depends on:** ENGINE_ARCHITECTURE_v0.1.md, RENDER_BENCHMARK_PLAN_v0.1.md, WINDOWS_7_11_COMPATIBILITY_v0.1.md, HERO_SCENE_TARGETS_v0.1.md, DECISION_REGISTER_v0.1.md.

## 1. Confirmed hardware and unverified fields

| Identifier | Supplied by owner | Still not known | Appropriate test classification |
|---|---|---|---|
| PC-W11-01 | Windows **11**, 10th-gen **Intel i3**, **4 GB system RAM** | CPU suffix, integrated GPU, drivers, Windows build, SSD/HDD and free disk, native screen resolution | Actual low-memory development PC, release Windows smoke, native renderer performance |
| PHONE-S10P-01 | **Samsung Galaxy S10+** | full model number, Android version, SoC (Exynos/Snapdragon), exact RAM, battery condition and available storage | Actual Android native smoke, touch notches, lifecycle, input, sustained thermals |
| PHONE-2GB-01 | **Not available/identified** | device, OS, GPU, storage and thermal conditions | BLOCKED target for 2 GB compatibility claim |
| PC-W7-01 | **Not available/identified** | licensed Win7 SP1 x64 VM/PC and updates | BLOCKED real x64 Win7 runtime verification |
| PC-W8-01 | **Not available/identified** | licensed Win8 x64 test | BLOCKED old-OS startup verification |
| PC-W81-01 | **Not available/identified** | licensed Win8.1 x64 test | BLOCKED old-OS startup verification |
| PC-W10-01 | **Not available/identified** | Win10 x64 test machine/VM | BLOCKED independent Win10 verification |

**Owner approval:** Windows 7–11 **64-bit only**; no 32-bit binary or 32-bit OS coverage. Proposed practical oldest Windows test floor: Windows 7 **SP1** x64 with relevant signing/servicing updates. Compatibility and performance are not identical: a Win7 VM can prove startup and APIs but **not** authentic old integrated-GPU FPS.

## 2. Toolchain strategy for the actual 4GB i3 PC (candidate, not installed)

- Keep a text editor, small C++ compiler and Ninja/CMake local when practical; benchmark compilation RAM/time, not just game memory. Codex can propose exact pinned toolchain only after owner's explicit implementation authorization.
- Use cloud CI for full Android NDK/SDK/Gradle build and optionally Windows x64 release, so local Android Studio is not mandatory on 4GB RAM. Windows x64 native smoke remains important on owner's Win11 box.
- **Win7 binary compatibility:** test actual chosen `SDL3.dll` (if distributed), C++ standard runtime, image/audio libs, TLS optional adapter and all DLL import tables. A Windows version macro alone cannot guarantee Win7 startup.
- On Galaxy S10+, use an installable Android arm64 debug APK in a *later* authorized experiment. Avoid claiming minimum Android API simply from that one phone.
- Game offline-first: no login, Steam client, internet or external browser needed for Chapter 0 on any supported OS. Steam itself dropped Windows 7/8/8.1 support; standalone game distribution for those systems needs separate selection and QA.
- Hardware reports must not include personal account IDs, serial numbers, IMEI, local file paths revealing identity or unredacted private email addresses.

## 3. Scenario coverage by platform and feature

| Test ID | Scenario | Win11 i3/4GB | S10+ | Win7/8/8.1/10 x64 | 2GB Android |
|---|---|---|---|---|---|
| HARD-001 | Offline airplane/network-disconnected boot & menu | required | required | required | required if obtained |
| HARD-002 | 320×180 world; crisp independent UI | required | required | required | required |
| HARD-003 | 400×225 and 480×270 screen quality | required | required | required | conditional |
| HARD-004 | Z01 open harbor sightline, 1/8 sprites | required | required | render functional | required |
| HARD-005 | Z03 fern transparency 1/8/24 sprites | required | required | render functional | required |
| HARD-006 | Z04 ceiling, arches, floor/pillar occlusion | required | required | render functional | required |
| HARD-007 | Input + mouse/touch focus and restart | mouse/keyboard | touch, cutouts | mouse/keyboard | touch |
| HARD-008 | 20-minute sustained frame/memory/temperature | required | required | optional with real GPU | required if obtained |
| HARD-009 | Sleep/lock/background/resume and save recovery | app lifecycle | Android lifecycle | app lifecycle | app lifecycle |
| HARD-010 | Local save/reload across Windows versions | required | local | required Win7↔Win11 | local |
| HARD-011 | Game runs with **no Steam client** | required | N/A | required for Win7/8/8.1 | N/A |
| HARD-012 | Optional online authentication and two-client co-op | later | later | **security audit before promises** | later if supported |
| HARD-013 | UI touch targets at native device dp/sp | N/A | required | N/A | required |
| HARD-014 | Android cold start, battery and audio focus | N/A | required | N/A | required |
| HARD-015 | Invalid/old dependency DLL import failure | required at build | N/A | required | N/A |

**All cases above: NOT RUN.** Platform cells describe required future test coverage, not measured success.

## 4. Render benchmark measurement contract

For each sample record build SHA, compiler/SDL/NDK version, OS, anonymous device hardware profile, scene/camera/geometry source version, world buffer resolution, sprite count, frame rate cap and power mode.

- **Frame times:** p50/p95/p99 `ms` after ≥1 minute warm-up and ≥3 minutes recording; test 20-minute thermal soak on S10+ with performance trend by minute.
- **Components:** fixed simulation time, wall/floor draw, sprite alpha/sort, UI, upload/present and audio overhead separately.
- **Memory:** Windows process peak RSS/working set with method; Android PSS with method; atlas decoded bytes and object counts. A "2GB phone" guarantee cannot come from an 8GB S10+ or a memory cap on desktop.
- **Performance objective:** ~30 FPS at *measured* selected settings (for 30FPS cap p95≤33.3ms, p99 desirable≤50ms), not a verified game claim. Lower resolution may help but only if visual recognition is acceptable.
- **Qualitative visual check:** compare same first-person hero shot from Z01/Z03/Z04, with identical depth/actor counts, 320×180 vs 400×225 vs 480×270. Avoid claiming sector renderer is better if its scene uses less content.
- **Screenshots:** original game test builds only, not third-party reference art embedded in game. Owner chooses if WILDSIGNAL resemblance and character readability pass.
- **No current benchmark result:** all planned-run entries must start as `NOT_RUN` and be updated from real logs/measurements.

## 5. Future milestone exit criteria

**Before Codex ENGINE-001 is approved:** this plan is reviewed; actual PC Win11 and S10+ model/OS optionally recorded; game x64-only Windows release is explicit; no unsupported 2GB promise; three scene test views approved; original creature source privacy decided; toolchain choice reviewed.

**Before selecting RENDER-001 or RENDER-002 for production:** required scenes built/tested on the actual PC and Galaxy S10+ with evidence; owner has visually accepted dungeon architectures, foliage and companion sprites; camera height/FOV and SDL presenting chosen; performance headroom logged.

**Before Windows 7–11 support is marketed:** each target OS tested (Win7 SP1 x64, Win8 x64, Win8.1 x64, Win10 x64, Win11 x64); legacy standalone distribution mechanism selected, offline functional tests pass. Secure online feature parity on old unsupported OSes separately reviewed. Physical old GPU test if claiming low-end FPS.

**Before 2GB RAM Android support is marketed:** at least one actual supported-API low-end ~2 GB RAM phone validated in sustained gameplay, with frame-time, PSS, thermal, input and save data. Without this evidence, keep 2GB RAM goal explicitly unverified.

**Before commercial Steam/Play launch:** signed release compatibility, independent store receipt verification, co-op across actual Windows and Android clients, account/support/moderation policy, accessibility and legal audit all have separate acceptance plans.

## 6. Reporting template (fill only after actual future test)

```text
Run ID:
Build SHA:
Device profile (anonymous) / OS:
Architecture: windows-x86_64 | android-arm64
SDL/toolchain versions:
Scene / camera / world buffer:
Sprite count / overdraw setting:
Duration and environment:
P50/P95/P99 frame time: NOT RUN
Process RAM / Android PSS: NOT RUN
Battery/temperature: NOT RUN
Input/pause/save pass: NOT RUN
Image comparison pass: NOT REVIEWED
Known feature missing or compromise:
Screenshots/log references:
Result: NOT RUN
```

**This document is a test and architecture preparation plan, not an implemented engine, a passing device certification or an authorization to begin code.**
