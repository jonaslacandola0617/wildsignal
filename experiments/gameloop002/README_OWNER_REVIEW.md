# GAMELOOP-002 — OWNER PLAYTEST BUGS AND GRASS COVERAGE

**Scaffold only. No fixes implemented at this commit.**

Read [engine Issue #49](https://github.com/jonaslacandola0617/wildsignal/issues/49) and [paired private asset/style Issue #39](https://github.com/jonaslacandola0617/wildsignal-art/issues/39).

Owner confirms exploration, battle, defense, recruitment, interaction and save/load work on physical Windows. Keep these intact. **Blockers:** follower target based on mouse yaw (main.cpp) despite stationary player; greedy avoidance tangent and no progress hysteresis around rocks (game.cpp); static terrain+0.55 height for flying models near solid footprints; meadow still has visual gaps despite a combined painted+mesh coverage metric >75%; MegaKit house/trees not matching authored pixel-art style (separate private study).

New work must include **real** yaw-only and obstacle/catch-up replay tests, actual native 480/640 gameplay after fixes, vegetation medium/high before/after comparison and no regressions. Preserve old branch and source art, owner-only models, no auto-merge. Physical Intel UHD 4800 benchmark frames exist in owner runs(3).zip, but no code fix is yet made.
