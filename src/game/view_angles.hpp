#pragma once
#include "core.hpp"
#include <vector>
namespace game {
// LOOKDEV-005 flat camera: the view always looks from one of four diagonal corners.
// Quarter 0 looks along +x/-z (north-east), then each step turns 90 degrees.
float quarterYaw(int quarter);
int nearestQuarter(float yaw);
int wrapQuarter(int quarter);
// How badly a quarter shows a fight, lower is better: things standing between the
// camera and either fighter, and the two fighters stacked on one line of sight.
float fightViewCost(int quarter, p3::Vec you, p3::Vec foe,
                    const std::vector<p3::Box> &blockers, float pitch);
// Keeps the current quarter unless another one is clearly better.
int betterFightQuarter(int current, p3::Vec you, p3::Vec foe,
                       const std::vector<p3::Box> &blockers, float pitch);
// Picks when the fight camera may turn: only after the current view stayed worse
// for a while, never during a roll, and not again soon after a turn.
struct FightCamera {
  float worse = 0, cooldown = 0, hands = 0;
  int pick(int current, int best, bool rolling, float dt);
  void manualTurn() { hands = 6; worse = 0; }
};
} // namespace game
