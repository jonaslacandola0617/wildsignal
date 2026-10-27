#include "mesh_clumps.hpp"
#include <random>
namespace p3 {
MeshClumps::MeshClumps(const std::filesystem::path &r,
                       const std::vector<game::Solid> &solid) {
  const char *files[] = {"WL_R3_FINE_A.glb",   "WL_R3_FINE_B.glb",
                         "WL_R3_FINE_FAR.glb", "WL_R5_SOFT_A.glb",
                         "WL_R5_SOFT_B.glb",   "WL_R5_SOFT_FAR.glb"};
  for (int i = 0; i < 6; i++) {
    models[i] = Model::load(r / files[i]);
    if (models[i].parts.size() != 1)
      throw std::runtime_error("grass requires one source primitive");
    batches[i].reserve(12000);
  }
  // Stable jittered sampling in coherent fields. Density profiles are nested;
  // no frame noise or camera-facing cards. Ground/root colors add distant
  // cover.
  std::mt19937 rng(47035);
  std::uniform_real_distribution<float> u(0, 1);
  float spacing = .115f;
  for (float z = -5.75f; z < 5.75f; z += spacing)
    for (float x = -5.75f; x < 5.75f; x += spacing) {
      Vec p{x + (u(rng) - .5f) * spacing * .8f, 0,
            z + (u(rng) - .5f) * spacing * .8f};
      if (!game::meadow(p.x, p.z, solid))
        continue;
      p.y = game::terrain(p.x, p.z);
      int variant =
          int(all.size() % 10) < 3 ? 3 + int(rng() % 2) : int(rng() % 2);
      float scale = .83f + u(rng) * .24f;
      auto m = transform(p, u(rng) * 2 * pi, scale);
      if (variant < 3)
        m.v[5] *= .78f;
      all.push_back({m});
      variants.push_back(variant);
      bounds.push_back(transformed(models[variant].bounds, m));
    }
  if (all.size() > 16000 || all.empty())
    throw std::runtime_error("grass population budget");
}
MeshVisibility MeshClumps::select(const Camera &camera, const Frustum &f,
                                  int height) {
  for (auto &b : batches)
    b.clear();
  MeshVisibility s;
  s.total = all.size();
  if (!enabled || density == 0)
    return s;
  for (std::size_t i = 0; i < all.size(); i++) {
    if (density == 1 && i % 3 != 0)
      continue;
    if (density == 2 && i % 5 == 0)
      continue;
    auto p = point(all[i].matrix, {});
    float distance = std::sqrt(dot(p - camera.position, p - camera.position));
    if (distance > 18 || (options.cull && !f.visible(bounds[i], .05f))) {
      s.culled++;
      continue;
    }
    int variant = variants[i];
    float projected = height * .16f / std::max(distance, .5f);
    if (options.lod && projected < 5) {
      if (i % 2)
        continue;
      variant = variant >= 3 ? 5 : 2;
    }
    batches[variant].push_back(all[i]);
    s.visible++;
    if (variant == 2 || variant == 5)
      s.farCount++;
    else
      s.nearCount++;
  }
  return s;
}
} // namespace p3
