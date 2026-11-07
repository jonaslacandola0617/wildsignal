#pragma once
#include "core.hpp"
#include "gpu.hpp"
#include "mesh_clumps.hpp"
#include "region.hpp"
#include "terrain_mesh.hpp"
#include <condition_variable>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>
namespace p3 {
struct StreamStats {
  std::size_t chunksActive = 0, chunksCached = 0, chunkLoads = 0, chunkUnloads = 0,
              modelsResident = 0, modelLoads = 0, modelUnloads = 0, pending = 0,
              instancesActive = 0, slots = 0;
  double applyMs = 0, maxApplyMs = 0, readMs = 0, maxReadMs = 0;
  double maxUploadMs = 0, maxActivateMs = 0, maxDeactivateMs = 0, maxReleaseMs = 0;
  double maxDropMs = 0, maxGrassMs = 0, maxCollectMs = 0;
  int centerX = 0, centerZ = 0;
};
// REGION-001 step 2 (chapter 7.4): the 3 x 3 chunks around the player are
// switched on; the 5 x 5 ring around them is read ahead on a loading thread,
// which also turns each chunk file into ready-to-use lists, so the game
// thread only switches finished data on. Anything further is forgotten.
// Models are shared by the whole region and counted: a model loads when an
// active chunk needs it and is freed when none does. The game thread does at
// most one heavy step (a model upload or a chunk switched on) per frame.
class RegionStream {
public:
  struct Hooks {
    std::function<void(Model &)> prepare;                 // classify materials before upload
    std::function<void(const World &, Instance &)> place; // per-object fix-ups (kit relief)
    std::function<void(std::size_t model)> uploaded;      // surface policy etc.
    // Called only for objects that belong to a chunk (they unload, and their
    // slot is reused). The game rejects actors here: anything it moves by slot
    // must be a persistent row.
    std::function<void(const World &, const Instance &)> streamed;
  };
  RegionStream(world::Region region, bool houses, Hooks hooks);
  ~RegionStream();
  void stop(); // ends the loading thread (also done on destruction)
  // Start-up, before the GPU exists: the chunks around `at` and every model
  // they use, loaded on this thread.
  World start(Vec at, std::shared_ptr<const Pack> pack);
  // The start chunks' grass, then the GPU (which also starts the loading thread).
  void attachGrass(MeshClumps *grass);
  void attachGpu(Gpu *gpu);
  // Each frame. Returns true when objects were switched on or off.
  bool update(Vec player, World &world);
  // Colliders of the active chunks in their fixed order (houses dropped when off).
  std::vector<game::Solid> solids() const;
  const StreamStats &stats() const { return stats_; }
  // True for the slots of persistent rows: they are never switched off or reused.
  bool persistent(std::size_t slot) const { return persistentSlots_.contains(slot); }
  // True for objects of edge-ring chunks (build 019): scenery beyond the walk bounds.
  bool ringScenery(std::size_t slot) const { return ringSlots_.contains(slot); }
  // Text map for the F11 overlay: one character per chunk.
  std::vector<std::string> overlay(Vec player) const;

private:
  struct Prepared {
    std::vector<Placement> objects; // fixed order; houses dropped when off
    std::vector<std::pair<double, world::RegionSolid>> solids;
    MeshClumps::Prepared grass;
    int i0 = 0, j0 = 0, ni = 0, nj = 0;
    std::vector<float> heights;
    std::set<std::string> models;
  };
  struct ModelData {
    Model model;
    std::map<std::filesystem::path, Image> images;
  };
  struct Job {
    int entry = -1;    // chunk to read, or
    std::string model; // model to load
  };
  struct Live { // an active chunk
    std::vector<std::size_t> slots;
    std::vector<std::pair<double, world::RegionSolid>> solids;
    std::map<std::size_t, int> models; // model slot -> count
  };
  Prepared prepare(world::ChunkData &&) const;
  void worker();
  void request(Job);
  std::set<int> window(int cx, int cz, int radius) const;
  void applyHeights(const Prepared &);
  void activate(int entry, World &world);
  void deactivate(int entry, World &world);
  ModelData loadModel(const std::string &id) const;
  // Build 018 terrain: a chunk's ground needs its neighbours' border heights,
  // so it switches on only once every neighbour's heights are in the lattice.
  bool neighboursKnown(int entry) const;
  std::string terrainName(int entry) const;
  bool terrainResident(int entry, const World &world) const;
  TerrainModels buildChunkTerrain(int entry) const;
  void placeTerrain(int entry, World &world, Live &live);

  world::Region region_;
  std::set<int> heightsKnown_; // chunks whose heights are in the lattice
  bool houses_ = true;
  Hooks hooks_;
  Gpu *gpu_ = nullptr;
  MeshClumps *grass_ = nullptr;
  std::shared_ptr<const Pack> pack_;
  std::vector<float> legacyHeights_;
  std::map<int, Prepared> cached_;
  std::map<int, ws::Json::Array> startGrass_; // start chunks' grass, until the grass exists
  std::map<int, Live> activeChunks_;
  std::map<std::size_t, int> modelRefs_;
  std::vector<std::size_t> freeSlots_;
  std::set<std::size_t> persistentSlots_;
  std::set<std::size_t> ringSlots_;
  std::set<int> requested_;
  std::set<std::string> modelsRequested_;
  std::map<std::string, ModelData> pendingModels_; // read ahead, waiting to upload
  int centerX_ = 0, centerZ_ = 0;
  StreamStats stats_;
  // Loading thread.
  std::thread thread_;
  mutable std::mutex mutex_;
  std::condition_variable wake_;
  std::deque<Job> jobs_;
  std::vector<std::pair<int, Prepared>> readyChunks_;
  std::vector<std::pair<std::string, ModelData>> readyModels_;
  std::vector<Prepared> trash_; // dropped chunk data, freed on the loading thread
  std::vector<std::string> errors_;
  std::vector<double> readTimes_;
  bool stop_ = false;
};
} // namespace p3
