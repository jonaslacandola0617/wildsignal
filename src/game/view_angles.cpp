#include "view_angles.hpp"
#include <algorithm>
#include <array>
#include <cmath>
namespace game {
int wrapQuarter(int q) { return ((q % 4) + 4) % 4; }
float quarterYaw(int q) { return std::remainder(p3::pi * .25f + p3::pi * .5f * float(wrapQuarter(q)), 2 * p3::pi); }
int nearestQuarter(float yaw) {
  return wrapQuarter(int(std::lround((yaw - p3::pi * .25f) / (p3::pi * .5f))));
}
namespace {
// True when the segment from a to b passes through the box.
bool crosses(const p3::Box &box, p3::Vec a, p3::Vec b) {
  float enter = 0, leave = 1;
  for (auto [from, d, lo, hi] : {std::array{a.x, b.x - a.x, box.lo.x, box.hi.x},
                                 std::array{a.y, b.y - a.y, box.lo.y, box.hi.y},
                                 std::array{a.z, b.z - a.z, box.lo.z, box.hi.z}}) {
    if (std::abs(d) < 1e-6f) {
      if (from < lo || from > hi)
        return false;
      continue;
    }
    float t0 = (lo - from) / d, t1 = (hi - from) / d;
    enter = std::max(enter, std::min(t0, t1));
    leave = std::min(leave, std::max(t0, t1));
  }
  return enter <= leave;
}
} // namespace
float fightViewCost(int quarter, p3::Vec you, p3::Vec foe,
                    const std::vector<p3::Box> &blockers, float pitch) {
  float y = quarterYaw(quarter), c = std::cos(pitch);
  // Toward the camera: back along the view direction.
  p3::Vec back{-std::sin(y) * c, -std::sin(pitch), std::cos(y) * c};
  float cost = 0;
  for (auto at : {you, foe, (you + foe) * .5f}) {
    p3::Vec from = at + p3::Vec{0, .7f, 0}, to = from + back * 16.f;
    for (const auto &b : blockers)
      if (crosses(b, from, to))
        cost += 1;
  }
  // Side by side on screen reads best; one behind the other hides the far one.
  p3::Vec gap = foe - you;
  float apart = std::hypot(gap.x, gap.z);
  if (apart > .3f) {
    float across = std::abs(gap.x * std::cos(y) + gap.z * std::sin(y)) / apart;
    cost += (1 - across) * .6f;
  }
  return cost;
}
int betterFightQuarter(int current, p3::Vec you, p3::Vec foe,
                       const std::vector<p3::Box> &blockers, float pitch) {
  current = wrapQuarter(current);
  float here = fightViewCost(current, you, foe, blockers, pitch);
  int best = current;
  float bestCost = here - .5f;
  // The nearer turns first, so a tie never swings the camera all the way round.
  for (int step : {1, -1, 2}) {
    int q = wrapQuarter(current + step);
    float cost = fightViewCost(q, you, foe, blockers, pitch);
    if (cost < bestCost) {
      best = q;
      bestCost = cost;
    }
  }
  return best;
}
int FightCamera::pick(int current, int best, bool rolling, float dt) {
  cooldown = std::max(0.f, cooldown - dt);
  hands = std::max(0.f, hands - dt);
  worse = best != current ? worse + dt : 0;
  if (best == current || rolling || cooldown > 0 || hands > 0 || worse < 1)
    return current;
  worse = 0;
  cooldown = 4;
  return best;
}
} // namespace game
