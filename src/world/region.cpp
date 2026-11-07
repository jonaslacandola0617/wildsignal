#include "region.hpp"
#include "core.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <set>
#include <stdexcept>
namespace world {
namespace {
float finiteNumber(const ws::Json &j, const char *what) {
  double v = j.number();
  if (!std::isfinite(v))
    throw std::runtime_error(std::string("region ") + what);
  return float(v);
}
int whole(const ws::Json &j, int lo, int hi, const char *what) {
  double v = j.number();
  if (v != std::floor(v) || v < lo || v > hi)
    throw std::runtime_error(std::string("region ") + what);
  return int(v);
}
const ws::Json *field(const ws::Json &j, const char *name) {
  const auto &fields = std::get<ws::Json::Object>(j.value);
  auto it = fields.find(name);
  return it == fields.end() ? nullptr : &it->second;
}
// Rows keep a fixed "order" so draws and collisions never depend on which
// chunk was read first; rows without one follow in file order.
double order(const ws::Json &row) {
  auto o = field(row, "order");
  if (!o)
    return 1e18;
  double n = o->number();
  if (!std::isfinite(n) || n < 0 || n != std::floor(n))
    throw std::runtime_error("region row order");
  return n;
}
std::uint32_t u32(const std::string &b, std::size_t at) {
  std::uint32_t n = 0;
  for (int i = 0; i < 4; i++)
    n |= std::uint32_t(static_cast<unsigned char>(b[at + i])) << (8 * i);
  return n;
}
} // namespace
std::pair<int, int> Region::chunkOf(float x, float z) const {
  return {int(std::floor((x - originX) / chunkSize)), int(std::floor((z - originZ) / chunkSize))};
}
int Region::entryAt(int x, int z) const {
  for (std::size_t i = 0; i < chunkFiles.size(); i++)
    if (chunkFiles[i].x == x && chunkFiles[i].z == z)
      return int(i);
  return -1;
}
Region Region::open(const std::filesystem::path &assets, const std::string &id) {
  if (id.empty() || id.size() > 64 ||
      id.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_") != std::string::npos)
    throw std::runtime_error("region id");
  Region r;
  r.dir = p3::safePath(assets, "regions/" + id);
  const auto text = p3::readText(r.dir / "region.json", 1024 * 1024);
  r.bytesRead += text.size();
  r.config = ws::parseJson(text);
  const auto &c = r.config;
  if (c.at("revision").number() != 1)
    throw std::runtime_error("region revision");
  if (c.at("id").string() != id)
    throw std::runtime_error("region id differs from folder");
  r.id = id;
  r.name = c.at("name").string();
  r.chunkSize = finiteNumber(c.at("chunk_size"), "chunk size");
  if (r.chunkSize < 8 || r.chunkSize > 128)
    throw std::runtime_error("region chunk size");
  const auto &grid = c.at("grid");
  const auto &origin = grid.at("origin").array();
  if (origin.size() != 2)
    throw std::runtime_error("region grid origin");
  r.originX = finiteNumber(origin[0], "grid origin");
  r.originZ = finiteNumber(origin[1], "grid origin");
  r.columns = whole(grid.at("columns"), 1, 64, "grid columns");
  r.rows = whole(grid.at("rows"), 1, 64, "grid rows");
  r.groundSpacing = finiteNumber(c.at("ground_spacing"), "ground spacing");
  const auto &height = c.at("height");
  if (field(height, "half_extent")) {
    r.legacyHeight = true;
    r.heightHalf = finiteNumber(height.at("half_extent"), "height extent");
    r.heightNX = r.heightNZ = whole(height.at("samples"), 2, 1025, "height samples");
    r.walkHalf = finiteNumber(c.at("walk_half_extent"), "walk extent");
    r.walkMinX = r.walkMinZ = -r.walkHalf;
    r.walkMaxX = r.walkMaxZ = r.walkHalf;
  } else {
    r.legacyHeight = false;
    const auto &o = height.at("origin").array(), &n = height.at("count").array();
    if (o.size() != 2 || n.size() != 2)
      throw std::runtime_error("region height lattice");
    r.heightOriginX = finiteNumber(o[0], "height origin");
    r.heightOriginZ = finiteNumber(o[1], "height origin");
    r.heightStep = finiteNumber(height.at("step"), "height step");
    r.heightNX = whole(n[0], 2, 4096, "height count");
    r.heightNZ = whole(n[1], 2, 4096, "height count");
    const auto &walk = c.at("walk");
    const auto &lo = walk.at("min").array(), &hi = walk.at("max").array();
    if (lo.size() != 2 || hi.size() != 2)
      throw std::runtime_error("region walk bounds");
    r.walkMinX = finiteNumber(lo[0], "walk bounds");
    r.walkMinZ = finiteNumber(lo[1], "walk bounds");
    r.walkMaxX = finiteNumber(hi[0], "walk bounds");
    r.walkMaxZ = finiteNumber(hi[1], "walk bounds");
  }
  const auto &shared = c.at("shared");
  r.pack = shared.at("packs").at("default").string();
  r.sourcePack = shared.at("packs").at("source").string();
  r.stylePack = shared.at("packs").at("style").string();
  r.vegetation = shared.at("vegetation").string();
  if (auto f = field(c, "shadow_follow"))
    r.shadowFollow = std::get<bool>(f->value);
  if (auto f = field(c, "persistent"))
    r.persistent = f->array();
  if (auto t = field(c, "terrain")) {
    if (r.legacyHeight)
      throw std::runtime_error("region terrain needs a height lattice (origin, step, count)");
    r.terrain.on = true;
    r.terrain.step = finiteNumber(t->at("step"), "terrain step");
    const float cells = r.chunkSize / r.terrain.step;
    if (r.terrain.step < .25f || r.terrain.step > 4 || cells != std::floor(cells) || cells > 256)
      throw std::runtime_error("region terrain step must divide the chunk size");
    r.terrain.cliffNormalY = finiteNumber(t->at("cliff_normal_y"), "terrain cliff_normal_y");
    if (r.terrain.cliffNormalY < .1f || r.terrain.cliffNormalY > .99f)
      throw std::runtime_error("region terrain cliff_normal_y must be .1 to .99");
    for (auto [name, out] : {std::pair{"lawn", &r.terrain.lawn}, std::pair{"rock", &r.terrain.rock}}) {
      const auto &rgb = t->at(name).array();
      if (rgb.size() != 3)
        throw std::runtime_error(std::string("region terrain ") + name + " colour");
      for (int k = 0; k < 3; k++) {
        (*out)[k] = finiteNumber(rgb[k], "terrain colour");
        if ((*out)[k] < 0 || (*out)[k] > 1)
          throw std::runtime_error(std::string("region terrain ") + name + " colour");
      }
    }
  }
  std::set<std::pair<int, int>> seen;
  for (const auto &entry : c.at("chunks").array()) {
    Entry e;
    e.x = whole(entry.at("x"), 0, r.columns - 1, "chunk x");
    e.z = whole(entry.at("z"), 0, r.rows - 1, "chunk z");
    if (!seen.insert({e.x, e.z}).second)
      throw std::runtime_error("region chunk listed twice");
    e.file = p3::safePath(r.dir, entry.at("file").string());
    if (auto ring = field(entry, "ring"))
      e.ring = std::get<bool>(ring->value);
    r.chunkFiles.push_back(e);
  }
  if (r.chunkFiles.empty())
    throw std::runtime_error("region has no chunks");
  return r;
}
ChunkData Region::readChunk(std::size_t index) const {
  const auto &entry = chunkFiles.at(index);
  const auto &file = entry.file;
  ChunkData d;
  d.x = entry.x;
  d.z = entry.z;
  const auto chunkText = p3::readText(file, 4 * 1024 * 1024);
  d.bytes += chunkText.size();
  const auto chunk = ws::parseJson(chunkText);
  if (chunk.at("revision").number() != 1 || chunk.at("x").number() != d.x ||
      chunk.at("z").number() != d.z)
    throw std::runtime_error("chunk header " + file.filename().string());
  // Everything in a chunk file must stand inside that chunk's square.
  auto inside = [&](const ws::Json::Array &p, std::size_t xi, std::size_t zi) {
    if (chunkOf(float(p.at(xi).number()), float(p.at(zi).number())) != std::pair{d.x, d.z})
      throw std::runtime_error("chunk content outside its square: " + file.filename().string());
  };
  for (const auto &row : chunk.at("instances").array()) {
    inside(row.at("position").array(), 0, 2);
    d.instances.push_back(row);
  }
  for (const auto &row : chunk.at("solids").array()) {
    const auto &b = row.at("box").array();
    if (b.size() != 4)
      throw std::runtime_error("solid arity");
    inside(b, 0, 1);
    d.solids.push_back({order(row),
                        {{float(b[0].number()), float(b[1].number()),
                          float(b[2].number()), float(b[3].number())},
                         std::get<bool>(row.at("house").value)}});
  }
  for (const auto &row : chunk.at("grass").array()) {
    inside(row.at("position").array(), 0, 2);
    d.grass.push_back(row);
  }
  if (auto h = field(chunk, "height")) {
    const auto bytes = p3::readText(p3::safePath(file.parent_path(), h->string()), 16 * 1024 * 1024);
    d.bytes += bytes.size();
    if (bytes.size() < 20 || bytes.substr(0, 4) != "WSH1")
      throw std::runtime_error("chunk height header");
    const std::size_t i0 = u32(bytes, 4), j0 = u32(bytes, 8), ni = u32(bytes, 12), nj = u32(bytes, 16);
    if (ni == 0 || nj == 0 || i0 + ni > std::size_t(heightNX) || j0 + nj > std::size_t(heightNZ) ||
        bytes.size() != 20 + ni * nj * 4)
      throw std::runtime_error("chunk height size");
    d.i0 = int(i0);
    d.j0 = int(j0);
    d.ni = int(ni);
    d.nj = int(nj);
    d.heights.resize(ni * nj);
    for (std::size_t k = 0; k < ni * nj; k++)
      d.heights[k] = std::bit_cast<float>(u32(bytes, 20 + k * 4));
  }
  return d;
}
Region Region::load(const std::filesystem::path &assets, const std::string &id) {
  Region r = open(assets, id);
  const std::size_t nx = std::size_t(r.heightNX), nz = std::size_t(r.heightNZ);
  r.heights.assign(nx * nz, 0);
  std::vector<unsigned char> covered(nx * nz, 0);
  std::vector<std::pair<double, RegionSolid>> solids;
  r.instances = r.persistent;
  for (std::size_t e = 0; e < r.chunkFiles.size(); e++) {
    auto d = r.readChunk(e);
    r.bytesRead += d.bytes;
    r.instances.insert(r.instances.end(), d.instances.begin(), d.instances.end());
    r.grass.insert(r.grass.end(), d.grass.begin(), d.grass.end());
    solids.insert(solids.end(), d.solids.begin(), d.solids.end());
    for (int j = 0; j < d.nj; j++)
      for (int i = 0; i < d.ni; i++) {
        const std::size_t at = std::size_t(d.j0 + j) * nx + std::size_t(d.i0 + i);
        if (covered[at]++)
          throw std::runtime_error("height sample owned twice");
        r.heights[at] = d.heights[std::size_t(j * d.ni + i)];
      }
    r.chunksLoaded++;
  }
  for (auto k : covered)
    if (k != 1)
      throw std::runtime_error("height lattice not fully covered by chunks");
  if (r.instances.empty())
    throw std::runtime_error("region has no instances");
  std::stable_sort(r.instances.begin(), r.instances.end(),
                   [](const ws::Json &a, const ws::Json &b) { return order(a) < order(b); });
  std::stable_sort(solids.begin(), solids.end(),
                   [](const auto &a, const auto &b) { return a.first < b.first; });
  for (auto &s : solids)
    r.solids.push_back(s.second);
  return r;
}
} // namespace world
