# HEARTHGROVE001 — authentic MegaKit playable pilot

Isolated copy of the existing GAMELOOP002/HOLLAND runtime, based on PR54commit2fa860e. Earlier targets/branches remain unchanged. No artwork is authored here. Private Issue54authorizes assembly of existing Quaternius Standard assets, approved grass/ground and unchanged Petalyn.

The private scene contains three cottage exteriors, original Nature MegaKit trees/rocks and source props over repeated existing ground modules. Scene configuration supplies spawn, rest/encounter locations, up to128conservative collision solids and world bounds. Existing exploration, two-companion trail/Idle/Fly behavior, battle/defend/recruitment and transactional saves are retained. The follower corner graph uses16nearby obstacles while all segments still test every solid; difficult routes may wait safely. Saves use a separate Hearthgrove path.

Build (VS2022x64/CMake3.24+/Python3):

```
cmake -S experiments/hearthgrove001 -B build/hearthgrove -G "Visual Studio 17 2022" -A x64
cmake --build build/hearthgrove --config Release --parallel 2
ctest --test-dir build/hearthgrove -C Release --output-on-failure
cmake --install build/hearthgrove --config Release --prefix out/hearthgrove
```

Linux logic: add `-DPIXEL3D_BUILD_GL=OFF`. Optional `-DGAMELOOP_ASSETS=<private-runtime>` enables actual private skin/grass checks; `-DPIXEL3D_TEST_GL=ON` also enables the full actual SDL route/reopen/recovery test where GL is available. CI is source-only and does not verify Windows GPU rendering. The private review supplies assets; never put them in this public repository.

Run `wildsignal_hearthgrove001 --assets <private-runtime> --save saves/hearthgrove.json --resolution 480 --cap 30`. PLAY_GAME.cmd is the default; A_SOURCE/B_PIXEL_TOON launchers select the controlled material profiles. WASD/mouse, E interact,1/2/3battle actions, Enter return/resume, F5/F9save/load, Gdensity, R480/640, Escpause/release mouse, Qsave/quit from pause. Cottage interiors/NPCs/quests are not added.

A: `--worldstyle source --render-mode original`; original source base-color textures, source linear/mipmap-linear sampling and supplied normals under existing diffuse lighting. B: `--worldstyle pixel --render-mode toon`; identical geometry/cameras/sun, existing banded lighting/outlines, original Nature textures and previously existing compatible cottage maps. No newly authored materials/textures. Grass/ground retain exact approved source maps/ramps and nearest sampling. Original normal maps/PBRroughness/specular are not implemented by this diffuse renderer; A is not a reproduction of another engine's MegaKit shader.

Native resolutions:480×270,640×360, optional1280×720for diagnostic comparison. Capture with `--benchmark --pose 0..8 --frames 4 --resolution 1280 --output <private-dir>`; output final_world.ppmis actual native framebuffer data. Grass shadow casting is always0; static object-on-grass receiving is preserved. Volumetric shafts stay OFF. BENCHMARK_AB.cmd performs four matched360-frame runs (60warmup), records machine identity and p50/p95/p99/API GPU/memory/geometry; it refuses software unless explicitly requested via executable flags. App/swap/API timings are not displayed-FPScertification.

Existing12mterrain modules are placed on11.9mcentres with10cmoverlap; no source mesh/UV/color art is modified. Collision heights sample the existing97×97height data and choose the higher existing surface in overlap regions. Grass maps into each unchanged local12mUV domain using its placement origin; four color families remain fixed. Terrain repetition and small edge steps remain limitations. A data-selected old tiny-understory primitive is hidden, retaining only the approved ground component; original asset bytes are preserved.

New main fields in private game.json: `world_half_extent`, `ground_spacing`, `spawn`, `wild`, `rest_spots`, `exclude_materials`; existing solids/house indices/review cameras remain. vegetation.json accepts optional `ground_spacing`, keeps64KiBchunked placement parsing,32opaque near/far variants and bounded16000roots. This is a35.8mpilot, not a general world streaming system or a new renderer architecture.

Stop: HEARTHGROVE001_ASSET_NATIVE_OWNER_VISUAL_AND_PLAY_REVIEW_PENDING. No merge or final art/hardware approval.
