#include "core.hpp"
#include "json.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
int main(int argc, char **argv) {
  try {
    if (argc == 5 && std::string(argv[1]) == "--density") {
      auto w = p3::World::load(argv[2], argv[3], false);
      struct Metric {
        double area = 0, uvArea = 0;
        std::size_t triangles = 0;
      };
      std::map<std::string, Metric> metrics;
      for (auto &instance : w.instances) {
        auto &model = w.models[instance.model];
        for (auto &part : model.parts) {
          auto &mat = model.materials.at(part.material);
          if (!mat.architecture)
            continue;
          auto &entry = metrics[mat.texture.filename().string()];
          for (std::size_t i = 0; i < part.indices.size(); i += 3) {
            auto a = part.vertices.at(part.indices[i]),
                 b = part.vertices.at(part.indices[i + 1]),
                 c = part.vertices.at(part.indices[i + 2]);
            auto e1 = p3::point(instance.matrix, b.p) -
                      p3::point(instance.matrix, a.p),
                 e2 = p3::point(instance.matrix, c.p) -
                      p3::point(instance.matrix, a.p);
            auto cross = p3::cross(e1, e2);
            double area = .5 * std::sqrt(p3::dot(cross, cross));
            double uv = .5 * std::abs((b.u - a.u) * (c.v - a.v) -
                                      (c.u - a.u) * (b.v - a.v));
            if (area > 1e-8) {
              entry.area += area;
              entry.uvArea += uv;
              entry.triangles++;
            }
          }
        }
      }
      std::ofstream out(argv[4]);
      out << "[";
      bool first = true;
      for (auto &[name, m] : metrics) {
        if (!first)
          out << ',';
        first = false;
        out << "{\"texture\":" << ws::quoteJson(name)
            << ",\"world_area\":" << m.area << ",\"uv_area\":" << m.uvArea
            << ",\"uv_per_world_rms\":" << std::sqrt(m.uvArea / m.area)
            << ",\"triangles\":" << m.triangles << "}";
      }
      out << "]";
      if (!out)
        throw std::runtime_error("density output failed");
      return 0;
    }
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
                << m.bounds.hi.z << "]],\"material_data\":[";
      for (std::size_t i=0;i<m.materials.size();i++) {
        if(i)std::cout<<',';
        const auto &a=m.materials[i];
        std::cout<<"{\"name\":"<<ws::quoteJson(a.name)<<",\"color\":[";
        for(int k=0;k<4;k++){if(k)std::cout<<',';std::cout<<a.color[k];}
        std::cout<<"],\"emissive\":[";
        for(int k=0;k<3;k++){if(k)std::cout<<',';std::cout<<a.emissive[k];}
        std::cout<<"],\"alpha\":"<<int(a.alpha)<<'}';
      }
      std::cout<<"],\"groups\":[";
      for(std::size_t i=0;i<m.parts.size();i++){
        if(i)std::cout<<',';
        std::cout<<"{\"name\":"<<ws::quoteJson(m.parts[i].group)
                 <<",\"triangles\":"<<m.parts[i].indices.size()/3
                 <<",\"material\":"<<m.parts[i].material<<'}';
      }
      std::cout<<"]}\n";
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
