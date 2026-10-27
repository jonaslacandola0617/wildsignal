#pragma once
#include "game.hpp"
#include <deque>
namespace game {
// Small scene-local visibility graph, not a general navmesh. No camera input.
bool clearRoute(p3::Vec, p3::Vec, const std::vector<Solid> &,
                float radius = .56f);
const char *followName(FollowState);
struct Trail {
  std::deque<p3::Vec> points;
  p3::Vec last{};
  void reset(p3::Vec);
  bool record(p3::Vec);
  p3::Vec behind(float distance) const;
};
struct Followers {
  Trail trail;
  void reset(Game &, bool spawn = true);
  void update(Game &, float dt);
};
} // namespace game
