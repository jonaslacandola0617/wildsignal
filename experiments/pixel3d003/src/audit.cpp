#include "core.hpp"
#include "json.hpp"
#include <algorithm>
#include <iostream>
int main(int argc, char **argv) {
  try {
    if (argc != 2 && argc != 3)
      throw std::runtime_error(
          "pixel3d_audit model.gltf OR pack.json scene.json");
    if (argc == 2) {
      auto m = p3::Model::load(argv[1]);
      float colorMin = 1, colorMax = 0;
      for (auto &part : m.parts)
        for (auto &v : part.vertices) {
          colorMin = std::min(colorMin, v.color[0]);
          colorMax = std::max(colorMax, v.color[1]);
        }
      std::cout << "{\"vertex_color_min_r\":" << colorMin
                << ",\"vertex_color_max_g\":" << colorMax
                << ",\"triangles\":" << m.triangles()
                << ",\"primitives\":" << m.parts.size()
                << ",\"materials\":" << m.materials.size() << ",\"bounds\":[["
                << m.bounds.lo.x << ',' << m.bounds.lo.y << ',' << m.bounds.lo.z
                << "],[" << m.bounds.hi.x << ',' << m.bounds.hi.y << ','
                << m.bounds.hi.z << "]]}\n";
    } else {
      auto w = p3::World::load(argv[1], argv[2], false);
      std::uint64_t n = 0;
      for (auto &i : w.instances)
        n += w.models[i.model].triangles();
      std::cout << "{\"models\":" << w.models.size()
                << ",\"instances\":" << w.instances.size()
                << ",\"chunks\":" << w.chunks.size() << ",\"triangles\":" << n
                << "}\n";
    }
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "ASSET_REJECTED: " << e.what() << '\n';
    return 1;
  }
}
