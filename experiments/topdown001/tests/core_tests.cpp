#include "playable.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
namespace audit {
bool active = false;
int count = 0;
} // namespace audit
void *operator new(std::size_t n) {
  if (audit::active)
    ++audit::count;
  if (auto *p = std::malloc(n ? n : 1))
    return p;
  throw std::bad_alloc();
}
void operator delete(void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void *operator new[](std::size_t n) { return ::operator new(n); }
void operator delete[](void *p) noexcept { ::operator delete(p); }
void operator delete[](void *p, std::size_t) noexcept { ::operator delete(p); }
int main() {
  try {
    int checks = 0;
    const auto expect = [&](bool ok, const char *message) {
      ++checks;
      if (!ok)
        throw std::runtime_error(message);
    };
    td::Buffer b;
    b.fill(10, 20, 30);
    td::Image pixel{1, 1, {200, 100, 50, 0}};
    b.blit(pixel, 0, 0, 1, 1, 0, 0);
    expect(b.rgba[0] == 10, "transparent sprite overwrites ground");
    pixel.rgba[3] = 128;
    b.blit(pixel, 0, 0, 1, 1, 0, 0);
    expect(b.rgba[0] == 105 && b.rgba[1] == 60, "partial alpha composition");
    td::Image strip{2, 1, {255, 0, 0, 255, 0, 255, 0, 255}};
    b.blit(strip, 0, 0, 2, 1, 0, 0, 2, true);
    expect(b.rgba[1] == 255 && b.rgba[4 + 1] == 255 && b.rgba[8] == 255,
           "nearest zoom or source flip");
    bool rejected = false;
    try {
      b.blit(strip, 1, 0, 2, 1, 0, 0);
    } catch (const std::exception &) {
      rejected = true;
    }
    expect(rejected, "source index validation");
    rejected = false;
    try {
      td::loadImage("THIS_FILE_DOES_NOT_EXIST.rgba");
    } catch (const std::exception &) {
      rejected = true;
    }
    expect(rejected, "missing file silently accepted");
    const auto v = td::letterbox(1003, 577);
    expect(v.w == 960 && v.h == 540 && v.x == 21 && v.y == 18,
           "odd-aspect integer letterboxing");
    expect(td::letterbox(240, 135).w == 240, "small-window fallback");
    td::Animation clip;
    clip.fps = 9;
    clip.count = 6;
    expect(clip.frame(0) == 0 && clip.frame(.5) == 4 &&
               clip.frame(2.0 / 3) == 0,
           "walk animation frame cycle");
    td::Art art;
    for (const auto dims : std::array<std::array<int, 2>, 4>{
             {{16, 16}, {32, 64}, {16, 32}, {16, 32}}}) {
      td::Image im;
      im.w = dims[0];
      im.h = dims[1];
      im.rgba.resize(static_cast<std::size_t>(im.w * im.h * 4));
      const int index = static_cast<int>(art.images.size());
      for (std::size_t i = 0; i < im.rgba.size(); i += 4) {
        im.rgba[i] = index == 2 ? 240 : 20;
        im.rgba[i + 1] = index == 1 ? 220 : 20;
        im.rgba[i + 2] = index == 3 ? 230 : 20;
        im.rgba[i + 3] = 255;
      }
      art.images.push_back(std::move(im));
    }
    for (auto *actor : {&art.player, &art.companion})
      for (auto &state : *actor)
        for (auto &anim : state) {
          anim.image = actor == &art.player ? 2 : 3;
          anim.w = 16;
          anim.h = 32;
          anim.ax = 8;
          anim.ay = 32;
          anim.count = 1;
          anim.fps = 5;
        }
    td::Map map;
    map.w = 40;
    map.h = 28;
    map.ground.resize(40 * 28, 0);
    map.collision.resize(40 * 28, 0);
    map.spawn = {160, 150};
    map.follower = {220, 180};
    for (int y = 0; y < map.h; ++y)
      for (int x = 0; x < map.w; ++x)
        if (x == 0 || y == 0 || x == map.w - 1 || y == map.h - 1)
          map.collision[static_cast<std::size_t>(y * map.w + x)] = 1;
    map.objects.push_back({1, 160, 140, 16, 64, 140, 1});
    td::Game game(art, map);
    game.camera = {0, 0};
    game.player.p = {160, 130};
    game.companion.p = {400, 400};
    game.render(b);
    expect(b.rgba[static_cast<std::size_t>((120 * 320 + 160) * 4 + 1)] == 220,
           "canopy must occlude actor behind foot");
    game.player.p = {160, 150};
    game.render(b);
    expect(b.rgba[static_cast<std::size_t>((130 * 320 + 160) * 4)] == 240,
           "front actor must occlude canopy");
    game.player.p = {320, 240};
    game.companion.p = {320, 280};
    game.followCamera(0, true);
    const auto initial = game.player.p;
    for (int n = 0; n < 60; ++n)
      game.step(1.0 / 60, 1, 0);
    expect(std::abs(game.player.p.x - initial.x - 64) < .01,
           "delta-time movement speed");
    expect(std::hypot(game.player.p.x - game.companion.p.x,
                      game.player.p.y - game.companion.p.y) < 50,
           "follower distance while walking");
    for (int n = 0; n < 180; ++n)
      game.step(1.0 / 60, 0, 0);
    expect(std::hypot(game.player.p.x - game.companion.p.x,
                      game.player.p.y - game.companion.p.y) <= 24.01,
           "follower must stop at follow distance");
    for (int n = 0; n < 1000; ++n) {
      game.step(.5, 1, -1);
      expect(map.free(game.player.p) && map.free(game.companion.p),
             "collision substeps/tunnelling");
    }
    for (int z : {1, 2}) {
      game.setZoom(z);
      game.render(b);
      const auto cam = game.cameraPixel();
      expect(cam.x == std::floor(cam.x) && cam.y == std::floor(cam.y) &&
                 cam.x >= 0 && cam.y >= 0 && cam.x <= map.w * 16 - 320.0 / z &&
                 cam.y <= map.h * 16 - 180.0 / z,
             "camera clamp or grid alignment");
      if (z == 2)
        for (int y = 0; y < 180; y += 2)
          for (int x = 0; x < 320; x += 2) {
            const auto i = static_cast<std::size_t>((y * 320 + x) * 4);
            for (int c = 0; c < 4; ++c)
              expect(b.rgba[i + c] == b.rgba[i + 4 + c] &&
                         b.rgba[i + c] == b.rgba[i + 320 * 4 + c],
                     "zoom must duplicate native pixels exactly");
          }
    }
    // A blocked direct route requires actual path navigation, not teleportation
    // through a building.
    for (int y = 9; y <= 14; ++y)
      for (int x = 10; x <= 12; ++x)
        map.collision[static_cast<std::size_t>(y * 40 + x)] = 1;
    td::Game path(art, map);
    path.player.p = {240, 190};
    path.companion.p = {140, 190};
    expect(!map.line(path.player.p, path.companion.p), "blocked route fixture");
    for (int n = 0; n < 900; ++n) {
      path.step(1.0 / 60, 0, 0);
      expect(map.free(path.companion.p), "follower crosses collision");
    }
    expect(std::hypot(path.player.p.x - path.companion.p.x,
                      path.player.p.y - path.companion.p.y) <= 24.01,
           "follower cannot navigate around house");
    audit::count = 0;
    audit::active = true;
    for (int n = 0; n < 240; ++n) {
      path.step(1.0 / 60, n < 120 ? 1 : -1, 0);
      path.render(b);
    }
    audit::active = false;
    expect(audit::count == 0, "hot simulation/render allocations");
    expect(art.bytes() < 1024 * 1024, "synthetic texture budget");
    std::cout
        << "PASS: " << checks
        << " top-down invariants: alpha, bounds, grid/zoom, y-sort, collision, "
           "camera, animation, follow route/distance, no hot allocations\n";
    return 0;
  } catch (const std::exception &e) {
    audit::active = false;
    std::cerr << e.what() << '\n';
    return 1;
  }
}
