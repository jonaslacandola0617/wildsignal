#include "game.hpp"
#include "animation.hpp"
#include "mesh_clumps.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
void require(bool b, const char *m) {
  if (!b)
    throw std::runtime_error(m);
}
int main(int argc, char **argv) {
  try {
    int checks = 0;
    auto check = [&](bool b, const char *m) {
      require(b, m);
      checks++;
    };
    std::vector<game::Solid> s = {{0, 0, 1, 1}};
    auto p = game::move({0, 0, 3}, {0, 0, -20}, s);
    check(p.z > 1.22f, "swept collision tunneling");
    check(!game::freeAt({0, 0, 0}, s), "solid center");
    p = game::move({2, 0, 2}, {20, 0, 0}, s);
    check(p.x <= 5.62f, "world bound");
    bool threw = false;
    try {
      game::move({}, {NAN, 0, 0}, s);
    } catch (...) {
      threw = true;
    }
    check(threw, "nonfinite movement rejected");
    game::Follower follower;
    follower.position = {0, 0, 3};
    for (int i = 0; i < 120; i++)
      game::follow(follower, {2, 0, 3}, 1 / 30.f, {});
    check(std::hypot(follower.position.x - 2, follower.position.z - 3) < .3f,
          "follower reaches target");
    game::follow(follower, {2, 0, 3}, 1 / 30.f, {});
    check(!follower.moving, "follower stops");
    check(std::abs(follower.yaw) > 1, "follower turns");
    for (int i = 0; i < 200; i++) {
      game::follow(follower, {0, 0, 0}, 1 / 30.f, s);
      check(game::freeAt(follower.position, s),
            "follower stays outside obstacle");
    }
    game::Game g;
    g.state.player = g.wild;
    check(g.encounter(), "in-world encounter");
    g.act(game::Action::Befriend);
    check(!g.state.recruited && g.state.hp == 22, "premature recruit reply");
    g.act(game::Action::Defend);
    check(g.state.hp == 20 && g.defends == 1, "defend reduced damage");
    g.act(game::Action::Attack);
    g.act(game::Action::Attack);
    check(g.enemyHp == 12 && g.state.hp == 4, "temporary deterministic attack");
    g.act(game::Action::Befriend);
    check(g.phase == game::Phase::Recruited && g.state.recruited,
          "recruit threshold");
    g.resume();
    check(g.phase == game::Phase::Explore, "return explore");
    check(!g.encounter(), "recruited encounter not duplicated");
    g.rest();
    check(g.state.hp == 30 && g.state.visited, "cottage rest");
    game::Game lost;
    lost.state.player = lost.wild;
    lost.encounter();
    for (int i = 0; i < 4; i++)
      lost.act(game::Action::Befriend);
    check(lost.phase == game::Phase::Lost, "lose path");
    lost.resume();
    check(lost.state.hp == 30 && lost.phase == game::Phase::Explore,
          "loss recovery");
    game::Game won;
    won.state.player = won.wild;
    won.encounter();
    for (int i = 0; i < 4; i++) {
      won.act(game::Action::Attack);
    }
    check(won.phase == game::Phase::Won, "win path");
    won.act(game::Action::Befriend);
    check(won.state.recruited, "win recruitment");
    auto dir =
        std::filesystem::temp_directory_path() /
        ("wildsignal-gameloop-test-" +
         std::to_string(
             std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(dir);
    auto file = dir / "game.json";
    game::State state;
    state.player = {2, 0, 3};
    state.recruited = true;
    state.visited = true;
    state.hp = 13;
    game::save(file, state);
    auto loaded = game::load(file);
    check(loaded.loaded && loaded.state.hp == 13 && loaded.state.recruited &&
              loaded.state.visited && loaded.state.player.x == 2,
          "save/reopen complete state");
    std::ofstream(file) << "broken{";
    loaded = game::load(file);
    check(loaded.loaded && loaded.recovered && loaded.state.hp == 13,
          "first-save backup recovery");
    state.hp = 20;
    game::save(file, state);
    std::ofstream(file) << "corrupt";
    loaded = game::load(file);
    check(loaded.recovered && loaded.state.hp == 13,
          "corrupt primary did not poison good backup");
    std::ofstream(file.string() + ".bak") << "also corrupt";
    loaded = game::load(file);
    check(!loaded.loaded && loaded.state.hp == 30 && !loaded.state.recruited,
          "both bad safe default");
    std::ofstream(file.string() + ".tmp") << "partial interrupted write";
    check(!game::load(file).loaded, "uncommitted temp ignored");
    state.hp = 0;
    threw = false;
    try {
      game::save(file, state);
    } catch (...) {
      threw = true;
    }
    check(threw, "dead state cannot overwrite save");
    state.hp = 30;
    state.pitch = 2;
    threw = false;
    try {
      game::save(file, state);
    } catch (...) {
      threw = true;
    }
    check(threw, "invalid camera save rejected");
    std::filesystem::remove_all(dir);
    if (argc > 1) {
      auto root = std::filesystem::path(argv[1]);
      for (auto name : {"WS010_LOD0.glb", "WS010_LOD1.glb", "WS010_LOD2.glb"}) {
        auto m = p3::Model::load(root / name);
        check(bool(m.rig), "actual skeletal model");
        for (auto clip : {"Idle", "Fly", "Attack"}) {
          auto a = m.rig->sample(clip, 0), b = m.rig->sample(clip, .3f);
          bool changed = false;
          for (std::size_t i = 0; i < a.size(); i++)
            for (int j = 0; j < 16; j++)
              changed |= std::abs(a[i].v[j] - b[i].v[j]) > 1e-5f;
          check(changed, "actual skeleton animation changes");
        }
      }
      p3::MeshClumps grass(root, s);
      check(grass.all.size() > 4000, "actual populated grass");
      auto c = p3::preset(1);
      auto v = grass.select(
          c, p3::Frustum(p3::perspective(p3::pi / 3, 16.f / 9) * c.view()),
          270);
      check(v.visible > 0, "actual grass culling");
      grass.density = 0;
      check(grass.select(c,
                         p3::Frustum(p3::perspective(p3::pi / 3, 16.f / 9) *
                                     c.view()),
                         270)
                    .visible == 0,
            "grass off");
    }
    std::cout << "GAMELOOP CHECKS PASSED: " << checks << '\n';
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
