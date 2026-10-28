#include "mesh_clumps.hpp"
#include "json.hpp"
#include <algorithm>
#include <cctype>
#include <random>
namespace p3 {
MeshClumps::MeshClumps(const std::filesystem::path &r,
                       const std::vector<game::Solid> &solid, bool legacyLayout,
                       bool forceLegacy, int scaleFamily)
    : legacy(legacyLayout) {
  if (!forceLegacy && std::filesystem::is_regular_file(r / "vegetation.json")) {
    auto cfg = ws::parseJson(readText(r / "vegetation.json"));
    if (cfg.at("revision").number() != 1)
      throw std::runtime_error("vegetation revision");
    authored = receiveShadows = true;
    legacy = false;
    sunTint = float(cfg.at("sun_tint").number());
    shadowFloor = float(cfg.at("shadow_floor").number());
    if (!std::isfinite(shadowFloor) || shadowFloor < 0 || shadowFloor > .35f)
      throw std::runtime_error("receiver shadow floor");
    if (sunTint < 0 || sunTint > 1)
      throw std::runtime_error("sun tint range");
    groundMap = safePath(r, cfg.at("ground").string());
    patchMap = safePath(r, cfg.at("patch_ids").string());
    std::vector<ws::Json> rootBatches;
    const auto &object = std::get<ws::Json::Object>(cfg.value);
    if (auto step = object.find("ground_spacing"); step != object.end())
      tileSpacing = float(step->second.number());
    if (!std::isfinite(tileSpacing) || tileSpacing < 11.5f || tileSpacing > 12)
      throw std::runtime_error("ground tile spacing");
    if (auto files = object.find("instance_files"); files != object.end()) {
      if (files->second.array().size() > 64)
        throw std::runtime_error("vegetation chunk budget");
      for (const auto &file : files->second.array())
        rootBatches.push_back(
            ws::parseJson(readText(safePath(r, file.string()))));
    } else
      rootBatches.push_back(cfg.at("instances"));
    std::size_t population = 0;
    for (auto &batch : rootBatches) {
      population += batch.array().size();
      if (population > 16000)
        throw std::runtime_error("vegetation population budget");
    }
    if (cfg.at("models").array().empty() ||
        cfg.at("models").array().size() > 32)
      throw std::runtime_error("vegetation manifest budget");
    for (auto &entry : cfg.at("models").array()) {
      int near = int(models.size());
      models.push_back(Model::load(safePath(r, entry.at("near").string())));
      models.push_back(Model::load(safePath(r, entry.at("far").string())));
      farVariants.push_back(near + 1);
      farVariants.push_back(near + 1);
    }
    if (models.empty() || models.size() > 64)
      throw std::runtime_error("vegetation model budget");
    for (auto &m : models)
      if (m.parts.size() != 1 || m.materials.size() != 1 ||
          m.materials[0].alpha != Alpha::Opaque)
        throw std::runtime_error(
            "authored clumps require one opaque primitive");
    int color = 0;
    if (cfg.at("families").array().size() != 4)
      throw std::runtime_error("four palette families required");
    for (auto &family : cfg.at("families").array()) {
      if (family.array().size() != 3)
        throw std::runtime_error("root mid tip palette required");
      for (auto &item : family.array()) {
        auto h = item.string();
        if (h.size() != 7 || h[0] != '#' || color >= 12)
          throw std::runtime_error("family palette");
        for (std::size_t i = 1; i < h.size(); i++)
          if (!std::isxdigit(static_cast<unsigned char>(h[i])))
            throw std::runtime_error("palette hex digits");
        Vec c;
        float *v[] = {&c.x, &c.y, &c.z};
        for (int k = 0; k < 3; k++) {
          float s = std::stoi(h.substr(1 + k * 2, 2), nullptr, 16) / 255.f;
          *v[k] =
              s <= .04045f ? s / 12.92f : std::pow((s + .055f) / 1.055f, 2.4f);
        }
        familyRamp[color++] = c;
      }
    }
    if (color != 12)
      throw std::runtime_error("four root/mid/tip families required");
    for (auto &batch : rootBatches)
      for (auto &entry : batch.array()) {
        auto &p = entry.at("position").array();
        double variantValue = entry.at("variant").number(),
               familyValue = entry.at("family").number();
        if (variantValue < 0 || variantValue >= models.size() / 2 ||
            std::floor(variantValue) != variantValue || familyValue < 0 ||
            familyValue > 3 || std::floor(familyValue) != familyValue)
          throw std::runtime_error("vegetation integer indices");
        int variant = int(variantValue), family = int(familyValue);
        float width = float(entry.at("width").number()),
              height = float(entry.at("height").number());
        if (scaleFamily == 1) {
          width = 1;
          height = 1;
        } else if (scaleFamily == 2) {
          width = .88f;
          height = .82f;
        }
        if (p.size() != 3 || variant < 0 || variant * 2 >= int(models.size()) ||
            family < 0 || family > 3 || width < .6f || width > 1.3f ||
            height < .6f || height > 1.3f)
          throw std::runtime_error("vegetation placement bounds");
        Vec at{float(p[0].number()), float(p[1].number()),
               float(p[2].number())};
        if (!finite(at) || std::abs(at.x) > 18 || std::abs(at.z) > 18 ||
            std::abs(at.y) > 1)
          throw std::runtime_error("vegetation position");
        float yaw = float(entry.at("yaw").number());
        if (!std::isfinite(yaw))
          throw std::runtime_error("vegetation yaw");
        Mat m = transform(at, yaw, width);
        m.v[5] *= height / width;
        auto &normal = entry.at("normal").array();
        if (normal.size() != 3)
          throw std::runtime_error("ground normal");
        Vec n{float(normal[0].number()), float(normal[1].number()),
              float(normal[2].number())};
        if (!finite(n) || dot(n, n) < .25f || dot(n, n) > 4)
          throw std::runtime_error("ground normal range");
        n = normalized(n);
        all.push_back({m, {float(family), n.x, n.y, n.z}});
        variants.push_back(variant * 2);
        bounds.push_back(transformed(models[variant * 2].bounds, m));
      }
    if (all.empty() || all.size() > 16000)
      throw std::runtime_error("vegetation population budget");
    batches.resize(models.size());
    std::vector<std::size_t> capacities(models.size());
    for (int v : variants) {
      capacities[v]++;
      capacities[farVariants[v]]++;
    }
    for (std::size_t i = 0; i < batches.size(); i++)
      batches[i].reserve(capacities[i]);
    return;
  }
  models.resize(6);
  batches.resize(6);
  const char *files[] = {"WL_R3_FINE_A.glb",   "WL_R3_FINE_B.glb",
                         "WL_R3_FINE_FAR.glb", "WL_R5_SOFT_A.glb",
                         "WL_R5_SOFT_B.glb",   "WL_R5_SOFT_FAR.glb"};
  for (int i = 0; i < 6; i++) {
    models[i] = Model::load(r / files[i]);
    if (models[i].parts.size() != 1)
      throw std::runtime_error("grass requires one source primitive");
    batches[i].reserve(16000);
  }
  // Stable jittered sampling in coherent fields. Density profiles are nested;
  // no frame noise or camera-facing cards. Ground/root colors add distant
  // cover.
  std::mt19937 rng(47035);
  std::uniform_real_distribution<float> u(0, 1);
  float spacing = legacy ? .115f : .083f;
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
    if (density == 1 && i % (authored ? 2 : legacy ? 3 : 6) != 0)
      continue;
    if (legacy && density == 2 && i % 5 == 0)
      continue;
    auto p = point(all[i].matrix, {});
    float distance = std::sqrt(dot(p - camera.position, p - camera.position));
    if (distance > 18 || (options.cull && !f.visible(bounds[i], .05f))) {
      s.culled++;
      continue;
    }
    int variant = variants[i];
    if (authored) {
      float projected =
          height * (bounds[i].hi.y - bounds[i].lo.y) / std::max(distance, .5f);
      if (options.lod && projected < 10)
        variant = farVariants[variant];
      batches[variant].push_back(all[i]);
      s.visible++;
      if (variant % 2)
        s.farCount++;
      else
        s.nearCount++;
      continue;
    }
    float projected = height * .16f / std::max(distance, .5f);
    if (!legacy && density == 2 && distance > 5.5f && i % 4 == 0)
      continue;
    if (options.lod && projected < (legacy ? 5 : 3.2f)) {
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
