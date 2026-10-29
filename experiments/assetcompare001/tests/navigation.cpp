#include "comparison.hpp"
#include <iostream>
#include <stdexcept>
void check(bool x) {
  if (!x)
    throw std::runtime_error("comparison navigation contract");
}
int main() {
  try {
    for (int b = 0; b < 3; b++) {
      auto c = compare::camera(b);
      check(std::abs(c.position.x - (b - 1) * 11.9f) < 1e-5);
      check(std::abs(c.position.y - game::terrain(c.position.x, c.position.z) -
                     1.65f) < 1e-5);
      check(c.position.z == 12);
    }
    for (int f = 0; f < 450; f++) {
      auto a = compare::motion(f), b = compare::motion(f);
      check(p3::finite(a.position));
      check(a.yaw == b.yaw && a.pitch == b.pitch);
      check(std::abs(a.position.y - game::terrain(a.position.x, a.position.z) -
                     1.65f) < 1e-5);
    }
    auto p = compare::walk({0, 0, 4}, 0, 1, 0, .1f, {});
    check(p.z < 4 && p.z > 3.7f);
    std::vector<game::Solid> blocks{{0, 2, 1, 1}};
    p = {0, 0, 4};
    for (int i = 0; i < 100; i++)
      p = compare::walk(p, 0, 1, 0, .02f, blocks);
    check(p.z > 3.2f);
    std::cout << "PASS matched cameras, physical eye height, route "
                 "determinism, inherited walk/collision\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
