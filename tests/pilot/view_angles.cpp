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
      need(std::abs(std::remainder(game::quarterYaw(q + 1) - game::quarterYaw(q), 2 * p3::pi) - p3::pi * .5f) < 1e-5f, "quarter step");
    }
    need(std::abs(game::quarterYaw(0) - p3::pi * .25f) < 1e-6f, "first quarter is diagonal");
    std::cout << "view angles ok\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
