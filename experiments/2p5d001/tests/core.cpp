#include "hybrid.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
std::atomic<bool> audit{false};
std::atomic<int> allocations{0};
void *operator new(std::size_t n) {
  if (audit)
    ++allocations;
  if (auto *p = std::malloc(n ? n : 1))
    return p;
  throw std::bad_alloc();
}
#ifdef _MSC_VER
__declspec(noinline)
#else
__attribute__((noinline))
#endif
void operator delete(void *p) noexcept {
  std::free(p);
}
#ifdef _MSC_VER
__declspec(noinline)
#else
__attribute__((noinline))
#endif
void operator delete(void *p, std::size_t) noexcept {
  std::free(p);
}
void *operator new[](std::size_t n) { return ::operator new(n); }
void operator delete[](void *p) noexcept { ::operator delete(p); }
void operator delete[](void *p, std::size_t) noexcept { ::operator delete(p); }
namespace {
void require(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
td::Image image(int w, int h, unsigned r, unsigned g, unsigned b,
                unsigned a = 255) {
  td::Image i;
  i.w = w;
  i.h = h;
  i.rgba.resize(static_cast<std::size_t>(w * h * 4));
  for (std::size_t p = 0; p < i.rgba.size(); p += 4) {
    i.rgba[p] = static_cast<std::uint8_t>(r);
    i.rgba[p + 1] = static_cast<std::uint8_t>(g);
    i.rgba[p + 2] = static_cast<std::uint8_t>(b);
    i.rgba[p + 3] = static_cast<std::uint8_t>(a);
  }
  return i;
}
td::Map map() {
  td::Map m;
  m.w = 32;
  m.h = 28;
  m.ground.assign(896, 0);
  m.collision.assign(896, 0);
  for (int y = 0; y < m.h; ++y)
    for (int x = 0; x < m.w; ++x)
      if (x == 0 || y == 0 || x == m.w - 1 || y == m.h - 1)
        m.collision[static_cast<std::size_t>(y * m.w + x)] = 1;
  m.spawn = {208, 320};
  m.follower = {192, 336};
  return m;
}
} // namespace
int main() {
  try {
    td::Art art;
    art.images = {image(16, 16, 100, 130, 150), image(16, 16, 230, 50, 60),
                  image(16, 16, 30, 210, 80), image(16, 16, 255, 255, 255, 0),
                  image(16, 16, 240, 40, 20, 128)};
    for (int k = 0; k < 5; ++k)
      art.names.push_back(std::to_string(k));
    td::Animation clip;
    clip.image = 1;
    clip.w = clip.h = 16;
    clip.ax = 8;
    clip.ay = 16;
    for (auto &state : art.player)
      for (auto &c : state)
        c = clip;
    for (auto &state : art.companion)
      for (auto &c : state)
        c = clip;
    auto outside = map(), inside = map();
    for (int y = 7; y < 12; ++y)
      for (int x = 10; x < 16; ++x)
        outside.collision[static_cast<std::size_t>(y * 32 + x)] = 1;
    hybrid::Scene scene;
    scene.houses.push_back({10, 7, 6, 5, 3, 1.5, .32, .12});
    scene.door = {13, 12.6};
    scene.exit = {16.5, 18.5};
    scene.insideSpawn = {16.5, 17.5};
    scene.doorArt = 1;
    scene.window = 2;
    hybrid::Session g(art, outside, inside, scene);
    hybrid::Renderer r(art, scene, inside);
    hybrid::Frame f(320, 180);
    // A and D strafe in camera-local space; they never turn yaw.
    g.step(.05, {0, 1, 0, false});
    require(g.player.p.x > 208 && g.player.p.y == 320 && g.yaw == 0,
            "D did not strafe");
    g.step(.01, {0, 0, .125, false});
    require(std::abs(g.yaw - .125) < 1e-12, "Mouse turn not applied");
    td::Actor a;
    a.p = {208, 224};
    hybrid::move(a, outside, {0, -1}, 1000, 1);
    require(a.p.y >= 196 && outside.free(a.p),
            "Movement tunneled through cottage");
    g.setPose(13, 12.9, 180);
    require(!g.prompt(), "Back-facing entrance prompt");
    g.setPose(13, 12.9, 0);
    require(g.prompt(), "Front-facing door prompt missing");
    const auto saved = g.player;
    const double yaw = g.yaw;
    g.step(1.0 / 60, {0, 0, 0, true});
    for (int k = 0; k < 30; ++k)
      g.step(1.0 / 60, {});
    require(g.mode == hybrid::Mode::Indoor && g.transitions == 1,
            "Enter transition failed");
    require(!g.switching, "Fade failed to finish");
    const auto start = g.player.p;
    for (int k = 0; k < 20; ++k)
      g.step(1.0 / 60, {0, -1, 0, false});
    require(g.player.p.x < start.x, "Interior was not playable");
    g.player.p = {scene.exit.x * 16, scene.exit.y * 16};
    for (int k = 0; k < 30; ++k)
      g.step(1.0 / 60, {});
    require(g.prompt(), "Interior exit missing");
    g.step(1.0 / 60, {0, 0, 0, true});
    for (int k = 0; k < 30; ++k)
      g.step(1.0 / 60, {});
    require(g.mode == hybrid::Mode::Outdoor && g.transitions == 2 &&
                g.player.p.x == saved.p.x && g.player.p.y == saved.p.y &&
                g.yaw == yaw,
            "Outdoor pose not exactly preserved");
    // Two triangles share a diagonal: top-left rule gives hole-free,
    // order-independent coverage.
    const hybrid::Vertex v0{-1, -4, .45, 0, 1}, v1{1, -4, .45, 1, 1},
        v2{1, -4, 2.45, 1, 0}, v3{-1, -4, 2.45, 0, 0};
    const hybrid::Triangle t0{{v0, v1, v2}, 1, 1, false},
        t1{{v0, v2, v3}, 1, 1, false};
    f.clear({0, 0, 0});
    r.triangle(f, t0, {0, 0}, 0);
    r.triangle(f, t1, {0, 0}, 0);
    const auto expected = f.rgba;
    for (int y = 35; y < 145; ++y)
      for (int x = 105; x < 215; ++x)
        require(f.rgba[static_cast<std::size_t>((y * 320 + x) * 4)] == 230,
                "Shared diagonal coverage hole");
    f.clear({0, 0, 0});
    r.triangle(f, t1, {0, 0}, 0);
    r.triangle(f, t0, {0, 0}, 0);
    require(f.rgba == expected, "Triangle order altered shared-edge pixels");
    // Independent plane depth and perspective UV oracle on an oblique quad.
    for (int y = 0; y < 16; ++y)
      for (int x = 0; x < 16; ++x) {
        auto p = static_cast<std::size_t>((y * 16 + x) * 4);
        art.images[2].rgba[p] = static_cast<std::uint8_t>(x * 16);
        art.images[2].rgba[p + 1] = static_cast<std::uint8_t>(y * 16);
      }
    hybrid::Triangle oblique{
        {{{-1, -3, .45, 0, 1}, {1, -5, .45, 1, 1}, {1, -5, 2.45, 1, 0}}},
        2,
        1,
        false};
    hybrid::Triangle oblique2{
        {{{-1, -3, .45, 0, 1}, {1, -5, 2.45, 1, 0}, {-1, -3, 2.45, 0, 0}}},
        2,
        1,
        false};
    f.clear({0, 0, 0});
    r.triangle(f, oblique, {0, 0}, 0);
    r.triangle(f, oblique2, {0, 0}, 0);
    int checked = 0;
    const double focal = 320 / (2 * std::tan(70 * hybrid::pi / 360));
    for (int y = 50; y < 130; y += 11)
      for (int x = 120; x < 200; x += 13) {
        double rx = (x + .5 - 160) / focal, rz = -(y + .5 - 90) / focal,
               d = 4 / (1 - rx), wx = rx * d, z = 1.45 + rz * d,
               u = (wx + 1) / 2, v = (2.45 - z) / 2;
        if (u < .02 || u > .98 || v < .02 || v > .98)
          continue;
        auto p = static_cast<std::size_t>(y * 320 + x);
        require(std::abs(f.depth[p] - d) < .003,
                "Perspective depth differs from ray-plane oracle");
        require(f.rgba[p * 4] == static_cast<int>(u * 16) * 16 &&
                    f.rgba[p * 4 + 1] == static_cast<int>(v * 16) * 16,
                "UV differs from ray-plane oracle");
        ++checked;
      }
    require(checked > 10, "Insufficient perspective oracle samples");
    // Transparent card must not punch a depth hole through visible
    // architecture.
    f.clear({0, 0, 0});
    r.triangle(f, t0, {0, 0}, 0);
    auto transparent = t0;
    transparent.image = 3;
    for (auto &v : transparent.v)
      v.y = -2;
    r.triangle(f, transparent, {0, 0}, 0);
    require(f.rgba[static_cast<std::size_t>((90 * 320 + 170) * 4)] == 230,
            "Transparent texel overwrote geometry");
    // Roof surfaces use shared depth: reversing all mesh triangles leaves exact
    // output.
    g.setPose(19, 17, -40);
    r.draw(f, g);
    const auto normal = f.rgba;
    std::reverse(r.mesh.begin(), r.mesh.end());
    r.draw(f, g);
    require(normal == f.rgba, "Roof/house depth depends on triangle order");
    std::reverse(r.mesh.begin(), r.mesh.end());
    // Partial alpha remains available for top-down native sprite composition.
    f.clear({0, 0, 0});
    f.blit(art.images[4], 0, 0, 16, 16, 0, 0);
    require(f.rgba[0] == 120 && f.rgba[1] == 20,
            "Straight alpha blend incorrect");
    // Near-plane clipping and worst bounded render path must stay finite and
    // allocate nothing.
    hybrid::Triangle clipped{
        {{{-.1, -.02, 0, 0, 1}, {1, -2, 0, 1, 1}, {0, -2, 3, 0, 0}}},
        1,
        1,
        false};
    r.triangle(f, clipped, {0, 0}, 0);
    for (float d : f.depth)
      require(!std::isnan(d), "Near clip produced NaN");
    allocations = 0;
    audit = true;
    for (int k = 0; k < 60; ++k) {
      g.step(1.0 / 60, {});
      r.draw(f, g);
    }
    g.mode = hybrid::Mode::Indoor;
    g.player.p = inside.spawn;
    for (int k = 0; k < 60; ++k) {
      g.step(1.0 / 60, {});
      r.draw(f, g);
    }
    audit = false;
    require(allocations == 0, "Hot simulation/render allocated memory");
    std::cout << "PASS: strafe/mouse, collision, playable transition/return, "
                 "perspective UV/depth, shared edges, alpha, roof order, "
                 "clipping, zero hot allocations\n";
    return 0;
  } catch (const std::exception &e) {
    audit = false;
    std::cerr << e.what() << '\n';
    return 1;
  }
}
