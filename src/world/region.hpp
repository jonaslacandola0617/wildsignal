#pragma once
#include "game.hpp"
#include "json.hpp"
#include <filesystem>
#include <string>
#include <vector>
namespace world {
// REGION-001: a region is a grid of square chunks (chapter 7.4). This first
// format step loads every chunk at start; streaming around the player follows.
struct RegionSolid {
  game::Solid box;
  bool house = false;
};
struct Region {
  std::string id, name;
  float chunkSize = 32, originX = 0, originZ = 0;
  int columns = 0, rows = 0;
  float walkHalf = 0, groundSpacing = 12, heightHalf = 0;
  int heightSamples = 0;
  std::vector<float> heights; // the whole lattice, rebuilt from the chunks
  std::string pack, sourcePack, stylePack, vegetation;
  ws::Json config; // region.json: spawn, wild, rest spots, cameras, exclusions
  ws::Json::Array instances, grass;
  std::vector<RegionSolid> solids;
  std::size_t chunksLoaded = 0, bytesRead = 0;
  static Region load(const std::filesystem::path &assets, const std::string &id);
};
} // namespace world
