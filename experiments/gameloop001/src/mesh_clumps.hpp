#pragma once
#include "core.hpp"
#include "game.hpp"
namespace p3 {
struct MeshOptions {
  int profile = 2;
  bool cull = true, lod = true;
};
struct MeshTransform {
  Mat matrix;
  std::array<float, 4> tint{1, 1, 1, 1};
};
struct MeshVisibility {
  std::size_t total = 0, visible = 0, nearCount = 0, farCount = 0, culled = 0,
              draws = 0, triangles = 0, shadowDraws = 0, shadowTriangles = 0;
  double cpuMs = 0;
};
struct MeshClumps {
  MeshOptions options;
  std::array<Model, 6> models;
  Vec baseAlbedo{.16f, .3f, .08f};
  std::vector<MeshTransform> all;
  std::vector<int> variants;
  std::vector<Box> bounds;
  std::array<std::vector<MeshTransform>, 6> batches;
  int density = 2, coverage = 0;
  bool enabled = true;
  explicit MeshClumps(const std::filesystem::path &,
                      const std::vector<game::Solid> &);
  MeshVisibility select(const Camera &, const Frustum &, int height);
};
} // namespace p3
