# WILDSIGNAL — Visual Quality Reset after RENDER-001 Owner Review v0.1

**Owner review date:** 2026-10-21. **Authority:** explicit feedback in project chat, after reviewing nine on-device RENDER-001 screenshots and two uncapped screenshots.  
**Decision:** The owner **APPROVES the CONCEPTS/SETTINGS of Lantern Quay and Liltwood Forest**, but **REJECTS their ACTUAL GRAPHICAL LOOK** as acceptable game art. They were emphatic: “if youre asking me about the actual look or how it looks, then im definitely not happy.” This is a hard **art approval failure**, not a request to replace the locations or throw out the successful engineering proof.  
**No production renderer selected.** No full scene asset kit or gameplay-content pipeline should be locked until an improved representative visual passes owner review.  
**Source references:** `docs/design/VISUAL_BIBLE_v0.1.md`, `docs/design/HERO_SCENE_TARGETS_v0.1.md`, `docs/architecture/RENDER_BENCHMARK_PLAN_v0.1.md`, actual RENDER-001 screenshots in draft PR #6 / owner conversation.

## 1. Scope of approval vs rejection (prevent future misinterpretation)

| Item | Owner status / implication |
|---|---|
| Lantern Quay as harbor village, beacon, warm welcoming starting zone | **CONCEPT APPROVED**; layout/identity remains |
| Liltwood as enchanted forest, Singing Root, paths, mystery | **CONCEPT APPROVED**; layout/identity remains |
| Current flat programmatic façade/roof, repetitive buildings, roof billboards | **VISUAL LOOK REJECTED** |
| Current same-shape trees, layered silhouette cutout canopy, low detail foreground | **VISUAL LOOK REJECTED** |
| One-style generic tan NPC/creature placeholders (including 24-face stress view) | **NONFINAL PLACEHOLDERS; cannot be accepted as shipped characters** |
| Echo Hollow / entire original art direction | Earlier ENGINE-001 prototype subjectively positive; **DO NOT infer final environment art approved** |
| C++20/SDL3 grid raycaster technical proof | Functional prototype demonstrated, **PRODUCTION RENDERER NOT LOCKED** |
| First-person pixel aesthetic and original 100 user-drawn creature source | Confirmed high-level design direction, original character art sacred |
| Mouse/WASD improvement (#5) | **DEFERRED**; not the present visual gate |

The report's two uncapped user-device HUD screenshots show roughly **308.6 FPS at 320×180/1 actor (3.24ms)** and **137.2 FPS at 480×270/24 actors (7.29ms)**, and displayed process RSS near 65.6–68.0 MiB. **These are instantaneous samples, not sustained performance evidence**; nevertheless they suggest useful budget for more involved *art rendering* and indicate the old capped ~29 FPS HUD was not a decisive hardware ceiling. Do not automatically add expensive postprocessing without testing.

## 2. Concrete defects to remedy (evaluate art vs geometry separately)

**Quay**: facade-like houses with perfect repeated windows and horizontal bands; cardboard planar roofs/cutouts that billboard toward camera; limited building depth/side-wall variation; flat lighting with weak focal distinction; little human-scale harbor activity/dressing; painted/horizon layers may not hold spatially on traversal. Preserve open plaza, waterfront, timber/brass flavor and beacon identity.

**Liltwood**: repeated near-identical cylindrical trunk columns, geometric canopy blobs/flat cards, dull/uniform diffuse green color, sparse plant shape variety, unnatural object layering, absence of intricate roots/vines/ground breakup and dramatic magical light. Preserve clear forest paths, Singing Root recognizable silhouette, green mystical ecology, interactable clue visibility.

**Echo**: decorative camera-facing arch is not true open volume; roof/arch frame collapses from side. No final-architecture signoff; if accepted only as a façade trick, disclose viewing-angle restrictions.

**Placeholder actors**: repeated generic tan face sprites make dense scene look like a crowd of identical objects; stress setup is intentionally unreal and should never be marketed as fauna/NPC visual quality. Do not regenerate the owner's original creatures. Swap art in a privately reviewed next phase only with explicit public/export permission.

**Screen presentation**: 320×180 world remains legitimate low-resolution buffer, but a high pixel count or FPS is not visual quality. Need deliberate texture value hierarchy, hand-authored detail, material variation, focused highlights/shadows and irregular spatial silhouettes. Avoid random pixel noise as a substitute for art, and avoid smoothing filters that blur real pixel art.

## 3. Required NEXT milestone: VIS-001 Visual Fidelity Proof (no massive rewrite)

**First deliverable is an art-direction target and a *small* representative slice**, not creating all town buildings/forest textures and not implementing battle.

A. **Create owner-reviewable art targets for two identical views** (one Quay hero view; one Singing Root clearing view) with:
- high-quality hand-authored pixel texture/prop concept and foreground-midground-background readability;
- warm directional harbor accents (timber, plaster, colored roofs, brass/cloth, sea depth) with nuanced shadow/value;
- layered forest ecology (unique bark, roots, ferns/mushrooms/leaf silhouettes, dappled shading, mint signal as accent not everywhere);
- clear original user creature art integration *only after private export approval*, otherwise a clearly labeled tasteful temporary marker, not final wildlife;
- near/side/turning views to make sure geometry/tricks are honest.
Use original work, never copy the original third-party visual-reference screenshot's copyrighted pixel textures.

B. **Split proof tracks**:
- **VIS-ART:** one high-fidelity, owner-reviewable visual style guide/contact sheet and annotated art brief for each site (without asserting it is a game screenshot). Only make images when a future user instruction requests image generation or art production.
- **VIS-GEO:** one small controlled engine visual prototype that uses permitted new environment art at exactly the same cameras; list which intended forms are physically supported versus tricks that fail when turning. The existing grid raycaster is A; if the owner's demanded look requires real roof geometry, non-axis-aligned forms, curved/root arches, or pitch, commission a bounded RENDER-002 alternative comparison for those same views. **Do not automatically implement a second renderer without authorization.**
- **VIS-PERF:** compare at 320×180, 400×225, 480×270 on owner Windows i3/4GB, measure uncapped p95/p99 plus capped pacing, RSS and draw/present split. Carry measured Android Galaxy S10+ tests into BENCH-001B when compiled.

C. **Review priorities**, in order:
1. source-correct identity and distinctive forms, silhouettes/geometry at near and oblique views;
2. material quality and tonal/light contrast, not just palette hues;
3. prop diversity, believable scale, character/pixel fidelity;
4. low-resolution readability and environmental storytelling;
5. frame-time and memory proof on owner machine.

## 4. Art feasibility tradeoffs / stop conditions

- **No renderer ADR** merely because current screenshot has outdoors/sky and passes CTest. Owner explicitly rejected visual result.
- **No bulk expansion** to 48×32 finished maps, 6 commercial zones, full 18–24 original species, companion animation or backend while VIS-001 is unapproved; existing product ambition is not canceled.
- Do **not** promise a CPU grid raycaster can handle proper roofs/complex architecture with textures alone. If limited geometry is responsible, compare a small sector/portal geometry case instead of polishing wrong baseline.
- **No false benchmark failure:** 30fps sleep cap completion frame time can slightly exceed 33.3ms. Compare *actual renderer work* separately from wall-clock pacing.
- No commercial character replacements via AI or random stock sprites; user's original 100 creature IDs/source must remain unchanged and not automatically published.
- No silent relaxation of quality by calling placeholder art “intentionally retro.” Retro game worlds can be polished, coherent and high-fidelity in their own pixel style.

## 5. Acceptance for leaving visual-quality gate

The OWNER reviews stills **and a short moving/turning capture** from *both* Quay and Liltwood, on the same fixture/scene version with known asset attribution.
- **Accept concept** is already confirmed; do not re-ask.
- **Owner separately accepts visual look** for both locations as a viable art-production *baseline*, with any intentional cutout compromises enumerated.
- Visually unique Quay buildings/beacon and forest roots/tree/foliage look cohesive while turning.
- Placeholder actors are disclosed; source creature art not published without permission.
- Renderer and presentation method feasible by actual low-end target tests. Quality and measured mobile/old Windows requirements remain separately gated.
- Only then can production scene kits and renderer selection proceed.

**Decision log:** Add D-013 to both Markdown/JSON decision registers; owner concept APPROVED, actual RENDER-001 visual aesthetic REJECTED; future VIS-001 art-quality signoff BLOCKED. This supersedes any previous assistant characterization of RENDER-001 “visually approved.” 
