#pragma once
#include "atmosphere.hpp"
#include <algorithm>
#include <stdexcept>
namespace p3 {
struct Integration {
  int edges = 0; // 0 exact legacy, 1 off, 2 highlight-only, 3 combined
  int highlightMask = 3; // architecture=1, foliage=2
  float architectureStrength = .30f, foliageStrength = .24f;
  float sunThreshold = .18f, contactStrength = .12f;
  Vec highlightColor{1.f, .72f, .32f};
  bool edgeMask = false, stableShadow = false, boundedAir = false;
  bool wordlineContacts = false;
  void validate() const {
    if (edges < 0 || edges > 3 || highlightMask < 1 || highlightMask > 3 ||
        !std::isfinite(architectureStrength) || architectureStrength < 0 || architectureStrength > .6f ||
        !std::isfinite(foliageStrength) || foliageStrength < 0 || foliageStrength > .6f ||
        !std::isfinite(sunThreshold) || sunThreshold < 0 || sunThreshold > 1 ||
        !std::isfinite(contactStrength) || contactStrength < 0 || contactStrength > .25f ||
        !finite(highlightColor) || std::min({highlightColor.x,highlightColor.y,highlightColor.z}) < 0 ||
        std::max({highlightColor.x,highlightColor.y,highlightColor.z}) > 1)
      throw std::runtime_error("invalid bounded edge/light configuration");
  }
};
inline Mat proofLightProjection(Vec sun, bool stable) {
  // Still fixed in world space. 28x28 covers this seven-model garden; this is
  // a bounded proof volume, not a camera-following cascade or world solution.
  Mat m=lightProjection(sun);
  if(stable){Mat scale=Mat::identity();scale.v[0]=scale.v[5]=25.f/14.f;m=scale*m;}
  return m;
}
// Signed two-plane test: points on each face must be inside the other face's
// outward half-space. Coplanar, concave and near-parallel pairs are rejected.
inline bool convexPair(Vec n, Vec qn, Vec delta, float tolerance) {
  float agreement = dot(n,qn);
  return agreement < .82f && agreement > -.5f &&
         dot(n,delta) < -tolerance && dot(qn,delta*-1) < -tolerance;
}
} // namespace p3
