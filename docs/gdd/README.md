# WILDSIGNAL — Game Design Document

The single source of truth for what Wildsignal is and how it plays. Every agent and contributor reads this before design or gameplay work. When a chapter is **LOCKED**, it outranks every older design document in `docs/design` and `docs/planning`; those stay as history.

## How chapters are made

1. Claude drafts a chapter with options and a recommendation.
2. The owner chooses; the chapter is updated to match.
3. The owner approves it and it becomes **LOCKED** (with the date).
4. Changing a locked chapter needs the owner's explicit say-so and a line in the chapter's change log.

Anything not yet decided is marked **OPEN** in its chapter, so nobody guesses. Coding of new systems waits until chapters 1 and 2 are locked.

## Chapters

| # | Chapter | Covers | Status |
|---|---|---|---|
| 1 | [Vision and pillars](01_VISION_AND_PILLARS.md) | What the game is, who it's for, what makes it different, what we never do, release scope | **LOCKED 2026-11-02** |
| 2 | [Core gameplay](02_CORE_GAMEPLAY.md) | Exploring, fighting, recruiting, team building, growth, progression, homestead | **LOCKED 2026-11-02** |
| 3 | [World and story](03_WORLD_AND_STORY.md) | Regions and their order, the main mystery, characters, quests | **LOCKED 2026-11-02** |
| 4 | [Creatures](04_CREATURES.md) | The 50-Signari roster, types and matchups, kits, growth, habitats, model standard and production | **LOCKED 2026-11-02** |
| 5 | [UI and controls](05_UI_AND_CONTROLS.md) | Every screen and the HUD; keyboard, controller and touch | **PARTLY LOCKED 2026-11-02** (visual style OPEN) |
| 6 | [Art, audio and assets](06_ART_AUDIO_AND_ASSETS.md) | Style rules, the full asset list with sources and licences, audio | **LOCKED 2026-11-02** (style guide after LOOKDEV-001) |
| 7 | [Technical plan](07_TECHNICAL_PLAN.md) | Engine, region and chunk streaming, saves, performance budgets | DRAFT |
| 8 | [Production plan](08_PRODUCTION_PLAN.md) | Milestones from the first playable region to release | DRAFT |

Decisions that change earlier records are also logged in `docs/planning/DECISION_REGISTER_v0.1.md`.
