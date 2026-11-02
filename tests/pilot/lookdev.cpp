// LOOKDEV-001: hidden look-development instances are never drawn, and the plot
// sits on flat ground.
#include "core.hpp"
#include "game.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
void check(bool b, const char *m) {
  if (!b)
    throw std::runtime_error(m);
}
int main() {
  try {
    p3::World world;
    world.models.resize(1);
    world.models[0].bounds.add(p3::Vec{-1, 0, -1});
    world.models[0].bounds.add(p3::Vec{1, 2, 1});
    world.instances.resize(3);
    for (int k = 0; k < 3; k++) {
      auto &i = world.instances[std::size_t(k)];
      i.matrix = p3::transform({float(k) * 3, 0, -6}, 0, 1);
      i.bounds = p3::transformed(world.models[0].bounds, i.matrix);
    }
    world.instances[1].hidden = true;
    world.partition(3);
    std::vector<std::size_t> out;
    auto v = world.visible(p3::Frustum(p3::perspective(1.f, 16.f / 9)), false, out);
    check(out.size() == 2 && v.objectsVisible == 2, "hidden instance drawn");
    for (auto i : out)
      check(i != 1, "hidden instance listed");
    world.instances[1].hidden = false;
    world.visible(p3::Frustum(p3::perspective(1.f, 16.f / 9)), false, out);
    check(out.size() == 3, "shown instance missing");
    check(std::abs(game::terrain(4, 4)) > 1e-4f, "default terrain is not flat");
    game::flattenWorld();
    check(game::terrain(4, 4) == 0 && game::terrain(-20, 13) == 0, "look development ground not flat");
    std::cout << "lookdev tests passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
