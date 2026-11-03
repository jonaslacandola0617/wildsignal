#include "view_angles.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
void need(bool v, const char *m) {
  if (!v)
    throw std::runtime_error(m);
}
int main() {
  try {
    for (int q = -5; q < 9; q++) {
      need(game::nearestQuarter(game::quarterYaw(q)) == game::wrapQuarter(q), "quarter round trip");
      need(game::nearestQuarter(game::quarterYaw(q) + .6f) == game::wrapQuarter(q), "nearest quarter");
    }
    need(std::abs(game::quarterYaw(0) - p3::pi * .25f) < 1e-6f, "first quarter is diagonal");
    const float pitch = -.70f;
    p3::Vec you{0, 0, 0}, foe{3, 0, 0};
    // Open ground: no turn.
    need(game::betterFightQuarter(0, you, foe, {}, pitch) == 0, "turned on open ground");
    // A wall right behind the fight, between it and the quarter-0 camera.
    int q = 0;
    float y = game::quarterYaw(q);
    p3::Vec back{-std::sin(y), 0, std::cos(y)};
    p3::Vec c = (you + foe) * .5f + back * 3.5f;
    std::vector<p3::Box> wall{{{c.x - 1.5f, 0, c.z - 1.5f}, {c.x + 1.5f, 6, c.z + 1.5f}}};
    need(game::fightViewCost(0, you, foe, wall, pitch) > 1.5f, "wall not seen");
    int better = game::betterFightQuarter(0, you, foe, wall, pitch);
    need(better != 0, "did not leave the blocked view");
    need(game::fightViewCost(better, you, foe, wall, pitch) < game::fightViewCost(0, you, foe, wall, pitch), "worse pick");
    // The timer waits a second, holds still during a roll and after a manual turn.
    game::FightCamera f;
    need(f.pick(0, 1, false, .5f) == 0, "turned too soon");
    need(f.pick(0, 1, true, .6f) == 0, "turned during a roll");
    need(f.pick(0, 1, false, .1f) == 1, "never turned");
    need(f.pick(1, 2, false, 1.5f) == 1, "turned during cooldown");
    game::FightCamera g;
    g.manualTurn();
    need(g.pick(0, 1, false, 3) == 0, "overrode the player's turn");
    std::cout << "view angles ok\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
