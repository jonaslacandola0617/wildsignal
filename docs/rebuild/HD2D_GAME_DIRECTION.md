# Wildsignal: a fresh game foundation

Owner direction, 2026-11-11. Read this alongside the locked chapters in `docs/gdd/`.

The owner wants a fresh Wildsignal game built from its GDD, with the HD-2D/2.5D games as **game references**, not merely a rendering filter. Preserve historical prototypes and approved private assets. This is not authorization to copy another game's world, creatures, dialogue or UI. This direction supersedes the earlier dream-look/faux-pixel rendering choices and the instruction to build all environment assets as new 3D models. Use genuine Pixel Crawler environment art, approved 3D houses and solid terrain; the temporary humans use Pixel Crawler directional sprites. Signari retain their original owner-owned designs.

## Source and limits of this study

Marc Mills, *A look at HD-2D and 2.5D Video Games*, Medium, December 11, 2022:
https://medium.com/@dumarcm/a-look-at-hd-2d-and-2-5d-video-games-3f67a7897de0

The owner supplied the article text after this environment denied Medium access. The list and article descriptions below come from that supplied text; the design observations are synthesis from established game characteristics, **not a claim of fresh playthroughs or exhaustive reviews**. An article listing these games is not an official or exhaustive genre definition. Its 2022 descriptions of then-upcoming games are historical, not evidence of their current release status or final features. In particular, not every game listed is built with the same rendering method (Tactics Ogre: Reborn and SIGNALIS should not be treated as Octopath rendering specifications).

## What Wildsignal should learn

| Reference | Relevant lesson | Concrete Wildsignal application |
|---|---|---|
| Octopath Traveler | A place has character through people, local stories, routes and distinct regional presentation | Towns with identifiable roles and local problems; quiet discoveries between encounters; a recognisable mood for each island |
| Triangle Strategy | Readable elevation, deliberate spatial staging and character motivations that make conflict matter | Clear terraces, stairs, movement routes and attack telegraphs; Mara, Juno and Sol carry the emotional stakes |
| Live A Live | Shared mechanics can support very different narrative situations | Vary objectives: observation, a frightened Signari, a local errand, a rival, a station mystery and freeing a guardian; avoid six identical region checklists |
| Tactics Ogre: Reborn | Units, terrain and combat states must remain legible when choices matter | Separate friend/foe/Wayfinder silhouettes and clear threats; affinity information uses symbols and text, not colour alone |
| Dragon Quest III HD-2D Remake | Travel, party identity, settlements and discovery give an adventure its rhythm | A team the player recognises and builds; useful rest stops; landmarks and optional routes that reward curiosity |
| SacriFire | Music, story and combat presentation should reinforce one another | Design encounters and emotional transitions with audio cues; final music still needs the owner's licensed tracks |
| SIGNALIS | Environmental detail, repeated motifs and restraint can carry a mystery | Station echoes, broken bells and changes in the compass signal tell the Hush story; retain Wildsignal's warm, family-friendly tone |
| REPLACED | Purposeful composition and motion sell physical presence | Fixed-camera staging, grounded sprites, clear entrances/exits and readable movement; retain movement across a 3D ground plane |
| Eiyuden Chronicle: Rising | Adventure feels consequential when returning to town changes something | Forage, earn rewards, restore local amenities and develop the homestead; people acknowledge progress |
| Dave the Diver | Outward adventure and a familiar home activity can reinforce each other | Explore/fight/recruit, then return to heal, plant, craft and prepare; reserve Signari have a reason to remain at home |
| ANNO: Mutationem | Sprite characters can inhabit convincing 3D locations without flattening exploration | Human sprites use ground anchors, occlusion, lighting and appropriate facing; 3D houses and terrain provide volume |

These are selective influences, not eleven competing game templates. The article does not override the GDD's real-time combat, deterministic Tune In, original roster, six-region story or offline single-player release. No switch to turn-based grids, side-scrolling movement, horror, eight protagonists, a restaurant business or branching political campaigns is implied.

## The experience to build

A warm arrival at Lantern Quay establishes people and a place worth returning to. Mara gives the journey context; Fen gives the compass and the player chooses a companion. A short, readable path introduces a visible wild Signari. The player learns to command, dodge and listen, then returns with something that changes their team, field book or home. A station clue adds mystery before the next outing. Each new region develops that loop with its own landscape, inhabitants and narrative situation.

Depth comes from traversable ground, elevation, silhouettes, overlap, perspective, lighting and sound. Strong bloom, blur and a low-resolution framebuffer are not substitutes for level design or art direction. Use a single lower oblique view with screen-relative movement. Frame encounters so the Wayfinder, active Signari, foe and their telegraphs remain visible. Foreground occluders should reveal the player, not turn every nearby plant transparent.

## Production order and acceptance

Follow GDD chapter 8: foundations → first complete Hearthgrove slice → owner playtest → regions 2–6 → platform/polish/release. Do not populate a release with fabricated Signari substitutes or label technically passing art as visually approved.

The new executable is `wildsignal_game`. Its journey/controller state is independent of the old pilot's main loop and old map. It shares the tested C++ battle/content/story/save libraries and the HD-2D scene renderer. This is a fresh application foundation, not a new engine written without any reusable infrastructure.

First implementation gate: title/new/continue with three save slots; the approved arrival dialogue and starter choice; native directional human sprites; original Signari models; real-time encounter and Tune In; field-book/bag views; save/reopen. Validate those paths with headless rules tests and actual SDL/OpenGL input replays. The complete six-region game is not finished by this gate.

The current scene remains a small terrace/meadow scaffold. Outstanding: a purpose-built Lantern Quay/Hearthgrove map; full in-world attack markers and final HUD; rigged Signari animation; all region 1 art and encounters; shop and homestead presentation; all remaining regions; audio/music production; controller/touch/rebinding/accessibility; complete end-to-end playtests; Windows packaging and physical-device profiling. GDD inventory and historical prototype tests do not establish any of these as completed in the rebuild.
