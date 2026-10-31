#pragma once
#include "core.hpp"
#include <array>
#include <filesystem>
namespace p3 {
// Visual pilot: palette ramps plus world-anchored pixel tiles, read from a
// private pilot.json. Source meshes, UVs and textures stay untouched.
struct PilotLook {
  bool enabled = false;
  std::filesystem::path tiles;
  float density = 32, pools = .24f, sweep = .2f, rays = 0, time = 0, wide = 480, high = 270;
  std::array<Vec, 36> ramps{};
  std::array<float, 6> amps{};
  Vec lichen{}, right{1, 0, 0};
  int tuft3d = 0;
};
} // namespace p3
