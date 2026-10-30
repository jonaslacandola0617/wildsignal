#pragma once
#include "core.hpp"
#include "game.hpp"
#include "meadow_patches.hpp"
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
  std::array<std::size_t,3> lodCount{};
};
struct MeshClumps {
  MeshOptions options;
  std::vector<Model> models;
  Vec baseAlbedo{.16f, .3f, .08f};
  std::vector<MeshTransform> all;
  std::vector<int> variants;
  std::vector<Box> bounds;
  std::vector<std::vector<MeshTransform>> batches;
  std::vector<std::array<int,3>> lodModels;
  std::vector<unsigned char> lodState;
  std::array<float,2> lodDistances{3,7};
  bool hasLods=false;
  MeadowPatches patches;
  std::array<Vec, 12> familyRamp{};
  std::filesystem::path groundMap, patchMap;
  std::filesystem::path sourcePalette;
  std::string assetFamily = "EMACEART_SOURCE_ONLY";
  std::array<Vec, 3> instanceGrades{Vec{1,1,1}, Vec{1,1,1}, Vec{1,1,1}};
  bool alignToSurface = false;
  float normalUpBias = .5f, ambientLift = .25f;
  bool authored = false, receiveShadows = false, fragmentReceiver = false;
  int diagnostic = 0;
  float sunTint = 1;
  float tileSpacing = 12;
  float shadowFloor = 0;
  int density = 2, coverage = 0;
  bool enabled = true, legacy = false;
  explicit MeshClumps(const std::filesystem::path &,
                      const std::vector<game::Solid> &,
                      bool legacyLayout = false, bool forceLegacy = false,
                      int scaleFamily = 0);
  MeshVisibility select(const Camera &, const Frustum &, int height);
};
} // namespace p3
