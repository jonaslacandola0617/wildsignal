#pragma once
#include "core.hpp"
namespace p3 {
struct Look {
  bool cozy = false;
  Vec sunDirection{-.25f, .85f, .55f}, sunColor{1.4f, 1.12f, .72f};
  Vec ambientSky{.46f, .54f, .66f}, ambientGround{.34f, .40f, .45f};
  Vec skyTop{.36f, .72f, .98f}, skyHorizon{.78f, .88f, .84f},
      hazeColor{.62f, .80f, .65f};
  float exposure = 1.12f, bandSoftness = .08f, contactStrength = .38f;
  float hazeStart = 24, hazeEnd = 95, hazeStrength = .45f;
  float outlineStrength = .6f, foliageOutlineStrength = .22f;
  static Look load(const std::filesystem::path &);
};
} // namespace p3
