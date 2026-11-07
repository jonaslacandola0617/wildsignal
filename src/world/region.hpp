#pragma once
#include "game.hpp"
#include "json.hpp"
#include <array>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>
namespace world {
// REGION-001: a region is a grid of square chunks (chapter 7.4). open() reads
// region.json; readChunk() reads one chunk and may run on a loading thread.
struct RegionSolid {
  game::Solid box;
  bool house = false;
};
struct ChunkData {
  int x = 0, z = 0;
  ws::Json::Array instances, grass;
  std::vector<std::pair<double, RegionSolid>> solids; // fixed order, collider
  int i0 = 0, j0 = 0, ni = 0, nj = 0;                  // height block in the lattice
  std::vector<float> heights;
  std::size_t bytes = 0;
};
struct Region {
  std::string id, name;
  std::filesystem::path dir;
  float chunkSize = 32, originX = 0, originZ = 0;
  int columns = 0, rows = 0;
  float groundSpacing = 12;
  // Height lattice. The step 1 village keeps its old square grid (half extent
  // + samples) so heights stay bit for bit; new regions give origin/step/count.
  bool legacyHeight = true;
  float heightHalf = 0, walkHalf = 0;
  float heightOriginX = 0, heightOriginZ = 0, heightStep = 1;
  int heightNX = 0, heightNZ = 0;
  float walkMinX = 0, walkMinZ = 0, walkMaxX = 0, walkMaxZ = 0;
  std::string pack, sourcePack, stylePack, vegetation;
  bool shadowFollow = false; // shadow area follows the player (bigger regions)
  // REGION-001 build 018: ground drawn from the height lattice instead of
  // ground tiles. Faces steeper than cliffNormalY are rock (cliff faces).
  struct Terrain {
    bool on = false;
    float step = .5f, cliffNormalY = .6f;
    std::array<float, 4> lawn{.40f, .62f, .20f, 1}, rock{.48f, .45f, .50f, 1};
  } terrain;
  ws::Json config;              // region.json: spawn, wild, rest spots, cameras, exclusions
  ws::Json::Array persistent;   // rows that never unload (the characters)
  struct Entry {
    int x = 0, z = 0;
    std::filesystem::path file;
  };
  std::vector<Entry> chunkFiles;
  int entryAt(int x, int z) const; // -1 when the grid has no chunk there
  std::pair<int, int> chunkOf(float x, float z) const;
  static Region open(const std::filesystem::path &assets, const std::string &id);
  ChunkData readChunk(std::size_t entry) const;

  // Step 1 whole-region load, kept for tests and tools: every chunk gathered.
  std::vector<float> heights;
  ws::Json::Array instances, grass;
  std::vector<RegionSolid> solids;
  std::size_t chunksLoaded = 0, bytesRead = 0;
  static Region load(const std::filesystem::path &assets, const std::string &id);
};
} // namespace world
