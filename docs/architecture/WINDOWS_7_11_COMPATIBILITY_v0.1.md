# WILDSIGNAL — Windows 7–11 Compatibility & Distribution Plan v0.1

**Date:** 2026-10-20. **Status:** **Owner-confirmed 64-bit-only Windows 7–11 compatibility TARGET**, not yet tested or guaranteed. **32-bit Windows is explicitly out of scope. No implementation authorized.**
**Local developer computer:** Windows **11**, Intel Core i3 10th generation, 4 GB RAM (exact processor/GPU/Windows build still unknown).
**Intended game OS coverage:** Windows **7 SP1, 8, 8.1, 10, 11**. The owner said “Windows 7–11”; the explicit versions in that range are listed for testing.

## 1. Key distinction: binary compatibility vs store/online support

1. **Native binary target (OWNER APPROVED):** Design **only Windows x86_64** game binaries for **64-bit Windows OSes** that can launch and run offline content on Win7 SP1 through Win11. **Do not build x86/Win32 binaries or claim 32-bit OS support.** Windows 7 SP1 is a *proposed oldest OS test floor* subject to proof. The game engine, audio, input, save and graphics dependencies must all function on that oldest tested OS. Compiling on Win11 is not proof it runs on Win7.
2. **SDL3:** The upstream SDL3 Windows README explicitly says all desktop Windows versions back to Windows XP are supported. This suggests SDL3 is compatible, **but does not guarantee** our chosen SDL3 release, compiler runtime, third-party dependencies, renderer or the complete executable on Win7. Pin and test actual builds.
3. **Steam:** Valve stopped supporting Windows 7, 8 and 8.1 on **January 1, 2024**. Existing installations may persist but are not supported, cannot be relied upon to function or receive security updates. Thus WILDSIGNAL's **Steam channel is for Windows 10/11**, subject to Steam's current client restrictions, and older OS compatibility requires a **separate standalone/DRM-free distribution path** (proposed; choose provider/domain/installer/legal terms later). Do not advertise Steam on Windows 7/8/8.1.
4. **32-bit Windows OUT OF SCOPE (owner decision 2026-10-20):** The game does not target 32-bit Windows installations or 32-bit executables. No x86 compilation, QA matrix or 32-bit downloads are planned. This is distinct from Steam's legacy OS support status. Reopening 32-bit support requires a new owner decision.
5. **Security and multiplayer:** Microsoft no longer issues standard Windows 7 security updates and 8.1 is also out of support. Running optional authenticated internet features on insecure OSes requires explicit security review (TLS, certificate store/trust, server compatibility, safe updates and incident policy). **Local offline adventure remains primary compatibility goal; do not promise online co-op on Win7/8/8.1 until tested and approved.** Cross-platform online co-op remains an approved **commercial launch feature** for Windows/Android on supported OS clients.
6. **Windows legacy prerequisites:** Win7 baseline for discussion is **SP1 with SHA-2 signing and relevant servicing updates** (e.g., KB4474419/KB4490628 when needed), not any untouched 2009 installation. Avoid graphics features requiring DirectX 12/modern Vulkan, unsupported UCRT/Visual C++ runtime packages or missing APIs without a compatible fallback. Test whether selected C++20 runtime binaries import Win8+/Win10-only functions, not only Windows API macros.
7. **Store model:** Free-to-play with optional cosmetic-only purchases remains approved. For any standalone Win7/8/8.1 version, **online store checkout and receipt entitlements require separate policy/backend planning**. Do not circumvent Steam/Google Play payment rules or embed credentials; if checkout isn't safe/available on old OS, optional cosmetics may be unavailable while offline gameplay works.

## 2. Proposed Windows deployment matrix

| OS | Standalone native offline game | Steam distribution | Multiplayer / account | Status |
|---|---|---|---|---|
| Windows 7 **SP1 x64** | TARGET, must be proven with real build | **Not a supported Steam client OS** | Feasibility/security gated | Unverified |
| Windows 8 **x64** | TARGET | **Not supported by Steam** | Feasibility/security gated | Unverified |
| Windows 8.1 **x64** | TARGET | **Not supported by Steam** | Feasibility/security gated | Unverified |
| Windows 10 **x64** | TARGET | Supported distribution target, verify current policies | Launch co-op target | Unverified |
| Windows 11 **x64** | TARGET; **owner's development OS** | Supported distribution target, verify current policies | Launch co-op target | Unverified |
| Windows 7–11 **x86 32-bit OS** | **Out of scope by owner choice** | No supported distribution target | Not supported | **Excluded** |

No claim that a 4 GB RAM PC or a 2 GB legacy PC already achieves 30 FPS. Installed memory minimum is a separate benchmark decision; OS support does not imply a RAM guarantee.

## 3. C++/SDL3 baseline build-compatibility constraints

