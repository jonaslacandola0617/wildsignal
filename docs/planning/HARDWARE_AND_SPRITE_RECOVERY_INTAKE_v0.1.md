# WILDSIGNAL — Available Hardware & Original Sprite Recovery Intake v0.1

**Status:** Facts supplied directly by the owner on 2026-10-20, plus proposed testing/asset-restoration instructions. **Planning only; do not write game code or modify existing source art on the strength of this document.**
**Related:** `NEXT_MILESTONE_ASSET_ENGINE_READINESS_v0.1.md`, `RENDER_BENCHMARK_PLAN_v0.1.md`, `CREATURE_CATALOG_GUIDE_v0.1.md`, `ENGINE_ARCHITECTURE_v0.1.md`.

## A. Owner-confirmed available hardware

| Test profile | Owner-provided facts | Open details | Test role |
|---|---|---|---|
| `WIN_BASELINE_01` | Intel Core i3 **10th generation**, **4 GB system RAM**, **Windows 11** | exact CPU model, integrated or discrete GPU, OS build, disk/SSD/free space, display resolution, driver | **Owner's actual Win11 development machine / Windows smoke & performance test** |
| `ANDROID_FUNCTIONAL_01` | **Samsung Galaxy S10+** | exact model/SoC, RAM configuration, OS version, available storage, battery condition, screen resolution setting | **Real Android touch, lifecycle, screen-cutout and sustained-performance test** |
| `ANDROID_2GB_TARGET` | **No 2 GB Android device confirmed available** | obtain/borrow an actual low-end ~2 GB RAM Android handset if deciding to claim this minimum | **UNTESTED aspirational target** |
| `WIN_2GB_TARGET` | **No 2 GB Windows device confirmed available** | check whether modern supported Windows OS and Steam can work within this RAM; owner PC is 4 GB | **UNTESTED aspirational target; may prove unrealistic due to OS needs** |

**Device background (externally documented, do not confuse with owner-verified configuration):** Samsung's public S10+ specifications include 8 GB or 12 GB variants and Exynos/Snapdragon regional processors. The precise owner's unit is not established from the name alone; it should NOT be entered as a confirmed 8 GB, 12 GB, Snapdragon, Exynos or specific Android build until verified. S10+ was a flagship device, so passing its benchmark does **not** establish performance on 2 GB entry-level phones.

**Important distinctions:** 4 GB RAM on the Windows development/test PC is not identical to the maximum RAM allowed for the game's process. Android available RAM and thermal/GPU characteristics differ from desktop Windows. A 2 GB simulated memory cap or emulator is useful as a preliminary stress test but **not** a substitute for a real target phone. No claim of 30 FPS or 2 GB compatibility has been measured.

### Quick non-invasive identification for a later benchmark
- **Windows:** Settings → System → About for OS/RAM/CPU; Task Manager → Performance → CPU and GPU for full processor and graphics names, or Win+R → `dxdiag` for display details. We do not need screenshots of serial numbers or account identifiers.
- **Galaxy S10+:** Settings → About phone → Model number; Settings → About phone → Software information → Android version. Device care/memory can show available RAM. Do **not** share IMEI, phone number or serial number.
- **OS confirmed Windows 11**. Record OS build, available storage and CPU/GPU details later. The **game compatibility target** is Windows 7 SP1 through Windows 11, **not just the developer's Windows 11**. See `docs/architecture/WINDOWS_7_11_COMPATIBILITY_v0.1.md`; no supported minimum OS may be advertised until compiled and tested.

## B. Low-resource development plan (proposed)

1. Prioritize **small C++20 source modules + CMake/SDL3** and local text editing over Unity, Unreal or a heavyweight graphics-editor runtime.
2. Build/test a minimal **native Windows x64 binary** locally when feasible. Cloud GitHub Actions should build Android arm64 debug APK and optionally Windows artifacts; never force full Android Studio/SDK/NDK installation on a 4 GB PC if it slows the machine to unusability.
3. Keep source-art files and game assets separate from generated build artifacts; avoid committing giant uncompressed caches/build outputs. Respect private-versus-public artwork decision.
4. Benchmark initial scenes on **both available devices**: Windows i3 10th-gen/4 GB and actual Galaxy S10+. For 2 GB Android claim, add separate device test later.
5. Test airplane-mode cold boot, landscape cutout/notch controls, resource use, Android pause/resume, 20-minute thermal session, Windows resize/Alt+Tab, and actual FPS/memory rather than estimates.
6. The engine renderer choice is still **open**. CPU world rendering plus efficient SDL presentation is an experiment, not evidence that integrated/mobile GPU/display can be ignored.

## C. Original creature artwork: what survives

**User confirmed:**
- All designs on the original 10×10 creature pixel-art sheet were hand-created by the user and are owned by the user; they are **NOT AI-generated**.
- The owner **no longer has the original editable Aseprite project**. It was stored on an older **broken desktop**.
- A combined flattened image of the 100-creature sheet was previously provided/cataloged for planning. The creator should provide the highest-quality original PNG/copy still available; never assume the rendered preview is the lossless original.

**Consequences:** An Aseprite source is **NOT mandatory** for WILDSIGNAL. We can work from an existing raster sheet and create new individual editable project files after careful extraction. The flattened image does **not** preserve erased layer data or original animations; new animation frames and any missing backs/sides will have to be authored. Avoid claiming automatic reconstruction of lost layer information.

