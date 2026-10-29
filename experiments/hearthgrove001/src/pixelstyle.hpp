#pragma once
#include "core.hpp"
#include <stdexcept>
#include <string_view>
namespace p3 {
// Stage-one presentation switches. No source material/mesh mutation.
struct PixelStyleStudy {
  int mode = 0;     // A=0, B1=alpha, B2=light/edges, B3=both
  int sampling = 2; // 0 base LOD, 1 nearest mip, 2 filtered mip
  bool alpha() const { return (mode & 1) != 0; }
  bool lighting() const { return (mode & 2) != 0; }
  static int parse(std::string_view s) {
    if (s == "A")
      return 0;
    if (s == "B1")
      return 1;
    if (s == "B2")
      return 2;
    if (s == "B3")
      return 3;
    throw std::runtime_error("pixelstyle A|B1|B2|B3");
  }
  static int sample(std::string_view s) {
    if (s == "lod0")
      return 0;
    if (s == "nearest-mip")
      return 1;
    if (s == "filtered-mip")
      return 2;
    throw std::runtime_error("foliage-sampling lod0|nearest-mip|filtered-mip");
  }
};
inline bool pixelStyleLeaf(std::string_view model, const Material &m) {
  return model == "NATURE_CommonTree_1" && m.name == "Leaves_NormalTree";
}
inline bool pixelStyleRock(std::string_view model) {
  return model == "NATURE_Rock_Medium_1";
}
inline bool pixelStyleRig(std::string_view id, std::string_view model,
                          bool rig) {
  return rig || model == "TEST_GROUND" || id == "tree_0" ||
         id == "nature_rock_0" || id.starts_with("house_2_");
}
inline Camera pixelStyleCamera(int view, float time = -1) {
  Camera c;
  Vec target;
  if (view == 1) {
    c.position = {0, 1.65f, 14};
    target = {-4, 3.5f, 13};
  } else if (view == 2) {
    c.position = {4.8f, 1.65f, 11.5f};
    target = {3.3f, .7f, 7.7f};
  } else if (view == 3) {
    c.position = {9, 1.65f, 17};
    target = {-4, 3.3f, 13};
  } else {
    c.position = {4.5f, 1.65f, 16.5f};
    target = {-.5f, 2.5f, 10.5f};
  }
  if (time >= 0) {
    // Ten-second physical-height walk/strafe/look route, identical in every
    // mode.
    float t = std::clamp(time / 10.f, 0.f, 1.f);
    c.position = {4.5f - 4.f * std::sin(t * 3.14159265f), 1.65f,
                  16.5f - 3.f * std::sin(t * 3.14159265f)};
    target = {-.5f + 3.8f * t, 2.5f - 1.8f * t, 10.5f - 2.8f * t};
  }
  c.aim(target);
  return c;
}
} // namespace p3
