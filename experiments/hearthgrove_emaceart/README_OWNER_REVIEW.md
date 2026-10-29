# Hearthgrove — sparse original EmaceArt grass restored

Extract the entire private ZIP into a NEW folder and run PLAY_GAME.cmd. Default: original EmaceArt grass, sparse LOW density. PLAY_640.cmd uses640×360. This restores the selected source grass without the owner-grass replacement or a density boost.

The current lighting, shadows and sun shafts remain. B old glow/streaks; V shafts; O shadows; H haze. F source/toon; R480/640. LOW_SPEC.cmd keeps sparse grass and disables shafts. EMACEART_CONTROL.cmd uses the same sparse EmaceArt grass with the old glow; SHAFTS_OFF.cmd isolates volumetric light. OWNER_GRASS_GLOW.cmd is explicitly a preserved rejected-grass comparison, not the default.

Movement, following, interactions, combat, recruitment and save/load are unchanged. Existing original assets, prior builds and source history are preserved; nothing is merged. No new artwork, texture or geometry is generated.

The sparse preset selects a stable quarter of the existing candidate roots, with the existing12mvisibility limit and conservative frustum/projected-size culling. Original models, palette and scene placement data are unmodified. It intentionally leaves more ground visible; this is not a lush-meadow coverage target.

Windows build instructions remain in README.md. All local render captures use Mesa software. Physical Intel UHD/Windows GPU performance, Android and Win7compatibility remain NOT TESTED. No new performance claim accompanies this default-only revert.

## Thicker edges review

The default toon view now uses two-native-pixel outlines and sun-facing convex edge highlights, previously one pixel. Source textures, geometry, lighting and grass suppression are unchanged. This is a bounded second depth/normal sampling ring in the existing post pass; it adds no framebuffer or texture allocation. Foliage keeps its lighter ink and only outer-mass highlights. Ground and individual grass blades remain excluded.

Use `THIN_EDGES_COMPARE.cmd` for the previous one-pixel width at otherwise identical sparse EmaceArt settings. Individual controls: `--outline-width 1|2 --highlight-width 1|2`. Width is measured at the internal render resolution before nearest-neighbor presentation. Source-material mode still disables both effects. The two-pixel reach can emphasize small overlapping leaves at distance; it is a review setting, not final style approval.
