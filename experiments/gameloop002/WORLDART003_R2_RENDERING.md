# WORLDART003 R2 — bounded vegetation renderer extension

The existing GAMELOOP002 application loads optional private `vegetation.json`.
Without it, default scene, grass and lighting retain R1 behavior. Explicit
`--grass-family legacy` loads the old six models. Private art, references,
captures and owner hardware archives must stay outside this public repository.

The descriptor supplies opaque single-primitive near/far clumps, four root/mid/tip
palettes, exact surface-following roots/normals, scales, shared ground albedo and
patch IDs, plus per-model cast/receive policies. Placement chunks preserve the
inherited64KiB JSON limit. Paths/populations/indices/colors/scales are validated.
GPU instance streams are sized from actual per-model maximum populations.
No leaf billboards or per-frame allocation pipeline is introduced.

Controls:

- `--grass-family auto|legacy|r2`; auto detects the descriptor.
- `--grass-scale mixed|standard|small`; standard1.00, small0.88width/0.82height.
- `--shadow on|off` controls static maps independently of grass receiving.
- `--grass-receive on|off`; R2 defaults ON, legacy control defaults OFF.
- `--grass-cast off`; ON is rejected. The grass shadow pass stays empty.
- `--grass-receiver root|fragment` compares existing limited stable PCF methods.
- `--diagnostic lit|albedo|patch|visibility` outputs native debug renders.
- `--canopy-shadow solid|mask` compares solid canopy shadows with a bounded
  authored alpha-mask proxy in its shadow pass only, anchored to the fixed sunlight projection so front/back surfaces share a ray-coherent sample. This is not geometric leaf
  porosity or Holland's billboard-instance tree method.
- `--shadow-cache-probe` is an isolated automated test: caster transform atframe10,
  sun direction at20, camera-only yaw at30. Disabled for normal play.

New receivers attenuate direct light while retaining ambient. Ground/grass share
the bounded direct-light floor and authored sun-color response to avoid a root
brightness seam. Legacy shading remains available unchanged. Policy/sun/caster
transform changes invalidate the cache; camera motion does not. Uploaded GPU
geometry is immutable: replacing a model constructs a new renderer/cache, not a
claimed hot-reload API. Appended telemetry reports tree/rock/building casters,
zero grass caster draws/triangles, roots/culls and cache invalidations.

No battle/follower/interaction/save/GAMEUI/creature/cottage system is rewritten.
Visual terrain can still differ from gameplay heights; private handoffs must
report that art-benchmark limit. Sun shafts remain OFF. Build the existing
gameloop002 CMake project; branch CI provides Windows x64 code-only artifacts and
existing logic plus synthetic descriptor/culling/LOD/scale/error tests. Private
native renders and physical Intel UHD tests remain separate evidence.
