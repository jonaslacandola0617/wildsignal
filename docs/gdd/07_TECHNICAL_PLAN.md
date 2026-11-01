# Chapter 7 — Technical plan

**Status:** DRAFT 2026-11-02. Choices are in 7.9.
**Built on:** chapters 1–6. Facts about today's code come from `experiments/hearthgrove_emaceart` and its tests.

## 7.1 Stack (kept)

C++20, SDL3, OpenGL 3.3 core on Windows, CMake. Windows builds with MinGW-w64 GCC (installed locally) or MSVC; 64-bit only. Android uses OpenGL ES 3.0 (a port is needed, 7.7). No third-party engine.

## 7.2 Code layout

Today the whole game lives in one experiment folder that also pulls code from older experiments (`creature3d003`, `holland001`). Plan:
- Move the game into a proper source tree: `game/` (gameplay: fights, team, items, quests, homestead), `render/` (GPU, shaders, UI drawing), `world/` (regions, chunks, streaming, terrain), `platform/` (SDL, input, files, Android glue), `tests/`.
- Old experiment folders stay for history but nothing new depends on them.
- Gameplay rules stay free of rendering and timers (as the fight and bond code are today), so they can be tested headlessly and replayed exactly.

## 7.3 Data-driven content

Everything designers change lives in data files, not code:

| File | Holds |
|---|---|
| `species.json` | The 50 Signari: type, base stats, role, size class, body plan, field ability |
| `kits.json` | Basic, 3 skills and ultimate per species: shape, timing, cooldown, effects, rank-ups (replaces today's moves + energy) |
| `types.json` | The 8-type chart |
| `regions/<id>/region.json` | Region chunk grid, gates, rest spots, homestead sites, music, weather |
| `regions/<id>/chunk_x_z.json` | Instances, collision, spawn points for one chunk |
| `spawns.json`, `items.json`, `recipes.json`, `shops.json`, `quests.json`, `strings/<lang>.json` | As named |

All files are validated when loaded (types, ranges, references); bad data fails clearly at load, never mid-game. Private art and data stay in the private art repo; the public repo holds code and test fixtures.

## 7.4 Regions and chunk streaming

From D-073:
- A region is a grid of square chunks (*proposed* 32 m). The 3 × 3 chunks around the player are loaded; the next ring in the direction of travel is prefetched on a background thread; chunks behind are unloaded.
- Models and textures are shared per region and reference-counted; a chunk only lists instances.
- Moving between regions goes through a gate with a short loading screen; only one region is in memory at a time.
- Static shadows are cached per chunk (today's shadow cache works this way for the whole corner).
- The first proof chunks the existing corner and must look the same as today (REGION-001).

## 7.5 Saves

- Today: versioned JSON with a temporary file and a `.bak` of the last good save. Kept.
- 1.0 save holds: Wayfinder (name, look, position, region), team and reserve (species, level, health, skill ranks), field book, bag, money, quest flags, per-region changes (guardians freed, gates opened, items collected), the homestead (site and layout), settings separately.
- 3 save slots, autosave at rest spots, F5 anywhere. Each format change bumps the version and has a tested upgrade from the previous one.

## 7.6 Performance budgets

Minimum devices stay the i3 10th-gen / 4 GB Windows PC and the Galaxy S10+.

| Budget | Target |
|---|---|
| Frame rate | Steady 30 FPS at 640 × 360 world render on Intel UHD; 480 as the light option |
| Process memory | Under 1 GB on Windows, under 700 MB on Android (today's corner measures about 240 MB) |
| Region memory | Shared assets per region under ~250 MB of textures and meshes |
| Load times | Region gate under 10 s; chunk loads never cause a visible hitch |
| Signari | ≤ 6,000 triangles, ≤ 24 bones (chapter 4) |
| Skeleton limit | The shader holds 24 bone matrices; raising it to 32 is cheap if a body plan needs it |

Every milestone ends with a short performance check on the i3 PC (frame times and memory), so problems show up early.

## 7.7 Platforms

- **Windows 10/11 x64:** primary. Windows 7/8.1 support stays a goal but is tested late; the current MinGW build uses the Universal C runtime, which old Windows needs installed.
- **Android:** a port milestone (GLES 3.0, touch controls from chapter 5, app pause/resume, storage) once region 1 is playable, then profiling on the S10+.

## 7.8 Testing and workflow

- **Tests:** unit tests for rules (fights, Tune In, saves, data validation), replay tests that drive the real game with recorded input, and performance checks. All must pass before a milestone is done.
- **Branches:** today every task stacks a new branch on the previous one and nothing is ever merged, because `main` holds only the first commit. Proposed fix: one **`develop`** branch that collects finished, owner-approved milestones. Each task branches from `develop`, opens a draft pull request into it, and is merged when the owner approves. `main` gets release snapshots only.
- **Builds for review:** each milestone produces a Windows folder with the private assets, as done so far, plus screenshots and a short moving preview.

## 7.9 Owner choices

| # | Question | Recommendation |
|---|---|---|
| T1 | Move the game into a proper source tree (7.2)? | Yes, early, before new systems |
| T2 | All content in validated data files (7.3)? | Yes |
| T3 | Chunk streaming design and the 32 m starting size (7.4)? | Yes, size confirmed by measurement |
| T4 | Performance budgets (7.6)? | As listed |
| T5 | Android port after region 1 is playable (7.7)? | Yes |
| T6 | A `develop` branch where approved milestones are merged (7.8)? | Yes |

## Change log

- 2026-11-02 — First draft.
