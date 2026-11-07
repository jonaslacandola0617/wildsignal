#include "terrain_mesh.hpp"
#include <cmath>
#include <stdexcept>
#include <vector>
namespace p3 {
namespace {
Model emptyModel(const std::string &id, const std::array<float, 4> &colour) {
  Model m;
  m.id = id;
  Material mat;
  mat.name = "Terrain";
  mat.color = colour;
  m.materials.push_back(mat);
  m.parts.emplace_back();
  return m;
}
Vec normalOf(Vec a, Vec b, Vec c) {
  Vec u = b - a, v = c - a;
  Vec n{u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x};
  float l = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
  n = l > 0 ? n * (1 / l) : Vec{0, 1, 0};
  return n.y < 0 ? n * -1.f : n;
}
} // namespace
TerrainModels buildTerrain(const std::string &name, float x0, float z0, float size,
                           const TerrainSettings &s, const std::function<float(float, float)> &height) {
  const float cellsF = size / s.step;
  if (!(s.step > 0) || cellsF != std::floor(cellsF) || cellsF < 1 || cellsF > 256)
    throw std::runtime_error("terrain step must divide the chunk size");
  const int cells = int(cellsF), side = cells + 1;
  // Heights on the grid, read once.
  std::vector<Vec> grid(std::size_t(side) * side);
  for (int j = 0; j < side; j++)
    for (int i = 0; i < side; i++) {
      // Positions from the integer index, so both chunks of a border agree bit for bit.
      float x = x0 + float(i) * s.step, z = z0 + float(j) * s.step;
      float y = height(x, z);
      if (!std::isfinite(y))
        throw std::runtime_error("terrain height");
      grid[std::size_t(j) * side + i] = {x, y, z};
    }
  TerrainModels out;
  out.lawn = emptyModel("WS_TERRAIN_" + name, s.lawn);
  out.rock = emptyModel("WS_TERRAIN_ROCK_" + name, s.rock);
  // Each model keeps its own copy of the grid vertices it uses.
  std::vector<std::int64_t> lawnIndex(grid.size(), -1), rockIndex(grid.size(), -1);
  std::vector<Vec> lawnNormal, rockNormal;
  auto use = [&](Model &m, std::vector<std::int64_t> &index, std::vector<Vec> &normals, std::size_t g, Vec n) {
    auto &part = m.parts.front();
    if (index[g] < 0) {
      index[g] = std::int64_t(part.vertices.size());
      Vertex v;
      v.p = grid[g];
      v.u = grid[g].x;
      v.v = grid[g].z;
      part.vertices.push_back(v);
      normals.push_back({0, 0, 0});
    }
    normals[std::size_t(index[g])] = normals[std::size_t(index[g])] + n;
    part.indices.push_back(std::uint32_t(index[g]));
  };
  for (int j = 0; j < cells; j++)
    for (int i = 0; i < cells; i++) {
      const std::size_t a = std::size_t(j) * side + i, b = a + 1, c = a + side, d = c + 1;
      // Alternate the diagonal so slopes do not all lean one way.
      const bool flip = ((i + j) & 1) != 0;
      // Diagonal b-c, or a-d in the flipped cells.
      const std::size_t tris[2][3] = {{a, c, flip ? d : b}, {flip ? a : b, flip ? d : c, flip ? b : d}};
      for (auto &t : tris) {
        // Wind each triangle so it faces up.
        std::size_t p = t[0], q = t[1], r = t[2];
        Vec n = normalOf(grid[p], grid[q], grid[r]);
        Vec u = grid[q] - grid[p], v = grid[r] - grid[p];
        if (u.z * v.x - u.x * v.z < 0)
          std::swap(q, r);
        bool rock = n.y < s.cliffNormalY;
        if (rock)
          for (auto g : {p, q, r})
            use(out.rock, rockIndex, rockNormal, g, n);
        else
          for (auto g : {p, q, r})
            use(out.lawn, lawnIndex, lawnNormal, g, n);
      }
    }
  for (auto [m, normals] : {std::pair{&out.lawn, &lawnNormal}, std::pair{&out.rock, &rockNormal}}) {
    auto &part = m->parts.front();
    for (std::size_t k = 0; k < part.vertices.size(); k++) {
      Vec n = (*normals)[k];
      float l = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
      part.vertices[k].n = l > 0 ? n * (1 / l) : Vec{0, 1, 0};
      part.bounds.add(part.vertices[k].p);
    }
    part.retainedIndexCount = part.indices.size();
    if (!part.vertices.empty())
      m->bounds.add(part.bounds);
  }
  out.hasRock = !out.rock.parts.front().indices.empty();
  if (out.lawn.parts.front().indices.empty())
    throw std::runtime_error("terrain chunk " + name + " has no walkable ground");
  return out;
}
} // namespace p3
