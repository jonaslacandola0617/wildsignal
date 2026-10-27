# CREATUREVIEW-001 — Native WILDSIGNAL gallery for all ten redesigned static models

STATUS: SCAFFOLD ONLY. There are no screenshots or binaries from this branch yet.

Read [Engine Issue #51](https://github.com/jonaslacandola0617/wildsignal/issues/51) and [Private Art Issue #41](https://github.com/jonaslacandola0617/wildsignal-art/issues/41).

Branch is stacked on GAMELOOP001 PR #48. Reuse the actual C++20/SDL3/OpenGL3.3 HOLLAND toon renderer and private glade; don't modify gameplay battle, save, current Petalyn rig or unrelated code. Artwork and adapted GLBs stay only in private wildsignal-art.

Adapter prereq: C++ assets.cpp rejects GLB images embedded in bufferViews, while 005R GLBs embed their PNGs. Private art adapter converts to external PNG URI without changing mesh geometry/UV/normals. Run Model::load against every converted real GLB before using it.

Show 10 species individually and in scene; actual native 480/640 frames and HUD labels, first-person/orbit/reset view, optional source-vs-toon light comparison, owner-only portable Windows ZIP, no Blender images labeled real engine. All ten models are STATIC and production scale is unapproved. Source PRs stay draft/unmerged.