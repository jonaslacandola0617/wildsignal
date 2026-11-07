#include "streaming.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <stdexcept>
namespace p3 {
namespace {
using Clock = std::chrono::steady_clock;
double msSince(Clock::time_point t) {
  return std::chrono::duration<double, std::milli>(Clock::now() - t).count();
}
bool isHouse(const ws::Json &row) { return row.at("id").string().starts_with("house_"); }
// How far past a chunk's edge the player must walk before the loaded area
// moves with them, so walking along a border does not load and unload.
constexpr float StickyMetres = 2;
// Each frame's shadow and grass work stays small; see Gpu progressive shadows.
} // namespace
RegionStream::RegionStream(world::Region region, bool houses, Hooks hooks)
    : region_(std::move(region)), houses_(houses), hooks_(std::move(hooks)) {}
RegionStream::~RegionStream() { stop(); }
void RegionStream::stop() {
  {
    std::lock_guard lock(mutex_);
    stop_ = true;
  }
  wake_.notify_all();
  if (thread_.joinable())
    thread_.join();
}
std::set<int> RegionStream::window(int cx, int cz, int radius) const {
  std::set<int> out;
  for (int z = cz - radius; z <= cz + radius; z++)
    for (int x = cx - radius; x <= cx + radius; x++)
      if (int e = region_.entryAt(x, z); e >= 0)
        out.insert(e);
  return out;
}
RegionStream::Prepared RegionStream::prepare(world::ChunkData &&d) const {
  Prepared out;
  for (auto &row : d.instances) {
    if (!houses_ && isHouse(row))
      continue;
    out.objects.push_back(placementOf(row, 1e17));
    out.models.insert(out.objects.back().model);
  }
  std::stable_sort(out.objects.begin(), out.objects.end(),
                   [](const Placement &a, const Placement &b) { return a.order < b.order; });
  for (auto &s : d.solids)
    if (houses_ || !s.second.house)
      out.solids.push_back(s);
  if (grass_)
    out.grass = grass_->prepare(d.grass, 0);
  out.i0 = d.i0;
  out.j0 = d.j0;
  out.ni = d.ni;
  out.nj = d.nj;
  out.heights = std::move(d.heights);
  return out;
}
void RegionStream::applyHeights(const Prepared &d) {
  if (d.heights.empty())
    return;
  if (!region_.legacyHeight) {
    game::setLatticeBlock(d.i0, d.j0, d.ni, d.nj, d.heights.data());
    return;
  }
  const std::size_t n = std::size_t(region_.heightNX);
  for (int j = 0; j < d.nj; j++)
    for (int i = 0; i < d.ni; i++)
      legacyHeights_[std::size_t(d.j0 + j) * n + std::size_t(d.i0 + i)] = d.heights[std::size_t(j * d.ni + i)];
}
RegionStream::ModelData RegionStream::loadModel(const std::string &id) const {
  ModelData out;
  out.model = pack_->load(id);
  for (auto &mat : out.model.materials)
    for (auto &file : {mat.texture, mat.styleTexture})
      if (!file.empty() && !out.images.contains(file))
        out.images.emplace(file, loadImage(file));
  return out;
}
bool RegionStream::neighboursKnown(int entry) const {
  const auto &f = region_.chunkFiles[std::size_t(entry)];
  for (int dz = -1; dz <= 1; dz++)
    for (int dx = -1; dx <= 1; dx++)
      if (int n = region_.entryAt(f.x + dx, f.z + dz); n >= 0 && !heightsKnown_.contains(n))
        return false;
  return true;
}
std::string RegionStream::terrainName(int entry) const {
  const auto &f = region_.chunkFiles[std::size_t(entry)];
  return std::to_string(f.x) + "_" + std::to_string(f.z);
}
bool RegionStream::terrainResident(int entry, const World &world) const {
  auto it = world.modelIds.find("WS_TERRAIN_" + terrainName(entry));
  return it != world.modelIds.end() && world.models[it->second].resident;
}
TerrainModels RegionStream::buildChunkTerrain(int entry) const {
  const auto &f = region_.chunkFiles[std::size_t(entry)];
  TerrainSettings s;
  s.step = region_.terrain.step;
  s.cliffNormalY = region_.terrain.cliffNormalY;
  s.lawn = region_.terrain.lawn;
  s.rock = region_.terrain.rock;
  auto m = buildTerrain(terrainName(entry), region_.originX + float(f.x) * region_.chunkSize,
                        region_.originZ + float(f.z) * region_.chunkSize, region_.chunkSize, s,
                        [](float x, float z) { return game::terrain(x, z); });
  if (hooks_.prepare) {
    hooks_.prepare(m.lawn);
    if (m.hasRock)
      hooks_.prepare(m.rock);
  }
  return m;
}
void RegionStream::placeTerrain(int entry, World &world, Live &live) {
  std::vector<std::string> ids{"WS_TERRAIN_" + terrainName(entry)};
  if (auto r = world.modelIds.find("WS_TERRAIN_ROCK_" + terrainName(entry));
      r != world.modelIds.end() && world.models[r->second].resident)
    ids.push_back(r->first);
  for (std::size_t k = 0; k < ids.size(); k++) {
    Placement p;
    p.model = p.id = ids[k];
    p.matrix = Mat::identity(); // the ground is built in world space
    p.order = -1e9 + double(entry) * 2 + double(k); // ground first, fixed by chunk
    auto inst = world.instanceAt(p);
    std::size_t slot;
    if (!freeSlots_.empty()) {
      std::sort(freeSlots_.begin(), freeSlots_.end(), std::greater<>());
      slot = freeSlots_.back();
      freeSlots_.pop_back();
      world.instances[slot] = std::move(inst);
    } else {
      slot = world.instances.size();
      world.instances.push_back(std::move(inst));
    }
    live.slots.push_back(slot);
    if (region_.chunkFiles[std::size_t(entry)].ring)
      ringSlots_.insert(slot);
    live.models[world.instances[slot].model]++;
    modelRefs_[world.instances[slot].model]++;
  }
}
World RegionStream::start(Vec at, std::shared_ptr<const Pack> pack) {
  pack_ = std::move(pack);
  if (region_.legacyHeight)
    legacyHeights_.assign(std::size_t(region_.heightNX) * std::size_t(region_.heightNZ), 0.f);
  else
    game::configureLattice(region_.heightOriginX, region_.heightOriginZ, region_.heightStep,
                           region_.heightNX, region_.heightNZ, region_.walkMinX,
                           region_.walkMinZ, region_.walkMaxX, region_.walkMaxZ);
  auto [cx, cz] = region_.chunkOf(at.x, at.z);
  centerX_ = cx;
  centerZ_ = cz;
  auto entries = window(cx, cz, 1);
  for (int e : entries) {
    auto d = region_.readChunk(std::size_t(e));
    startGrass_[e] = d.grass; // the grass exists only after start-up
    cached_[e] = prepare(std::move(d));
  }
  // Models in the pack's own order, so start-up matches the step 1 village.
  std::set<std::string> required;
  std::vector<Placement> objects;
  for (auto &row : region_.persistent) {
    objects.push_back(placementOf(row, 1e17));
    required.insert(objects.back().model);
  }
  const std::size_t persistent = objects.size();
  std::vector<int> owner(persistent, -1);
  for (int e : entries)
    for (auto &p : cached_[e].objects) {
      objects.push_back(p);
      owner.push_back(e);
      required.insert(p.model);
    }
  World w;
  w.pack = pack_;
  for (auto &entry : pack_->entries)
    if (required.contains(entry.first)) {
      auto m = pack_->load(entry.first);
      if (hooks_.prepare)
        hooks_.prepare(m);
      w.addModel(std::move(m));
    }
  for (auto &id : required)
    if (!w.modelIds.contains(id))
      throw std::runtime_error("missing model " + id);
  // Objects in their fixed order across the persistent rows and all chunks.
  std::vector<std::size_t> sequence(objects.size());
  for (std::size_t i = 0; i < sequence.size(); i++)
    sequence[i] = i;
  std::stable_sort(sequence.begin(), sequence.end(),
                   [&](std::size_t a, std::size_t b) { return objects[a].order < objects[b].order; });
  for (int e : entries)
    activeChunks_[e];
  for (auto k : sequence) {
    auto inst = w.instanceAt(objects[k]);
    if (hooks_.place)
      hooks_.place(w, inst);
    const std::size_t slot = w.instances.size();
    modelRefs_[inst.model]++;
    if (owner[k] >= 0) {
      if (hooks_.streamed)
        hooks_.streamed(w, inst);
      activeChunks_[owner[k]].slots.push_back(slot);
      if (region_.chunkFiles[std::size_t(owner[k])].ring)
        ringSlots_.insert(slot);
      activeChunks_[owner[k]].models[inst.model]++;
    } else {
      modelRefs_[inst.model]++; // the characters' models never unload
      persistentSlots_.insert(slot);
    }
    w.instances.push_back(std::move(inst));
  }
  for (int e : entries) {
    activeChunks_[e].solids = cached_[e].solids;
    applyHeights(cached_[e]);
    stats_.chunkLoads++;
  }
  if (region_.terrain.on) {
    // The start chunks' ground needs the ring around them: read only its heights
    // now (the chunks themselves are read ahead as usual once the game runs).
    for (int e : entries)
      heightsKnown_.insert(e);
    for (int e : window(cx, cz, 2))
      if (!heightsKnown_.contains(e)) {
        Prepared ring;
        auto d = region_.readChunk(std::size_t(e));
        ring.i0 = d.i0;
        ring.j0 = d.j0;
        ring.ni = d.ni;
        ring.nj = d.nj;
        ring.heights = std::move(d.heights);
        applyHeights(ring);
        heightsKnown_.insert(e);
      }
    for (int e : entries) {
      auto m = buildChunkTerrain(e);
      w.addModel(std::move(m.lawn));
      if (m.hasRock)
        w.addModel(std::move(m.rock));
      placeTerrain(e, w, activeChunks_[e]);
    }
  }
  if (region_.legacyHeight)
    game::configureHeights(region_.heightHalf, legacyHeights_, region_.groundSpacing);
  return w;
}
void RegionStream::attachGrass(MeshClumps *grass) {
  grass_ = grass;
  for (auto &[e, rows] : startGrass_) {
    cached_[e].grass = grass_->prepare(rows, 0);
    grass_->addPrepared(cached_[e].grass, e);
  }
  grass_->order();
  startGrass_.clear();
}
void RegionStream::attachGpu(Gpu *gpu) {
  gpu_ = gpu;
  thread_ = std::thread([this] { worker(); });
}
void RegionStream::request(Job job) {
  {
    std::lock_guard lock(mutex_);
    jobs_.push_back(std::move(job));
  }
  wake_.notify_one();
}
void RegionStream::worker() {
  for (;;) {
    Job job;
    std::vector<Prepared> trash;
    {
      std::unique_lock lock(mutex_);
      wake_.wait(lock, [&] { return stop_ || !jobs_.empty() || !trash_.empty(); });
      if (stop_)
        return;
      trash.swap(trash_);
      if (jobs_.empty())
        continue; // `trash` is freed here, off the game thread
      job = std::move(jobs_.front());
      jobs_.pop_front();
    }
    auto t = Clock::now();
    try {
      if (job.entry >= 0) {
        auto d = prepare(region_.readChunk(std::size_t(job.entry)));
        std::lock_guard lock(mutex_);
        readyChunks_.push_back({job.entry, std::move(d)});
        readTimes_.push_back(msSince(t));
      } else {
        auto m = loadModel(job.model);
        std::lock_guard lock(mutex_);
        readyModels_.push_back({job.model, std::move(m)});
        readTimes_.push_back(msSince(t));
      }
    } catch (const std::exception &e) {
      std::lock_guard lock(mutex_);
      errors_.push_back(e.what());
    }
  }
}
void RegionStream::activate(int entry, World &world) {
  auto &d = cached_.at(entry);
  auto &live = activeChunks_[entry];
  // Reuse the lowest free slots first: same history, same slots.
  std::sort(freeSlots_.begin(), freeSlots_.end(), std::greater<>());
  for (auto &p : d.objects) {
    auto inst = world.instanceAt(p);
    if (hooks_.place)
      hooks_.place(world, inst);
    if (hooks_.streamed)
      hooks_.streamed(world, inst);
    std::size_t slot;
    if (!freeSlots_.empty()) {
      slot = freeSlots_.back();
      freeSlots_.pop_back();
      world.instances[slot] = std::move(inst);
    } else {
      slot = world.instances.size();
      world.instances.push_back(std::move(inst));
    }
    live.slots.push_back(slot);
    if (region_.chunkFiles[std::size_t(entry)].ring)
      ringSlots_.insert(slot);
    live.models[world.instances[slot].model]++;
    modelRefs_[world.instances[slot].model]++;
  }
  live.solids = d.solids;
  if (grass_) {
    auto t = Clock::now();
    grass_->addPrepared(d.grass, entry);
    grass_->order();
    stats_.maxGrassMs = std::max(stats_.maxGrassMs, msSince(t));
  }
  applyHeights(d);
  if (region_.legacyHeight)
    game::configureHeights(region_.heightHalf, legacyHeights_, region_.groundSpacing);
  if (region_.terrain.on)
    placeTerrain(entry, world, live);
  stats_.chunkLoads++;
}
void RegionStream::deactivate(int entry, World &world) {
  auto it = activeChunks_.find(entry);
  if (it == activeChunks_.end())
    return;
  for (auto slot : it->second.slots) {
    auto &inst = world.instances[slot];
    inst.active = false;
    inst.palette.clear();
    ringSlots_.erase(slot);
    freeSlots_.push_back(slot);
  }
  for (auto [model, count] : it->second.models)
    modelRefs_[model] -= count;
  if (grass_)
    grass_->removeChunk(entry);
  activeChunks_.erase(it);
  stats_.chunkUnloads++;
}
bool RegionStream::update(Vec player, World &world) {
  auto begin = Clock::now();
  // Collect the loading thread's results.
  std::vector<std::pair<int, Prepared>> chunks;
  std::vector<std::pair<std::string, ModelData>> models;
  {
    std::lock_guard lock(mutex_);
    if (!errors_.empty())
      throw std::runtime_error("region streaming: " + errors_.front());
    chunks.swap(readyChunks_);
    models.swap(readyModels_);
    for (double t : readTimes_) {
      stats_.readMs = t;
      stats_.maxReadMs = std::max(stats_.maxReadMs, t);
    }
    readTimes_.clear();
    stats_.pending = jobs_.size();
  }
  // Move the loaded area only once the player is well inside another chunk.
  auto [px, pz] = region_.chunkOf(player.x, player.z);
  if (px != centerX_ || pz != centerZ_) {
    const float x0 = region_.originX + float(centerX_) * region_.chunkSize,
                z0 = region_.originZ + float(centerZ_) * region_.chunkSize,
                x1 = x0 + region_.chunkSize, z1 = z0 + region_.chunkSize;
    const float outside = std::max({x0 - player.x, player.x - x1, z0 - player.z, player.z - z1});
    if (outside >= StickyMetres) {
      centerX_ = px;
      centerZ_ = pz;
    }
  }
  const auto desired = window(centerX_, centerZ_, 1), keep = window(centerX_, centerZ_, 2);
  stats_.maxCollectMs = std::max(stats_.maxCollectMs, msSince(begin));
  std::vector<Prepared> dropped;
  for (auto &[e, d] : chunks) {
    requested_.erase(e);
    if (keep.contains(e)) {
      cached_[e] = std::move(d);
      if (region_.terrain.on) { // neighbours' ground needs these border heights
        applyHeights(cached_[e]);
        heightsKnown_.insert(e);
      }
    } else
      dropped.push_back(std::move(d));
  }
  for (auto &[id, m] : models) {
    modelsRequested_.erase(id);
    pendingModels_.emplace(id, std::move(m));
  }
  for (int e : keep)
    if (!cached_.contains(e) && !requested_.contains(e)) {
      requested_.insert(e);
      request({e, {}});
    }
  auto dropStart = Clock::now();
  for (auto it = cached_.begin(); it != cached_.end();)
    if (keep.contains(it->first) || activeChunks_.contains(it->first))
      ++it;
    else {
      dropped.push_back(std::move(it->second));
      it = cached_.erase(it);
    }
  if (!dropped.empty()) { // freed on the loading thread
    {
      std::lock_guard lock(mutex_);
      for (auto &d : dropped)
        trash_.push_back(std::move(d));
    }
    wake_.notify_one();
  }
  stats_.maxDropMs = std::max(stats_.maxDropMs, msSince(dropStart));
  bool changed = false;
  std::vector<int> leaving;
  for (auto &[e, live] : activeChunks_)
    if (!desired.contains(e))
      leaving.push_back(e);
  for (int e : leaving) {
    auto t = Clock::now();
    deactivate(e, world);
    stats_.maxDeactivateMs = std::max(stats_.maxDeactivateMs, msSince(t));
    changed = true;
  }
  // One heavy step per frame: a model upload or a chunk switched on.
  bool stepped = false;
  for (int e : desired) {
    if (stepped || activeChunks_.contains(e) || !cached_.contains(e))
      continue;
    if (region_.terrain.on && !neighboursKnown(e))
      continue;
    bool missing = false;
    for (auto &id : cached_[e].models) {
      if (auto m = world.modelIds.find(id); m != world.modelIds.end() && world.models[m->second].resident)
        continue;
      missing = true;
      if (auto ready = pendingModels_.find(id); ready != pendingModels_.end()) {
        auto data = std::move(ready->second);
        pendingModels_.erase(ready);
        auto t = Clock::now();
        if (hooks_.prepare)
          hooks_.prepare(data.model);
        auto slot = world.addModel(std::move(data.model));
        gpu_->uploadModel(world, slot, &data.images, true);
        if (hooks_.uploaded)
          hooks_.uploaded(slot);
        stats_.maxUploadMs = std::max(stats_.maxUploadMs, msSince(t));
        stats_.modelLoads++;
        stepped = true;
        break;
      }
      if (!modelsRequested_.contains(id)) {
        if (!pack_->has(id))
          throw std::runtime_error("missing model " + id);
        modelsRequested_.insert(id);
        request({-1, id});
      }
    }
    if (!missing && !stepped && region_.terrain.on && !terrainResident(e, world)) {
      // Building and uploading a chunk's ground is this frame's heavy step.
      auto t = Clock::now();
      auto m = buildChunkTerrain(e);
      for (Model *part : {&m.lawn, m.hasRock ? &m.rock : nullptr})
        if (part) {
          auto slot = world.addModel(std::move(*part));
          gpu_->uploadModel(world, slot, nullptr, true);
          if (hooks_.uploaded)
            hooks_.uploaded(slot);
          stats_.modelLoads++;
        }
      stats_.maxUploadMs = std::max(stats_.maxUploadMs, msSince(t));
      stepped = true;
      continue;
    }
    if (!missing && !stepped) {
      auto t = Clock::now();
      activate(e, world);
      stats_.maxActivateMs = std::max(stats_.maxActivateMs, msSince(t));
      changed = stepped = true;
    }
  }
  // Free models no active chunk uses and no chunk about to switch on needs.
  std::set<std::string> soonIds;
  for (int e : desired)
    if (auto c = cached_.find(e); c != cached_.end())
      soonIds.insert(c->second.models.begin(), c->second.models.end());
  for (auto it = modelRefs_.begin(); it != modelRefs_.end();) {
    if (it->second <= 0 && !soonIds.contains(world.models[it->first].id) && world.models[it->first].resident) {
      auto t = Clock::now();
      gpu_->releaseModel(world, it->first);
      stats_.maxReleaseMs = std::max(stats_.maxReleaseMs, msSince(t));
      stats_.modelUnloads++;
      it = modelRefs_.erase(it);
    } else
      ++it;
  }
  for (auto it = pendingModels_.begin(); it != pendingModels_.end();)
    it = soonIds.contains(it->first) ? std::next(it) : pendingModels_.erase(it);
  stats_.chunksActive = activeChunks_.size();
  stats_.chunksCached = cached_.size();
  stats_.modelsResident = std::size_t(std::count_if(world.models.begin(), world.models.end(),
                                                    [](const Model &m) { return m.resident; }));
  stats_.instancesActive = std::size_t(std::count_if(world.instances.begin(), world.instances.end(),
                                                     [](const Instance &i) { return i.active; }));
  stats_.slots = world.instances.size();
  stats_.centerX = centerX_;
  stats_.centerZ = centerZ_;
  stats_.applyMs = msSince(begin);
  stats_.maxApplyMs = std::max(stats_.maxApplyMs, stats_.applyMs);
  return changed;
}
std::vector<game::Solid> RegionStream::solids() const {
  std::vector<std::pair<double, world::RegionSolid>> all;
  for (auto &[e, live] : activeChunks_)
    all.insert(all.end(), live.solids.begin(), live.solids.end());
  std::stable_sort(all.begin(), all.end(), [](const auto &a, const auto &b) { return a.first < b.first; });
  std::vector<game::Solid> out;
  for (auto &s : all)
    out.push_back(s.second.box);
  return out;
}
std::vector<std::string> RegionStream::overlay(Vec player) const {
  std::vector<std::string> lines;
  lines.push_back("CHUNKS " + region_.id + "  # ON  + READ AHEAD  . NOT LOADED  @ YOU");
  auto [px, pz] = region_.chunkOf(player.x, player.z);
  for (int z = 0; z < region_.rows; z++) {
    std::string row = "  ";
    for (int x = 0; x < region_.columns; x++) {
      int e = region_.entryAt(x, z);
      char c = e < 0 ? ' ' : activeChunks_.contains(e) ? '#' : cached_.contains(e) ? '+' : '.';
      if (x == px && z == pz)
        c = '@';
      row += c;
      row += ' ';
    }
    lines.push_back(row);
  }
  auto fmt = [](double v) {
    char b[32];
    std::snprintf(b, sizeof b, "%.1f", v);
    return std::string(b);
  };
  lines.push_back("ON " + std::to_string(stats_.chunksActive) + " / AHEAD " +
                  std::to_string(stats_.chunksCached) + " / LOADS " + std::to_string(stats_.chunkLoads) +
                  " / UNLOADS " + std::to_string(stats_.chunkUnloads));
  lines.push_back("MODELS " + std::to_string(stats_.modelsResident) + " IN MEMORY / LOADED " +
                  std::to_string(stats_.modelLoads) + " / FREED " + std::to_string(stats_.modelUnloads));
  lines.push_back("OBJECTS " + std::to_string(stats_.instancesActive) + " / READ " + fmt(stats_.readMs) +
                  " MS (MAX " + fmt(stats_.maxReadMs) + ") / STEP MAX " + fmt(stats_.maxApplyMs) + " MS");
  return lines;
}
} // namespace p3