- **Feature/API floor:** Compile Win7 SP1 compatible code path, with target Windows version `_WIN32_WINNT=0x0601` where toolchain honors it. Macros **do not** guarantee link/runtime compatibility. Check the import table of `wildsignal.exe`, SDL3 DLL, C++ runtime and third-party DLLs for unsupported calls.
- **Toolchain candidates:** A pinned MinGW-w64 cross-build for Win7 x64 or MSVC toolchain/runtime version proven on Win7; choose by *working binary*, not compiler popularity. Never assume newest C++20 runtime has legacy OS support. CMake/Ninja can orchestrate builds on developer's Win11 device/CI while emitting Win7-compatible binaries.
- **Present/render:** SDL3 CPU raycaster or sector renderer still needs SDL presentation backend; check compatibility with older GDI/Direct3D/OpenGL and integrated graphic drivers. No mandatory D3D12/modern Vulkan. Avoid external browsers/webviews or modern embedded Chromium as offline runtime dependency.
- **Save and asset paths:** use SDL filesystem APIs or explicit Win7-compatible known-folder/path APIs; avoid long-path or UTF-8 assumptions without tested conversion. Save schema and checksum recovery must work across Win7→Win11 migration.
- **Libraries:** test SDL3, audio decoding, TLS client library, compression, image decoding, launcher/update mechanism and any C++ runtime for OS import compatibility. A single Win10-only dependency breaks the Win7 target.
- **Crypto and online:** keep secure authenticated networking a **separate opt-in adapter** from core offline runtime. Do not weaken security to keep unsupported Win7 online; audit certificate validation, OS root stores and server TLS versions. If legacy OS online must be restricted, label it clearly and obtain owner review rather than silently claiming full feature parity.
- **No OS installer dependence:** a ZIP-based portable standalone build may be an experiment for Win7/8.1, but polished packaging, signed distribution and update checks require their own supported legacy path. No auto-update executable running with unsafe privileges.

## 4. Test procedure / platform acceptance checklist

### Early technical proof (still future work)
1. On owner's **Win11 i3/4GB** PC: launch C++/SDL3 executable, capture startup and 30-min memory/frame use in representative town/forest/ruins. Distinguish debug/release.
2. On a licensed Windows 7 SP1 **x64 VM or physical machine**: verify installation/portable launch, DLL imports, window/framebuffer, correct input/audio, save/reload and no Win10+ API import crash. Virtual test alone is *not* proof of performance on ancient integrated GPUs.
3. Repeat Windows **8 and 8.1** functional compatibility (separate tests, not inferred from Win7+Win11 success).
4. Test Windows **10 x64**, **11 x64** for rendering, Steam distribution later, Alt+Tab/HiDPI, file paths/antivirus false positives.
5. Run **file format cross-version check**: save in Win7 and load under Win11, and vice versa, with no undefined platform serialization.
6. Run **without Steam** on Win7/8/8.1: no Steam APIs injected into required offline game startup, no account/login prompt or online validation.
7. Crossplay account/co-op on legacy Windows OSes only if security audit passes; never request disabling TLS checks or installing obsolete browser components.
8. Record hardware/model/GPU/OS service pack, memory, FPS p50/p95/p99, audio/input correctness, longest session/thermal, screenshots and exact failure reason.
9. Where physical Win7/Win8 systems unavailable, log **NOT TESTED** and do not claim support on store/system requirements page.
10. 32-bit OS/build is explicitly **excluded** and must not enter required test/build checklists. Reopening requires a new owner decision.

### Test cases for issue/task tracking
- `WINLEG-001` Win7 SP1 x64 boot, graphics, mouse/keyboard, audio and saves offline.
- `WINLEG-002` Win8 x64 same coverage.
- `WINLEG-003` Win8.1 x64 same coverage.
- `WINLEG-004` Win10 x64 same coverage, future Steam smoke test.
- `WINLEG-005` Win11 x64 same coverage, actual owner's i3 test.
- `WINLEG-006` Win7–11 save-schema migration.
- `WINLEG-007` standalone build and no-Steam offline launch on Win7–8.1.
- `WINLEG-008` per-library Windows API import scan and Win7-compatible CRT/TLS versions.
- `WINLEG-009` online account/co-op *security feasibility*, including fallback/clear error.
- `WINLEG-010` minimum RAM/integrated GPU **real hardware performance**; do not infer from OS version.

## 5. Development/distribution choices awaiting signoff
- **Resolved:** only 64-bit Windows OS and x86_64 game executables are in scope. **Unresolved:** whether Win7 SP1 x64 can actually run the selected compiled C++/SDL3 dependencies, and whether the separate legacy distribution path is practical.
- Which separate distribution provider can legally, safely and conveniently deliver a free-to-play standalone binary for Windows 7/8/8.1? Candidate examples: direct website or third-party storefront that permits a DRM-free standalone package; **no provider yet selected or validated**.
- Does the Windows 7/8.1 version require optional online co-op, or is offline-game support sufficient if a security audit shows risk? This is not a stealth downgrade from the approved Android↔Windows co-op launch feature; identify supported Windows client OSes clearly.
- How can the developer obtain Windows 7/8/8.1 clean test environments? Do not ship or distribute Windows OS images without rights.
- Which compiler/SDL3 DLL versions demonstrably run on Win7? Test binary imports and actual startup prior to freezing `WINLEG-001`.

## 6. External authoritative resources (checked 2026-10-20)
- SDL3 Windows platform notes (legacy support): https://wiki.libsdl.org/SDL3/README-windows
- Steam legacy Windows support policy: https://help.steampowered.com/en/faqs/view/4784-4F2B-1321-800A
- Steam 32-bit client support policy: https://help.steampowered.com/en/faqs/view/49A1-B944-48B8-FF00
- Microsoft Windows 7 SP1 SHA-2 update prerequisites: https://support.microsoft.com/en-us/help/4474419 and https://support.microsoft.com/en-us/help/4490628

**Source-policy note:** This draft incorporates actual policies as of check date but is not a guarantee of future Steam/API changes. Re-check near shipping.


## 7. 64-bit-only scope confirmation

**Owner's exact answer (2026-10-20): “64 bit is enough.”** This settles the x86 versus x64 architecture question for WILDSIGNAL: target only **Windows x64 on 64-bit Windows 7, 8, 8.1, 10, 11**. Previous passages in other revisions treating 32-bit support as an open question are superseded. Windows 7 SP1 x64 is the first proposed *test floor*, not verified compatibility. Optional online features on old unsupported Windows OSes remain security/feasibility-gated; the offline core must not depend on Steam login. No game code was written under this planning update.
