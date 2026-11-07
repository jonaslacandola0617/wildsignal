#pragma once
#include "core.hpp"
#include <array>
#include <functional>
#include <string>
namespace p3 {
// REGION-001 build 018: the ground of one chunk square drawn from the height
// lattice. Gentle faces become the lawn model (shaded like the ground tiles);
// faces whose normal points less than cliffNormalY upward become the rock
// model (shaded like the dream look's rocks, mossy where they face up).
// Vertices are in world space; both chunks of a border compute the border
// vertices from the same height function, so neighbouring squares meet exactly.
struct TerrainSettings {
  float step = .5f, cliffNormalY = .6f;
  std::array<float, 4> lawn{.40f, .62f, .20f, 1}, rock{.48f, .45f, .50f, 1};
};
struct TerrainModels {
  Model lawn, rock;
  bool hasRock = false;
};
// Model ids: "WS_TERRAIN_<name>" and "WS_TERRAIN_ROCK_<name>".
TerrainModels buildTerrain(const std::string &name, float x0, float z0, float size,
                           const TerrainSettings &, const std::function<float(float, float)> &height);
} // namespace p3
