# Current state: previous god rays, latest grass

See [README_SHAPED_RAYS.md](README_SHAPED_RAYS.md) for current defaults and controls. The new package uses one PLAY_GAME.cmd, previous shadow-based god rays, plain-color grass, grass outlines OFF and grass highlights ON. Prior R3 notes below are historical; current dense placements and full effects follow the latest review manifest.

# Hearthgrove — one playable review

Extract the complete private Windows ZIP into a new folder and run **PLAY_GAME.cmd**. There is one launcher and one finished default look: BALANCED cottage/materials, much brighter saturated sun-facing edge highlights, selective outlines, corrected object shadows, original kit terrain at85% relief and the existing sparse EmaceArt grass. No comparison commands or extra runnable variants are installed.

WASD and mouse explore; E interacts;1 attack /2 defend /3 befriend in combat; F5 save /F9 load. R switches480/640 in the SAME game. J/K/O remain optional live highlight/outline/shadow controls; no need to exit or switch launchers. Haze/shafts remain OFF by default. All existing gameplay, source textures, meshes, terrain, brown preservation, sparse grass placements and shadow correction are unchanged from R2.

Revision3 increases edge radiance and keeps the material hue saturated rather than washing it toward white. The existing one-pixel sun-facing depth/normal masks remain; neither outline width nor full-scene brightness is raised. Leaves stay green-gold, wood stays warm brown/gold, stone gets a pale warm edge. Only those edge pixels change. No new artwork, bloom, textures, geometry or extra render passes. Revision1/2 and source/baseline comparisons remain available through the existing CLI for engineering use; legacy launchers are preserved in source but omitted from the installed review.

The clean shadow rule still suppresses rock/foliage self-shadowing (including genuine self-occlusion) while retaining other-object shadows and larger architectural roof/eave shadows. This intentional stylized compromise is unchanged. Source model facets remain.

Build Windows x64 with CMake/Visual Studio2022:

```sh
cmake -S experiments/hearthgrove_emaceart -B build/hg -G "Visual Studio 17 2022" -A x64
cmake --build build/hg --config Release --parallel 2
ctest --test-dir build/hg -C Release --output-on-failure
cmake --install build/hg --config Release --prefix out/hg
```

Public CI contains code only; the authorized private package supplies runtime assets. Owner visual review is required. Mesa software captures/timings are not physical Intel UHD performance or displayed FPS. Windows GPU launch, Intel UHD, Android and Win7 remain unverified until tested on those devices. No merges.
