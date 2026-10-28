#include "mesh_clumps.hpp"
#include <iostream>
#include <stdexcept>
int main(int argc, char **argv) {
  try {
    if (argc != 2)
      throw std::runtime_error("fixture directory required");
    p3::MeshClumps m(argv[1], {});
    if (!m.authored || !m.receiveShadows || m.all.size() != 3 ||
        m.models.size() != 2)
      throw std::runtime_error("authored-only fixture population");
    if (std::abs(m.all[0].matrix.v[13] - .12f) > .00001f)
      throw std::runtime_error("visible ground placement changed");
    p3::MeshClumps standard(argv[1], {}, false, false, 1),
        small(argv[1], {}, false, false, 2);
    if (std::abs(standard.all[0].matrix.v[5] - 1) > .00001f ||
        std::abs(small.all[0].matrix.v[5] - .82f) > .00001f)
      throw std::runtime_error("scale family mismatch");
    auto c = p3::Camera{};
    c.position = {0, 1, 3};
    c.aim({0, .1f, 0});
    auto vp = p3::perspective(p3::pi / 3, 16.f / 9) * c.view();
    m.options.cull = false;
    m.density = 3;
    auto near = m.select(c, p3::Frustum(vp), 270);
    if (near.visible != 3 || near.nearCount != 3)
      throw std::runtime_error("near batch");
    c.position = {0, 1, 17};
    c.aim({0, .1f, 0});
    vp = p3::perspective(p3::pi / 3, 16.f / 9) * c.view();
    auto far = m.select(c, p3::Frustum(vp), 270);
    if (far.visible != 3 || far.farCount != 3 || m.batches[1].size() != 3)
      throw std::runtime_error("LOD population lost");
    m.options.cull = true;
    c.position = {0, 1, 3};
    c.aim({0, 1, 8});
    vp = p3::perspective(p3::pi / 3, 16.f / 9) * c.view();
    if (m.select(c, p3::Frustum(vp), 270).visible)
      throw std::runtime_error("behind-camera culling");
    m.options.cull = false;
    m.density = 0;
    if (m.select(c, p3::Frustum(vp), 270).visible)
      throw std::runtime_error("density OFF");
    std::cout << "PASS authored-only import, exact heights, scale profiles, "
                 "culling, LOD and density\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
