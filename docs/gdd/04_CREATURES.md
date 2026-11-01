# Chapter 4 — Creatures

**Status:** LOCKED 2026-11-02 by the owner (R1–R6 accepted as recommended). Owner answers behind it: no evolutions for 1.0; Claude decides the type chart, picks the other 30 Signari and proposes names; WS-001 to WS-020 are approved models; Claude builds the remaining 3D models.
**Built on:** chapters 1–3 (locked). Sources: `CREATURE_CATALOG_v0.1.json` (100 designs), the turnaround sheets and models in `wildsignal-art`, and the reference models in `assets/samples` (measured for budgets only, never copied).

## 4.1 Ground rules

- **50 Signari in 1.0**, one of each species (chapter 2). **No evolutions in 1.0**: each species is complete as it is. Families can come later with new designs.
- Designs come from the owner's 100-design catalogue (AI-assisted concepts the owner owns). Every Signari keeps its catalogue ID (`WS-001` … `WS-100`) forever; names can change, IDs don't.
- **WS-001 to WS-020 are approved** as models. Small tweaks to them happen **after** all 50 are published (owner's order).

## 4.2 Eight types and the matchup chart

All eight types from the catalogue stay. Each type is **strong against two** and **weak to two**, so the chart is balanced and learnable. Strong = 125% damage, resisted = 80% (as in today's fight code).

| Type | Feel | Strong against | Weak to |
|---|---|---|---|
| **Flora** | plants, growth | Tide, Stone | Ember, Gale |
| **Tide** | water, sea | Ember, Stone | Flora, Wild |
| **Ember** | fire, heat | Flora, Gloom | Tide, Stone |
| **Stone** | rock, earth, metal | Ember, Gale | Flora, Tide |
| **Gale** | wind, sky, cold | Flora, Wild | Stone, Lumen |
| **Lumen** | light, signal, spirit | Gloom, Gale | Gloom, Wild |
| **Gloom** | shadow, fear, mystery | Lumen, Wild | Ember, Lumen |
| **Wild** | beasts, raw strength | Tide, Lumen | Gale, Gloom |

Lumen and Gloom are strong against each other: light and shadow hit hard both ways. Reading the chart in plain words: *plants drink water and split rock; water douses fire and wears stone; fire burns plants and drives off shadow; rock smothers fire and breaks wind; wind scatters leaves and tires beasts; light outshines shadow and cuts through storms; shadow frightens beasts and swallows light; beasts brave water and ignore tricks of light.*

## 4.3 The roster of 50

**Type balance:** Flora 6, Wild 6, Stone 6, Gale 6, Gloom 6, Lumen 6, Tide 7, Ember 7.
**Roles** (for the kit, 4.5): *Striker* (damage), *Guardian* (tough, protects), *Trickster* (control: slow, push, blind), *Support* (heal, shield, speed).
**Names** are proposals except the nine already named in earlier documents (marked ✓). The owner can rename any of them.

### Starters (given in region 1)

| ID | Name | Type | Look | Role |
|---|---|---|---|---|
| WS-004 | Glintle ✓ | Flora | green-gold forest sentinel | Guardian |
| WS-008 | Driftfin ✓ | Tide | blue bubbly amphibian | Support |
| WS-050 | Brumlet ✓ | Ember | red-orange ember creature | Striker |

### Region 1 — Hearthgrove Vale

| ID | Name | Type | Look | Role |
|---|---|---|---|---|
| WS-002 | Berrikin | Flora | red fruit creature | Support |
| WS-005 | Tumbletail ✓ | Wild | brown squirrel, bushy tail | Striker |
| WS-007 | Hornkid | Wild | pink-tan horned quadruped | Striker |
| WS-009 | Fluffet | Wild | cream fluffy small creature | Support |
| WS-010 | Petalwing | Gale | green-pink winged insect | Trickster |
| WS-013 | Lopkin | Wild | cream-brown long-eared animal | Trickster |
| WS-021 | Puddlefin ✓ | Tide | round green fish | Support |
| WS-036 | Puffpine | Wild | pink puff hedgehog | Guardian |
| WS-052 | Logwarden | Stone | brown standing log giant | **Region guardian** |

### Region 2 — Liltwood Forest and Echo Hollow

| ID | Name | Type | Look | Role |
|---|---|---|---|---|
| WS-001 | Signalit ✓ | Lumen | glowing dark bug | Trickster |
| WS-003 | Hoodsprout | Flora | green hooded plant spirit | Trickster |
| WS-012 | Mossmelt ✓ | Flora | green slime, pink core | Support |
| WS-014 | Capling | Flora | green-capped tiny wanderer | Trickster |
| WS-015 | Burrbun | Flora | green spiny round critter | Guardian |
| WS-019 | Jarrow | Stone | cream jar-headed creature | Support |
| WS-022 | Hootle | Gale | brown small owl | Trickster |
| WS-046 | Shieldlet | Lumen | blue-white shield creature | Guardian |
| WS-063 | Coilmouse | Lumen | cream spiral mouse | Trickster |
| WS-065 | Slinkit | Wild | grey-brown stoat | Striker |
| WS-086 | Veilmask | Lumen | pale floating masked figure | **Region guardian** |

### Region 3 — Tideglass Coast

| ID | Name | Type | Look | Role |
|---|---|---|---|---|
| WS-020 | Olivane | Gale | olive-winged insect | Striker |
| WS-032 | Navigill | Tide | navy aquatic animal | Striker |
| WS-037 | Rippaw | Tide | blue beast, orange ears | Striker |
| WS-053 | Puffglow | Lumen | white-blue puff creature | Support |
| WS-081 | Pincerra | Tide | red-blue crab | Guardian |
| WS-062 | Sharkle | Tide | grey-blue shark-like animal | **Region guardian** |

### Region 4 — Ashbell Works

| ID | Name | Type | Look | Role |
|---|---|---|---|---|
| WS-006 | Spiralug | Stone | orange burrowing snail | Guardian |
| WS-011 | Cragclaw | Ember | red horned crab | Striker |
| WS-018 | Shagstone | Stone | brown upright shaggy creature | Guardian |
| WS-024 | Craglet ✓ | Stone | red rocky creature | Striker |
| WS-038 | Blazebloom | Ember | red flower of flame | Trickster |
| WS-042 | Gustbug | Gale | blue-yellow beetle | Striker |
| WS-058 | Chuffle | Ember | chimney-train critter | Guardian |
| WS-076 | Bullbrand | Ember | horned bull head | **Region guardian** |

### Region 5 — Gloamfen Marsh

| ID | Name | Type | Look | Role |
|---|---|---|---|---|
| WS-016 | Wispel | Gloom | white wispy spirit | Trickster |
| WS-017 | Gazebloom | Gloom | purple stalk-eyed flower | Support |
| WS-030 | Skullop | Gloom | cream skull-faced critter | Striker |
| WS-047 | Jellum | Gloom | pink round jelly | Support |
| WS-067 | Hushflame ✓ | Lumen | blue ghostly flame | Trickster |
| WS-085 | Mirejaw | Tide | green flat crocodile | Striker |
| WS-068 | Masquel | Gloom | magenta masked flower | **Region guardian** |

### Region 6 — Emberglass Caldera and the Deep Station

| ID | Name | Type | Look | Role |
|---|---|---|---|---|
| WS-031 | Spinehide | Stone | blue armoured spiny beast | Guardian |
| WS-059 | Trumpuff | Gale | white fluffy elephant | Guardian |
| WS-060 | Emberhop | Ember | orange long-tailed rabbit | Striker |
| WS-087 | Cindershell | Ember | yellow round shell | Guardian |
| WS-095 | Gustram | Gale | blue-white ram | Striker |
| WS-092 | Duskwyrm | Gloom | navy small dragon | **Sol's partner — final guardian** |

**Totals:** 3 starters + 9 + 11 + 6 + 8 + 7 + 6 = **50**. Region 3 and 6 are smaller on purpose: the coast also hosts Signari from earlier regions along its meadows, and the Caldera mixes in late-level versions of earlier species.
**Guardians** are fought bound (chapter 3), freed with Tune In, and can be recruited after the story through a guardian trial.
**Kept for later** (not in 1.0): the other 50 catalogue designs.

## 4.4 Habitats and behaviour

Each species gets: a home region and spots (meadow, canopy, shore, ruins, night only…), one behaviour from chapter 2 (skittish, curious, territorial, roaming), and its **field ability** if it has one (chapter 2, C1): Tide wades, Stone moves boulders, Lumen lights caves, Gale lifts you over short gaps, Flora grows vine bridges, Ember melts ice or clears thorns, Gloom reveals hidden paths at night, Wild digs up buried items. Exact spots are set when each region is built.

## 4.5 Kits

Every species: **basic attack** (from its type), **3 skills**, **1 ultimate**, innate dodge (chapter 2). A kit is built from its role:

| Role | Skill 1 | Skill 2 | Skill 3 | Ultimate |
|---|---|---|---|---|
| Striker | Burst damage | Gap-closer or ranged hit | Damage over time or combo finisher | Big area or multi-hit blast |
| Guardian | Shield self | Taunt or knock back | Heal over time | Team-wide barrier or slam |
| Trickster | Slow or root | Teleport or decoy | Blind or confuse aim | Field effect (storm, darkness, vines) |
| Support | Heal | Speed or shield for the next Signari | Cleanse a status | Team heal plus buff |

Each species then gets its own names, shapes and timing so no two feel the same. Kits are written in the fight data (`fight.json`), one entry per species, and reviewed with the owner in batches.

## 4.6 Growth (from chapter 2)

- Levels 1–50. Skills 1 and 2 from level 1, skill 3 at level 5, the ultimate at level 10 (*proposed*).
- Each skill ranks up twice (*proposed* levels 15 and 30): more damage, shorter cooldown or a bonus effect.
- Stats grow with level from the species' base stats (might, guard, pulse, ward, swift, health), as in today's fight code.

## 4.7 Model and animation standard

Measured from the reference models (Pokémon samples, 4,000–6,500 triangles, 1–4 materials) and our approved Signari:

| Item | Standard |
|---|---|
| Triangles | **Up to 6,000** at full detail, plus two lower-detail versions (~40% and ~12%), like WS-010 |
| Materials and textures | 1–3 materials, textures up to 512 × 512 |
| Skeleton | **Up to 24 bones** (the engine's current limit; raising it is a chapter 7 question) |
| Animations (12) | idle, walk, run, basic attack, skill 1, skill 2, skill 3, ultimate, dodge, hurt, faint, joined (a happy reaction after Tune In) |
| Look | Same style as the approved WS-001 to WS-020: soft, rounded, glossy, big readable eyes |
| Scale | Set per species from a size class (tiny, small, medium, large, guardian) so they read correctly next to the Wayfinder |

**Body plans:** to keep 50 rigs and 600 animations achievable, every Signari uses one of six shared skeleton templates: **quadruped, biped, blob, flyer, swimmer, serpent**. Animations are made per template, then adjusted per species.

## 4.8 Production plan

Claude builds the models, as the owner asked, using the existing pipeline in `wildsignal-art/WILDSIGNAL_CREATURE_MODELS/_pipeline` (Blender, scripted, one script per creature):

1. **Models WS-021 … WS-095 (the 30 new ones)** from their turnaround sheets, in batches of 5, each with a preview sheet (front, side, back, three-quarter, in-game size). Owner reviews each batch.
2. **Rigs and animations for all 50**: the six body-plan skeletons first, then the 12 animations per template, then per-species fitting. Owner reviews in batches.
3. **Detail levels** and engine import checks for all 50.
4. **Tweaks to WS-001 … WS-020**, once all 50 are published.

Everything stays in the private art repository. The reference models in `assets/samples` are for measurements and proportion study only; nothing from them is copied or shipped.

## 4.9 Owner choices

| # | Question | Decision (owner, 2026-11-02) |
|---|---|---|
| R1 | The eight-type chart (4.2)? | As drafted |
| R2 | The 30 picks and their regions (4.3)? | As drafted |
| R3 | The proposed names (4.3)? | Accept for now; rename any later |
| R4 | Guardians: Logwarden, Veilmask, Sharkle, Bullbrand, Masquel, Duskwyrm as Sol's partner? | Yes |
| R5 | Model and animation standard, including six shared skeletons (4.7)? | Yes |
| R6 | Production order: 30 models first, then rigs and animations for all 50 (4.8)? | Yes |

## Change log

- 2026-11-02 — First draft from the owner's answers.
- 2026-11-02 — Owner accepted R1–R6 as recommended. Locked.
