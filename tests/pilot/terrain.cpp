#include "terrain_mesh.hpp"
#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>
#include <tuple>
// REGION-001 build 018: ground built from heights. Flat ground is all lawn,
// a step makes rock faces, every face looks up, and neighbouring chunk squares
// share their border vertices exactly.
void check(bool b, const char *m) {
  if (!b)
    throw std::runtime_error(m);
}
namespace {
using p3::Vec;
float upY(const p3::Primitive &part, std::size_t t) {
  const Vec a = part.vertices[part.indices[t]].p, b = part.vertices[part.indices[t + 1]].p,
            c = part.vertices[part.indices[t + 2]].p;
  const Vec u = b - a, v = c - a;
  Vec n{u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x};
  float l = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
  return n.y / l;
}
std::set<std::tuple<float, float, float>> border(const p3::Model &m, float x) {
  std::set<std::tuple<float, float, float>> out;
  for (auto &v : m.parts.front().vertices)
    if (v.p.x == x)
      out.insert({v.p.x, v.p.y, v.p.z});
  return out;
}
} // namespace
int main() {
  try {
    p3::TerrainSettings s;
    // Flat: one lawn model, a full 65 x 65 grid, no rock, normals straight up.
    auto flat = p3::buildTerrain("0_0", 0, 0, 32, s, [](float, float) { return 1.5f; });
    const auto &lawn = flat.lawn.parts.front();
    check(!flat.hasRock, "flat ground has rock");
    check(flat.lawn.id == "WS_TERRAIN_0_0" && flat.rock.id == "WS_TERRAIN_ROCK_0_0", "terrain ids");
    check(lawn.vertices.size() == 65 * 65, "flat grid vertex count");
    check(lawn.indices.size() == 64 * 64 * 6, "flat grid triangle count");
    for (auto &v : lawn.vertices)
      check(v.n.y > .999f && v.p.y == 1.5f, "flat normal or height");
    check(flat.lawn.bounds.lo.x == 0 && flat.lawn.bounds.hi.x == 32 && flat.lawn.bounds.hi.y == 1.5f, "flat bounds");

    // A 3 m step at x = 16: rock faces along it, the rest lawn, every face looking up.
    auto step = p3::buildTerrain("1_0", 0, 0, 32, s, [](float x, float) { return x >= 16 ? 3.f : 0.f; });
    check(step.hasRock, "step has no rock");
    const auto &rock = step.rock.parts.front();
    check(rock.indices.size() == 64 * 6, "one row of rock cells along the step");
    for (std::size_t t = 0; t < rock.indices.size(); t += 3)
      check(upY(rock, t) > 0 && upY(rock, t) < s.cliffNormalY, "rock face steepness or winding");
    const auto &top = step.lawn.parts.front();
    for (std::size_t t = 0; t < top.indices.size(); t += 3)
      check(upY(top, t) >= s.cliffNormalY, "lawn face steepness or winding");

    // A gentle slope (1 in 4) stays lawn.
    auto slope = p3::buildTerrain("2_0", 0, 0, 32, s, [](float x, float) { return x * .25f; });
    check(!slope.hasRock, "walkable slope became rock");

    // Neighbours meet exactly.
    auto hill = [](float x, float z) { return 2.f * std::sin(x * .13f) * std::cos(z * .07f) + .01f * x; };
    auto a = p3::buildTerrain("0_0", 0, 0, 32, s, hill), b = p3::buildTerrain("1_0", 32, 0, 32, s, hill);
    check(border(a.lawn, 32) == border(b.lawn, 32) && border(a.lawn, 32).size() == 65, "chunk seam");

    bool threw = false;
    try {
      p3::buildTerrain("bad", 0, 0, 32, {.3f}, hill);
    } catch (const std::exception &) {
      threw = true;
    }
    check(threw, "a step that does not divide the chunk was accepted");
    std::cout << "terrain mesh: flat, step, slope, seam ok\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << "\n";
    return 1;
  }
}
