#include "core.hpp"
#include "atmosphere.hpp"
#include "look.hpp"
#include "style.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
using namespace p3;
static void require(bool b, const char *s) {
  if (!b)
    throw std::runtime_error(s);
}
static bool near(float a, float b) { return std::abs(a - b) < .0002f; }
int main() {
  try {
    auto m = transform({5, 2, -3}, .7f, 2);
    Vec a{1, 2, 3}, b = point(inverse(m), point(m, a));
    require(near(a.x, b.x) && near(a.y, b.y) && near(a.z, b.z),
            "matrix inverse");
    World owned;
    Model mesh;
    Primitive part;
    part.vertices.resize(3);
    part.indices = {0, 1, 2};
    mesh.parts.push_back(std::move(part));
    owned.models.push_back(std::move(mesh));
    require(owned.cpuGeometryBytes() > 0, "allocated CPU mesh");
    owned.releaseGeometry();
    owned.releaseGeometry();
    require(owned.cpuGeometryBytes() == 0 && owned.models[0].triangles() == 1,
            "idempotent release preserves triangle metadata");
    auto profile = Look::load(PIXEL3D_LOOK_FIXTURE);
    require(profile.cozy &&
                near(dot(profile.sunDirection, profile.sunDirection), 1),
            "valid normalized adjustable light");
    Style style;
    style.mode = 2;
    style.atmosphereWidth = 320;
    style.validate();
    style.atmosphereWidth = 960;
    bool unsafe = false;
    try {
      style.validate();
    } catch (...) {
      unsafe = true;
    }
    require(unsafe,
            "atmosphere width must remain bounded independently of geometry");
    Effects e;
    e.automatic = true;
    e.shafts = Shafts::Volume;
    Quality q;
    for (int i = 0; i < 120; i++)
      q.observe(40, e);
    require(e.divisor == 4 && e.steps == 8 && q.stage == 1,
            "bounded quality downgrade");
    for (int i = 0; i < 120; i++)
      q.observe(40, e);
    require(e.shafts == Shafts::Off, "automatic OFF fallback");
    e.automatic = false;
    e.shafts = Shafts::Volume;
    for (int i = 0; i < 240; i++)
      q.observe(100, e);
    require(e.shafts == Shafts::Volume,
            "manual benchmark never changes quality");
    auto light = lightProjection({-.55f, .48f, .68f});
    auto lightOrigin = point(light, {0, 4, 0});
    require(std::abs(lightOrigin.x) < .0001f &&
                std::abs(lightOrigin.y) < .0001f && std::abs(lightOrigin.z) < 1,
            "light projection center");
    for (float element : lightProjection({0, 1, 0}).v)
      require(std::isfinite(element), "vertical sun remains valid");
    e.shadowSize = 2048;
    bool oversized = false;
    try {
      e.validate();
    } catch (...) {
      oversized = true;
    }
    require(oversized, "high resolution shadow map rejected");
    Camera camera;
    camera.position = {0, 0, 0};
    camera.aim({0, 0, -1});
    auto projection = perspective(pi / 3, 16.f / 9, .1f, 100);
    Frustum f(projection * camera.view());
    Box inside;
    inside.add(Vec{-1, -1, -5});
    inside.add(Vec{1, 1, -3});
    require(f.visible(inside), "in-frustum object");
    Box behind;
    behind.add(Vec{-1, -1, 2});
    behind.add(Vec{1, 1, 3});
    require(!f.visible(behind), "behind object");
    Box spanning;
    spanning.add(Vec{-100, -100, -200});
    spanning.add(Vec{100, 100, 5});
    require(f.visible(spanning),
            "intersecting enormous box must not be falsely rejected");
    // Independent homogeneous clip-corner oracle. A plane may reject only if
    // ALL eight corners are outside it; crossing/near-plane boxes must be
    // retained.
    std::mt19937 random(217081);
    std::uniform_real_distribution<float> d(-100, 100), extent(.001f, 20);
    for (int view = 0; view < 20; view++) {
      camera.position = {d(random), d(random) * .1f, d(random)};
      camera.yaw = d(random);
      camera.pitch = d(random) * .01f;
      auto vp = projection * camera.view();
      Frustum fr(vp);
      World w;
      Model dummy;
      Primitive primitive;
      primitive.indices = {0, 1, 2};
      dummy.parts.push_back(primitive);
      w.models.push_back(dummy);
      for (int k = 0; k < 500; k++) {
        Vec p{d(random), d(random), d(random)},
            e{extent(random), extent(random), extent(random)};
        Box box;
        box.add(p - e);
        box.add(p + e);
        bool expected = true;
        for (int plane = 0; plane < 6; plane++) {
          bool all = true;
          int axis = plane / 2;
          float sign = plane % 2 ? -1.f : 1.f;
          for (int corner = 0; corner < 8; corner++) {
            Vec q{corner & 1 ? box.hi.x : box.lo.x,
                  corner & 2 ? box.hi.y : box.lo.y,
                  corner & 4 ? box.hi.z : box.lo.z};
            auto row = [&](int r) {
              return static_cast<double>(vp.v[r]) * q.x +
                     static_cast<double>(vp.v[4 + r]) * q.y +
                     static_cast<double>(vp.v[8 + r]) * q.z + vp.v[12 + r];
            };
            if (row(3) + sign * row(axis) >= 0)
              all = false;
          }
          if (all)
            expected = false;
        }
        if (expected)
          require(fr.visible(box), "false-negative object cull");
        w.instances.push_back({0, Mat::identity(), box, std::to_string(k)});
      }
      w.partition(12);
      std::vector<std::size_t> out;
      auto v = w.visible(fr, true, out);
      require(v.objectsVisible + v.objectsCulled == 500, "object statistics");
      require(v.chunksVisible + v.chunksCulled == w.chunks.size(),
              "chunk statistics");
      for (std::size_t i = 0; i < w.instances.size(); i++)
        if (fr.visible(w.instances[i].bounds))
          require(std::find(out.begin(), out.end(), i) != out.end(),
                  "chunk hides potentially visible member");
      auto off = w.visible(fr, false, out);
      require(off.objectsVisible == 500 && off.objectsCulled == 0 &&
                  off.triangles == 500,
              "OFF must submit all geometry");
    }
    camera.position = {0, 1.7f, 0};
    camera.yaw = pi / 2;
    camera.move(1, 1, 1);
    require(near(std::sqrt(dot(camera.position - Vec{0, 1.7f, 0},
                               camera.position - Vec{0, 1.7f, 0})),
                 3),
            "diagonal speed");
    require(camera.position.x > 0 && camera.position.z > 0,
            "camera handedness");
    Mat nonuniform = Mat::identity();
    nonuniform.v[0] = 2;
    Vec n = normalized(
        direction(transpose(inverse(nonuniform)), normalized({1, 1, 0})));
    require(near(n.x, 1 / std::sqrt(5.f)) && near(n.y, 2 / std::sqrt(5.f)),
            "inverse transpose normal");
    require(near(point(lookAt({0, 0, 5}, {0, 0, 0}), {}).z, -5),
            "view forward -Z");
    bool rejected = false;
    try {
      safePath(".", "../secret");
    } catch (...) {
      rejected = true;
    }
    require(rejected, "path traversal");
    std::cout << "PASS: matrix/normal/handedness/camera; 10000 independently "
                 "checked boxes and chunk conservativeness\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
