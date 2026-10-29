# Hearthgrove balanced corrections R2

Owner feedback keeps the BALANCED cottage look and rejects R1's generated flat ground and green treatment of brown texels. This isolated branch restores original EmaceArt terrain/road/cover mesh files, preserves source textures/UVs, and changes only instance placement and bounded vertical relief. Default relief is85% of the original, with100% available in ORIGINAL_KIT_RELIEF.cmd. No source vertices/normals/indices are rewritten and no new ground texture or model is authored. The numeric support cache is scaled by the SAME world-Y transform as the rendered instances; props and sparse existing grass roots are reprojected. Existing source bumps and field patterns remain. This does not claim R1's flattening-based slope targets.

Material revision2 keeps balanced architecture/lighting, retains the previously selected green hue direction ONLY on green vegetation texels, and increases their saturation modestly. Brown/cream terrain, soil/path texels and brown vegetation texels bypass grading. Original source textures remain byte-identical. Revision1 settings and baseline remain supported for historical comparisons. Sky, character sources and approved building look are preserved.

Trees/bushes get stronger one-pixel silhouettes and sun-facing warm rims. Grass gets a separate G-buffer category so its depth silhouettes and sun-facing rims are enabled without outlining every internal triangle. Rocks get readable outer/convex sun-facing highlights instead of the old tiny capped change. Architecture keeps its balanced treatment. All use real depth/normal masks; no image paint, random noise or glow pass. Use **J** to toggle highlights and **K** for outlines at the SAME camera position. Material grading stays unchanged during these toggles. F still toggles the source look.

## Shadow correction and its tradeoff

An R16UI owner target is written beside the existing shadow depth map in the SAME pass; an R16UI scene-owner target is written beside existing color/normals. No extra geometry pass. Clean mode omits same-object shadowing for rocks/foliage, removing the rejected per-facet cut marks. This is a deliberate stylized rule and also removes genuine rock/foliage self-occlusion. Architecture filters near self-contact within12cm while retaining separated roof/eave self-shadows. Shadows cast ONTO OTHER OBJECTS keep their normal depth comparison. Grass still casts zero shadows and receives other-object shadows. Source gray facets visible with shadows OFF remain part of the mesh/material/toon lighting.

`--shadow-policy clean|legacy` preserves the original comparison. The owner-ID diagnostic separates same-object darkening from other-object shadows; it does not claim every same-object shadow was a mathematical error. At1024² shadows and640×360 the two integer targets require about2.44MiB logical storage, excluding driver overhead; LOW shadows reduce that. No large kernels, high-resolution map, full-screen blur or new volumetrics. Existing atmosphere is retained and defaults OFF.

## Build and review

```sh
cmake -S experiments/hearthgrove_emaceart -B build/hg-r2 -G "Visual Studio 17 2022" -A x64
cmake --build build/hg-r2 --config Release --parallel 2
ctest --test-dir build/hg-r2 -C Release --output-on-failure
cmake --install build/hg-r2 --config Release --prefix out/hg-r2
```

Use the complete PRIVATE Windows ZIP, extract into a NEW folder, then PLAY_GAME.cmd or PLAY_640.cmd. SHADOWS_ON.cmd uses the corrected rule; LEGACY_SHADOWS.cmd reproduces the cut-mark problem. HIGHLIGHTS_OFF/OUTLINES_OFF isolate the effects. ORIGINAL_KIT_RELIEF.cmd uses the same corrected look with uncompressed source relief. Earlier builds and source assets remain separate and unmerged. The public code-only CI artifact cannot run without private assets.

Existing movement/mouse, companion Idle/Fly/Attack, E interactions, battle/recruit, F5/F9 saves, F2 HUD, O shadows, V shafts, H haze, R resolution and G density remain. Added flags: `--vegetation-edges on|off`, `--highlights on|off`, `--outlines on|off`, `--shadow-policy clean|legacy`. Diagnostic flags: graded (albedo after grading), self-shadow (red legacy vs green clean occlusion), highlights (eligible sun-rim mask); numeric owner buffers are private QA output, not artwork. J/K controls are ordinary live renderer comparisons.

Private review contains real native480/640 captures, independent effect comparisons, color/ownership diagnostics, real first-person motion, full gameplay/save regressions and measured software timings. Physical Windows GPU/Intel UHD, Android and Win7 remain NOT TESTED until run on those devices. Mesa software is not hardware FPS proof. No visual approval and no merges.

BENCHMARK_EFFECTS.cmd runs a bounded8-case suite (480/640; shadow OFF, clean ON, legacy ON, highlights OFF). All use the same intact kit geometry and camera,30warmup+90measured frames. Adapter/driver, counters, per-frame timings and working set are recorded. This is application/API cadence, not displayed-FPS certification.
