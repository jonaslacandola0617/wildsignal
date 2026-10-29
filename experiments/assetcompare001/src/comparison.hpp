#pragma once
#include "game.hpp"
#include <algorithm>
namespace compare {
constexpr float spacing = 11.9f, eye = 1.65f;
inline p3::Camera camera(int bay, int angle = 0) {
  p3::Camera c;
  float x = (bay - 1) * spacing;
  c.position = {x, 0, 12};
  if (angle == 1)
    c.position = {x + 5.5f, 0, 10.665365f};
  if (angle == 2)
    c.position = {x + 5.5f, 0, 5.1f};
  if (angle == 3)
    c.position = {x, 0, -12};
  c.position.y = game::terrain(c.position.x, c.position.z) + eye;
  c.aim({x, game::terrain(x, 0) + 2.5f, 0});
  return c;
}
inline p3::Camera overview() {
  p3::Camera c;
  c.position = {15, 13, 17};
  c.aim({0, 2, 0});
  return c;
}
inline p3::Camera motion(int frame) {
  float u = std::clamp(frame / 150.f, 0.f, 2.9999f);
  int bay = int(u);
  float q = u - bay;
  float x = (bay - 1) * spacing, target = x, z = 12;
  if (q < 2.f / 3) {
    float a = q * 1.5f;
    x += 2 - 4 * a;
    z -= 2 * std::sin(a * p3::pi);
  } else if (bay < 2) {
    float a = (q - 2.f / 3) * 3;
    a = a * a * (3 - 2 * a);
    x += -2 + (spacing + 4) * a;
    target += spacing * a;
  } else
    x -= 2;
  p3::Camera c;
  c.position = {x, game::terrain(x, z) + eye, z};
  c.aim({target, game::terrain(target, 0) + 2.5f, 0});
  return c;
}
inline p3::Vec walk(p3::Vec p, float yaw, float f, float side, float dt,
                    const std::vector<game::Solid> &solids) {
  float n = std::hypot(f, side);
  if (n > 0) {
    f /= n;
    side /= n;
    p = game::move(p,
                   {(std::sin(yaw) * f + std::cos(yaw) * side) * dt * 2.2f, 0,
                    (-std::cos(yaw) * f + std::sin(yaw) * side) * dt * 2.2f},
                   solids);
  }
  p.y = game::terrain(p.x, p.z);
  return p;
}
} // namespace compare