### Safe, nondestructive sprite reconstruction procedure (design plan, NOT executed)
1. **Archive** the original surviving combined image byte-for-byte. Keep a read-only `source_original` file, record hash/dimensions/bit depth/color mode and where it was sourced from; never paint onto or overwrite this file.
2. **Grid measurement:** verify the actual 10 rows × 10 columns, border widths, cell boundaries and any uneven padding. Do not assume exact 85×83 pixels because combined file dimensions may include margins and gridlines. Keep permanent `WS-001...WS-100` row-major mapping.
3. **Lossless crop:** save raw region crops for all 100 cells to a `staging` folder; avoid interpolation, filtering, arbitrary auto-resize, palette quantization and sharpening.
4. **Background segmentation:** create transparent alpha masks around creature pixels. Lime background can be a seed for masking, but do not blindly global-color-key; some creatures/eyes/highlights may share its color. First clean the three starters `WS-004`, `WS-008`, `WS-050` and guardian `WS-024`.
5. **Edge QA:** inspect nearest-neighbor ×8 zoom on checkerboard plus light/forest/dark-ruin backgrounds. Verify no lime halos, lost outline pixels, holes in creature bodies, or accidentally removed pale/highlight details. Do manual cleanup where automatic segmentation fails.
6. **Rebuild editable files:** create fresh transparent per-species Aseprite / LibreSprite / Krita / other pixel-editor layers from those cleaned PNGs. Label that these are **reconstructed derivative masters**, not the lost historical original.
7. **Set anchors:** record original-canvas crop box, opaque bounding box, pixel pivot/footpoint (or hover anchor for flying), species ID, source copy hash and any source repair notes. Center and scale visually across battle and overworld, not by forcibly equalizing body dimensions.
8. **New production animations:** plan minimal idle, follow, attack, hurt and defeat frames; newly drawn side/back angles must be owner-reviewed to keep style consistent. Stage animation work, starting with three starters + one guardian; do not animate all 100 at once.
9. **Export data:** lossless transparent PNG + metadata frames/anchors, optional atlas packer preview; retain original source and all intermediate cleanup so mistakes can be reverted.
10. **Rights/privacy:** user artwork is owned by user. Never upload original source masters into a **public** Git repository unless they approve that publication. A private Drive or other approved asset archive may hold the masters; public game builds can include only approved exports. Maintain an asset-provenance ledger.

### Could the old Aseprite project be recovered?
Potentially, **if the broken desktop's SSD/HDD still works**. If owner wants, a technician or appropriately fitted USB SATA/NVMe enclosure may permit *read-only copying* of files to a separate drive. Do not attempt repeated boot/reformat/chkdsk on a failing storage drive, and avoid unnecessary access if physical drive damage is suspected; professional recovery may be warranted. This is **optional**: the combined PNG is enough for *manual sprite reconstruction*, though not automatic recovery of original layers.

## D. Immediate documentation deliverables, no code/asset edits

| ID | Required planning artifact | Evidence / exit |
|---|---|---|
| PRE-HW-001 | This available-hardware intake + final model/OS details when shared | clearly labels owner-confirmed vs device-family spec vs unknown |
| PRE-HW-002 | Actual baseline-test protocol for i3 10th-gen/4 GB Windows and S10+ | benchmark scene/script/checklist; no fabricated FPS |
| PRE-ASSET-001 | Original source file retention/provenance and privacy decision | copy/hash inventory; publicly published assets need owner approval |
| PRE-ASSET-002 | 4 pilot species' crop, alpha, pivot, new-animation review criteria | checkable checklist only, until user authorizes manipulation |
| PRE-VIS-001 | 3 scene boards with camera composition and creature-size examples | link to existing World and Visual Bibles; owner reviews desired quality |
| PRE-TECH-001 | C++20/SDL3/Raycaster-vs-sector proof task contract | future Codex task, not started |

**Next gate:** owner reviews precise existing device variants, artwork archive policy and visual-quality preview method. Only then consider authorization for small experimental game code or sprite cleanup; not implied by this hardware message.

## References for model-family facts
- Samsung original Galaxy S10 Interactive Specs: https://news.samsung.com/us/galaxy-s10-interactive/
- Samsung S10 family device variants: https://www.samsung.com/sg/support/mobile-devices/key-differences-between-the-galaxy-s10-phones/


## E. Owner platform compatibility update — 2026-10-20

- Owner's development PC is **Windows 11**, Intel Core i3 10th gen, 4 GB system RAM.
- Owner explicitly requests the **game** to work on **Windows 7, Windows 8, Windows 8.1, Windows 10 and Windows 11**. Windows 7 SP1 **x64** is our proposed first compatibility baseline; native 32-bit Windows support awaits clarification.
- **Separate software support from distribution:** Steam stopped supporting Windows 7/8/8.1 in 2024. Therefore plan a standalone client for legacy Windows, not a guarantee that the Steam client works there. Windows 10/11 remain Steam distribution targets.
- SDL3's Windows README documents support back to XP, but C++ runtime, SDL build/version, plugins, TLS, graphics presentation and the exact game must be verified on Windows 7.
- Legacy Windows online features require security review. Offline story is fully targeted on old OS; co-op feature parity there is not yet proven.
- See `docs/architecture/WINDOWS_7_11_COMPATIBILITY_v0.1.md` for OS matrix, legacy import checks, distribution and test cases. **No Win7/8 build or test has occurred.**
