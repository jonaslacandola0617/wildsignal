# PIXELSTYLE-001: first controlled rendering gate

Issue #56, based on draft PR #55 / a9887aa. This is a bounded runtime material-parameter test in the existing Hearthgrove executable. Source pixels, geometry, normals, UVs, placement transforms, R2 grass/ground, sunlight and gameplay are unchanged. No authored artwork, texture derivatives, noise/dither detail, alpha-to-coverage, MSAA, new lighting system or village expansion.

Use the unchanged private Hearthgrove runtime from private art PR #55. Build with the existing README commands. The following are **benchmark/render-only** controls; omitting them retains the original playable scene and baseline rendering:

```
wildsignal_hearthgrove001 --assets <private-runtime> --benchmark --pixelstyle-rig --pixelstyle A --pose 0 --resolution 640 --frames 4 --cap 0 --output runs/A
```

Select A / B1 / B2 / B3 with `--pixelstyle`. A is current B_PIXEL_TOON. B1 changes CommonTree_1 leaf sampling only. B2 changes only CommonTree_1 leaf lighting/edges and Rock_Medium_1 form lighting/edges. B3 combines them. Targets are scoped by exact model/material names, independently of material pooling. Other surfaces retain old shader branches. Use `--pose 0` medium, 1 close foliage, 2 close rock, 3 far tree. `--pixelstyle-motion --frames 300 --capture-every 2` gives the same 10-second 30-Hz physical-height camera walk/strafe/turn route in all modes. This diagnostic camera does not claim collision navigation testing. Selected instances keep their original transforms: tree_0, nature_rock_0, house_2_*, unchanged ground/grass. Creatures continue the original simulation but are hidden in the rig's render selection only.

## Exact first candidates

- B1 default `--foliage-sampling filtered-mip`: existing hardware-generated mip chain, LINEAR_MIPMAP_LINEAR minification and NEAREST magnification; same original 0.200000003 cutoff in color/depth and shadow alpha. No pixel/alpha edits, coverage rescaling or silhouette dilation. Alternative `nearest-mip` retains baseline NEAREST_MIPMAP_NEAREST, and `lod0` explicitly forces base-level nearest in both passes for diagnosis. A actually uses implicit nearest mip, **not** textureLod(0); the legacy style sampling=1 path is the forced-base option. Shadow and camera derivatives cover different footprints despite matching sampler/cutoff policy.
- B2 leaf lighting: wrapped normal cosine `(dot+.35)/1.35`, three form levels .20/.56/.95, thresholds .24/.62 with .04 transition half-width; local ambient floor (.28,.36,.32), shadow visibility floor .30, existing sky ambient × .10 × (1-sun) indirect fill. Source two-sided normals are preserved. This is presentation lighting, not authored leaf-color variation.
- B2 rock: three form levels .23/.57/.93, thresholds .25/.67, .035 half-width; local ambient floor (.18,.22,.25). Original smooth-painted source color remains.
- B2 outlines: same one-pixel kernel. Selected leaf-to-leaf depth gaps no longer receive internal silhouette ink/highlights; outer leaf ink .07 instead of .14, edge highlight capped at .10 instead of .24. Selected rock silhouette ink .24 instead of .48; no per-triangle crease lines. Architecture/grass postprocessing stays inherited. The shared inherited edge shader file is unchanged.

The fixed 512 shadow map, sun, exposure, sky, source cutoff and R2 distribution remain unchanged. Grass casts zero shadows and receives object shadows. No volumetrics. Source normal-map/PBR shading remains unsupported. The chosen moderate leaf alpha filter can still change small gaps and distant coverage; this must be judged in the matched moving clip, not asserted solved.

Private comparison sheet, native images, measured first-candidate results and exact before/after hashes are linked from the draft PR. They are actual engine outputs, not generated art. Software Mesa timings do not establish Intel UHD performance or displayed FPS. Stage one stops at the first owner review; no later artist-texture pilot is authorized by this implementation.

PIXELSTYLE001_OWNER_VISUAL_GATE_PENDING
