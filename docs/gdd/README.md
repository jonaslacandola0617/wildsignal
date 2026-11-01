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
| 1 | [Vision and pillars](01_VISION_AND_PILLARS.md) | What the game is, who it's for, what makes it different, what we never do, release scope | DRAFT — awaiting owner approval |
| 2 | Core gameplay | Exploring, fighting, recruiting, team building, growth, progression | Not started |
| 3 | World and story | Regions and their order, the main mystery, characters, quests | Not started |
| 4 | Creatures | The 50-Signari roster, types and matchups, moves, growth, habitats | Not started |
| 5 | UI and controls | Every screen and the HUD; keyboard, controller and touch | Not started |
| 6 | Art and assets | Style rules and the full asset list with sources and licences | Not started |
| 7 | Technical plan | Engine, region and chunk streaming, saves, performance budgets | Not started |
| 8 | Production plan | Milestones from the first playable region to release | Not started |

Decisions that change earlier records are also logged in `docs/planning/DECISION_REGISTER_v0.1.md`.
