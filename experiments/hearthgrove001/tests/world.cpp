#include "followers.hpp"
#include "game.hpp"
#include <bit>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
void check(bool b, const char *m) {
  if (!b)
    throw std::runtime_error(m);
}
int main() {
  try {
    auto root = std::filesystem::temp_directory_path() /
                "wildsignal-hearthgrove-world-test";
    std::filesystem::create_directories(root);
    auto p = root / "height.bin";
    {
      std::ofstream f(p, std::ios::binary);
      auto n = std::bit_cast<std::uint32_t>(.125f);
      for (int i = 0; i < 97 * 97; i++)
        for (int k = 0; k < 4; k++)
          f.put(char((n >> (8 * k)) & 255));
    }
    game::configureWorld(17.85f, p);
    check(game::freeAt({14, 0, 14}, {}), "expanded village inaccessible");
    check(!game::freeAt({18, 0, 0}, {}), "world escape");
    check(std::abs(game::terrain(14, 14) - .125f) < 1e-6,
          "existing heightfield sampling");
    auto pos = game::move({14, 0, 0}, {10, 0, 0}, {});
    check(pos.x < 17.63f && pos.x > 17.5f, "expanded world movement clamp");
    game::State state;
    state.player = {14, .125f, 14};
    game::save(root / "save.json", state);
    auto load = game::load(root / "save.json");
    check(load.loaded && load.state.player.x == 14,
          "expanded-world save/reopen");
    {
      std::ofstream f(root / "bad.bin");
      f << "bad";
    }
    bool rejected = false;
    try {
      game::configureWorld(48, root / "bad.bin");
    } catch (const std::exception &) {
      rejected = true;
    }
    check(rejected && std::abs(game::terrain(14, 14) - .125f) < 1e-6,
          "invalid terrain replaced valid world");
    std::vector<game::Solid> solids;
    for (int i = 0; i < 24; i++)
      solids.push_back({float(i % 6) * 2 - 8, float(i / 6) * 2 - 8, .2f, .2f});
    game::Game g;
    g.solids = solids;
    g.state.player = {3, 0, 4};
    g.follower.position = {0, 0, 4};
    game::Followers followers;
    followers.reset(g, false);
    for (int i = 0; i < 120; i++)
      followers.update(g, 1 / 30.f);
    check(game::freeAt(g.follower.position, solids, .5f),
          "large-scene follower collision");
    std::filesystem::remove_all(root);
    std::cout << "PASS expanded village bounds/height/save validation and "
                 "bounded obstacle following\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
